#ifndef CLOUD_JSON_H
#define CLOUD_JSON_H

#include <stdbool.h>
#include <stddef.h>

/*
 * JSON mínimo para a nuvem (cloud.c): monta o corpo das chamadas e lê as respostas do Supabase
 * (um texto, um número ou uma lista de objetos simples). Sem Raylib e sem Windows, para os testes.
 */

// ----------------------------------------------------------------------------- Leitura

typedef struct
{
    const char *at;
    bool error;      // Algo veio fora do formato; todas as leituras seguintes falham
} cj_reader;

void cj_init(cj_reader *reader, const char *text);

/** @brief Pula os espaços e, se o próximo caractere for c, consome e devolve true. */
bool cj_consume(cj_reader *reader, char c);

/** @brief Como cj_consume, mas a falta do caractere é erro. */
bool cj_expect(cj_reader *reader, char c);

/** @brief Lê um texto (null vira texto vazio). O chamador libera com free. NULL se der erro. */
char *cj_read_string(cj_reader *reader);

/** @brief Lê a chave de um objeto e os dois-pontos. Chaves maiores que o espaço são cortadas. */
bool cj_read_key(cj_reader *reader, char *key, size_t size);

/** @brief Lê um número inteiro (a parte decimal é ignorada; null vira 0). */
bool cj_read_long(cj_reader *reader, long long *value);

/** @brief Lê true ou false (null vira false). */
bool cj_read_bool(cj_reader *reader, bool *value);

/** @brief Pula um valor qualquer. */
bool cj_skip(cj_reader *reader);

/** @brief "2026-11-25T12:34:56.789+00:00" em segundos desde 1970 (UTC). 0 se não entender. */
long long cj_parse_time(const char *text);

// ----------------------------------------------------------------------------- Escrita

typedef struct
{
    char *text;
    size_t length;
    size_t capacity;
    bool failed;     // Faltou memória
    bool has_field;
} cj_writer;

void cj_begin(cj_writer *writer);
void cj_add_string(cj_writer *writer, const char *key, const char *value);
void cj_add_int(cj_writer *writer, const char *key, long long value);
void cj_add_bool(cj_writer *writer, const char *key, bool value);

/** @brief Fecha o objeto e entrega o texto (o chamador libera). NULL se faltou memória. */
char *cj_end(cj_writer *writer);

#endif // CLOUD_JSON_H
