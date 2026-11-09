#include "raylib.h"
#include "rlgl.h"
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

// DrawTriangle só preenche triângulos em sentido anti-horário; esta versão aceita qualquer ordem.
static void draw_triangle_any_order(Vector2 a, Vector2 b, Vector2 c, Color color)
{
    float cross = ((b.x - a.x) * (c.y - a.y)) - ((b.y - a.y) * (c.x - a.x));

    if (cross > 0.0f) DrawTriangle(a, c, b, color);
    else DrawTriangle(a, b, c, color);
}

// Post-it colado: inclinado em volta do centro, com uma fita segurando a borda de cima e a pontinha
// de baixo levemente dobrada, para parecer papel de verdade preso ao monitor.
void draw_cheat_sheet_stuck(Rectangle bounds, float tilt_degrees)
{
    Vector2 center = {bounds.x + (bounds.width / 2.0f), bounds.y + (bounds.height / 2.0f)};

    rlPushMatrix();
    rlTranslatef(center.x, center.y, 0.0f);
    rlRotatef(tilt_degrees, 0.0f, 0.0f, 1.0f);
    rlTranslatef(-center.x, -center.y, 0.0f);

    draw_cheat_sheet(bounds);

    // Pontinha dobrada no canto de baixo à direita
    float fold = bounds.height * 0.12f;
    Vector2 corner = {bounds.x + bounds.width, bounds.y + bounds.height};
    draw_triangle_any_order((Vector2){corner.x - fold, corner.y}, (Vector2){corner.x, corner.y - fold}, (Vector2){corner.x - fold, corner.y - fold}, (Color){214, 190, 90, 255});
    draw_triangle_any_order((Vector2){corner.x - fold, corner.y}, (Vector2){corner.x, corner.y}, (Vector2){corner.x, corner.y - fold}, (Color){0, 0, 0, 60});

    // Fita adesiva meio transparente, um pouco torta em relação ao papel
    float tape_width = bounds.width * 0.40f;
    float tape_height = bounds.height * 0.13f;
    Vector2 tape_center = {center.x + (bounds.width * 0.03f), bounds.y + (tape_height * 0.1f)};

    rlPushMatrix();
    rlTranslatef(tape_center.x, tape_center.y, 0.0f);
    rlRotatef(2.5f, 0.0f, 0.0f, 1.0f);
    Rectangle tape = {-tape_width / 2.0f, -tape_height / 2.0f, tape_width, tape_height};
    DrawRectangleRec((Rectangle){tape.x, tape.y + (tape_height * 0.12f), tape.width, tape.height}, (Color){0, 0, 0, 55});
    DrawRectangleRec(tape, (Color){232, 226, 196, 175});
    DrawRectangleRec((Rectangle){tape.x, tape.y, tape.width, tape_height * 0.22f}, (Color){255, 252, 232, 120});
    // Pontas serrilhadas da fita
    for (int i = 0; i < 4; i++)
    {
        float tooth = tape_height / 4.0f;
        draw_triangle_any_order((Vector2){tape.x, tape.y + (i * tooth)}, (Vector2){tape.x - (tooth * 0.5f), tape.y + ((i + 0.5f) * tooth)}, (Vector2){tape.x, tape.y + ((i + 1) * tooth)}, (Color){232, 226, 196, 175});
        draw_triangle_any_order((Vector2){tape.x + tape.width, tape.y + ((i + 1) * tooth)}, (Vector2){tape.x + tape.width + (tooth * 0.5f), tape.y + ((i + 0.5f) * tooth)}, (Vector2){tape.x + tape.width, tape.y + (i * tooth)}, (Color){232, 226, 196, 175});
    }
    rlPopMatrix();

    rlPopMatrix();
}
