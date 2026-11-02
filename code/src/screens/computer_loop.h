#ifndef COMPUTER_LOOP_H
#define COMPUTER_LOOP_H

#include <stdbool.h>

// true quando o jogador terminou todos os passos do quiz do computador.
extern bool computer_tasks_finished;

// false deixa o mouse livre: os testes rodam sem prender o ponteiro de quem os executa.
extern bool computer_mouse_capture_enabled;

// Andamento da entrada na cena do computador, de 0 (escritório) a 1 (monitor no lugar).
extern float computer_scene_progress;

/**
 * @brief Carrega as texturas da janela do computador e dos ícones de acerto/erro.
 */
void initialize_computer_loop(void);

/**
 * @brief Executa um quadro da tela do computador (mesa, monitor, desktop e quiz).
 */
void handle_computer_loop(void);

/**
 * @brief Libera as texturas carregadas por initialize_computer_loop().
 */
void unload_computer_loop(void);

#endif // COMPUTER_LOOP_H
