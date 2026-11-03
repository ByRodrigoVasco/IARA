#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <stdbool.h>

extern bool is_game_to_close; // true quando o jogador escolheu SAIR no menu; a main.c encerra o loop

/**
 * @brief Executa a opção escolhida no menu e informa a próxima tela.
 *
 * @param screen Opção do menu (INICIAR, CREDITOS, CONFIGURACOES ou SAIR).
 * @return Próxima tela, ou NONE.
 */
int handle_menu_action(int screen);

/**
 * @brief Prepara o estado inicial da máquina (carrega as texturas do pré-menu).
 */
void handle_state_initialization(void);

/**
 * @brief Executa um quadro da máquina de estados: renderiza a tela atual
 * (pré-menu, menu ou jogo) e decide a próxima tela.
 */
void handle_state_machine(void);

/**
 * @brief Informa se a tecla ESC fecha o jogo na tela atual. Créditos e configurações usam ESC para voltar ao menu.
 */
bool escape_closes_current_screen(void);

/**
 * @brief Libera as texturas da tela aberta. A main.c chama ao encerrar o jogo.
 */
void handle_state_shutdown(void);

#endif // STATE_MACHINE_H
