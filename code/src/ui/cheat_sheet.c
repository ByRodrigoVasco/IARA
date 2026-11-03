#include "raylib.h"
#include "cheat_sheet.h"
#include "fonts.h"
#include "logic_text.h"

#define CHEAT_SHEET_ENTRY_COUNT 5

// Proporções do post-it, relativas à sua altura.
#define SHEET_PADDING_RATIO 0.07f
#define SHEET_SHADOW_RATIO 0.03f
#define SHEET_OUTLINE_RATIO 0.012f

// Proporções do texto, relativas à altura de cada linha e ao tamanho da fonte.
#define ROW_FONT_RATIO 0.92f
#define SHEET_LETTER_SPACING_RATIO 0.04f
#define SYMBOL_COLUMN_RATIO 1.25f      // Largura da coluna do símbolo (tamanho da fonte)
#define SYMBOL_SIZE_RATIO 0.75f        // Lado do símbolo (tamanho da fonte)
#define SYMBOL_THICKNESS_RATIO 0.1f    // Espessura do traço do símbolo (tamanho da fonte)

#define SHEET_COLOR (Color){245, 225, 120, 255}
#define SHEET_TEXT_COLOR (Color){30, 25, 20, 255}
#define SHEET_SHADOW_COLOR (Color){0, 0, 0, 120}

#define SHEET_TITLE "Colinha"

// A fonte do jogo não tem os símbolos lógicos, então eles são desenhados em traços.
typedef struct {
    int codepoint;
    const char *text;
} CheatSheetEntry;

static const CheatSheetEntry cheat_sheet_entries[CHEAT_SHEET_ENTRY_COUNT] = {
    {0x00AC, "não P"},
    {0x2227, "P e Q"},
    {0x2228, "P ou Q"},
    {0x2192, "se P, então Q"},
    {0x2194, "P se e só se Q"}};

int get_cheat_sheet_entry_count(void)
{
    return CHEAT_SHEET_ENTRY_COUNT;
}

const char *get_cheat_sheet_entry_text(int index)
{
    return cheat_sheet_entries[index].text;
}

float get_cheat_sheet_font_size(Rectangle bounds)
{
    float padding = bounds.height * SHEET_PADDING_RATIO;
    float row_height = (bounds.height - (padding * 2.0f)) / (CHEAT_SHEET_ENTRY_COUNT + 1);
    float font_size = row_height * ROW_FONT_RATIO;

    // Reduz a fonte se a maior linha não couber em um post-it estreito
    float longest = 0.0f;

    for (int i = 0; i < CHEAT_SHEET_ENTRY_COUNT; i++)
    {
        float width = (font_size * SYMBOL_COLUMN_RATIO) + MeasureTextEx(default_game_font, cheat_sheet_entries[i].text, font_size, font_size * SHEET_LETTER_SPACING_RATIO).x;
        if (width > longest) longest = width;
    }

    float available = bounds.width - (padding * 2.0f);

    return (longest > available) ? font_size * (available / longest) : font_size;
}

void draw_cheat_sheet(Rectangle bounds)
{
    float shadow = bounds.height * SHEET_SHADOW_RATIO;
    float padding = bounds.height * SHEET_PADDING_RATIO;
    float row_height = (bounds.height - (padding * 2.0f)) / (CHEAT_SHEET_ENTRY_COUNT + 1);
    float font_size = get_cheat_sheet_font_size(bounds);
    float letter_spacing = font_size * SHEET_LETTER_SPACING_RATIO;
    float symbol_size = font_size * SYMBOL_SIZE_RATIO;
    float outline = bounds.height * SHEET_OUTLINE_RATIO;

    if (outline < 1.0f) outline = 1.0f;

    DrawRectangleRec((Rectangle){bounds.x + shadow, bounds.y + shadow, bounds.width, bounds.height}, SHEET_SHADOW_COLOR);
    DrawRectangleRec(bounds, SHEET_COLOR);
    DrawRectangleLinesEx(bounds, outline, SHEET_TEXT_COLOR);

    float x = bounds.x + padding;
    float y = bounds.y + padding;

    DrawTextEx(default_game_font, SHEET_TITLE, (Vector2){x, y + ((row_height - font_size) / 2.0f)}, font_size, letter_spacing, SHEET_TEXT_COLOR);

    for (int i = 0; i < CHEAT_SHEET_ENTRY_COUNT; i++)
    {
        y += row_height;

        draw_logic_symbol(cheat_sheet_entries[i].codepoint, (Vector2){x, y + ((row_height - symbol_size) / 2.0f)}, symbol_size, font_size * SYMBOL_THICKNESS_RATIO, SHEET_TEXT_COLOR);
        DrawTextEx(default_game_font, cheat_sheet_entries[i].text, (Vector2){x + (font_size * SYMBOL_COLUMN_RATIO), y + ((row_height - font_size) / 2.0f)},
                   font_size, letter_spacing, SHEET_TEXT_COLOR);
    }
}
