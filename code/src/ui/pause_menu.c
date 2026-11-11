#include <stddef.h>

#include "raylib.h"
#include "rlgl.h"
#include "pause_menu.h"
#include "confirm_dialog.h"
#include "ui_theme.h"
#include "fonts.h"
#include "helper.h"
#include "accessibility.h"
#include "macros.h"

#define PAUSE_OPTION_COUNT 3
#define PAUSE_RESUME_OPTION 0
#define PAUSE_MENU_OPTION 1
#define PAUSE_QUIT_OPTION 2

static bool pause_open = false;
static int pause_selected = 0;
static int pause_skip_frames = 0;
static Texture2D pause_background = {0};

void pause_menu_open(void)
{
    rlDrawRenderBatchActive();   // Sem isto a imagem não traz o que foi desenhado neste quadro
    Image snapshot = LoadImageFromScreen();

    if (pause_background.id != 0) UnloadTexture(pause_background);
    pause_background = snapshot.data != NULL ? LoadTextureFromImage(snapshot) : (Texture2D){0};
    UnloadImage(snapshot);

    pause_open = true;
    pause_selected = PAUSE_RESUME_OPTION;
    pause_skip_frames = 1;
}

bool pause_menu_is_open(void)
{
    return pause_open;
}

void pause_menu_close(void)
{
    pause_open = false;
    if (pause_background.id != 0) UnloadTexture(pause_background);
    pause_background = (Texture2D){0};
    confirm_close();
    SetMouseCursor(MOUSE_CURSOR_ARROW);
}

static pause_action choose(int option)
{
    if (option == PAUSE_RESUME_OPTION) return PAUSE_RESUME;
    if (option == PAUSE_MENU_OPTION) return PAUSE_TO_MENU;

    confirm_open("SAIR PARA A ÁREA DE TRABALHO?", "O jogo fecha. O seu progresso até aqui é salvo antes de fechar.", "SAIR", "CONTINUAR", true);
    return PAUSE_NONE;
}

pause_action pause_menu_update_and_draw(void)
{
    static const char *labels[PAUSE_OPTION_COUNT] = {"CONTINUAR", "SAIR PARA O MENU", "SAIR PARA A ÁREA DE TRABALHO"};
    float scale = get_ui_scale();
    float width = (float)GetScreenWidth();
    float height = (float)GetScreenHeight();
    float button_width = 460.0f * scale;
    float button_height = 70.0f * scale;
    float top = height * 0.36f;
    Rectangle buttons[PAUSE_OPTION_COUNT];
    pause_action action = PAUSE_NONE;
    int hovered = -1;

    if (!pause_open) return PAUSE_NONE;

    for (int i = 0; i < PAUSE_OPTION_COUNT; i++)
    {
        buttons[i] = (Rectangle){(width - button_width) / 2.0f, top + (float)i * (button_height + 16.0f * scale), button_width, button_height};
        if (ui_mouse_over(buttons[i])) hovered = i;
    }

    // Fundo: a última imagem do jogo, escurecida
    ClearBackground(BG_COLOR);
    if (pause_background.id != 0)
    {
        DrawTexturePro(pause_background, (Rectangle){0, 0, (float)pause_background.width, (float)pause_background.height}, (Rectangle){0, 0, width, height}, (Vector2){0, 0}, 0.0f, WHITE);
    }
    ui_draw_dim();

    if (confirm_is_open())
    {
        confirm_result answer;

        // Os botões ficam desenhados por baixo da janela
        ui_draw_centered("PAUSADO", width / 2.0f, height * 0.14f, ui_font_size(3.6f), UI_BLOOD_COLOR);
        for (int i = 0; i < PAUSE_OPTION_COUNT; i++) ui_draw_button(buttons[i], labels[i], i == pause_selected, i == PAUSE_QUIT_OPTION);
        answer = confirm_update_and_draw();
        if (answer == CONFIRM_YES) return PAUSE_QUIT;
        return PAUSE_NONE;
    }

    if (pause_skip_frames > 0)
    {
        pause_skip_frames--;
    }
    else
    {
        if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) pause_selected = (pause_selected + 1) % PAUSE_OPTION_COUNT;
        if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) pause_selected = (pause_selected + PAUSE_OPTION_COUNT - 1) % PAUSE_OPTION_COUNT;
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) action = PAUSE_RESUME;
        else if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) action = choose(pause_selected);

        if (hovered >= 0)
        {
            Vector2 delta = GetMouseDelta();

            if (delta.x != 0.0f || delta.y != 0.0f) pause_selected = hovered;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                pause_selected = hovered;
                action = choose(hovered);
            }
        }
    }

    SetMouseCursor(hovered >= 0 ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_ARROW);

    ui_draw_centered("PAUSADO", width / 2.0f, height * 0.14f, ui_font_size(3.6f), UI_BLOOD_COLOR);
    for (int i = 0; i < PAUSE_OPTION_COUNT; i++) ui_draw_button(buttons[i], labels[i], i == pause_selected && !confirm_is_open(), i == PAUSE_QUIT_OPTION);
    if (confirm_is_open()) confirm_update_and_draw();   // Abriu neste quadro: a janela já aparece (a primeira leitura de teclas é pulada)
    ui_draw_hint("SETAS: escolher    ENTER: confirmar    ESC ou P: continuar");

    return action;
}
