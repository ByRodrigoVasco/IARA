#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "sv_ranking.h"

static SvRankEntry entries[SV_RANKING_MAX];
static int entry_count = 0;
static long next_order = 1;
static char ranking_path[512] = SV_RANKING_PATH;

void sv_ranking_clear(void)
{
    entry_count = 0;
    next_order = 1;
}

void sv_ranking_set_path(const char *path)
{
    snprintf(ranking_path, sizeof(ranking_path), "%s", (path != NULL && path[0] != '\0') ? path : SV_RANKING_PATH);
}

const char *sv_ranking_path(void)
{
    return ranking_path;
}

bool sv_ranking_better(const SvRankEntry *a, const SvRankEntry *b)
{
    if (a->won != b->won) return a->won;
    if (a->seconds != b->seconds) return a->seconds > b->seconds;
    if (a->kills != b->kills) return a->kills > b->kills;
    return a->order < b->order;
}

// Quantos bytes tem o caractere UTF-8 que começa com este byte (1 a 4; 0 se for um byte de continuação)
static int utf8_length(unsigned char c)
{
    if (c < 0x80) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 0;
}

void sv_ranking_clean_name(const char *in, char *out, int out_size)
{
    int o = 0, letters = 0;
    bool pending_space = false;

    if (out_size <= 0) return;
    for (int i = 0; in != NULL && in[i] != '\0';)
    {
        unsigned char c = (unsigned char)in[i];
        int len = utf8_length(c);

        if (len == 0) { i++; continue; }   // byte solto: ignora
        for (int k = 1; k < len; k++)
        {
            if (((unsigned char)in[i + k] & 0xC0) != 0x80) { len = 0; break; }   // sequência quebrada
        }
        if (len == 0) { i++; continue; }

        if (len == 1 && (c < 32 || c == 127 || c == '|' || c == ' ' || c == '\t'))
        {
            // Espaço (ou algo que quebraria o arquivo): vira um espaço só, e só entre palavras
            if (o > 0) pending_space = true;
            i++;
            continue;
        }
        if (letters >= SV_NAME_MAX_CHARS) break;
        if (pending_space)
        {
            if (letters + 1 >= SV_NAME_MAX_CHARS || o + 1 + len >= out_size) break;
            out[o++] = ' ';
            letters++;
            pending_space = false;
        }
        if (o + len >= out_size) break;
        memcpy(out + o, in + i, (size_t)len);
        o += len;
        letters++;
        i += len;
    }
    out[o] = '\0';
    if (o == 0) snprintf(out, (size_t)out_size, "%s", "Anônimo");
}

static bool same_name(const char *a, const char *b)
{
    while (*a != '\0' && *b != '\0')
    {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return false;
        a++;
        b++;
    }
    return *a == *b;
}

int sv_ranking_find(const char *name)
{
    char clean[SV_NAME_BYTES];

    sv_ranking_clean_name(name, clean, sizeof(clean));
    for (int i = 0; i < entry_count; i++)
    {
        if (same_name(entries[i].name, clean)) return i;
    }
    return -1;
}

static void remove_at(int index)
{
    memmove(&entries[index], &entries[index + 1], sizeof(SvRankEntry) * (size_t)(entry_count - 1 - index));
    entry_count--;
}

bool sv_ranking_remove(const char *name)
{
    int index = sv_ranking_find(name);

    if (index < 0) return false;
    remove_at(index);
    return sv_ranking_save();
}

static void insert_sorted(const SvRankEntry *e)
{
    int pos = entry_count;

    while (pos > 0 && sv_ranking_better(e, &entries[pos - 1])) pos--;
    if (pos >= SV_RANKING_MAX) return;   // pior que todos com o ranking cheio
    if (entry_count < SV_RANKING_MAX) entry_count++;
    memmove(&entries[pos + 1], &entries[pos], sizeof(SvRankEntry) * (size_t)(entry_count - 1 - pos));
    entries[pos] = *e;
}

// Fica só com o melhor resultado de cada nome. true se a entrada nova ficou no ranking.
static bool keep_best(const SvRankEntry *e)
{
    int old = sv_ranking_find(e->name);

    if (old >= 0)
    {
        if (!sv_ranking_better(e, &entries[old])) return false;
        remove_at(old);
    }
    insert_sorted(e);
    return true;
}

int sv_ranking_add(const char *name, int seconds, int kills, int level, int bosses, int character, bool won, bool *new_record)
{
    SvRankEntry e = {0};
    bool record;
    int index;

    sv_ranking_clean_name(name, e.name, sizeof(e.name));
    e.seconds = seconds < 0 ? 0 : seconds;
    e.kills = kills < 0 ? 0 : kills;
    e.level = level;
    e.bosses = bosses;
    e.character = character;
    e.won = won;
    e.order = next_order++;

    record = keep_best(&e) && sv_ranking_find(e.name) >= 0 && entries[sv_ranking_find(e.name)].order == e.order;
    if (new_record) *new_record = record;
    if (record) sv_ranking_save();
    index = sv_ranking_find(e.name);
    return index >= 0 ? index + 1 : 0;
}

int sv_ranking_count(void)
{
    return entry_count;
}

const SvRankEntry *sv_ranking_get(int index)
{
    if (index < 0 || index >= entry_count) return NULL;
    return &entries[index];
}

// Arquivo de texto, uma partida por linha: nome|segundos|abates|nível|chefes|personagem|venceu|ordem
bool sv_ranking_load(void)
{
    FILE *file = fopen(ranking_path, "r");
    char line[256];

    sv_ranking_clear();
    if (!file) return false;
    while (fgets(line, sizeof(line), file))
    {
        SvRankEntry e = {0};
        char *bar = strchr(line, '|');
        int won = 0;

        if (!bar) continue;
        *bar = '\0';
        sv_ranking_clean_name(line, e.name, sizeof(e.name));
        if (sscanf(bar + 1, "%d|%d|%d|%d|%d|%d|%ld", &e.seconds, &e.kills, &e.level, &e.bosses, &e.character, &won, &e.order) != 7) continue;
        e.won = won != 0;
        keep_best(&e);   // Arquivo de uma versão antiga pode ter o mesmo nome várias vezes

        if (e.order >= next_order) next_order = e.order + 1;
    }
    fclose(file);
    return true;
}

bool sv_ranking_save(void)
{
    char temp_path[530];
    FILE *file;

    // Grava num arquivo temporário e só depois troca, para uma queda de energia não apagar o ranking
    snprintf(temp_path, sizeof(temp_path), "%s.tmp", ranking_path);
    file = fopen(temp_path, "w");
    if (!file) return false;
    for (int i = 0; i < entry_count; i++)
    {
        const SvRankEntry *e = &entries[i];

        fprintf(file, "%s|%d|%d|%d|%d|%d|%d|%ld\n", e->name, e->seconds, e->kills, e->level, e->bosses, e->character, e->won ? 1 : 0, e->order);
    }
    if (fclose(file) != 0) return false;
    remove(ranking_path);
    return rename(temp_path, ranking_path) == 0;
}
