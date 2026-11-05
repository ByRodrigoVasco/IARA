#ifndef SV_GAME_H
#define SV_GAME_H

#include <stdbool.h>
#include "raylib.h"
#include "sv_rules.h"
#include "sv_assets.h"

/*
 * Simulação de uma partida do Modo Sobrevivência: jogador, armas, inimigos, chefes e
 * itens. Nada aqui desenha ou lê o teclado; quem chama passa o movimento e o desenho
 * fica em sv_render.c. Assim a partida pode ser simulada sem janela, nos testes.
 */

// Mapa: 8 x 6 áreas, cada uma com o tamanho de uma sala do catálogo.
#define SV_COLS 8
#define SV_ROWS 6
#define SV_CELL_W 531
#define SV_CELL_H 438
#define SV_WORLD_W (SV_COLS * SV_CELL_W)
#define SV_WORLD_H (SV_ROWS * SV_CELL_H)

#define SV_MAX_ENEMIES 340
#define SV_MAX_SHOTS 120
#define SV_MAX_ZONES 80
#define SV_MAX_HAZARDS 140
#define SV_MAX_GEMS 700
#define SV_MAX_FX 160
#define SV_MAX_WEAPONS 6
#define SV_MAX_PASSIVES 6
#define SV_WEAPON_MAX_LEVEL 5
#define SV_PASSIVE_MAX_LEVEL 5
#define SV_TRAIL 16

typedef enum { SV_PLAYING, SV_LEVELUP, SV_DEAD, SV_WON } SvPhase;

typedef enum
{
    W_GRAMPEADOR, W_PONTEIRO, W_CANECA, W_POSTITS, W_NAOPERTURBE, W_DEPLOY, W_TARJA, W_MODUS, W_COUNT
} SvWeaponId;

typedef enum
{
    P_CAFE, P_MANUAL, P_COLINHA, P_ERGONOMIA, P_CRACHA, P_SAUDE, P_NETWORK, P_HORAEXTRA, P_COUNT
} SvPassiveId;

typedef enum
{
    EN_CHAMADO, EN_NOTIFICACAO, EN_ESVAZIADO, EN_DADO, EN_ALUCINACAO, EN_AUTORESPOSTA,
    EN_CRACHA, EN_AFOGADO, EN_AUDITOR, EN_ECO, EN_MINI, EN_COUNT
} SvEnemyType;

typedef struct
{
    Vector2 pos;
    Vector2 vel;
    float hp, max_hp;
    float hurt_flash;
    float anim;
    float timer;          // Relógio próprio (zigue-zague, cuspe, puxão)
    float confused;       // Segundos de confusão (Tarja Preta)
    float wcd[W_COUNT];   // Intervalo de dano de cada arma sobre este inimigo
    float aux_angle;      // Direção fixa (Dado Enviesado)
    int type;
    int facing;           // 0 frente, 1 lado, 2 costas
    bool flip;
    bool fake;            // Cópia da Alucinação: some ao ser tocada e não dá nada
    bool wall;            // Anda reto, sem perseguir
    unsigned id;
    bool alive;
} SvEnemy;

typedef struct
{
    Vector2 pos, vel;
    float damage, radius, life;
    int pierce;
    unsigned hit_ids[4];
    bool alive;
} SvShot;

typedef enum { ZONE_CIRCLE, ZONE_RECT } SvZoneShape;
typedef enum { ZK_CANECA, ZK_DEPLOY, ZK_PONTEIRO, ZK_TARJA, ZK_PROMPT } SvZoneKind;

typedef struct
{
    Vector2 pos;
    float radius;          // Círculo: raio. Retângulo: meia largura
    float length;          // Retângulo: comprimento a partir de pos
    float angle;           // Graus
    float delay, life, max_life;
    float damage;
    float confuse;
    int shape, kind;
    bool fired;
    bool alive;
} SvZone;

typedef enum { HZ_CIRCLE, HZ_PROJ, HZ_BEAM } SvHazardKind;
typedef enum { EFF_NONE, EFF_SLOW, EFF_ROOT, EFF_INVERT } SvEffect;

typedef struct
{
    int kind;
    int sprite;            // SvSpriteId, ou -1 para desenhar um cartão
    Vector2 pos, vel;
    float radius;          // Raio (círculo e projétil) ou meia largura (feixe)
    float length;          // Feixe
    float angle;           // Graus
    float warn, warn_total;
    float life, max_life;
    float damage;
    float tick, tick_timer; // Dano repetido (0 = dano único)
    float effect_time;
    int effect;
    bool homing;
    bool applied;
    bool teleport_boss;    // Diego sai por esta porta
    bool alive;
} SvHazard;

typedef struct
{
    Vector2 pos;
    float value;
    int kind;              // 0 XP, 1 Banco de Horas, 2 Marmita, 3 Baú
    bool pulled;
    bool alive;
} SvGem;

typedef enum { FX_DEATH, FX_EXPLOSION, FX_LINK, FX_FLASH } SvFxKind;

typedef struct
{
    int kind;
    Vector2 pos, pos2;
    float t, life;
    float scale;
    bool alive;
} SvFx;

typedef struct { int id; int level; float cooldown; float angle; } SvWeapon;
typedef struct { int id; int level; } SvPassive;

typedef struct
{
    bool active;
    int id;
    Vector2 pos;
    float hp, max_hp;
    float radius;
    float timer;
    float cooldown[4];
    float attack_anim;     // Tempo restante da animação de ataque
    float scheduled;       // Ação marcada para daqui a alguns segundos
    int scheduled_id;
    Vector2 sched_pos;     // Dados da ação marcada
    Vector2 sched_dir;
    int shields;
    float invulnerable;
    float hurt_flash;
    int facing;
    bool flip;
    bool announced_final; // IARA: a trilha final já começou
} SvBoss;

typedef struct
{
    Vector2 pos;
    Vector2 facing_vector;
    int direction;         // Índice de direção do personagem (EAST, SOUTH...)
    float walk_time;
    float hp, max_hp;
    float invulnerable;
    float hurt_flash;
    float slow, root, invert;
    int level;
    float xp;
    int xp_needed;
    int pending_levels;
    bool revive_ready;
    float dmg_mult, cooldown_mult, area_mult, speed_mult, xp_mult, magnet_range, armor, duration_mult;
    float step_timer;
    SvWeapon weapons[SV_MAX_WEAPONS];
    int weapon_count;
    SvPassive passives[SV_MAX_PASSIVES];
    int passive_count;
} SvPlayer;

typedef struct { int kind; int id; } SvChoice; // kind: 0 arma nova, 1 evolui arma, 2 passivo novo, 3 evolui passivo, 4 marmita, 5 banco de horas

typedef struct
{
    SvPhase phase;
    float time;
    int act;
    int kills;
    int coins;             // Banco de Horas coletado na partida
    int bosses_defeated;
    int next_boss;
    float spawn_accumulator;
    unsigned next_enemy_id;
    unsigned rng;
    SvPlayer player;
    SvBoss boss;
    SvEnemy enemies[SV_MAX_ENEMIES];
    SvShot shots[SV_MAX_SHOTS];
    SvZone zones[SV_MAX_ZONES];
    SvHazard hazards[SV_MAX_HAZARDS];
    SvGem gems[SV_MAX_GEMS];
    SvFx fx[SV_MAX_FX];
    Vector2 trail[SV_TRAIL];
    float trail_timer;
    SvChoice choices[3];
    int choice_count;
    int area;              // Área do mapa sob o jogador
    int enemy_count;
    float banner;          // Aviso grande na tela (novo ato, chefe)
    const char *banner_text;
    float shake;
    bool overtime;
} SvRun;

extern SvRun sv_run;

/** @brief Começa uma partida nova com os upgrades permanentes aplicados. */
void sv_game_start(unsigned seed);

/** @brief Avança a partida. move é o movimento desejado, de -1 a 1 em cada eixo. */
void sv_game_update(float dt, Vector2 move);

/** @brief Aplica a escolha do nível (0 a 2) e retoma a partida. */
void sv_game_choose(int index);

/** @brief Coloca no Banco de Horas o que a partida rendeu e atualiza os recordes. Chamar uma vez ao fim. */
int sv_game_finish(void);

// Dados para os menus e o desenho.
const char *sv_weapon_name(int weapon);
const char *sv_weapon_description(int weapon);
SvSpriteId sv_weapon_icon(int weapon);
const char *sv_passive_name(int passive);
const char *sv_passive_description(int passive);
SvSpriteId sv_passive_icon(int passive);
const char *sv_choice_title(const SvChoice *choice);
const char *sv_choice_text(const SvChoice *choice);
SvSpriteId sv_choice_icon(const SvChoice *choice);
int sv_choice_level(const SvChoice *choice);
const char *sv_boss_name(int boss);
const char *sv_act_name(int act);
int sv_area_ambient(int area);
int sv_area_room(int row, int col);

// Auxiliares de simulação, usados também por sv_boss.c.
float sv_rand(void);                       // 0 a 1
float sv_rand_range(float lo, float hi);
Vector2 sv_player_pos(void);
void sv_hurt_player(float amount, int effect, float effect_time);
void sv_apply_status(int effect, float effect_time);
SvSpriteId sv_enemy_sprite(int type);
void sv_spawn_enemy_at(int type, Vector2 pos, bool fake);
void sv_spawn_fx(int kind, Vector2 pos, float life, float scale);
void sv_add_gem(Vector2 pos, int kind, float value);
void sv_banner(const char *text, float seconds);
void sv_boss_spawn(int id);
void sv_boss_update(float dt);
void sv_hazards_update(float dt);
bool sv_damage_boss(float amount);
void sv_boss_clear_hazards(void);

#endif // SV_GAME_H
