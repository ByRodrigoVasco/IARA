#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cloud_json.h"

#define CJ_MAX_DEPTH 32   // Respostas da nuvem têm 2 níveis; o limite protege de um texto malicioso

// -----------------------------------------------------------------------------
// Leitura
// -----------------------------------------------------------------------------

void cj_init(cj_reader *reader, const char *text)
{
    reader->at = text != NULL ? text : "";
    reader->error = text == NULL;
}

static void skip_spaces(cj_reader *reader)
{
    while (*reader->at == ' ' || *reader->at == '\t' || *reader->at == '\n' || *reader->at == '\r') reader->at++;
}

bool cj_consume(cj_reader *reader, char c)
{
    if (reader->error) return false;
    skip_spaces(reader);
    if (*reader->at != c) return false;
    reader->at++;
    return true;
}

bool cj_expect(cj_reader *reader, char c)
{
    if (cj_consume(reader, c)) return true;
    reader->error = true;
    return false;
}

static bool consume_word(cj_reader *reader, const char *word)
{
    size_t length = strlen(word);

    skip_spaces(reader);
    if (strncmp(reader->at, word, length) != 0) return false;
    reader->at += length;
    return true;
}

static int hex_digit(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static bool read_hex4(const char *text, unsigned *value)
{
    unsigned result = 0;

    for (int i = 0; i < 4; i++)
    {
        int digit = hex_digit(text[i]);

        if (digit < 0) return false;
        result = result * 16 + (unsigned)digit;
    }
    *value = result;
    return true;
}

static size_t write_utf8(char *out, unsigned code)
{
    if (code < 0x80) { out[0] = (char)code; return 1; }
    if (code < 0x800)
    {
        out[0] = (char)(0xC0 | (code >> 6));
        out[1] = (char)(0x80 | (code & 0x3F));
        return 2;
    }
    if (code < 0x10000)
    {
        out[0] = (char)(0xE0 | (code >> 12));
        out[1] = (char)(0x80 | ((code >> 6) & 0x3F));
        out[2] = (char)(0x80 | (code & 0x3F));
        return 3;
    }
    out[0] = (char)(0xF0 | (code >> 18));
    out[1] = (char)(0x80 | ((code >> 12) & 0x3F));
    out[2] = (char)(0x80 | ((code >> 6) & 0x3F));
    out[3] = (char)(0x80 | (code & 0x3F));
    return 4;
}

char *cj_read_string(cj_reader *reader)
{
    const char *at;
    char *out;
    size_t length = 0;

    if (reader->error) return NULL;
    if (consume_word(reader, "null"))
    {
        out = calloc(1, 1);
        if (out == NULL) reader->error = true;
        return out;
    }
    if (!cj_expect(reader, '"')) return NULL;

    // O texto decodificado nunca é maior que o texto com os escapes
    for (at = reader->at; *at != '"'; at++)
    {
        if (*at == '\0' || (*at == '\\' && *++at == '\0'))
        {
            reader->error = true;
            return NULL;
        }
    }
    out = malloc((size_t)(at - reader->at) + 1);
    if (out == NULL)
    {
        reader->error = true;
        return NULL;
    }

    at = reader->at;
    while (*at != '"')
    {
        if (*at != '\\')
        {
            out[length++] = *at++;
            continue;
        }
        at++;
        switch (*at)
        {
        case '"':  out[length++] = '"';  at++; break;
        case '\\': out[length++] = '\\'; at++; break;
        case '/':  out[length++] = '/';  at++; break;
        case 'b':  out[length++] = '\b'; at++; break;
        case 'f':  out[length++] = '\f'; at++; break;
        case 'n':  out[length++] = '\n'; at++; break;
        case 'r':  out[length++] = '\r'; at++; break;
        case 't':  out[length++] = '\t'; at++; break;
        case 'u':
        {
            unsigned code = 0, low = 0;

            if (!read_hex4(at + 1, &code)) goto bad;
            at += 5;
            if (code >= 0xD800 && code <= 0xDBFF)
            {
                if (at[0] != '\\' || at[1] != 'u' || !read_hex4(at + 2, &low) || low < 0xDC00 || low > 0xDFFF) goto bad;
                code = 0x10000 + ((code - 0xD800) << 10) + (low - 0xDC00);
                at += 6;
            }
            else if (code >= 0xDC00 && code <= 0xDFFF) goto bad;
            if (code == 0) goto bad;   // Um '\0' no meio cortaria o texto em silêncio
            length += write_utf8(out + length, code);
            break;
        }
        default:
            goto bad;
        }
    }
    out[length] = '\0';
    reader->at = at + 1;
    return out;

bad:
    free(out);
    reader->error = true;
    return NULL;
}

bool cj_read_key(cj_reader *reader, char *key, size_t size)
{
    char *text = cj_read_string(reader);

    if (text == NULL) return false;
    snprintf(key, size, "%s", text);
    free(text);
    return cj_expect(reader, ':');
}

bool cj_read_long(cj_reader *reader, long long *value)
{
    char *end = NULL;

    *value = 0;
    if (reader->error) return false;
    if (consume_word(reader, "null")) return true;
    skip_spaces(reader);
    if (*reader->at != '-' && (*reader->at < '0' || *reader->at > '9'))
    {
        reader->error = true;
        return false;
    }
    *value = strtoll(reader->at, &end, 10);
    if (end == reader->at)
    {
        reader->error = true;
        return false;
    }
    while (*end == '.' || *end == 'e' || *end == 'E' || *end == '+' || *end == '-' || (*end >= '0' && *end <= '9')) end++;
    reader->at = end;
    return true;
}

bool cj_read_bool(cj_reader *reader, bool *value)
{
    *value = false;
    if (reader->error) return false;
    if (consume_word(reader, "true")) { *value = true; return true; }
    if (consume_word(reader, "false") || consume_word(reader, "null")) return true;
    reader->error = true;
    return false;
}

static bool skip_value(cj_reader *reader, int depth)
{
    if (reader->error || depth > CJ_MAX_DEPTH)
    {
        reader->error = true;
        return false;
    }
    skip_spaces(reader);
    if (*reader->at == '"')
    {
        char *text = cj_read_string(reader);

        free(text);
        return text != NULL;
    }
    if (*reader->at == '{' || *reader->at == '[')
    {
        bool object = *reader->at == '{';
        char close = object ? '}' : ']';

        reader->at++;
        if (cj_consume(reader, close)) return true;
        do
        {
            if (object)
            {
                char key[2];

                if (!cj_read_key(reader, key, sizeof(key))) return false;
            }
            if (!skip_value(reader, depth + 1)) return false;
        } while (cj_consume(reader, ','));
        return cj_expect(reader, close);
    }
    if (consume_word(reader, "true") || consume_word(reader, "false") || consume_word(reader, "null")) return true;
    {
        long long ignored;

        return cj_read_long(reader, &ignored);
    }
}

bool cj_skip(cj_reader *reader)
{
    return skip_value(reader, 0);
}

// Dias entre 1970-01-01 e a data (calendário gregoriano)
static long long days_from_civil(int year, int month, int day)
{
    long long y = year - (month <= 2);
    long long era = (y >= 0 ? y : y - 399) / 400;
    long long yoe = y - era * 400;
    long long doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
    long long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;

    return era * 146097 + doe - 719468;
}

long long cj_parse_time(const char *text)
{
    int year, month, day, hour, minute, second;
    const char *zone;
    long long result;

    if (text == NULL || sscanf(text, "%4d-%2d-%2dT%2d:%2d:%2d", &year, &month, &day, &hour, &minute, &second) != 6) return 0;
    if (month < 1 || month > 12 || day < 1 || day > 31) return 0;
    result = days_from_civil(year, month, day) * 86400LL + hour * 3600 + minute * 60 + second;

    zone = text + 19;
    while (*zone == '.' || (*zone >= '0' && *zone <= '9')) zone++;
    if (*zone == '+' || *zone == '-')
    {
        int zone_hour = 0, zone_minute = 0;

        if (sscanf(zone + 1, "%2d:%2d", &zone_hour, &zone_minute) >= 1)
        {
            long long offset = zone_hour * 3600 + zone_minute * 60;

            result += *zone == '+' ? -offset : offset;
        }
    }
    return result;
}

// -----------------------------------------------------------------------------
// Escrita
// -----------------------------------------------------------------------------

static void append(cj_writer *writer, const char *text, size_t length)
{
    if (writer->failed) return;
    if (writer->length + length + 1 > writer->capacity)
    {
        size_t capacity = writer->capacity != 0 ? writer->capacity : 256;
        char *grown;

        while (writer->length + length + 1 > capacity) capacity *= 2;
        grown = malloc(capacity);
        if (grown == NULL)
        {
            writer->failed = true;
            return;
        }
        if (writer->text != NULL)
        {
            memcpy(grown, writer->text, writer->length);
            // O corpo pode ter a senha de apagar saves: o buffer antigo não fica na memória
            memset(writer->text, 0, writer->capacity);
            free(writer->text);
        }
        writer->text = grown;
        writer->capacity = capacity;
    }
    memcpy(writer->text + writer->length, text, length);
    writer->length += length;
    writer->text[writer->length] = '\0';
}

static void append_text(cj_writer *writer, const char *text)
{
    append(writer, text, strlen(text));
}

static void append_quoted(cj_writer *writer, const char *text)
{
    append_text(writer, "\"");
    for (const unsigned char *c = (const unsigned char *)(text != NULL ? text : ""); *c != '\0'; c++)
    {
        char escape[8];

        if (*c == '"') append_text(writer, "\\\"");
        else if (*c == '\\') append_text(writer, "\\\\");
        else if (*c == '\n') append_text(writer, "\\n");
        else if (*c == '\r') append_text(writer, "\\r");
        else if (*c == '\t') append_text(writer, "\\t");
        else if (*c < 0x20)
        {
            snprintf(escape, sizeof(escape), "\\u%04x", *c);
            append_text(writer, escape);
        }
        else append(writer, (const char *)c, 1);
    }
    append_text(writer, "\"");
}

void cj_begin(cj_writer *writer)
{
    memset(writer, 0, sizeof(*writer));
    append_text(writer, "{");
}

static void add_key(cj_writer *writer, const char *key)
{
    if (writer->has_field) append_text(writer, ",");
    writer->has_field = true;
    append_quoted(writer, key);
    append_text(writer, ":");
}

void cj_add_string(cj_writer *writer, const char *key, const char *value)
{
    add_key(writer, key);
    append_quoted(writer, value);
}

void cj_add_int(cj_writer *writer, const char *key, long long value)
{
    char number[32];

    add_key(writer, key);
    snprintf(number, sizeof(number), "%lld", value);
    append_text(writer, number);
}

void cj_add_bool(cj_writer *writer, const char *key, bool value)
{
    add_key(writer, key);
    append_text(writer, value ? "true" : "false");
}

char *cj_end(cj_writer *writer)
{
    char *text;

    append_text(writer, "}");
    if (writer->failed)
    {
        free(writer->text);
        writer->text = NULL;
    }
    text = writer->text;
    writer->text = NULL;
    return text;
}
