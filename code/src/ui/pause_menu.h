#ifndef PAUSE_MENU_H
#define PAUSE_MENU_H

#include <stdbool.h>

/*
 * Menu de pausa da campanha: congela o jogo na imagem em que estava e oferece CONTINUAR,
 * COMO JOGAR, VOLTAR AO MENU e SAIR DO JOGO. Sair pede confirmação; voltar ao menu salva antes.
 */

typedef enum
{
    PAUSE_NONE,      // Segue pausado
    PAUSE_RESUME,    // Voltar ao jogo
    PAUSE_TO_MENU,   // Voltar ao menu principal
    PAUSE_QUIT       // Fechar o jogo
} pause_action;

/** @brief Pausa. Chamar com o quadro já desenhado, entre begin e end do desenho: a imagem vira o fundo da pausa. */
void pause_menu_open(void);

/** @brief Se o jogo está pausado. */
bool pause_menu_is_open(void);

/** @brief Lê as teclas e o mouse e desenha a pausa. Chamar entre begin e end do desenho. */
pause_action pause_menu_update_and_draw(void);

/** @brief Fecha a pausa e solta a imagem de fundo. */
void pause_menu_close(void);

#endif // PAUSE_MENU_H
