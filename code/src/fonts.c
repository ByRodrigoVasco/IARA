#include "raylib.h"
#include "fonts.h"

Font default_font_menu;


void initialize_fonts(void)
{
    // Load the font
    default_font_menu = LoadFont("code/assets/fonts/VT323-Regular.ttf");
}

void unload_fonts(void)
{
    // Unload the font
    UnloadFont(default_font_menu);
}