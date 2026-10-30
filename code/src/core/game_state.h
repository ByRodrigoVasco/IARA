#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "macros.h"

// Quantidade total de dias da campanha (Dia 1 ao Dia 100).
#define TOTAL_GAME_DAYS 100

// Estado geral da partida.
struct game_state_variables {
    int current_screen;      // Tela atual (PRE_MENU_SCREEN, MENU_SCREEN, ...)
    int current_day;         // Dia atual do jogo
    int day_type;            // Tipo do dia (PLAYABLE_DAY, FLASH_DAY, JUMP_DAY)
    bool mode_type;          // Modo de resolução escolhido (manual ou automático)
    bool user_investigated;  // Se o jogador investigou antes de responder
    bool is_resolved;        // Se a tarefa do dia foi resolvida

    // Puzzles já resolvidos na campanha. O índice é o número do dia
    // (resolved_puzzles[1] = Dia 1); a posição 0 não é usada.
    bool resolved_puzzles[TOTAL_GAME_DAYS + 1];
};

// Estatísticas ocultas que definem o final do jogo.
struct hidden_statistic {
    signed int current_overall_points;
    unsigned int current_automation_points;
    unsigned int current_perception_points;
    unsigned int current_human_bond_points;
};

extern struct game_state_variables current_game_state_variables;
extern struct hidden_statistic current_hidden_statistic;

void initialize_game_state_variables(void);
void initialize_hidden_statistic(void);

#endif