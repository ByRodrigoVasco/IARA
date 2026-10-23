#include "raylib.h"
#include "credits.h"
#include "fonts.h"
#include "macros.h"

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

    float font_size = screen_height * 0.036f;
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
    Rectangle back_button = {
        screen_width * 0.34f,
        screen_height * 0.87f,
        screen_width * 0.32f,
        screen_height * 0.1f};
    Vector2 mouse_position = GetMousePosition();
    bool is_back_button_hovered = IsWindowFocused() && CheckCollisionPointRec(mouse_position, back_button);
    bool is_back_button_clicked = is_back_button_hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    float button_font_size = screen_height * 0.04f;
    const char *button_text = "VOLTAR AO MENU";
    float button_letter_spacing = button_font_size * 0.06f;
    Vector2 button_text_size = MeasureTextEx(default_game_font, button_text, button_font_size, button_letter_spacing);
    Vector2 button_text_position = {
        back_button.x + (back_button.width - button_text_size.x) / 2,
        back_button.y + (back_button.height - button_text_size.y) / 2};

    SetMouseCursor(is_back_button_hovered ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_ARROW);

    BeginDrawing();
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

    DrawRectangleRounded(back_button, 0.25f, 8, is_back_button_hovered ? (Color){96, 64, 96, 255} : (Color){48, 32, 48, 255});
    DrawTextEx(default_game_font, button_text, button_text_position, button_font_size, button_letter_spacing, WHITE);

    EndDrawing();

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
