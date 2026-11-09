#include "helper.h"
#include "macros.h"

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

void update_screen_metrics(void)
{
    screen_width = GetScreenWidth();
    screen_height = GetScreenHeight();
    middle_screen_x = screen_width / 2;
    middle_screen_y = screen_height / 2;
}

float get_ui_scale(void)
{
    float scale_x = GetScreenWidth() / UI_REFERENCE_WIDTH;
    float scale_y = GetScreenHeight() / UI_REFERENCE_HEIGHT;

    return (scale_x < scale_y) ? scale_x : scale_y;
}

Texture2D load_smooth_texture(const char *path)
{
    Texture2D texture = LoadTexture(path);

    GenTextureMipmaps(&texture);
    SetTextureFilter(texture, TEXTURE_FILTER_TRILINEAR);

    return texture;
}

void draw_texture_cover(Texture2D texture, Rectangle dest, Color tint)
{
    Rectangle source = {0, 0, (float)texture.width, (float)texture.height};
    float dest_aspect = dest.width / dest.height;
    float texture_aspect = source.width / source.height;

    if (texture_aspect > dest_aspect)
    {
        source.width = source.height * dest_aspect;
        source.x = (texture.width - source.width) / 2.0f;
    }
    else
    {
        source.height = source.width / dest_aspect;
        source.y = (texture.height - source.height) / 2.0f;
    }

    DrawTexturePro(texture, source, dest, (Vector2){0, 0}, 0.0f, tint);
}

void toggle_fullscreen(void)
{
    ToggleBorderlessWindowed();
}

bool is_fullscreen_enabled(void)
{
    return IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE);
}
