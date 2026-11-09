#include <stdbool.h>

#include "raylib.h"
#include "button.h"
#include "macros.h"
#include "fonts.h"

// Espessura da borda e do relevo do botão
#define BUTTON_BORDER_THICKNESS 1.0f
#define BUTTON_BEVEL_THICKNESS 2.0f

// Cores do botão em cada estado
#define BUTTON_HOVER_COLOR (Color){242, 242, 234, 255}
#define BUTTON_PRESSED_COLOR (Color){170, 172, 168, 255}
#define BUTTON_LIGHT_BEVEL_COLOR WHITE
#define BUTTON_DARK_BEVEL_COLOR (Color){110, 112, 110, 255}
#define BUTTON_TEXT_COLOR BLACK
#define BUTTON_BORDER_COLOR BLACK

void draw_button(Rectangle bounds, const char *text, float font_size, Color fill, bool enabled, bool *is_clicked)
{
    Color current_color = fill;
    Vector2 mouse_point = GetMousePosition();

    // Verifica se o mouse está sobre a área do botão
    if (enabled && CheckCollisionPointRec(mouse_point, bounds))
    {
        current_color = BUTTON_HOVER_COLOR;

        // Mouse pressionado sobre o botão
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            current_color = BUTTON_PRESSED_COLOR;
        }

        // O clique é confirmado quando o botão do mouse é solto sobre a área do botão
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        {
            *is_clicked = true;
        }
    }

    // Renderiza o fundo, o relevo e a borda do botão
    DrawRectangleRec(bounds, current_color);
    DrawRectangleRec((Rectangle){bounds.x, bounds.y, bounds.width, BUTTON_BEVEL_THICKNESS}, BUTTON_LIGHT_BEVEL_COLOR);
    DrawRectangleRec((Rectangle){bounds.x, bounds.y, BUTTON_BEVEL_THICKNESS, bounds.height}, BUTTON_LIGHT_BEVEL_COLOR);
    DrawRectangleRec((Rectangle){bounds.x, bounds.y + bounds.height - BUTTON_BEVEL_THICKNESS, bounds.width, BUTTON_BEVEL_THICKNESS}, BUTTON_DARK_BEVEL_COLOR);
    DrawRectangleRec((Rectangle){bounds.x + bounds.width - BUTTON_BEVEL_THICKNESS, bounds.y, BUTTON_BEVEL_THICKNESS, bounds.height}, BUTTON_DARK_BEVEL_COLOR);
    DrawRectangleLinesEx(bounds, BUTTON_BORDER_THICKNESS, BUTTON_BORDER_COLOR);

    // Centraliza o texto dentro do botão
    float letter_spacing = font_size * BUTTON_LETTER_SPACING_RATIO;
    Vector2 text_size = MeasureTextEx(default_game_font, text, font_size, letter_spacing);

    Vector2 text_position = {
        bounds.x + (bounds.width / 2.0f) - (text_size.x / 2.0f),
        bounds.y + (bounds.height / 2.0f) - (text_size.y / 2.0f)};

    DrawTextEx(default_game_font, text, text_position, font_size, letter_spacing, BUTTON_TEXT_COLOR);
}
