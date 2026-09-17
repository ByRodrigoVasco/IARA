#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX_ALPHA_CHANNEL_INT 255


void fade_in_texture(unsigned int fadeSpeed, Color *color, bool *is_animation_finished)
{
    if((color->a + fadeSpeed) < MAX_ALPHA_CHANNEL_INT)
    {
        color->a += fadeSpeed;
    }
    else
    {
        *is_animation_finished = true;
    }
}

void fade_out_texture(unsigned int fadeSpeed, Color *color, bool *is_animation_finished)
{
    if(color->a > fadeSpeed)
    {
        color->a -= fadeSpeed;
    }
    else
    {
        color->a = 0;
        *is_animation_finished = true;
    }
}