#ifndef SV_ASSETS_H
#define SV_ASSETS_H

#include <stdbool.h>
#include "raylib.h"
#include "sv_sprite_table.h"

/*
 * Sprites do Modo Sobrevivência. Cada imagem é uma faixa com os quadros lado a lado,
 * listada em sv_sprite_table.h (gerado a partir do catálogo de sprites).
 */

#define SV_SPRITE_PATH "code/assets/sprites/sobrevivencia/"

typedef enum
{
#define X(id, name, w, h, n) SPR_##id,
    SV_SPRITE_LIST(X)
#undef X
    SPR_COUNT
} SvSpriteId;

typedef struct
{
    Texture2D texture;
    int frame_width;
    int frame_height;
    int frames;
} SvSprite;

extern SvSprite sv_sprites[SPR_COUNT];

void sv_load_sprites(void);
void sv_unload_sprites(void);

/**
 * @brief Desenha um quadro de uma faixa, centrado em center.
 * @param rotation Graus. @param flip Espelha na horizontal.
 */
void sv_draw(SvSpriteId id, int frame, Vector2 center, float scale, float rotation, bool flip, Color tint);

/** @brief Desenha um quadro esticado em largura e altura próprias (feixes e linhas). */
void sv_draw_stretched(SvSpriteId id, int frame, Vector2 center, float width, float height, float rotation, Color tint);

// O personagem usa os quadros do andar que já existem no jogo (pasta em PERSONAGENS).
// Carregar outro personagem descarrega o anterior.
void sv_load_player(int character);
void sv_unload_player(void);
void sv_draw_player(int direction, int frame, Vector2 center, float scale, Color tint);

#endif // SV_ASSETS_H
