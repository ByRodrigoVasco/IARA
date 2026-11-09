#ifndef HELP_H
#define HELP_H

#include <stdbool.h>

/*
 * Ajuda do jogo: como jogar, em páginas curtas, sempre ao alcance. Abre pelo AJUDA do menu
 * principal e pelo COMO JOGAR do menu de pausa. As teclas aparecem na própria tela.
 */

/** @brief Volta para a primeira página. */
void initialize_help_screen(void);

/** @brief Abre numa página específica (0 campanha, 1 Sobrevivência). Número fora da lista volta à primeira. */
void help_select_page(int page);

/** @brief Tela cheia (a do menu principal). Devolve true quando o jogador pediu para voltar. */
bool render_help_screen(void);

/** @brief Painel por cima de outra tela (a pausa). Devolve true quando o jogador fechou. Chamar entre begin e end do desenho. */
bool help_update_and_draw_panel(void);

/** @brief Quantas páginas a ajuda tem. */
int help_page_count(void);

#endif // HELP_H
