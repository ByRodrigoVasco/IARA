#include "macros.h"
#include "helper.h"

typedef struct GAME_TEXTURE
{
    Texture2D texture;
    // Width e Height para o objeto ficar corretamente no meio
    int middle_width;
    int middle_height;

} GAME_TEXTURE;

GAME_TEXTURE CreateGameTextureFromTexture2D(Texture2D texture);

GAME_TEXTURE CreateGameTextureFromTexture2D(Texture2D texture)
{
    GAME_TEXTURE game_txtr;

    game_txtr.texture = texture;

    // Aqui já está calculando o número exato para colocar a textura no meio considerando seu próprio tamanho.
    game_txtr.middle_height = (WINDOW_HEIGHT - texture.height) / 2;
    game_txtr.middle_width = (WINDOW_WIDTH - texture.width) / 2;

    return game_txtr;
}

void UnloadGameTexture(GAME_TEXTURE Texture)
{
    UnloadTexture(Texture.texture);
}