#ifndef CONFIRM_DIALOG_H
#define CONFIRM_DIALOG_H

#include <stdbool.h>

/*
 * Janela de confirmação para ações difíceis de desfazer (sair do jogo, desistir da partida).
 * O botão que cancela já vem escolhido, ESC também cancela, e o texto diz o que vai acontecer
 * e o que fica salvo.
 */

typedef enum
{
    CONFIRM_PENDING,   // Ainda aberta, sem resposta
    CONFIRM_YES,
    CONFIRM_NO
} confirm_result;

/** @brief Abre a janela. A tecla que a abriu neste quadro não vale como resposta. */
void confirm_open(const char *title, const char *message, const char *yes_label, const char *no_label, bool danger);

/** @brief Se a janela está aberta. */
bool confirm_is_open(void);

/** @brief Lê as teclas e o mouse e desenha a janela por cima do quadro. Chamar entre begin e end do desenho. */
confirm_result confirm_update_and_draw(void);

/** @brief Fecha sem resposta. */
void confirm_close(void);

#endif // CONFIRM_DIALOG_H
