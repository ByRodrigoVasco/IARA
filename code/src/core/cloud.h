#ifndef CLOUD_H
#define CLOUD_H

#include <stdbool.h>
#include <stddef.h>

/*
 * Saves e rankings compartilhados entre notebooks (Supabase, tools/nuvem/).
 *
 * Nada aqui espera a rede: o jogo faz um pedido, recebe um número (ticket) e pergunta a cada
 * quadro com cloud_poll se a resposta chegou. Uma thread separada conversa com o servidor, renova
 * a trava do save aberto e reenvia o que ficou na fila offline.
 *
 * O save é sempre gravado neste dispositivo. Com conexão, cada gravação também sobe; sem ela, fica
 * na fila e sobe depois. Se a nuvem e o dispositivo mudaram desde a última sincronização, abrir o
 * save devolve CLOUD_DIVERGED e o jogador escolhe qual vale (cloud_resolve_save).
 *
 * Arquivos da pasta "nuvem", ao lado dos saves:
 *   notebook.id                      código deste dispositivo, gerado uma vez
 *   save-<modo>-<nome>.pend          a gravação mais nova de um save que ainda não subiu
 *   link-<modo>-<nome>.sync          versão e resumo (hash) da última sincronização do save
 *   partida-<código>.pend            partida do ranking que ainda não subiu
 *   conflito-<modo>-<nome>-*.txt     cópia da gravação que perdeu a escolha (nunca é apagada pelo jogo)
 *
 * Sem a URL e a chave (build sem tools/nuvem/nuvem.local) a nuvem fica desligada: cloud_enabled()
 * é falso, os pedidos devolvem 0 e o jogo funciona só local.
 *
 * Sem Raylib, para os testes rodarem com um transporte falso, sem rede.
 */

#define CLOUD_NAME_MAX 20          // Igual a SAVE_NAME_MAX e à regra do servidor
#define CLOUD_ROW_NAME_BYTES 64
#define CLOUD_CONTENT_MAX 65536    // Limite do servidor

typedef enum
{
    CLOUD_OK = 0,
    CLOUD_IN_USE,           // em_uso: o save está aberto em outro notebook
    CLOUD_CONFLICT,         // conflito: o servidor tem uma gravação mais nova
    CLOUD_EXISTS,           // existe: já há um save com esse nome
    CLOUD_INVALID,          // invalido: nome recusado
    CLOUD_NOT_FOUND,        // nao_existe
    CLOUD_WRONG_PASSWORD,   // senha
    CLOUD_LOST,             // perdido: outro notebook assumiu o save
    CLOUD_OFFLINE,          // Sem resposta do servidor
    CLOUD_SERVER_ERROR,     // O servidor respondeu com erro ou com algo que não dá para entender
    CLOUD_DIVERGED          // Abrir: a nuvem e este dispositivo têm gravações diferentes; falta escolher
} cloud_status;

typedef enum
{
    CLOUD_LINK_UNKNOWN = 0,   // Ainda sem a primeira resposta (ou nuvem desligada)
    CLOUD_LINK_ONLINE,
    CLOUD_LINK_OFFLINE
} cloud_link;

typedef struct
{
    char name[CLOUD_NAME_MAX + 1];
    long long updated;      // Segundos desde 1970 (UTC)
    bool has_progress;
    bool in_use;            // Aberto em outro notebook
} cloud_save_info;

typedef struct
{
    char name[CLOUD_ROW_NAME_BYTES];
    int seconds, kills, level, bosses, character;
    bool won;
    int extra_kills;      // Abates depois dos 10 minutos (valem o dobro); só vai ao servidor se houver
} cloud_match;

typedef struct
{
    cloud_status status;
    char *content;              // Abrir: o conteúdo que vale ("" se não há progresso). Divergência: o da nuvem
    char *local_content;        // Divergência: o deste dispositivo
    bool cloud_missing;         // Divergência: o save foi apagado na nuvem
    int position;               // Partida: posição no ranking (0 = recusada)
    cloud_save_info *saves;     // Listar
    int save_count;
    cloud_match *rows;          // Ranking, do primeiro ao último
    int row_count;
} cloud_result;

typedef enum
{
    CLOUD_NOTICE_CONFLICT = 0,  // O envio parou: o servidor tem outra gravação
    CLOUD_NOTICE_LOST           // Outro dispositivo assumiu (ou apagou) o save aberto
} cloud_notice_kind;

typedef struct
{
    cloud_notice_kind kind;
    char name[CLOUD_NAME_MAX + 1];
} cloud_notice;

/**
 * @brief Faz uma chamada: POST em rest/v1/rpc/<function> com o corpo JSON.
 * @return false se o servidor não respondeu; *http_status pode levar o erro de rede do Windows com
 * sinal negativo. Se true, *response é alocado com malloc.
 */
typedef bool (*cloud_transport)(const char *function, const char *body, int *http_status, char **response);

/** @brief Liga a nuvem. folder guarda o código do notebook e a fila. transport NULL = desligada. */
bool cloud_start(const char *folder, cloud_transport transport);

bool cloud_enabled(void);

/**
 * @brief Envia a fila, libera o save aberto e encerra a thread, esperando no máximo timeout_ms.
 * O que não subir continua na fila para a próxima vez.
 */
void cloud_stop(unsigned timeout_ms);

const char *cloud_machine_id(void);

/** @brief O último contato com o servidor deu certo? */
bool cloud_online(void);

/** @brief Situação da conexão. Offline, reason recebe o motivo (sem a URL e sem a chave). */
cloud_link cloud_connection(char *reason, size_t size);

/** @brief Quantas gravações e partidas ainda estão na fila. */
int cloud_pending_count(void);

/** @brief Intervalos de renovar a trava e de reenviar a fila (os testes encurtam). */
void cloud_set_timing(unsigned renew_ms, unsigned retry_ms);

// Pedidos. mode é "normal", "sobrevivencia" ou "dupla"; type é "solo" ou "dupla".
// Devolvem o ticket do pedido, ou 0 se a nuvem está desligada ou os dados são inválidos.
int cloud_list_saves(const char *mode);
int cloud_create_save(const char *mode, const char *name);
// local é o save deste dispositivo (NULL ou "" se não há). Com CLOUD_OK ou CLOUD_DIVERGED, passa a
// ser o save aberto.
int cloud_open_save(const char *mode, const char *name, const char *local);
// Decide uma divergência do save aberto: fica o deste dispositivo (keep_local) ou o da nuvem.
int cloud_resolve_save(const char *mode, const char *name, bool keep_local);
int cloud_delete_save(const char *mode, const char *name, const char *password);
int cloud_register_match(const char *type, const cloud_match *match);
int cloud_read_ranking(const char *type, int limit);

/** @brief Guarda na fila a gravação nova de um save e manda subir (mesmo sem conexão). */
void cloud_store_save(const char *mode, const char *name, const char *content);

/** @brief O save saiu deste dispositivo: esquece a fila e a sincronização dele. */
void cloud_forget_save(const char *mode, const char *name);

/** @brief Sobe o que falta e libera o save aberto. */
void cloud_close_save(void);

/** @brief true quando a resposta chegou (uma vez só). Liberar com cloud_result_free. */
bool cloud_poll(int ticket, cloud_result *out);

void cloud_result_free(cloud_result *result);

/** @brief Próximo aviso para o jogador (conflito ou save perdido). */
bool cloud_poll_notice(cloud_notice *out);

#endif // CLOUD_H
