#include <stdbool.h>

#include "raylib.h"
#include "button.h"
#include "macros.h"
#include "fonts.h"

// Função reutilizável para desenhar um botão e processar o clique
void draw_button(Rectangle bounds, const char *text, bool *is_clicked)
{
    // Cores de estado do botão (podem ser extraídas para macros ou parâmetros se preferir)
    Color normal_color = LIGHTGRAY;
    Color hover_color = GRAY;
    Color pressed_color = DARKGRAY;
    Color text_color = BLACK;
    Color border_color = BLACK;

    Color current_color = normal_color;
    Vector2 mouse_point = GetMousePosition();
    
    // Verifica colisão do mouse com a área do botão
    if (CheckCollisionPointRec(mouse_point, bounds))
    {
        current_color = hover_color;
        
        // Verifica se o usuário está segurando o clique
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            current_color = pressed_color;
        }
        
        // O clique é confirmado quando o botão do mouse é solto sobre a área do botão
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        {
            *is_clicked = true; 
        }
    }

    // Renderiza o fundo e a borda do botão
    DrawRectangleRec(bounds, current_color);
    DrawRectangleLinesEx(bounds, 2.0f, border_color);

    // Centraliza o texto dentro do botão usando a fonte do seu jogo
    // Substitua `default_game_font` pela sua variável de fonte
    float font_size = 20.0f; // Ajuste conforme a necessidade
    Vector2 text_size = MeasureTextEx(default_game_font, text, font_size, DEFAULT_LETTER_SPACING);
    
    Vector2 text_pos = {
        bounds.x + (bounds.width / 2.0f) - (text_size.x / 2.0f),
        bounds.y + (bounds.height / 2.0f) - (text_size.y / 2.0f)
    };

    DrawTextEx(default_game_font, text, text_pos, font_size, DEFAULT_LETTER_SPACING, text_color);
}