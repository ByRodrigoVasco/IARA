#include <stdio.h>
#include <string.h>

#include "raylib.h"
#include "screen_style.h"
#include "fonts.h"
#include "helper.h"
#include "macros.h"

#define ARROW_ICON_PATH "code/assets/icons/arrow-icon.png"
#define ARROW_ICON_SCALE_DIVISOR 2.5f

static Texture2D arrow_texture;

float style_font_size(float ratio)
{
    return DEFAULT_MENU_FONT_SIZE * ratio * get_ui_scale();
}

static float letter_spacing(float size)
{
    return size * 0.06f;
}

bool style_mouse_over(Rectangle area)
{
    return IsWindowFocused() && CheckCollisionPointRec(GetMousePosition(), area);
}

void style_backdrop(void)
{
    float width = (float)GetScreenWidth();
    float height = (float)GetScreenHeight();

    ClearBackground(BG_COLOR);
    // Vinheta: o centro fica um pouco mais claro e roxo, como o fundo do menu
    DrawCircleGradient((Vector2){width * 0.5f, height * 0.5f}, width * 0.62f, (Color){36, 18, 40, 255}, BG_COLOR);
}

void style_title(const char *title, const char *subtitle)
{
    float scale = get_ui_scale();
    float margin_x = TEXT_HORIZONTAL_MARGIN * scale;
    float title_size = style_font_size(1.0f);
    Vector2 title_pos = {margin_x, TEXT_VERTICAL_MARGIN * 1.6f * scale};

    DrawTextEx(default_game_font, title, title_pos, title_size, letter_spacing(title_size), STYLE_TEXT_COLOR);
    if (subtitle != NULL && subtitle[0] != '\0')
    {
        float size = style_font_size(0.42f);

        DrawTextEx(default_game_font, subtitle, (Vector2){margin_x, title_pos.y + title_size + 6.0f * scale}, size, letter_spacing(size), STYLE_DIM_COLOR);
    }
}

float style_row_height(void)
{
    return (style_font_size(0.62f) + style_font_size(0.34f) + 26.0f * get_ui_scale());
}

float style_list_top(void)
{
    float scale = get_ui_scale();

    return TEXT_VERTICAL_MARGIN * 1.6f * scale + style_font_size(1.0f) + style_font_size(0.42f) + 70.0f * scale;
}

Rectangle style_row(float y, const char *label, const char *detail, bool selected)
{
    float scale = get_ui_scale();
    float margin_x = TEXT_HORIZONTAL_MARGIN * scale;
    float label_size = style_font_size(0.62f);
    float detail_size = style_font_size(0.34f);
    Vector2 label_measure = MeasureTextEx(default_game_font, label, label_size, letter_spacing(label_size));
    float row_width = (float)GetScreenWidth() - margin_x * 2.0f;
    Rectangle area = {margin_x, y, row_width, label_size + detail_size + 12.0f * scale};

    if (selected)
    {
        if (!arrow_texture.id) arrow_texture = acquire_smooth_texture(ARROW_ICON_PATH);
        if (arrow_texture.id)
        {
            float arrow_width = (arrow_texture.width / ARROW_ICON_SCALE_DIVISOR) * scale;
            float arrow_height = (arrow_texture.height / ARROW_ICON_SCALE_DIVISOR) * scale;

            DrawTexturePro(arrow_texture, (Rectangle){0, 0, (float)arrow_texture.width, (float)arrow_texture.height},
                           (Rectangle){margin_x - arrow_width - 6.0f * scale, y + (label_size - arrow_height) / 2.0f, arrow_width, arrow_height}, (Vector2){0, 0}, 0.0f, RAYWHITE);
        }
    }

    DrawTextEx(default_game_font, label, (Vector2){margin_x, y}, label_size, letter_spacing(label_size), selected ? STYLE_TEXT_COLOR : (Color){200, 192, 206, 255});
    if (detail != NULL && detail[0] != '\0')
    {
        DrawTextEx(default_game_font, detail, (Vector2){margin_x + 4.0f * scale, y + label_size + 2.0f * scale}, detail_size, letter_spacing(detail_size), STYLE_DIM_COLOR);
    }
    (void)label_measure;
    return area;
}

void style_hint(const char *text)
{
    float scale = get_ui_scale();
    float size = DEFAULT_TEXT_FONT_SIZE * scale;
    Vector2 measure = MeasureTextEx(default_game_font, text, size, letter_spacing(size));

    DrawTextEx(default_game_font, text, (Vector2){(GetScreenWidth() - measure.x) / 2.0f, GetScreenHeight() - measure.y - TEXT_VERTICAL_MARGIN * scale}, size, letter_spacing(size), STYLE_DIM_COLOR);
}

void style_panel(Rectangle bounds)
{
    float scale = get_ui_scale();

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){0, 0, 0, 150});
    DrawRectangleRec((Rectangle){bounds.x + 6.0f * scale, bounds.y + 6.0f * scale, bounds.width, bounds.height}, (Color){0, 0, 0, 120});
    DrawRectangleRec(bounds, STYLE_PANEL_COLOR);
    DrawRectangleLinesEx(bounds, 3.0f * scale, STYLE_BLOOD_COLOR);
}

Rectangle style_button(Rectangle bounds, const char *label, bool selected, bool danger)
{
    float size = style_font_size(0.46f);
    Vector2 measure = MeasureTextEx(default_game_font, label, size, letter_spacing(size));
    Color accent = danger ? STYLE_BLOOD_COLOR : (Color){120, 90, 140, 255};

    DrawRectangleRec(bounds, selected ? accent : (Color){40, 28, 46, 255});
    DrawRectangleLinesEx(bounds, 2.0f * get_ui_scale(), selected ? STYLE_TEXT_COLOR : (Color){90, 70, 100, 255});
    DrawTextEx(default_game_font, label, (Vector2){bounds.x + (bounds.width - measure.x) / 2.0f, bounds.y + (bounds.height - measure.y) / 2.0f}, size, letter_spacing(size), selected ? WHITE : STYLE_DIM_COLOR);
    return bounds;
}

float style_wrapped_text(const char *text, float x, float y, float width, float size, Color color)
{
    char line[256];
    const char *cursor = text;
    float line_height = size * 1.25f;
    float used = 0.0f;

    line[0] = '\0';
    while (*cursor != '\0')
    {
        char word[96];
        int length = 0;
        char candidate[320];

        while (*cursor == ' ') cursor++;
        while (*cursor != '\0' && *cursor != ' ' && length < (int)sizeof(word) - 1) word[length++] = *cursor++;
        word[length] = '\0';
        if (length == 0) break;

        snprintf(candidate, sizeof(candidate), "%s%s%s", line, line[0] != '\0' ? " " : "", word);
        if (line[0] != '\0' && MeasureTextEx(default_game_font, candidate, size, letter_spacing(size)).x > width)
        {
            DrawTextEx(default_game_font, line, (Vector2){x, y + used}, size, letter_spacing(size), color);
            used += line_height;
            snprintf(line, sizeof(line), "%s", word);
        }
        else
        {
            snprintf(line, sizeof(line), "%s", candidate);
        }
    }
    if (line[0] != '\0')
    {
        DrawTextEx(default_game_font, line, (Vector2){x, y + used}, size, letter_spacing(size), color);
        used += line_height;
    }
    return used;
}

void style_progress(float percent, const char *label)
{
    float scale = get_ui_scale();
    float width = (float)GetScreenWidth();
    float height = (float)GetScreenHeight();
    float bar_width = width * 0.5f;
    float bar_height = 26.0f * scale;
    Rectangle bar = {(width - bar_width) / 2.0f, height * 0.62f, bar_width, bar_height};
    char percent_text[16];
    float percent_size = style_font_size(1.15f);
    Vector2 measure;

    if (percent < 0.0f) percent = 0.0f;
    if (percent > 100.0f) percent = 100.0f;

    snprintf(percent_text, sizeof(percent_text), "%d%%", (int)(percent + 0.5f));
    measure = MeasureTextEx(default_game_font, percent_text, percent_size, letter_spacing(percent_size));
    DrawTextEx(default_game_font, percent_text, (Vector2){(width - measure.x) / 2.0f, bar.y - measure.y - 18.0f * scale}, percent_size, letter_spacing(percent_size), STYLE_TEXT_COLOR);

    DrawRectangleRec(bar, (Color){40, 28, 46, 255});
    DrawRectangleRec((Rectangle){bar.x, bar.y, bar.width * percent / 100.0f, bar.height}, STYLE_BLOOD_COLOR);
    DrawRectangleLinesEx(bar, 2.0f * scale, (Color){120, 90, 140, 255});

    if (label != NULL && label[0] != '\0')
    {
        float label_size = DEFAULT_TEXT_FONT_SIZE * scale;
        Vector2 label_measure = MeasureTextEx(default_game_font, label, label_size, letter_spacing(label_size));

        DrawTextEx(default_game_font, label, (Vector2){(width - label_measure.x) / 2.0f, bar.y + bar.height + 14.0f * scale}, label_size, letter_spacing(label_size), STYLE_DIM_COLOR);
    }
}

void style_progress_small(float percent)
{
    float scale = get_ui_scale();
    float width = (float)GetScreenWidth();
    float height = (float)GetScreenHeight();
    Rectangle bar = {width * 0.35f, height - 70.0f * scale, width * 0.30f, 12.0f * scale};
    char text[16];
    float size = DEFAULT_TEXT_FONT_SIZE * 0.8f * scale;
    Vector2 measure;

    if (percent < 0.0f) percent = 0.0f;
    if (percent > 100.0f) percent = 100.0f;
    snprintf(text, sizeof(text), "%d%%", (int)(percent + 0.5f));
    measure = MeasureTextEx(default_game_font, text, size, letter_spacing(size));
    DrawTextEx(default_game_font, text, (Vector2){(width - measure.x) / 2.0f, bar.y - measure.y - 6.0f * scale}, size, letter_spacing(size), STYLE_DIM_COLOR);
    DrawRectangleRec(bar, (Color){40, 28, 46, 255});
    DrawRectangleRec((Rectangle){bar.x, bar.y, bar.width * percent / 100.0f, bar.height}, STYLE_BLOOD_COLOR);
}

void style_unload(void)
{
    if (arrow_texture.id)
    {
        release_smooth_texture(arrow_texture);
        arrow_texture = (Texture2D){0};
    }
}
