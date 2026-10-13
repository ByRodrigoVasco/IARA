#ifndef COMPUTER_LOOP_H
#define COMPUTER_LOOP_H

#include <stdbool.h>

// true quando o jogador terminou todos os passos do quiz do computador.
extern bool computer_tasks_finished;

/**
 * @brief Carrega as texturas da janela do computador e dos ícones de acerto/erro.
 */
void initialize_computer_loop(void);

/**
 * @brief Executa um quadro da tela do computador (janela, texto e botões do quiz).
 */
void handle_computer_loop(void);

/**
 * @brief Libera as texturas carregadas por initialize_computer_loop().
 */
void unload_computer_loop(void);

#endif // COMPUTER_LOOP_H
