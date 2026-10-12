#include <stdio.h>

#include "state_machine.h"
#include "game_state.h"
#include "pre_menu.h"
#include "menu.h"
#include "fonts.h"

const char *title = "IARA";

int main(void)
{
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, IARA_TITLE);

    SetTargetFPS(60);

    initialize_game_state_variables();
    initialize_hidden_statistic();

    initialize_fonts();

    initialize_menu_variables();
    handle_state_initialization();

    current_game_state_variables.current_screen = PRE_MENU_SCREEN;

    while (!WindowShouldClose())
    {
        handle_state_machine();
    }

    unload_fonts();
    CloseWindow();
    return 0;
}