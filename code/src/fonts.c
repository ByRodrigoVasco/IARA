#include "raylib.h"
#include "fonts.h"

Font default_game_font;

void initialize_fonts(void)
{
    // Load the font
    default_game_font = LoadFont("code/assets/fonts/VT323-Regular.ttf");
}

void unload_fonts(void)
{
    // Unload the font
    UnloadFont(default_game_font);
}