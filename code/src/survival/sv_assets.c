#include "sv_assets.h"
#include "characters.h"

SvSprite sv_sprites[SPR_COUNT];

static Texture2D player_frames[DIRECTION_COUNT][WALK_FRAME_COUNT];
static bool player_loaded = false;
static bool sprites_loaded = false;

void sv_load_sprites(void)
{
    int index = 0;

    if (sprites_loaded) return;

#define X(id, name, w, h, n)                                                                   \
    sv_sprites[index].texture = LoadTexture(SV_SPRITE_PATH name ".png");                      \
    SetTextureFilter(sv_sprites[index].texture, TEXTURE_FILTER_POINT);                         \
    sv_sprites[index].frame_width = w;                                                         \
    sv_sprites[index].frame_height = h;                                                        \
    sv_sprites[index].frames = n;                                                              \
    index++;
    SV_SPRITE_LIST(X)
#undef X

    sprites_loaded = true;
}

void sv_unload_sprites(void)
{
    if (!sprites_loaded) return;
    for (int i = 0; i < SPR_COUNT; i++) UnloadTexture(sv_sprites[i].texture);
    sprites_loaded = false;
}

void sv_draw(SvSpriteId id, int frame, Vector2 center, float scale, float rotation, bool flip, Color tint)
{
    SvSprite *sprite = &sv_sprites[id];
    float w = (float)sprite->frame_width, h = (float)sprite->frame_height;

    if (frame < 0) frame = 0;
    frame %= sprite->frames;

    Rectangle source = {frame * w, 0, flip ? -w : w, h};
    Rectangle dest = {center.x, center.y, w * scale, h * scale};

    DrawTexturePro(sprite->texture, source, dest, (Vector2){w * scale / 2.0f, h * scale / 2.0f}, rotation, tint);
}

void sv_draw_stretched(SvSpriteId id, int frame, Vector2 center, float width, float height, float rotation, Color tint)
{
    SvSprite *sprite = &sv_sprites[id];
    float w = (float)sprite->frame_width, h = (float)sprite->frame_height;

    frame %= sprite->frames;

    Rectangle source = {frame * w, 0, w, h};
    Rectangle dest = {center.x, center.y, width, height};

    DrawTexturePro(sprite->texture, source, dest, (Vector2){width / 2.0f, height / 2.0f}, rotation, tint);
}

void sv_load_player(void)
{
    if (player_loaded) return;

    for (int d = 0; d < DIRECTION_COUNT; d++)
    {
        if (!load_walk_frames("Estagiarios", d, player_frames[d]))
        {
            for (int f = 0; f < WALK_FRAME_COUNT; f++) player_frames[d][f] = (Texture2D){0};
        }
        for (int f = 0; f < WALK_FRAME_COUNT; f++)
        {
            if (player_frames[d][f].id) SetTextureFilter(player_frames[d][f], TEXTURE_FILTER_POINT);
        }
    }
    player_loaded = true;
}

void sv_unload_player(void)
{
    if (!player_loaded) return;
    for (int d = 0; d < DIRECTION_COUNT; d++)
    {
        for (int f = 0; f < WALK_FRAME_COUNT; f++)
        {
            if (player_frames[d][f].id) UnloadTexture(player_frames[d][f]);
        }
    }
    player_loaded = false;
}

void sv_draw_player(int direction, int frame, Vector2 center, float scale, Color tint)
{
    Texture2D texture = player_frames[direction][frame % WALK_FRAME_COUNT];
    float w, h;

    if (!texture.id) return;
    w = (float)texture.width;
    h = (float)texture.height;
    DrawTexturePro(texture, (Rectangle){0, 0, w, h}, (Rectangle){center.x, center.y, w * scale, h * scale}, (Vector2){w * scale / 2.0f, h * scale / 2.0f}, 0.0f, tint);
}
