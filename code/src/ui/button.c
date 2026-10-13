#include <stdbool.h>

#include "raylib.h"
#include "button.h"
#include "macros.h"
#include "fonts.h"

// Tamanho da fonte do texto do botão
#define BUTTON_FONT_SIZE 20.0f
// Espessura da borda do botão
#define BUTTON_BORDER_THICKNESS 2.0f

// Cores do botão em cada estado
#define BUTTON_NORMAL_COLOR LIGHTGRAY
#define BUTTON_HOVER_COLOR GRAY
#define BUTTON_PRESSED_COLOR DARKGRAY
#define BUTTON_TEXT_COLOR BLACK
#define BUTTON_BORDER_COLOR BLACK

void draw_button(Rectangle bounds, const char *text, bool *is_clicked)
{
    Color current_color = BUTTON_NORMAL_COLOR;
    Vector2 mouse_point = GetMousePosition();

    // Verifica se o mouse está sobre a área do botão
    if (CheckCollisionPointRec(mouse_point, bounds))
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

    // Renderiza o fundo e a borda do botão
    DrawRectangleRec(bounds, current_color);
    DrawRectangleLinesEx(bounds, BUTTON_BORDER_THICKNESS, BUTTON_BORDER_COLOR);

    // Centraliza o texto dentro do botão
    Vector2 text_size = MeasureTextEx(default_game_font, text, BUTTON_FONT_SIZE, DEFAULT_LETTER_SPACING);

    Vector2 text_position = {
        bounds.x + (bounds.width / 2.0f) - (text_size.x / 2.0f),
        bounds.y + (bounds.height / 2.0f) - (text_size.y / 2.0f)};

    DrawTextEx(default_game_font, text, text_position, BUTTON_FONT_SIZE, DEFAULT_LETTER_SPACING, BUTTON_TEXT_COLOR);
}
