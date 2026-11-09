#ifndef CALENDAR_H
#define CALENDAR_H

#include <stdbool.h>

// Linhas de texto de cada bilhete do mural, nos dias com investigação.
#define CALENDAR_NOTE_LINE_COUNT 4

/**
 * @brief Tipo do dia na campanha: PLAYABLE_DAY, FLASH_DAY ou JUMP_DAY (GDD 2.7).
 */
int calendar_get_day_type(int day);

/**
 * @brief Quantos dias de salto puro seguidos começam em day, contando com ele.
 */
int calendar_get_jump_block_length(int day);

/**
 * @brief Frase do Flash do dia, ou NULL se o dia não for um flash.
 */
const char *calendar_get_flash_text(int day);

/**
 * @brief Linha (0 ou 1) do cartão de chegada de um dia jogável.
 */
const char *calendar_get_arrival_line(int day, int line);

/**
 * @brief Segunda linha do cartão de fim de um dia jogável.
 */
const char *calendar_get_day_end_line(int day);

/**
 * @brief Informa se o mural tem algo a examinar nesse dia (GDD 6.1: cerca de 6 dos 9 dias).
 */
bool calendar_has_investigation(int day);

/**
 * @brief Bilhete do mural do dia, com CALENDAR_NOTE_LINE_COUNT linhas. O Dia 1 usa o bilhete padrão e devolve NULL.
 */
const char *const *calendar_get_note_lines(int day);

/**
 * @brief O quanto o escritório já está sob a água, de 0 a 1. É o sintoma visual da campanha (GDD 6.1).
 */
float calendar_get_water_level(int day);

#endif // CALENDAR_H
