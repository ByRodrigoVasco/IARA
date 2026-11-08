#include "sv_assets.h"
#include "characters.h"
#include "sv_rules.h"

SvSprite sv_sprites[SPR_COUNT];

static Texture2D player_frames[DIRECTION_COUNT][WALK_FRAME_COUNT];
static bool player_loaded = false;
static bool sprites_loaded = false;

typedef struct
{
    const char *file;
    int frame_width, frame_height, frames;
} SpriteInfo;

static const SpriteInfo sprite_info[SPR_COUNT] = {
#define X(id, name, w, h, n) {name, w, h, n},
    SV_SPRITE_LIST(X)
#undef X
};

static int sprites_next = 0;   // Quantas imagens já foram carregadas

float sv_load_sprites_step(void)
{
    if (sprites_loaded) return 1.0f;

    if (sprites_next < SPR_COUNT)
    {
        const SpriteInfo *info = &sprite_info[sprites_next];
        SvSprite *sprite = &sv_sprites[sprites_next];

        sprite->texture = LoadTexture(TextFormat("%s%s.png", SV_SPRITE_PATH, info->file));
        SetTextureFilter(sprite->texture, TEXTURE_FILTER_POINT);
        sprite->frame_width = info->frame_width;
        sprite->frame_height = info->frame_height;
        sprite->frames = info->frames;
        sprites_next++;
    }

    if (sprites_next >= SPR_COUNT)
    {
        sprites_loaded = true;
        return 1.0f;
    }
    return (float)sprites_next / (float)SPR_COUNT;
}

void sv_load_sprites(void)
{
    while (sv_load_sprites_step() < 1.0f) { }
}

void sv_unload_sprites(void)
{
    int count = sprites_loaded ? SPR_COUNT : sprites_next;

    for (int i = 0; i < count; i++) UnloadTexture(sv_sprites[i].texture);
    sprites_loaded = false;
    sprites_next = 0;
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

void sv_load_player(int character)
{
    const char *folder = sv_character(character)->folder;

    sv_unload_player();

    for (int d = 0; d < DIRECTION_COUNT; d++)
    {
        if (!load_walk_frames(folder, d, player_frames[d]))
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
