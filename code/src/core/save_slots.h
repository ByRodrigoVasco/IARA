#ifndef SAVE_SLOTS_H
#define SAVE_SLOTS_H

#include <stdbool.h>
#include <stddef.h>

/*
 * Saves em pastas. Cada jogador tem o seu save, e cada save tem a sua pasta:
 *
 *   saves/
 *     normal/
 *       <nome do save>/progresso.save
 *     sobrevivencia/
 *       <nome do save>/progresso.survival
 *
 * Os dois modos têm saves separados, e cada modo pode ter vários. Dentro de um
 * modo, o nome de um save não pode se repetir (sem diferenciar maiúsculas de
 * minúsculas, porque o Windows não diferencia). A pasta "saves" fica ao lado do
 * executável, como o resto dos arquivos do jogo.
 *
 * Este módulo só cuida de pastas e nomes. O que vai dentro do arquivo de
 * progresso é com save.c (modo normal) e sv_meta.c (modo sobrevivência).
 */

#define SAVES_ROOT_DEFAULT "saves"

#define SAVE_NAME_MAX 20     // Caracteres do nome de um save
#define SAVE_SLOTS_MAX 32    // Saves listados por modo

typedef enum
{
    SAVE_MODE_NORMAL = 0,
    SAVE_MODE_SURVIVAL,
    SAVE_MODE_COUNT
} save_mode;

typedef struct
{
    char name[SAVE_NAME_MAX + 1];
    long modified;           // Quando o progresso foi gravado pela última vez (segundos desde 1970, 0 se nunca)
} save_slot_info;

typedef enum
{
    SAVE_NAME_OK = 0,
    SAVE_NAME_EMPTY,         // Vazio
    SAVE_NAME_TOO_LONG,      // Passa de SAVE_NAME_MAX
    SAVE_NAME_INVALID,       // Tem caractere que não pode (só letras, números, espaço, - e _)
    SAVE_NAME_RESERVED,      // Nome que o Windows não aceita como pasta (CON, NUL, COM1...)
    SAVE_NAME_EXISTS         // Já existe um save com esse nome neste modo
} save_name_check;

/** @brief Muda a pasta raiz dos saves (os testes usam uma pasta temporária). NULL volta ao padrão. */
void save_slots_set_root(const char *root);

/** @brief Pasta raiz dos saves. */
const char *save_slots_root(void);

/** @brief Nome da pasta do modo ("normal" ou "sobrevivencia"). */
const char *save_mode_folder(save_mode mode);

/** @brief Nome do modo para mostrar na tela ("MODO NORMAL" ou "MODO SOBREVIVÊNCIA"). */
const char *save_mode_title(save_mode mode);

/** @brief Cria saves/normal e saves/sobrevivencia se ainda não existirem. */
bool save_slots_ensure_folders(void);

/** @brief Lista os saves do modo, do mais recente para o mais antigo. Devolve quantos achou (até max). */
int save_slots_list(save_mode mode, save_slot_info *out, int max);

/** @brief Se já existe um save com esse nome neste modo (ignora maiúsculas). */
bool save_slot_exists(save_mode mode, const char *name);

/** @brief Tira os espaços do começo e do fim do nome, no próprio texto. */
void save_name_trim(char *name);

/** @brief Confere se o nome serve para um save novo neste modo. O nome é conferido sem os espaços das pontas. */
save_name_check save_slot_check_name(save_mode mode, const char *name);

/** @brief Mensagem curta, em português, para mostrar ao jogador. Vazia se o nome estiver ok. */
const char *save_name_check_message(save_name_check result);

/** @brief Cria a pasta do save. Falha se o nome não for aceito. */
bool save_slot_create(save_mode mode, const char *name);

/** @brief Apaga o save inteiro: os arquivos e a pasta. Só mexe dentro de saves/<modo>/. */
bool save_slot_delete(save_mode mode, const char *name);

/** @brief Monta o caminho do arquivo de progresso dentro da pasta do save. Falha se o nome tiver caracteres perigosos. */
bool save_slot_path(save_mode mode, const char *name, char *buffer, size_t size);

/** @brief Se o save já tem arquivo de progresso (ou só a pasta, de um save recém-criado). */
bool save_slot_has_progress(save_mode mode, const char *name);

/** @brief Resumo do progresso para a lista ("Dia 12", "340 horas, 6 partidas"). "Sem progresso" se não houver. */
void save_slot_summary(save_mode mode, const char *name, char *buffer, size_t size);

/** @brief Escolhe o save que a partida atual usa. */
void active_save_set(save_mode mode, const char *name);

/** @brief Nenhum save ativo (fora de uma partida). */
void active_save_clear(void);

bool active_save_is_set(void);
save_mode active_save_mode(void);
const char *active_save_name(void);

/** @brief Caminho do arquivo de progresso do save ativo. Falso se não houver save ativo. */
bool active_save_path(char *buffer, size_t size);

/**
 * @brief Leva os saves antigos (iara.save e iara.survival, soltos ao lado do executável) para a pasta nova.
 *
 * Cada um vira o save "Principal" do seu modo, mas só se o modo ainda não tiver nenhum save.
 * Devolve quantos arquivos foram levados.
 */
int save_slots_migrate_legacy(void);

/** @brief O mesmo, procurando os arquivos antigos em outra pasta (os testes usam uma pasta temporária). */
int save_slots_migrate_legacy_from(const char *folder);

#endif // SAVE_SLOTS_H
