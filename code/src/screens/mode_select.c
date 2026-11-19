#include "raylib.h"
#include "mode_select.h"
#include "accessibility.h"
#include "screen_style.h"
#include "macros.h"
#include "helper.h"
#include "fonts.h"

// Opções: os três modos, lado a lado, e o botão de voltar embaixo
#define OPTION_NORMAL 0
#define OPTION_SURVIVAL 1
#define OPTION_DUO 2
#define OPTION_BACK 3
#define MODE_CARDS 3

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
    else if (option == OPTION_DUO) mode_select_choose(SAVE_MODE_DUO);
    else
    {
        mode_select_result = MODE_SELECT_BACK;
        SetMouseCursor(MOUSE_CURSOR_ARROW);
    }
}

int render_mode_select(void)
{
    static const char *labels[MODE_CARDS] = {"MODO NORMAL", "MODO SOBREVIVÊNCIA", "SOBREVIVÊNCIA EM DUPLA"};
    static const char *tags[MODE_CARDS] = {"CAMPANHA", "PARTIDAS DE 10 MINUTOS", "2 JOGADORES, MESMO TECLADO"};
    static const char *details[MODE_CARDS] = {"A história da IARA: dias de trabalho, enigmas no computador e a verdade sobre ela.",
                                              "Aguente 10 minutos contra os pesadelos da empresa, escolha poderes e derrote a IARA.",
                                              "O Sobrevivência para dois: WASD e setas. Levante o parceiro quando ele cair. Ranking próprio."};
    static int last_card = OPTION_NORMAL;
    float scale = get_ui_scale();
    float width = (float)GetScreenWidth();
    float height = (float)GetScreenHeight();
    float gap = 48.0f * scale;
    float card_width = (width - gap * (MODE_CARDS + 1)) / MODE_CARDS;
    float card_height = height * 0.40f;
    float top = style_content_top() + (height - style_content_top() - 150.0f * scale - card_height) / 2.0f;
    Rectangle cards[MODE_CARDS];
    Rectangle back_button;
    int hovered = -1;

    if (card_width > 560.0f * scale) card_width = 560.0f * scale;
    if (mode_selected_option != OPTION_BACK) last_card = mode_selected_option;

    // Esquerda e direita passam de um modo para o outro; para baixo vai ao VOLTAR e para cima volta aos modos
    if (mode_selected_option != OPTION_BACK)
    {
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) mode_selected_option = (mode_selected_option + 1) % MODE_CARDS;
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) mode_selected_option = (mode_selected_option + MODE_CARDS - 1) % MODE_CARDS;
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) mode_selected_option = OPTION_BACK;
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) mode_selected_option = last_card;
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) confirm_option(mode_selected_option);
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) confirm_option(OPTION_BACK);

    for (int i = 0; i < MODE_CARDS; i++)
    {
        float row_width = card_width * MODE_CARDS + gap * (MODE_CARDS - 1);

        cards[i] = (Rectangle){(width - row_width) / 2.0f + (float)i * (card_width + gap), top, card_width, card_height};
    }
    back_button = (Rectangle){(width - 260.0f * scale) / 2.0f, height - 135.0f * scale, 260.0f * scale, 62.0f * scale};

    begin_game_drawing();
    style_backdrop();
    style_title_centered("ESCOLHA O MODO", NULL);

    for (int i = 0; i < MODE_CARDS; i++)
    {
        bool selected = (i == mode_selected_option);
        Rectangle area = cards[i];
        float padding = 34.0f * scale;
        float title_size = style_font_size(0.62f);
        float tag_size = style_font_size(0.34f);
        float y;
        Vector2 measure;

        if (selected) area.y -= 12.0f * scale;
        cards[i] = area;
        style_card_frame(area, selected, false);

        y = area.y + padding;
        measure = MeasureTextEx(default_game_font, tags[i], tag_size, tag_size * 0.06f);
        DrawTextEx(default_game_font, tags[i], (Vector2){area.x + (area.width - measure.x) / 2.0f, y}, tag_size, tag_size * 0.06f, selected ? STYLE_WARNING_COLOR : STYLE_DIM_COLOR);
        y += tag_size + 18.0f * scale;
        measure = MeasureTextEx(default_game_font, labels[i], title_size, title_size * 0.06f);

        // Com três cartões lado a lado, o título comprido diminui até caber
        while (measure.x > area.width - padding * 2.0f && title_size > 12.0f)
        {
            title_size *= 0.92f;
            measure = MeasureTextEx(default_game_font, labels[i], title_size, title_size * 0.06f);
        }
        DrawTextEx(default_game_font, labels[i], (Vector2){area.x + (area.width - measure.x) / 2.0f, y}, title_size, title_size * 0.06f, selected ? STYLE_TEXT_COLOR : (Color){200, 192, 206, 255});
        y += title_size + 22.0f * scale;
        DrawRectangle((int)(area.x + padding), (int)y, (int)(area.width - padding * 2.0f), (int)(2.0f * scale), selected ? STYLE_BLOOD_COLOR : (Color){90, 70, 100, 255});
        y += 26.0f * scale;
        style_wrapped_text_centered(details[i], area.x + padding, y, area.width - padding * 2.0f, DEFAULT_TEXT_FONT_SIZE * scale, selected ? STYLE_TEXT_COLOR : STYLE_DIM_COLOR);

        if (style_mouse_over(area)) hovered = i;
    }

    style_button(back_button, "VOLTAR", mode_selected_option == OPTION_BACK, false);
    if (style_mouse_over(back_button)) hovered = OPTION_BACK;

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

    style_hint("ESQUERDA / DIREITA: passar de modo    ENTER: confirmar    ESC: voltar");
    end_game_drawing();

    return mode_select_result;
}
