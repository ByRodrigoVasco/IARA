#include "raylib.h"
#include "animations.h"

// Valor máximo do canal alpha (opacidade total)
#define MAX_ALPHA_CHANNEL_INT 255

void fade_in_texture(unsigned int fade_speed, Color *color, bool *is_animation_finished)
{
    // Sobe o alpha até o limite; ao atingir, sinaliza o fim da animação
    if ((color->a + fade_speed) < MAX_ALPHA_CHANNEL_INT)
    {
        color->a += fade_speed;
    }
    else
    {
        *is_animation_finished = true;
    }
}

void fade_out_texture(unsigned int fade_speed, Color *color, bool *is_animation_finished)
{
    // Desce o alpha até zero; ao atingir, sinaliza o fim da animação
    if (color->a > fade_speed)
    {
        color->a -= fade_speed;
    }
    else
    {
        color->a = 0;
        *is_animation_finished = true;
    }
}
