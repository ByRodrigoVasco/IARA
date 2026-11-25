#ifndef COMPUTER_LOOP_H
#define COMPUTER_LOOP_H

#include <stdbool.h>

// true quando o jogador terminou todos os passos do quiz do computador.
extern bool computer_tasks_finished;

// true quando a tela já desligou e o monitor saiu de cena: o game loop volta ao mapa.
extern bool computer_exit_finished;

// true quando a IARA assumiu a tarefa depois de 3 erros seguidos: o expediente acaba.
extern bool computer_failsafe_triggered;

// false deixa o mouse livre: os testes rodam sem prender o ponteiro de quem os executa.
extern bool computer_mouse_capture_enabled;

// Diz de quem é a dica do dia (nome do NPC) ou NULL. A tela do computador só a anuncia, de leve,
// quando o jogador erra ou demora: a dica em si é dada pelo NPC, no escritório.
void computer_set_hint_giver(const char *npc_name);

// Andamento da entrada na cena do computador, de 0 (escritório) a 1 (monitor no lugar).
extern float computer_scene_progress;

/**
 * @brief Volta os chamados ao começo, para uma partida nova.
 */
void reset_computer_progress(void);

/**
 * @brief Carrega as texturas da janela do computador e dos ícones de acerto/erro.
 * Os chamados continuam de onde o jogador parou.
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
