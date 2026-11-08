#include "raylib.h"
#include "mode_select.h"
#include "accessibility.h"
#include "screen_style.h"
#include "macros.h"

// Opções da lista: os dois modos e o botão de voltar
#define OPTION_NORMAL 0
#define OPTION_SURVIVAL 1
#define OPTION_BACK 2
#define OPTION_COUNT 3

static int mode_selected_option = OPTION_NORMAL;
static int mode_select_result = MODE_SELECT_NONE;
static save_mode mode_chosen = SAVE_MODE_NORMAL;

void initialize_mode_select(void)
{
    mode_selected_option = OPTION_NORMAL;
    mode_select_result = MODE_SELECT_NONE;
}

void mode_select_choose(save_mode mode)
{
    mode_chosen = mode;
    mode_select_result = MODE_SELECT_CHOSEN;
    SetMouseCursor(MOUSE_CURSOR_ARROW);
}

void mode_select_go_back(void)
{
    mode_select_result = MODE_SELECT_BACK;
}

save_mode mode_select_chosen_mode(void)
{
    return mode_chosen;
}

static void confirm_option(int option)
{
    if (option == OPTION_NORMAL) mode_select_choose(SAVE_MODE_NORMAL);
    else if (option == OPTION_SURVIVAL) mode_select_choose(SAVE_MODE_SURVIVAL);
    else
    {
        mode_select_result = MODE_SELECT_BACK;
        SetMouseCursor(MOUSE_CURSOR_ARROW);
    }
}

int render_mode_select(void)
{
    static const char *labels[OPTION_COUNT] = {"MODO NORMAL", "MODO SOBREVIVÊNCIA", "VOLTAR"};
    static const char *details[OPTION_COUNT] = {"A história da IARA: dias de trabalho, enigmas no computador e a verdade sobre ela.",
                                                "Aguente 10 minutos contra os pesadelos da empresa, escolha poderes e derrote a IARA.", ""};
    Rectangle areas[OPTION_COUNT];
    int hovered = -1;
    float y;

    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) mode_selected_option = (mode_selected_option + 1) % OPTION_COUNT;
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) mode_selected_option = (mode_selected_option + OPTION_COUNT - 1) % OPTION_COUNT;
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) confirm_option(mode_selected_option);
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) confirm_option(OPTION_BACK);

    begin_game_drawing();
    style_backdrop();
    style_title("ESCOLHA O MODO", NULL);

    y = style_list_top();
    for (int i = 0; i < OPTION_COUNT; i++)
    {
        areas[i] = style_row(y, labels[i], details[i], i == mode_selected_option);
        if (style_mouse_over(areas[i])) hovered = i;
        y += style_row_height();
    }

    if (hovered >= 0)
    {
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        if (GetMouseDelta().x != 0.0f || GetMouseDelta().y != 0.0f) mode_selected_option = hovered;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            mode_selected_option = hovered;
            confirm_option(hovered);
        }
    }
    else
    {
        SetMouseCursor(MOUSE_CURSOR_ARROW);
    }

    style_hint("SETAS: escolher    ENTER: confirmar    ESC: voltar");
    end_game_drawing();

    return mode_select_result;
}
