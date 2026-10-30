#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "save.h"
#include "game_state.h"

// Um save válido tem pouco mais de 300 bytes; qualquer coisa muito maior que
// isso não é um save do IARA.
#define SAVE_MAX_FILE_SIZE 4096

#define CHECKSUM_KEY "checksum="

// Cada campo obrigatório do save liga um bit ao ser lido. O save só é aceito
// se todos os bits estiverem ligados, ou seja, se nenhum campo faltar.
enum
{
    FIELD_CURRENT_DAY        = 1 << 0,
    FIELD_DAY_TYPE           = 1 << 1,
    FIELD_MODE_TYPE          = 1 << 2,
    FIELD_USER_INVESTIGATED  = 1 << 3,
    FIELD_IS_RESOLVED        = 1 << 4,
    FIELD_OVERALL_POINTS     = 1 << 5,
    FIELD_AUTOMATION_POINTS  = 1 << 6,
    FIELD_PERCEPTION_POINTS  = 1 << 7,
    FIELD_HUMAN_BOND_POINTS  = 1 << 8,
    FIELD_RESOLVED_PUZZLES   = 1 << 9,
    ALL_FIELDS               = (1 << 10) - 1
};

// ---------------------------------------------------------------------------
// FUNÇÕES AUXILIARES
// ---------------------------------------------------------------------------

/**
 * @brief Checksum FNV-1a de 32 bits. Não é segurança, é só detecção de
 * arquivo truncado ou alterado.
 */
static uint32_t compute_checksum(const char *data, size_t length)
{
    uint32_t hash = 2166136261u;

    for (size_t i = 0; i < length; i++)
    {
        hash ^= (unsigned char)data[i];
        hash *= 16777619u;
    }

    return hash;
}

/**
 * @brief Converte texto em long, aceitando só números inteiros completos
 * (rejeita vazio, "12abc", etc.).
 */
static bool parse_long(const char *text, long *out)
{
    char *end = NULL;

    if (text == NULL || *text == '\0')
    {
        return false;
    }

    long value = strtol(text, &end, 10);

    if (*end != '\0')
    {
        return false;
    }

    *out = value;
    return true;
}

/**
 * @brief Lê um booleano gravado como 0 ou 1.
 */
static bool parse_bool(const char *text, bool *out)
{
    if (strcmp(text, "0") == 0)
    {
        *out = false;
        return true;
    }

    if (strcmp(text, "1") == 0)
    {
        *out = true;
        return true;
    }

    return false;
}

/**
 * @brief Lê um inteiro e confere se está no intervalo [min, max].
 */
static bool parse_ranged(const char *text, long min, long max, long *out)
{
    return parse_long(text, out) && *out >= min && *out <= max;
}

/**
 * @brief Aplica uma linha "chave=valor" nas cópias temporárias do estado.
 * Retorna false se o valor for inválido para aquela chave.
 */
static bool apply_save_line(const char *key, const char *value,
                            struct game_state_variables *state,
                            struct hidden_statistic *stats,
                            unsigned int *fields_read)
{
    long number = 0;

    if (strcmp(key, "current_day") == 0)
    {
        if (!parse_ranged(value, 1, TOTAL_GAME_DAYS, &number)) return false;
        state->current_day = (int)number;
        *fields_read |= FIELD_CURRENT_DAY;
    }
    else if (strcmp(key, "day_type") == 0)
    {
        if (!parse_ranged(value, NOT_YET_DEFINED, JUMP_DAY, &number)) return false;
        state->day_type = (int)number;
        *fields_read |= FIELD_DAY_TYPE;
    }
    else if (strcmp(key, "mode_type") == 0)
    {
        if (!parse_bool(value, &state->mode_type)) return false;
        *fields_read |= FIELD_MODE_TYPE;
    }
    else if (strcmp(key, "user_investigated") == 0)
    {
        if (!parse_bool(value, &state->user_investigated)) return false;
        *fields_read |= FIELD_USER_INVESTIGATED;
    }
    else if (strcmp(key, "is_resolved") == 0)
    {
        if (!parse_bool(value, &state->is_resolved)) return false;
        *fields_read |= FIELD_IS_RESOLVED;
    }
    else if (strcmp(key, "overall_points") == 0)
    {
        // Pontuação geral pode ficar negativa.
        if (!parse_ranged(value, -1000000, 1000000, &number)) return false;
        stats->current_overall_points = (signed int)number;
        *fields_read |= FIELD_OVERALL_POINTS;
    }
    else if (strcmp(key, "automation_points") == 0)
    {
        if (!parse_ranged(value, 0, 1000000, &number)) return false;
        stats->current_automation_points = (unsigned int)number;
        *fields_read |= FIELD_AUTOMATION_POINTS;
    }
    else if (strcmp(key, "perception_points") == 0)
    {
        if (!parse_ranged(value, 0, 1000000, &number)) return false;
        stats->current_perception_points = (unsigned int)number;
        *fields_read |= FIELD_PERCEPTION_POINTS;
    }
    else if (strcmp(key, "human_bond_points") == 0)
    {
        if (!parse_ranged(value, 0, 1000000, &number)) return false;
        stats->current_human_bond_points = (unsigned int)number;
        *fields_read |= FIELD_HUMAN_BOND_POINTS;
    }
    else if (strcmp(key, "resolved_puzzles") == 0)
    {
        // Um caractere por dia, do Dia 1 ao Dia TOTAL_GAME_DAYS.
        if (strlen(value) != TOTAL_GAME_DAYS) return false;

        state->resolved_puzzles[0] = false;
        for (int day = 1; day <= TOTAL_GAME_DAYS; day++)
        {
            char flag = value[day - 1];
            if (flag != '0' && flag != '1') return false;
            state->resolved_puzzles[day] = (flag == '1');
        }
        *fields_read |= FIELD_RESOLVED_PUZZLES;
    }
    // Chaves desconhecidas são ignoradas, para um save de uma versão futura
    // com campos extras não ser tratado como corrompido.

    return true;
}

// ---------------------------------------------------------------------------
// SERIALIZAÇÃO
// ---------------------------------------------------------------------------

/**
 * @brief Monta o conteúdo do save (sem o checksum) em buffer.
 * Retorna o tamanho escrito, ou -1 se não couber.
 */
static int build_save_body(char *buffer, size_t size)
{
    const struct game_state_variables *state = &current_game_state_variables;
    const struct hidden_statistic *stats = &current_hidden_statistic;

    char puzzles[TOTAL_GAME_DAYS + 1];
    for (int day = 1; day <= TOTAL_GAME_DAYS; day++)
    {
        puzzles[day - 1] = state->resolved_puzzles[day] ? '1' : '0';
    }
    puzzles[TOTAL_GAME_DAYS] = '\0';

    int written = snprintf(buffer, size,
                           SAVE_FILE_HEADER "\n"
                           "current_day=%d\n"
                           "day_type=%d\n"
                           "mode_type=%d\n"
                           "user_investigated=%d\n"
                           "is_resolved=%d\n"
                           "overall_points=%d\n"
                           "automation_points=%u\n"
                           "perception_points=%u\n"
                           "human_bond_points=%u\n"
                           "resolved_puzzles=%s\n",
                           state->current_day,
                           state->day_type,
                           state->mode_type ? 1 : 0,
                           state->user_investigated ? 1 : 0,
                           state->is_resolved ? 1 : 0,
                           stats->current_overall_points,
                           stats->current_automation_points,
                           stats->current_perception_points,
                           stats->current_human_bond_points,
                           puzzles);

    if (written < 0 || (size_t)written >= size)
    {
        return -1;
    }

    return written;
}

save_result save_game_state(const char *path)
{
    char body[SAVE_MAX_FILE_SIZE];
    int body_length = build_save_body(body, sizeof(body));

    if (body_length < 0)
    {
        return SAVE_IO_ERROR;
    }

    uint32_t checksum = compute_checksum(body, (size_t)body_length);

    // Grava num arquivo temporário e só depois troca pelo save de verdade.
    char temp_path[512];
    if (snprintf(temp_path, sizeof(temp_path), "%s.tmp", path) >= (int)sizeof(temp_path))
    {
        return SAVE_IO_ERROR;
    }

    FILE *file = fopen(temp_path, "wb");
    if (file == NULL)
    {
        return SAVE_IO_ERROR;
    }

    bool write_ok = fwrite(body, 1, (size_t)body_length, file) == (size_t)body_length;
    write_ok = write_ok && fprintf(file, CHECKSUM_KEY "%08lx\n", (unsigned long)checksum) > 0;
    write_ok = (fclose(file) == 0) && write_ok;

    if (!write_ok)
    {
        remove(temp_path);
        return SAVE_IO_ERROR;
    }

    // No Windows, rename() falha se o destino já existe, então o save antigo
    // é removido antes. O arquivo novo já está completo no .tmp a essa altura.
    remove(path);
    if (rename(temp_path, path) != 0)
    {
        remove(temp_path);
        return SAVE_IO_ERROR;
    }

    return SAVE_OK;
}

// ---------------------------------------------------------------------------
// CARREGAMENTO
// ---------------------------------------------------------------------------

save_result load_game_state(const char *path)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL)
    {
        return SAVE_NOT_FOUND;
    }

    char content[SAVE_MAX_FILE_SIZE + 1];
    size_t length = fread(content, 1, SAVE_MAX_FILE_SIZE + 1, file);
    bool read_error = ferror(file);
    fclose(file);

    if (read_error)
    {
        return SAVE_IO_ERROR;
    }

    if (length == 0 || length > SAVE_MAX_FILE_SIZE)
    {
        return SAVE_CORRUPTED;
    }
    content[length] = '\0';

    // Um '\0' no meio do arquivo indica lixo binário.
    if (strlen(content) != length)
    {
        return SAVE_CORRUPTED;
    }

    // 1. Cabeçalho com formato e versão.
    size_t header_length = strlen(SAVE_FILE_HEADER);
    if (strncmp(content, SAVE_FILE_HEADER, header_length) != 0 || content[header_length] != '\n')
    {
        return SAVE_CORRUPTED;
    }

    // 2. Checksum: última linha, cobrindo tudo o que vem antes dela.
    char *checksum_line = strstr(content, "\n" CHECKSUM_KEY);
    if (checksum_line == NULL)
    {
        return SAVE_CORRUPTED;
    }
    checksum_line++; // pula o '\n', que pertence ao corpo

    size_t body_length = (size_t)(checksum_line - content);
    char *checksum_text = checksum_line + strlen(CHECKSUM_KEY);
    checksum_text[strcspn(checksum_text, "\r\n")] = '\0';

    char *end = NULL;
    unsigned long stored_checksum = strtoul(checksum_text, &end, 16);
    if (*checksum_text == '\0' || *end != '\0' ||
        stored_checksum != (unsigned long)compute_checksum(content, body_length))
    {
        return SAVE_CORRUPTED;
    }

    // 3. Campos. Tudo é lido em cópias; o estado do jogo só muda no final,
    // se o save inteiro for válido.
    struct game_state_variables loaded_state = current_game_state_variables;
    struct hidden_statistic loaded_stats = current_hidden_statistic;
    unsigned int fields_read = 0;

    content[body_length] = '\0';
    char *line = content + header_length + 1;

    while (*line != '\0')
    {
        char *next_line = strchr(line, '\n');
        if (next_line != NULL)
        {
            *next_line = '\0';
            next_line++;
        }

        line[strcspn(line, "\r")] = '\0'; // tolera quebra de linha do Windows

        if (*line != '\0')
        {
            char *separator = strchr(line, '=');
            if (separator == NULL)
            {
                return SAVE_CORRUPTED;
            }

            *separator = '\0';
            if (!apply_save_line(line, separator + 1, &loaded_state, &loaded_stats, &fields_read))
            {
                return SAVE_CORRUPTED;
            }
        }

        if (next_line == NULL)
        {
            break;
        }
        line = next_line;
    }

    if (fields_read != ALL_FIELDS)
    {
        return SAVE_CORRUPTED;
    }

    // current_screen não vem do save: o jogo sempre abre pelo pré-menu.
    loaded_state.current_screen = current_game_state_variables.current_screen;

    current_game_state_variables = loaded_state;
    current_hidden_statistic = loaded_stats;

    return SAVE_OK;
}

const char *save_result_to_string(save_result result)
{
    switch (result)
    {
    case SAVE_OK:        return "ok";
    case SAVE_NOT_FOUND: return "arquivo de save nao encontrado";
    case SAVE_CORRUPTED: return "arquivo de save corrompido ou invalido";
    case SAVE_IO_ERROR:  return "erro de leitura/escrita no disco";
    default:             return "resultado desconhecido";
    }
}
