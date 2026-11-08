#ifndef MODE_SELECT_H
#define MODE_SELECT_H

#include "save_slots.h"

/*
 * Tela de escolha do modo de jogo, aberta pelo INICIAR do menu:
 * MODO NORMAL (a campanha) ou MODO SOBREVIVÊNCIA.
 */

#define MODE_SELECT_NONE 0     // Continua na tela
#define MODE_SELECT_BACK 1     // Voltar ao menu
#define MODE_SELECT_CHOSEN 2   // Um modo foi escolhido (veja mode_select_chosen_mode)

/** @brief Prepara a tela. Chamar sempre que ela for aberta. */
void initialize_mode_select(void);

/** @brief Um quadro da tela. Devolve MODE_SELECT_NONE, MODE_SELECT_BACK ou MODE_SELECT_CHOSEN. */
int render_mode_select(void);

/** @brief Modo escolhido, depois de render_mode_select() devolver MODE_SELECT_CHOSEN. */
save_mode mode_select_chosen_mode(void);

/** @brief Escolhe um modo, como se o jogador tivesse confirmado a opção. Usado pela tela e pelos testes. */
void mode_select_choose(save_mode mode);

/** @brief Volta ao menu, como se o jogador tivesse escolhido VOLTAR. Usado pelos testes. */
void mode_select_go_back(void);

#endif // MODE_SELECT_H
