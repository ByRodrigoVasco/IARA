#include "raylib.h"
#include "credits.h"
#include "fonts.h"
#include "macros.h"
#include "accessibility.h"
#include "ui_theme.h"
#include "helper.h"

#define CREDITS_MEMBER_COUNT 7
#define CREDITS_TOP_ROW_COUNT 5

typedef struct
{
    const char *name;
    const char *image_path;
    Texture2D texture;
} credits_member;

static credits_member members[CREDITS_MEMBER_COUNT] = {
    {"Laura Brayner", LAURA_BRAYNER_CREDITS_PATH, {0}},
    {"Rodrigo Vasco", RODRIGO_VASCO_CREDITS_PATH, {0}},
    {"Anderson Campos", ANDERSON_SANTOS_CREDITS_PATH, {0}},
    {"João Vitor", JOAO_VITOR_CREDITS_PATH, {0}},
    {"Luan Arruda", LUAN_ARRUDA_CREDITS_PATH, {0}},
    {"Nicollas Mauricio", NICOLAS_MAURICIO_CREDITS_PATH, {0}},
    {"Julio Lins", JULIO_LINS_CREDITS_PATH, {0}},
};

static void draw_member(const credits_member *member, float center_x, float top_y, float max_height, float max_width)
{
    float scale = max_height / member->texture.height;
    float width_scale = max_width / member->texture.width;

    if (width_scale < scale)
    {
        scale = width_scale;
    }

    float image_width = member->texture.width * scale;
    float image_height = member->texture.height * scale;
    Rectangle source = {0, 0, (float)member->texture.width, (float)member->texture.height};
    Rectangle destination = {center_x - image_width / 2, top_y, image_width, image_height};
    Vector2 origin = {0, 0};

    DrawTexturePro(member->texture, source, destination, origin, 0, WHITE);

    // O nome precisa caber na coluna também em janelas estreitas
    float font_size = screen_height * 0.036f;
    if (font_size > screen_width * 0.02f) font_size = screen_width * 0.02f;
    float letter_spacing = font_size * 0.06f;
    Vector2 text_size = MeasureTextEx(default_game_font, member->name, font_size, letter_spacing);
    Vector2 text_position = {center_x - text_size.x / 2, top_y + image_height + screen_height * 0.004f};

    DrawTextEx(default_game_font, member->name, text_position, font_size, letter_spacing, WHITE);
}

void initialize_credits_screen(void)
{
    for (int index = 0; index < CREDITS_MEMBER_COUNT; index++)
    {
        members[index].texture = LoadTexture(members[index].image_path);
    }
}

bool render_credits_screen(void)
{
    // Botão no mesmo lugar e no mesmo estilo do VOLTAR da escolha de modo: acima da dica do rodapé, sem encostar nela
    float scale = get_ui_scale();
    float window_width = (float)GetScreenWidth();
    float window_height = (float)GetScreenHeight();
    Rectangle back_button = {(window_width - 360.0f * scale) / 2.0f, window_height - 135.0f * scale, 360.0f * scale, 62.0f * scale};
    bool is_back_button_hovered = ui_mouse_over(back_button);
    bool is_back_button_clicked = is_back_button_hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    SetMouseCursor(is_back_button_hovered ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_ARROW);

    begin_game_drawing();
    ClearBackground(BG_COLOR);

    float column_width = screen_width / (float)CREDITS_TOP_ROW_COUNT;
    float top_row_y = screen_height * 0.055f;
    float top_row_height = screen_height * 0.355f;
    float bottom_row_y = screen_height * 0.505f;
    float bottom_row_height = screen_height * 0.31f;
    float max_member_width = screen_width * 0.17f;

    for (int index = 0; index < CREDITS_TOP_ROW_COUNT; index++)
    {
        draw_member(&members[index], column_width * (index + 0.5f), top_row_y, top_row_height, max_member_width);
    }

    float bottom_row_spacing = screen_width * 0.2f;
    for (int index = CREDITS_TOP_ROW_COUNT; index < CREDITS_MEMBER_COUNT; index++)
    {
        int bottom_row_index = index - CREDITS_TOP_ROW_COUNT;
        float horizontal_offset = bottom_row_index == 0 ? -bottom_row_spacing / 2 : bottom_row_spacing / 2;
        float center_x = screen_width / 2.0f + horizontal_offset;
        draw_member(&members[index], center_x, bottom_row_y, bottom_row_height, max_member_width);
    }

    ui_draw_button(back_button, "VOLTAR AO MENU", true, false);   // Única ação da tela: sempre destacado

    ui_draw_hint("ESC, ENTER ou ESPAÇO: voltar");

    end_game_drawing();

    return is_back_button_clicked || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
}

void unload_credits_screen(void)
{
    for (int index = 0; index < CREDITS_MEMBER_COUNT; index++)
    {
        UnloadTexture(members[index].texture);
        members[index].texture = (Texture2D){0};
    }
}
