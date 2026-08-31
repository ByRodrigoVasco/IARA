#include "raylib.h"

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


    while (!WindowShouldClose()) {
        BeginDrawing();
            ClearBackground(RAYWHITE);

            DrawText(text_in_middle, text_posX, text_posY, text_in_middle_font_size, GRAY);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}