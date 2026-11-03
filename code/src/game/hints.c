#include <stddef.h>

#include "raylib.h"
#include "hints.h"
#include "collisions.h"
#include "helper.h"
#include "fonts.h"

#define HINT_LINE_COUNT 5

// Medidas no mapa de referência, relativas ao centro da tela.
#define HINT_ZONE_REFERENCE {-400.0f, -235.0f, 35.0f, 95.0f} // Lado direito da sala do mural
#define HINT_ITEM_REFERENCE {-490.0f, -215.0f, 75.0f, 55.0f} // O mural

// Bilhete na tela, em 1920x1080.
#define NOTE_FONT_SIZE 40.0f
#define NOTE_LETTER_SPACING_RATIO 0.04f
#define NOTE_PADDING 40.0f
#define NOTE_LINE_GAP 12.0f
#define NOTE_SHADOW_OFFSET 10.0f
#define NOTE_MAX_WIDTH_RATIO 0.9f // Largura máxima do bilhete (largura da janela)

#define NOTE_COLOR (Color){245, 225, 120, 255}
#define NOTE_TEXT_COLOR (Color){30, 25, 20, 255}
#define NOTE_DIM_COLOR (Color){0, 0, 0, 150}
#define NOTE_SHADOW_COLOR (Color){0, 0, 0, 120}

#define NOTE_CLOSE_TEXT "ESPAÇO: guardar o bilhete"

static bool hint_note_open = false;

// A dica estreita o raciocínio sem citar o chamado nem as alternativas do quiz.
static const char *const first_day_note_lines[HINT_LINE_COUNT] = {
    "Bilhete colado no mural:",
    "P -> Q quer dizer: se P acontecer, Q vem junto.",
    "Confira se P aconteceu e siga o que a regra manda.",
    "- Estagiário do ano passado",
    "Você leva a colinha dos símbolos para o seu monitor."};

static const char *const *hint_note_lines = first_day_note_lines;
static int hint_note_line_count = HINT_LINE_COUNT;

void set_hint_note_lines(const char *const *lines, int line_count)
{
    hint_note_lines = (lines != NULL) ? lines : first_day_note_lines;
    hint_note_line_count = (lines != NULL) ? line_count : HINT_LINE_COUNT;
}

Rectangle get_hint_zone(float scale, Vector2 center)
{
    Rectangle reference = HINT_ZONE_REFERENCE;

    return scale_reference_rect(reference, scale, center);
}

Rectangle get_hint_item_rectangle(float scale, Vector2 center)
{
    Rectangle reference = HINT_ITEM_REFERENCE;

    return scale_reference_rect(reference, scale, center);
}

bool is_hitbox_on_hint(Rectangle hitbox, float scale, Vector2 center)
{
    Rectangle zone = get_hint_zone(scale, center);

    return collides_with_any(hitbox, &zone, 1);
}

void toggle_hint_note(void)
{
    hint_note_open = !hint_note_open;
}

bool is_hint_note_open(void)
{
    return hint_note_open;
}

int get_hint_note_line_count(void)
{
    return hint_note_line_count;
}

const char *get_hint_note_line(int index)
{
    return hint_note_lines[index];
}

void draw_hint_note(void)
{
    if (!hint_note_open) return;

    float window_w = (float)GetScreenWidth();
    float window_h = (float)GetScreenHeight();
    float scale = get_ui_scale();
    float font_size = NOTE_FONT_SIZE * scale;
    float padding = NOTE_PADDING * scale;
    float line_gap = NOTE_LINE_GAP * scale;

    // Reduz a fonte se a maior linha não couber em uma janela estreita
    float longest = MeasureTextEx(default_game_font, NOTE_CLOSE_TEXT, font_size, font_size * NOTE_LETTER_SPACING_RATIO).x;

    for (int i = 0; i < hint_note_line_count; i++)
    {
        float width = MeasureTextEx(default_game_font, hint_note_lines[i], font_size, font_size * NOTE_LETTER_SPACING_RATIO).x;
        if (width > longest) longest = width;
    }

    float max_text_width = (window_w * NOTE_MAX_WIDTH_RATIO) - (padding * 2.0f);

    if (longest > max_text_width)
    {
        font_size *= max_text_width / longest;
        longest = max_text_width;
    }

    float letter_spacing = font_size * NOTE_LETTER_SPACING_RATIO;
    float note_w = longest + (padding * 2.0f);
    float note_h = ((hint_note_line_count + 2) * (font_size + line_gap)) + (padding * 2.0f);
    Rectangle note = {(window_w - note_w) / 2.0f, (window_h - note_h) / 2.0f, note_w, note_h};
    float shadow = NOTE_SHADOW_OFFSET * scale;

    DrawRectangle(0, 0, (int)window_w, (int)window_h, NOTE_DIM_COLOR);
    DrawRectangleRec((Rectangle){note.x + shadow, note.y + shadow, note.width, note.height}, NOTE_SHADOW_COLOR);
    DrawRectangleRec(note, NOTE_COLOR);

    float y = note.y + padding;

    for (int i = 0; i < hint_note_line_count; i++)
    {
        DrawTextEx(default_game_font, hint_note_lines[i], (Vector2){note.x + padding, y}, font_size, letter_spacing, NOTE_TEXT_COLOR);
        y += font_size + line_gap;
    }

    // Uma linha em branco separa o texto da instrução de fechar
    y += font_size + line_gap;
    DrawTextEx(default_game_font, NOTE_CLOSE_TEXT, (Vector2){note.x + padding, y}, font_size, letter_spacing, NOTE_TEXT_COLOR);
}
