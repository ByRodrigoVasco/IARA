#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "raylib.h"
#include "logic_text.h"
#include "fonts.h"

#define SYMBOL_MAX_SEGMENTS 5
#define SYMBOL_CELL_RATIO 0.62f       // Lado da célula do símbolo (tamanho da fonte)
#define SYMBOL_ADVANCE_RATIO 0.78f    // Avanço do texto a cada símbolo (tamanho da fonte)
#define SYMBOL_THICKNESS_RATIO 0.1f   // Espessura do traço (tamanho da fonte)
#define TEXT_PIECE_MAX 128

// Traço de um símbolo, em fração da célula onde ele é desenhado.
typedef struct {
    Vector2 start;
    Vector2 end;
} SymbolSegment;

typedef struct {
    int codepoint;
    SymbolSegment segments[SYMBOL_MAX_SEGMENTS];
    int segment_count;
} LogicSymbol;

static const LogicSymbol logic_symbols[] = {
    // ¬
    {0x00AC, {{{0.10f, 0.40f}, {0.90f, 0.40f}}, {{0.90f, 0.40f}, {0.90f, 0.75f}}}, 2},
    // ∧
    {0x2227, {{{0.15f, 0.85f}, {0.50f, 0.15f}}, {{0.50f, 0.15f}, {0.85f, 0.85f}}}, 2},
    // ∨
    {0x2228, {{{0.15f, 0.15f}, {0.50f, 0.85f}}, {{0.50f, 0.85f}, {0.85f, 0.15f}}}, 2},
    // →
    {0x2192, {{{0.05f, 0.50f}, {0.95f, 0.50f}}, {{0.65f, 0.25f}, {0.95f, 0.50f}}, {{0.65f, 0.75f}, {0.95f, 0.50f}}}, 3},
    // ↔
    {0x2194, {{{0.05f, 0.50f}, {0.95f, 0.50f}}, {{0.65f, 0.25f}, {0.95f, 0.50f}}, {{0.65f, 0.75f}, {0.95f, 0.50f}},
              {{0.35f, 0.25f}, {0.05f, 0.50f}}, {{0.35f, 0.75f}, {0.05f, 0.50f}}}, 5},
    // ∀
    {0x2200, {{{0.10f, 0.10f}, {0.50f, 0.90f}}, {{0.50f, 0.90f}, {0.90f, 0.10f}}, {{0.27f, 0.45f}, {0.73f, 0.45f}}}, 3},
    // ∃
    {0x2203, {{{0.20f, 0.10f}, {0.80f, 0.10f}}, {{0.80f, 0.10f}, {0.80f, 0.90f}}, {{0.20f, 0.90f}, {0.80f, 0.90f}},
              {{0.30f, 0.50f}, {0.80f, 0.50f}}}, 4}};

static const LogicSymbol *find_logic_symbol(int codepoint)
{
    for (size_t i = 0; i < sizeof(logic_symbols) / sizeof(logic_symbols[0]); i++)
    {
        if (logic_symbols[i].codepoint == codepoint) return &logic_symbols[i];
    }

    return NULL;
}

void draw_logic_symbol(int codepoint, Vector2 origin, float size, float thickness, Color color)
{
    const LogicSymbol *symbol = find_logic_symbol(codepoint);

    if (symbol == NULL) return;

    for (int i = 0; i < symbol->segment_count; i++)
    {
        SymbolSegment segment = symbol->segments[i];

        DrawLineEx((Vector2){origin.x + (segment.start.x * size), origin.y + (segment.start.y * size)},
                   (Vector2){origin.x + (segment.end.x * size), origin.y + (segment.end.y * size)},
                   thickness, color);
    }
}

// Percorre o texto alternando trechos da fonte e símbolos. Com draw falso, só mede.
static float walk_logic_text(const char *text, Vector2 position, float font_size, float letter_spacing, Color color, bool draw)
{
    char piece[TEXT_PIECE_MAX];
    int piece_length = 0;
    float x = position.x;
    const char *cursor = text;

    while (true)
    {
        int byte_count = 0;
        int codepoint = (*cursor != '\0') ? GetCodepointNext(cursor, &byte_count) : 0;
        bool is_symbol = (*cursor != '\0') && find_logic_symbol(codepoint) != NULL;

        if (*cursor == '\0' || is_symbol || piece_length + byte_count >= TEXT_PIECE_MAX)
        {
            piece[piece_length] = '\0';

            if (piece_length > 0)
            {
                if (draw) DrawTextEx(default_game_font, piece, (Vector2){x, position.y}, font_size, letter_spacing, color);
                x += MeasureTextEx(default_game_font, piece, font_size, letter_spacing).x + letter_spacing;
            }

            piece_length = 0;
        }

        if (*cursor == '\0') break;

        if (is_symbol)
        {
            float cell = font_size * SYMBOL_CELL_RATIO;
            float advance = font_size * SYMBOL_ADVANCE_RATIO;

            if (draw)
            {
                draw_logic_symbol(codepoint, (Vector2){x + ((advance - cell) / 2.0f), position.y + ((font_size - cell) / 2.0f)},
                                  cell, font_size * SYMBOL_THICKNESS_RATIO, color);
            }

            x += advance;
        }
        else
        {
            memcpy(piece + piece_length, cursor, byte_count);
            piece_length += byte_count;
        }

        cursor += byte_count;
    }

    return x - position.x;
}

Vector2 measure_logic_text(const char *text, float font_size, float letter_spacing)
{
    return (Vector2){walk_logic_text(text, (Vector2){0, 0}, font_size, letter_spacing, BLACK, false), font_size};
}

void draw_logic_text(const char *text, Vector2 position, float font_size, float letter_spacing, Color color)
{
    walk_logic_text(text, position, font_size, letter_spacing, color, true);
}
