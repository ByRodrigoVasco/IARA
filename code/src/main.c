#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include "game_state.h"
#include "state_machine.h"

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 576

const char *title = "IARA";

int main(void) {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, title);

    SetTargetFPS(60);

    const char *text_in_middle = "Let's begin developing xD";
    int text_in_middle_font_size = 30;

    int textWidth = MeasureText(text_in_middle, text_in_middle_font_size);

    int text_posX = (WINDOW_WIDTH - textWidth) / 2;
    int text_posY = (WINDOW_HEIGHT - text_in_middle_font_size) / 2;

    initialize_game_state_variables();
    initialize_hidden_statistic();

    while (!WindowShouldClose()) {
        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawText(TextFormat("Dia atual: %d", current_game_state_variables.current_day), 512, 288, text_in_middle_font_size, GRAY);
            DrawText(TextFormat("Pontos atuais: %d", current_hidden_statistic.current_overall_points), 400, 200, text_in_middle_font_size, GRAY);
        EndDrawing();

        handle_state_machine();

        current_game_state_variables.current_screen++;

        if (current_game_state_variables.current_screen > JUMPING) {
            current_game_state_variables.current_screen = PRE_MENU;
        }
    }

    CloseWindow();
    return 0;
}