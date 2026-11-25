#include <string.h>
#include "sv_assets.h"
#include "characters.h"
#include "sv_rules.h"

SvSprite sv_sprites[SPR_COUNT];

// Dois conjuntos de quadros: o do jogador 1 e o do jogador 2 (só a dupla usa o segundo)
#define PLAYER_SLOTS 2
static Texture2D player_slot_frames[PLAYER_SLOTS][DIRECTION_COUNT][WALK_FRAME_COUNT];
static bool player_slot_loaded[PLAYER_SLOTS];
static bool player_slot_owned[PLAYER_SLOTS];   // Os quadros foram lidos só para este conjunto (e são descarregados com ele)

// Todos os personagens, carregados uma vez (sv_load_characters_step). São pequenos: 32 quadros de 32 x 32 cada.
static Texture2D character_frames[CH_COUNT][DIRECTION_COUNT][WALK_FRAME_COUNT];
static bool character_loaded[CH_COUNT];
static int characters_next = 0;
static int player_slot = 0;
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

// Vermelho vivo: olhos, boca e sangue dos monstros
static bool is_glow_pixel(Color c)
{
    return c.a > 0 && c.r >= 170 && c.g <= 95 && c.b <= 95;
}

// Monta a textura de brilho: só os pixels vermelhos-vivos, o resto transparente.
// Só inimigos (en_) e chefes (bo_) têm brilho.
static Texture2D load_glow(const char *file)
{
    Image image;
    Color *pixels;
    int count = 0;
    Texture2D texture = {0};

    if (strncmp(file, "en_", 3) != 0 && strncmp(file, "bo_", 3) != 0) return texture;
    image = LoadImage(TextFormat("%s%s.png", SV_SPRITE_PATH, file));
    if (!image.data) return texture;
    ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    pixels = (Color *)image.data;
    for (int i = 0; i < image.width * image.height; i++)
    {
        if (is_glow_pixel(pixels[i])) count++;
        else pixels[i] = BLANK;
    }
    if (count > 0)
    {
        texture = LoadTextureFromImage(image);
        SetTextureFilter(texture, TEXTURE_FILTER_POINT);
    }
    UnloadImage(image);
    return texture;
}

float sv_load_sprites_step(void)
{
    if (sprites_loaded) return 1.0f;

    if (sprites_next < SPR_COUNT)
    {
        const SpriteInfo *info = &sprite_info[sprites_next];
        SvSprite *sprite = &sv_sprites[sprites_next];

        sprite->texture = LoadTexture(TextFormat("%s%s.png", SV_SPRITE_PATH, info->file));
        // Uma tira mais alta que a tabela é arte em alta resolução: ocupa o mesmo espaço e é suavizada
        sprite->detail = sprite->texture.height > info->frame_height ? sprite->texture.height / info->frame_height : 1;
        if (sprite->detail > 1) set_character_filter(&sprite->texture);
        else SetTextureFilter(sprite->texture, TEXTURE_FILTER_POINT);
        sprite->frame_width = info->frame_width;
        sprite->frame_height = info->frame_height;
        sprite->frames = info->frames;
        sprite->glow = load_glow(info->file);
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

    for (int i = 0; i < count; i++)
    {
        UnloadTexture(sv_sprites[i].texture);
        if (sv_sprites[i].glow.id) UnloadTexture(sv_sprites[i].glow);
        sv_sprites[i].glow = (Texture2D){0};
    }
    sprites_loaded = false;
    sprites_next = 0;
}

void sv_draw(SvSpriteId id, int frame, Vector2 center, float scale, float rotation, bool flip, Color tint)
{
    SvSprite *sprite = &sv_sprites[id];
    float w = (float)sprite->frame_width, h = (float)sprite->frame_height;

    if (frame < 0) frame = 0;
    frame %= sprite->frames;

    float k = (float)sprite->detail;
    Rectangle source = {frame * w * k, 0, (flip ? -w : w) * k, h * k};
    Rectangle dest = {center.x, center.y, w * scale, h * scale};

    DrawTexturePro(sprite->texture, source, dest, (Vector2){w * scale / 2.0f, h * scale / 2.0f}, rotation, tint);
}

void sv_draw_glow(SvSpriteId id, int frame, Vector2 center, bool flip, Color tint)
{
    SvSprite *sprite = &sv_sprites[id];
    float w = (float)sprite->frame_width, h = (float)sprite->frame_height;

    if (!sprite->glow.id) return;
    if (frame < 0) frame = 0;
    frame %= sprite->frames;
    float k = (float)sprite->detail;
    DrawTexturePro(sprite->glow, (Rectangle){frame * w * k, 0, (flip ? -w : w) * k, h * k}, (Rectangle){center.x, center.y, w, h}, (Vector2){w / 2.0f, h / 2.0f}, 0.0f, tint);
}

void sv_draw_stretched(SvSpriteId id, int frame, Vector2 center, float width, float height, float rotation, Color tint)
{
    SvSprite *sprite = &sv_sprites[id];
    float w = (float)sprite->frame_width, h = (float)sprite->frame_height;

    frame %= sprite->frames;

    float k = (float)sprite->detail;
    Rectangle source = {frame * w * k, 0, w * k, h * k};
    Rectangle dest = {center.x, center.y, width, height};

    DrawTexturePro(sprite->texture, source, dest, (Vector2){width / 2.0f, height / 2.0f}, rotation, tint);
}

void sv_use_player_slot(int slot)
{
    if (slot >= 0 && slot < PLAYER_SLOTS) player_slot = slot;
}

void sv_unload_all_players(void)
{
    for (int i = 0; i < PLAYER_SLOTS; i++)
    {
        sv_use_player_slot(i);
        sv_unload_player();
    }
    sv_use_player_slot(0);
}

// Lê do disco as 8 direções do andar de um personagem
static void read_walk_set(int character, Texture2D frames[DIRECTION_COUNT][WALK_FRAME_COUNT])
{
    const char *folder = sv_character(character)->folder;

    for (int d = 0; d < DIRECTION_COUNT; d++)
    {
        if (!load_walk_frames(folder, d, frames[d]))
        {
            for (int f = 0; f < WALK_FRAME_COUNT; f++) frames[d][f] = (Texture2D){0};
        }
    }
}

static void unload_walk_set(Texture2D frames[DIRECTION_COUNT][WALK_FRAME_COUNT])
{
    for (int d = 0; d < DIRECTION_COUNT; d++)
    {
        for (int f = 0; f < WALK_FRAME_COUNT; f++)
        {
            if (frames[d][f].id) UnloadTexture(frames[d][f]);
            frames[d][f] = (Texture2D){0};
        }
    }
}

float sv_load_characters_step(void)
{
    if (characters_next < CH_COUNT)
    {
        if (!character_loaded[characters_next])
        {
            read_walk_set(characters_next, character_frames[characters_next]);
            character_loaded[characters_next] = true;
        }
        characters_next++;
    }
    return (float)characters_next / (float)CH_COUNT;
}

void sv_unload_characters(void)
{
    sv_unload_all_players();   // Os conjuntos do jogador podem estar apontando para estes quadros
    for (int i = 0; i < CH_COUNT; i++)
    {
        if (character_loaded[i]) unload_walk_set(character_frames[i]);
        character_loaded[i] = false;
    }
    characters_next = 0;
}

Texture2D sv_character_frame(int character, int direction, int frame)
{
    if (character < 0 || character >= CH_COUNT || !character_loaded[character]) return (Texture2D){0};
    if (direction < 0 || direction >= DIRECTION_COUNT) return (Texture2D){0};
    return character_frames[character][direction][frame % WALK_FRAME_COUNT];
}

void sv_load_player(int character)
{
    sv_unload_player();
    if (character < 0 || character >= CH_COUNT) character = CH_ESTAGIARIO;

    if (character_loaded[character])
    {
        // Já está na memória: o conjunto só aponta para os mesmos quadros
        memcpy(player_slot_frames[player_slot], character_frames[character], sizeof(player_slot_frames[player_slot]));
        player_slot_owned[player_slot] = false;
    }
    else
    {
        read_walk_set(character, player_slot_frames[player_slot]);
        player_slot_owned[player_slot] = true;
    }
    player_slot_loaded[player_slot] = true;
}

void sv_unload_player(void)
{
    if (!player_slot_loaded[player_slot]) return;
    if (player_slot_owned[player_slot]) unload_walk_set(player_slot_frames[player_slot]);
    else memset(player_slot_frames[player_slot], 0, sizeof(player_slot_frames[player_slot]));
    player_slot_owned[player_slot] = false;
    player_slot_loaded[player_slot] = false;
}

void sv_draw_player(int direction, int frame, Vector2 center, float scale, Color tint)
{
    Texture2D texture = player_slot_frames[player_slot][direction][frame % WALK_FRAME_COUNT];
    float w, h;

    if (!texture.id) return;
    w = (float)texture.width;
    h = (float)texture.height;
    scale *= CHARACTER_BASE_SIZE / w; // Arte maior que o pixel art ocupa o mesmo espaço no mundo
    DrawTexturePro(texture, (Rectangle){0, 0, w, h}, (Rectangle){center.x, center.y, w * scale, h * scale}, (Vector2){w * scale / 2.0f, h * scale / 2.0f}, 0.0f, tint);
}
