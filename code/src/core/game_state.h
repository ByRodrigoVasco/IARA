#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "macros.h"

struct game_state_variables {
    int current_screen;
    int current_day;
    int day_type;
    bool mode_type;
    bool user_investigated;
    bool is_resolved;
};

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