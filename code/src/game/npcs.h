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

typedef struct {
    int day;
    const char *const *lines;
    int line_count;
} NpcSpecialConversation;

typedef struct {
    const char *name;
    const char *sprite_folder;
    Vector2 position;             // Origem inicial, em pixels de referência relativos ao centro do mapa.
    Rectangle movement_area;      // Limites de movimento na mesma escala e referencial.
    NpcMovement movement;
    float movement_speed;
    int facing;
    bool seated_pose;
    const char *const *dialogue;
    int dialogue_line_count;
    const NpcSpecialConversation *special_conversations;
    int special_conversation_count;
    const char *const *hint;
    int hint_line_count;
} NpcDefinition;

/* Special conversations replace the default lines for their day; daily hints are appended. */
int npc_get_count(void);
const NpcDefinition *npc_get_definition(int npc_id);
int npc_get_daily_hint_owner(int day);
int npc_get_dialogue_line_count(int npc_id, int day);
const char *npc_get_dialogue_line(int npc_id, int day, int line_index);
bool npc_has_hint(int npc_id, int day);
bool npc_is_moving(int npc_id);
bool npc_blocks_player(int npc_id);
Rectangle npc_get_hitbox(int npc_id, float scale);
Rectangle npc_get_interaction_zone(int npc_id, float scale);
int npc_get_nearby(Rectangle player_hitbox, float scale);

bool initialize_npcs(float scale, Vector2 map_center);
void resize_npcs(float old_scale, float new_scale, Vector2 old_center, Vector2 new_center);
void update_npcs(float delta_time, float scale, Vector2 map_center, Rectangle map_bounds,
                 const Rectangle *obstacles, int obstacle_count, Rectangle player_hitbox);
void draw_npcs(float scale, Color tint, int day, float player_y, bool behind_player,
               bool hint_bubble_unlocked);
void draw_npc_portrait(int npc_id, Rectangle destination);
void unload_npcs(void);

#endif
