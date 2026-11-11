#ifndef NPCS_H
#define NPCS_H

#include <stdbool.h>
#include "raylib.h"

typedef enum {
    NPC_ANDERSON,
    NPC_JULIO,
    NPC_JV,
    NPC_LAURA,
    NPC_NICOLLAS,
    NPC_VASCO,
    NPC_ZITO,
    NPC_COUNT
} NpcId;

typedef enum {
    NPC_STATIONARY,
    NPC_MOVE_HORIZONTAL,
    NPC_MOVE_VERTICAL
} NpcMovement;

/* Quantos dias jogáveis a campanha tem; cada NPC tem uma fala própria para cada um. */
#define NPC_PLAYABLE_DAY_COUNT 9
/* Linhas da fala do dia e da dica de cada NPC. */
#define NPC_DAY_LINE_COUNT 2
#define NPC_HINT_LINE_COUNT 2

typedef struct {
    const char *name;
    const char *sprite_folder;
    Vector2 position;             // Lugar de origem (o "assento" que o NPC ocupa no primeiro dia), em pixels de referência relativos ao centro do mapa.
    Rectangle movement_area;      // Limites de movimento na mesma escala e referencial.
    NpcMovement movement;
    float movement_speed;
    int facing;
    bool seated_pose;
} NpcDefinition;

int npc_get_count(void);
const NpcDefinition *npc_get_definition(int npc_id);

/* Dia jogável (0 a 8) ou -1 se o dia não é jogável. */
int npc_get_playable_day_index(int day);
/* Quem dá a dica do dia (um só por dia jogável) ou -1. */
int npc_get_daily_hint_owner(int day);
bool npc_has_hint(int npc_id, int day);

/*
 * A conversa de um NPC: na primeira vez do dia ele cumprimenta (no Dia 1, dá as boas-vindas),
 * depois vem a fala do dia e, só para quem tem a dica do dia, a dica. Voltar a falar no mesmo
 * dia (first_today = false) pula o cumprimento.
 */
int npc_get_dialogue_line_count(int npc_id, int day, bool first_today);
const char *npc_get_dialogue_line(int npc_id, int day, bool first_today, int line_index);
const char *npc_get_greeting(int npc_id, int day);
const char *npc_get_day_line(int npc_id, int day, int line_index);
const char *npc_get_hint_line(int day, int line_index);

/* Qual "assento" (índice de NPC_DEFINITIONS) cada NPC ocupa no dia. Os lugares mudam a cada dia jogável. */
int npc_get_seat_for_day(int npc_id, int day);
void npc_arrange_for_day(int day, float scale, Vector2 map_center);

bool npc_is_moving(int npc_id);
bool npc_blocks_player(int npc_id);
Rectangle npc_get_hitbox(int npc_id, float scale);
Rectangle npc_get_interaction_zone(int npc_id, float scale);
int npc_get_nearby(Rectangle player_hitbox, float scale);

bool initialize_npcs(float scale, Vector2 map_center);
void resize_npcs(float old_scale, float new_scale, Vector2 old_center, Vector2 new_center);
void update_npcs(float delta_time, float scale, Vector2 map_center, Rectangle map_bounds,
                 const Rectangle *obstacles, int obstacle_count, Rectangle player_hitbox);
void draw_npcs(float scale, Color tint, float player_y, bool behind_player);
void draw_npc_portrait(int npc_id, Rectangle destination);
/* Posição do NPC em pixels da tela: serve para ancorar o balão e os indicadores. */
Vector2 npc_get_position(int npc_id);
void unload_npcs(void);

#endif
