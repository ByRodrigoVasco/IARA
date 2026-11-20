#ifndef COMPUTER_LOOP_H
#define COMPUTER_LOOP_H

#include "game_state.h"

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

/* Checkpoint alterado: a máquina principal o grava no slot ativo. */
extern bool computer_checkpoint_dirty;
void computer_suspend_input(void);

/* Dia 1: resultados para o coordenador e checkpoint sem dependência de desenho. */
int computer_first_outcome(void);
bool computer_mostly_unchecked(void);
void computer_store_checkpoint(void);
bool computer_restore_checkpoint(void);
bool computer_checkpoint_finished(void);
bool computer_day_three_first_retry(void);
int computer_story_day_choice(void);
bool computer_story_middle_pending(void);
TaskOutcome computer_story_middle_reaction(void);
void computer_complete_story_middle(void);
bool computer_day_three_lunch_accepted(void);
bool computer_day_three_choice_recorded(void);
bool computer_first_answer_correct(void);
bool computer_first_exposed_personal_data(void);

#endif // COMPUTER_LOOP_H
