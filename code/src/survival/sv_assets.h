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
    int detail;       // Pixels do arquivo por pixel do mundo: 1 no pixel art, 6 na arte em alta resolução
    int frames;
    Texture2D glow;   // Só os pixels vermelhos-vivos (olhos, sangue), que brilham no escuro. id 0 = sem brilho
} SvSprite;

extern SvSprite sv_sprites[SPR_COUNT];

/** @brief Carrega todas as imagens de uma vez. */
void sv_load_sprites(void);

/** @brief Carrega a próxima imagem e devolve o andamento, de 0 a 1 (1 quando acabou). Para a tela de carregamento. */
float sv_load_sprites_step(void);
void sv_unload_sprites(void);

/**
 * @brief Desenha um quadro de uma faixa, centrado em center.
 * @param rotation Graus. @param flip Espelha na horizontal.
 */
void sv_draw(SvSpriteId id, int frame, Vector2 center, float scale, float rotation, bool flip, Color tint);

/** @brief Desenha só a parte que brilha de um quadro (mesmos parâmetros de sv_draw). Não faz nada se a imagem não tem brilho. */
void sv_draw_glow(SvSpriteId id, int frame, Vector2 center, bool flip, Color tint);

/** @brief Desenha um quadro esticado em largura e altura próprias (feixes e linhas). */
void sv_draw_stretched(SvSpriteId id, int frame, Vector2 center, float width, float height, float rotation, Color tint);

// Quadros de andar de todos os personagens (pasta em PERSONAGENS), lidos do disco uma vez só, na tela
// de carregamento: um personagem por chamada. Devolve o andamento, de 0 a 1. Com eles prontos, trocar
// de personagem (na escolha, no TAB da dupla ou ao começar a partida) não lê nada do disco.
float sv_load_characters_step(void);
void sv_unload_characters(void);
/** @brief Um quadro de andar do personagem, do que já foi carregado (textura vazia se ainda não foi). */
Texture2D sv_character_frame(int character, int direction, int frame);

// O jogador em cena usa os quadros do personagem: os já carregados, ou do disco se ainda não estiverem.
void sv_load_player(int character);
void sv_unload_player(void);
void sv_draw_player(int direction, int frame, Vector2 center, float scale, Color tint);

// Dupla: as três funções acima valem para o conjunto escolhido aqui (0 = jogador 1, 1 = jogador 2).
// O padrão é 0, então o solo não precisa chamar nada.
void sv_use_player_slot(int slot);
void sv_unload_all_players(void);   // Descarrega os dois conjuntos e volta para o 0

#endif // SV_ASSETS_H
