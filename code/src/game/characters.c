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

int get_walk_frame(float walk_time)
{
    return (int)(walk_time / WALK_FRAME_TIME) % WALK_FRAME_COUNT;
}

void initialize_estagiario_1_walk_textures(Texture2D east[WALK_FRAME_COUNT], Texture2D west[WALK_FRAME_COUNT])
{
    for (int i = 0; i < WALK_FRAME_COUNT; i++)
    {
        Image frame = LoadImage(TextFormat(ESTAGIARIO_1_WALK_EAST_PATH, i));
        east[i] = LoadTextureFromImage(frame);

        // O oeste é o espelho do leste
        ImageFlipHorizontal(&frame);
        west[i] = LoadTextureFromImage(frame);
        UnloadImage(frame);
    }
}
