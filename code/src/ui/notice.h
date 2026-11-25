#ifndef NOTICE_H
#define NOTICE_H

#include <stdbool.h>

/*
 * Aviso curto no alto da tela: o jogo diz o que acabou de fazer ("Progresso salvo") ou o que deu
 * errado, com o que fazer a respeito. Some sozinho; o de erro fica mais tempo.
 */

typedef enum
{
    NOTICE_INFO,
    NOTICE_SUCCESS,
    NOTICE_ERROR
} notice_kind;

/** @brief Mostra o aviso, trocando o que estiver na tela. */
void notice_show(const char *text, notice_kind kind);

/** @brief Se há um aviso na tela. */
bool notice_is_active(void);

/** @brief Texto do aviso atual (vazio se não houver). */
const char *notice_text_now(void);

/** @brief Tira o aviso da tela. */
void notice_clear(void);

/** @brief Desenha o aviso. Chamada por end_game_drawing(), então toda tela mostra os avisos. */
void notice_draw(void);

#endif // NOTICE_H
