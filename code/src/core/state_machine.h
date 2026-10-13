#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

/**
 * @brief Prepara o estado inicial da máquina (carrega as texturas do pré-menu).
 */
void handle_state_initialization(void);

/**
 * @brief Executa um quadro da máquina de estados: renderiza a tela atual
 * (pré-menu, menu ou jogo) e decide a próxima tela.
 */
void handle_state_machine(void);

#endif // STATE_MACHINE_H
