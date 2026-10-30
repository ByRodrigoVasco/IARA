#ifndef SAVE_H
#define SAVE_H

#include <stdbool.h>

/*
 * Persistência local da partida (US01: save automático ao fechar).
 *
 * O save é um arquivo de texto simples, uma informação por linha, no formato
 * "chave=valor". A primeira linha identifica o formato e a versão, e a última
 * guarda um checksum de tudo o que vem antes. Assim dá para abrir o arquivo
 * num editor para depurar, e um save truncado ou editado é detectado e
 * descartado sem travar o jogo.
 *
 * Exemplo:
 *
 *   IARA_SAVE v1
 *   current_day=12
 *   day_type=1
 *   mode_type=0
 *   user_investigated=1
 *   is_resolved=0
 *   overall_points=4
 *   automation_points=1
 *   perception_points=3
 *   human_bond_points=2
 *   resolved_puzzles=11111111111000...0
 *   checksum=9f3a1c07
 */

// Arquivo de save, relativo à pasta de onde o jogo é executado (a raiz do
// repositório, a mesma usada para carregar os assets). Já está no .gitignore.
#define SAVE_FILE_PATH "iara.save"

// Primeira linha do arquivo. Mudar a versão quando o formato mudar.
#define SAVE_FILE_HEADER "IARA_SAVE v1"

typedef enum
{
    SAVE_OK = 0,        // Operação concluída
    SAVE_NOT_FOUND,     // Não existe arquivo de save (primeira partida)
    SAVE_CORRUPTED,     // Arquivo existe, mas está incompleto, alterado ou inválido
    SAVE_IO_ERROR       // Falha ao ler ou gravar o arquivo no disco
} save_result;

/**
 * @brief Grava current_game_state_variables e current_hidden_statistic em
 * path. A gravação é feita primeiro num arquivo temporário, que só substitui
 * o save antigo quando está completo: se o jogo cair no meio, o save anterior
 * continua intacto.
 */
save_result save_game_state(const char *path);

/**
 * @brief Lê o save em path e, se ele for válido, restaura
 * current_game_state_variables e current_hidden_statistic.
 *
 * Se o arquivo não existir ou estiver corrompido, NADA é alterado: o estado
 * continua o que estava (normalmente o de uma partida nova, no Dia 1).
 * current_screen nunca é restaurado, o jogo sempre abre pelo pré-menu.
 */
save_result load_game_state(const char *path);

/**
 * @brief Texto curto que descreve um save_result, para os logs.
 */
const char *save_result_to_string(save_result result);

#endif // SAVE_H
