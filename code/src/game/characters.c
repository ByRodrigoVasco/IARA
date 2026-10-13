#include <stdio.h>

#include "raylib.h"
#include "characters.h"
#include "macros.h"

#define CHAR_DEFAULT_WIDTH_HEIGHT 70.0f

// Serve para Height e Width
float character_height_rescale_factor;
float character_width_rescale_factor;

int default_character_width;
int default_character_height;

void initialize_estagiario_1_textures(char_textures out)
{
    out[EAST]      = LoadTexture(ESTAGIARIO_1_EAST_PATH);
    out[SOUTH]     = LoadTexture(ESTAGIARIO_1_SOUTH_PATH);
    out[NORTH]     = LoadTexture(ESTAGIARIO_1_NORTH_PATH);
    out[WEST]      = LoadTexture(ESTAGIARIO_1_WEST_PATH);
    out[SOUTHEAST] = LoadTexture(ESTAGIARIO_1_SOUTHEAST_PATH);
    out[NORTHWEST] = LoadTexture(ESTAGIARIO_1_NORTHWEST_PATH);
    out[SOUTHWEST] = LoadTexture(ESTAGIARIO_1_SOUTHWEST_PATH);
    out[NORTHEAST] = LoadTexture(ESTAGIARIO_1_NORTHEAST_PATH);

    for (int i = 0; i < DIRECTION_COUNT; i++)
    {
        printf("Character Width = %d and Height = %d \n", out[i].width, out[i].height);
    }
}