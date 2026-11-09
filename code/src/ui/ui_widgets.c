#include <stdio.h>
#include <string.h>

#include "raylib.h"
#include "ui_theme.h"
#include "fonts.h"
#include "helper.h"
#include "macros.h"

static float spacing_for(float size)
{
    return size * 0.06f;
}

float ui_font_size(float ratio)
{
    return DEFAULT_TEXT_FONT_SIZE * ratio * get_ui_scale();
}

bool ui_mouse_over(Rectangle area)
{
    return IsWindowFocused() && CheckCollisionPointRec(GetMousePosition(), area);
}

void ui_draw_backdrop(void)
{
    float width = (float)GetScreenWidth();
    float height = (float)GetScreenHeight();

    ClearBackground(BG_COLOR);
    DrawCircleGradient((Vector2){width * 0.5f, height * 0.5f}, width * 0.62f, (Color){36, 18, 40, 255}, BG_COLOR);
}

void ui_draw_dim(void)
{
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){0, 0, 0, 170});
}

void ui_draw_panel(Rectangle bounds)
{
    float scale = get_ui_scale();

    DrawRectangleRec((Rectangle){bounds.x + 6.0f * scale, bounds.y + 6.0f * scale, bounds.width, bounds.height}, (Color){0, 0, 0, 120});
    DrawRectangleRec(bounds, UI_PANEL_COLOR);
    DrawRectangleLinesEx(bounds, 3.0f * scale, UI_BLOOD_COLOR);
}

void ui_draw_button(Rectangle bounds, const char *label, bool selected, bool danger)
{
    float size = ui_font_size(1.3f);
    Vector2 measure = MeasureTextEx(default_game_font, label, size, spacing_for(size));
    Color accent = danger ? UI_BLOOD_COLOR : (Color){120, 90, 140, 255};

    DrawRectangleRec(bounds, selected ? accent : (Color){40, 28, 46, 255});
    DrawRectangleLinesEx(bounds, 2.0f * get_ui_scale(), selected ? UI_TEXT_COLOR : (Color){90, 70, 100, 255});
    DrawTextEx(default_game_font, label, (Vector2){bounds.x + (bounds.width - measure.x) / 2.0f, bounds.y + (bounds.height - measure.y) / 2.0f}, size, spacing_for(size), selected ? WHITE : UI_DIM_COLOR);
}

static bool wrap_measure_only = false;   // true: só mede a altura, sem desenhar

float ui_draw_wrapped(const char *text, float x, float y, float width, float size, Color color, bool centered)
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
        char candidate[360];

        while (*cursor == ' ') cursor++;
        while (*cursor != '\0' && *cursor != ' ' && length < (int)sizeof(word) - 1) word[length++] = *cursor++;
        word[length] = '\0';
        if (length == 0) break;

        snprintf(candidate, sizeof(candidate), "%s%s%s", line, line[0] != '\0' ? " " : "", word);
        if (line[0] != '\0' && MeasureTextEx(default_game_font, candidate, size, spacing_for(size)).x > width)
        {
            float line_x = centered ? x + (width - MeasureTextEx(default_game_font, line, size, spacing_for(size)).x) / 2.0f : x;

            if (!wrap_measure_only) DrawTextEx(default_game_font, line, (Vector2){line_x, y + used}, size, spacing_for(size), color);
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
        float line_x = centered ? x + (width - MeasureTextEx(default_game_font, line, size, spacing_for(size)).x) / 2.0f : x;

        if (!wrap_measure_only) DrawTextEx(default_game_font, line, (Vector2){line_x, y + used}, size, spacing_for(size), color);
        used += line_height;
    }
    return used;
}

void ui_draw_centered(const char *text, float center_x, float y, float size, Color color)
{
    Vector2 measure = MeasureTextEx(default_game_font, text, size, spacing_for(size));

    DrawTextEx(default_game_font, text, (Vector2){center_x - measure.x / 2.0f, y}, size, spacing_for(size), color);
}

void ui_draw_hint(const char *text)
{
    float size = ui_font_size(1.0f);
    Vector2 measure = MeasureTextEx(default_game_font, text, size, spacing_for(size));

    DrawTextEx(default_game_font, text, (Vector2){(GetScreenWidth() - measure.x) / 2.0f, GetScreenHeight() - measure.y - TEXT_VERTICAL_MARGIN * get_ui_scale()}, size, spacing_for(size), UI_DIM_COLOR);
}

float ui_measure_wrapped(const char *text, float width, float size)
{
    float used;

    wrap_measure_only = true;
    used = ui_draw_wrapped(text, 0.0f, 0.0f, width, size, WHITE, false);
    wrap_measure_only = false;
    return used;
}
