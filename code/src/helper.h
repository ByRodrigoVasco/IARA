#ifndef GAME_TEXTURE_H
#define GAME_TEXTURE_H

#include "raylib.h"

typedef struct GAME_TEXTURE
{
    Texture2D texture;
    int middle_width;
    int middle_height;
} GAME_TEXTURE;

GAME_TEXTURE CreateGameTextureFromTexture2D(Texture2D texture);

void UnloadGameTexture(GAME_TEXTURE Texture);

#endif // GAME_TEXTURE_H