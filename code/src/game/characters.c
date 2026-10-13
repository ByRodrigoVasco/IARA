#include "raylib.h"
#include "characters.h"
#include "macros.h"

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
}
