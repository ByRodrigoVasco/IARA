#include "helper.h" // Inclua o header do próprio arquivo
#include "macros.h"
#include "helper.h"

GAME_TEXTURE CreateGameTextureFromTexture2D(Texture2D texture)
{
    GAME_TEXTURE game_txtr;

    game_txtr.texture = texture;

    // Calcula a posição centralizada considerando o tamanho da textura
    game_txtr.middle_height = middle_screen_y - (texture.height / 2);
    game_txtr.middle_width = middle_screen_x - (texture.width / 2);

    return game_txtr;
}

void UnloadGameTexture(GAME_TEXTURE Texture)
{
    UnloadTexture(Texture.texture);
}