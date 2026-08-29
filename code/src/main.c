#include "raylib.h"

#define WINDOW_HEIGHT 576
#define WINDOW_WIDTH 1024

#define CAMINHO_ASSETS_PADRAO "C:/VSCode - Geral/ADS - Training/Projetos/IARA/engine-c/assets"

const char *title = "IARA";

int main(void) {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, title);

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("IARA is ready to be developed :)", WINDOW_HEIGHT / 2, WINDOW_HEIGHT / 2, 20, LIGHTGRAY);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}