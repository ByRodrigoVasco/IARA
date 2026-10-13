#include "raylib.h"
#include "fonts.h"

// Tamanho base (em pixels) com que a fonte é carregada.
#define FONT_BASE_SIZE 32

// Caracteres carregados: ASCII imprimível (32 a 126) e Latin-1 (160 a 255),
// que inclui os acentos do português: á, é, ç, ã, õ etc.
#define FIRST_ASCII_CODEPOINT 32
#define LAST_ASCII_CODEPOINT 126
#define FIRST_LATIN1_CODEPOINT 160
#define LAST_LATIN1_CODEPOINT 255
#define ASCII_CODEPOINT_COUNT (LAST_ASCII_CODEPOINT - FIRST_ASCII_CODEPOINT + 1)
#define LATIN1_CODEPOINT_COUNT (LAST_LATIN1_CODEPOINT - FIRST_LATIN1_CODEPOINT + 1)
#define TOTAL_CODEPOINT_COUNT (ASCII_CODEPOINT_COUNT + LATIN1_CODEPOINT_COUNT)

Font default_game_font;

void initialize_fonts(void)
{
    // LoadFont() só carrega o ASCII básico, e por isso letras acentuadas
    // apareciam como "?". Aqui informamos a lista completa de caracteres.
    int codepoints[TOTAL_CODEPOINT_COUNT];
    int codepoint_index = 0;

    for (int codepoint = FIRST_ASCII_CODEPOINT; codepoint <= LAST_ASCII_CODEPOINT; codepoint++)
    {
        codepoints[codepoint_index++] = codepoint;
    }

    for (int codepoint = FIRST_LATIN1_CODEPOINT; codepoint <= LAST_LATIN1_CODEPOINT; codepoint++)
    {
        codepoints[codepoint_index++] = codepoint;
    }

    default_game_font = LoadFontEx("code/assets/fonts/VT323-Regular.ttf", FONT_BASE_SIZE, codepoints, TOTAL_CODEPOINT_COUNT);
}

void unload_fonts(void)
{
    UnloadFont(default_game_font);
}
