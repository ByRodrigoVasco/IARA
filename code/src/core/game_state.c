#include "game_state.h"

// Variável de estado atual do jogo
struct game_state_variables current_game_state_variables;

// Variável de estatísticas ocultas do jogo
struct hidden_statistic current_hidden_statistic;

void initialize_game_state_variables(void)
{
    current_game_state_variables.current_day = 1;
    current_game_state_variables.day_type = PLAYABLE_DAY;
    current_game_state_variables.mode_type = false;
    current_game_state_variables.user_investigated = false;
    current_game_state_variables.is_resolved = false;
}

void initialize_hidden_statistic(void)
{
    current_hidden_statistic.current_overall_points = 1;
    current_hidden_statistic.current_automation_points = 1;
    current_hidden_statistic.current_perception_points = 1;
    current_hidden_statistic.current_human_bond_points = 1;
}