#include <math.h>
#include <string.h>
#include "raymath.h"
#include "characters.h"
#include "sv_game.h"
#include "sv_meta.h"
#include "sv_audio.h"
#include "sv_map.h"

SvRun sv_run;

static bool finished_flag = false;

// -----------------------------------------------------------------------------
// Dados
// -----------------------------------------------------------------------------

typedef struct
{
    float hp, speed, damage;
    int xp;
    float radius;
} EnemyStats;

static const EnemyStats enemy_stats[EN_COUNT] = {
    [EN_CHAMADO]      = {6.0f, 52.0f, 4.0f, 1, 9.0f},
    [EN_NOTIFICACAO]  = {8.0f, 80.0f, 5.0f, 2, 9.0f},
    [EN_ESVAZIADO]    = {30.0f, 36.0f, 8.0f, 3, 11.0f},
    [EN_DADO]         = {20.0f, 74.0f, 10.0f, 3, 12.0f},
    [EN_ALUCINACAO]   = {16.0f, 62.0f, 7.0f, 3, 10.0f},
    [EN_AUTORESPOSTA] = {24.0f, 55.0f, 8.0f, 4, 11.0f},
    [EN_CRACHA]       = {28.0f, 78.0f, 11.0f, 5, 10.0f},
    [EN_AFOGADO]      = {50.0f, 50.0f, 12.0f, 6, 11.0f},
    [EN_AUDITOR]      = {260.0f, 60.0f, 18.0f, 25, 13.0f},
    [EN_ECO]          = {90.0f, 60.0f, 14.0f, 10, 11.0f},
    [EN_MINI]         = {8.0f, 92.0f, 4.0f, 1, 7.0f},
};

static const SvSpriteId enemy_sprites[EN_COUNT] = {
    SPR_en_chamado, SPR_en_notificacao, SPR_en_esvaziado, SPR_en_dado, SPR_en_alucinacao,
    SPR_en_autoresposta, SPR_en_cracha, SPR_en_afogado, SPR_en_auditor, SPR_en_eco, SPR_en_autorespostaMini};

SvSpriteId sv_enemy_sprite(int type)
{
    return enemy_sprites[type];
}

// Pesos de nascimento por ato.
typedef struct { int type; int weight; } SpawnWeight;

static const SpawnWeight act_spawns[SV_ACT_COUNT][8] = {
    {{EN_CHAMADO, 70}, {EN_NOTIFICACAO, 20}, {EN_ESVAZIADO, 10}, {-1, 0}},
    {{EN_CHAMADO, 25}, {EN_NOTIFICACAO, 20}, {EN_ESVAZIADO, 15}, {EN_DADO, 20}, {EN_ALUCINACAO, 20}, {-1, 0}},
    {{EN_ESVAZIADO, 10}, {EN_NOTIFICACAO, 15}, {EN_ALUCINACAO, 15}, {EN_AUTORESPOSTA, 25}, {EN_CRACHA, 25}, {EN_DADO, 10}, {-1, 0}},
    {{EN_AFOGADO, 25}, {EN_AUDITOR, 4}, {EN_CRACHA, 15}, {EN_AUTORESPOSTA, 20}, {EN_ESVAZIADO, 15}, {EN_NOTIFICACAO, 10}, {EN_DADO, 10}, {-1, 0}},
    {{EN_ECO, 20}, {EN_AUDITOR, 7}, {EN_AFOGADO, 20}, {EN_CRACHA, 15}, {EN_AUTORESPOSTA, 15}, {EN_ALUCINACAO, 12}, {EN_DADO, 10}, {-1, 0}},
};

static const char *weapon_names[W_COUNT] = {"Grampeador", "Ponteiro", "Caneca Arremessada", "Post-its Orbitais", "Não Perturbe", "Deploy em Produção", "Tarja Preta", "Modus Ponens", "Tabela-Verdade", "Colega de Plantão", "Nova Conversa", "Prompt da IARA"};
static const char *weapon_descriptions[W_COUNT] = {
    "Tiros retos no inimigo mais próximo.",
    "Golpe em linha para os dois lados.",
    "Sobe em arco e cai com dano em área.",
    "Giram ao seu redor e protegem de perto.",
    "Aura de dano que afasta quem chega perto.",
    "Raios em alvos aleatórios, também distantes.",
    "Faixa que confunde quem atinge.",
    "Se acerta A, salta para B, e assim por diante.",
    "Grade no chão que fere quem pisa nas casas acesas.",
    "Um colega humano segue você e luta.",
    "Empurra tudo e apaga tiros, com recarga longa.",
    "Ataque muito forte em linha reta, com recarga longa."};
static const SvSpriteId weapon_icons[W_COUNT] = {SPR_ic_grampeador, SPR_ic_ponteiro, SPR_ic_caneca, SPR_ic_postits, SPR_ic_naoperturbe, SPR_ic_deploy, SPR_ic_tarja, SPR_ic_modus, SPR_ic_tabela, SPR_ic_colega, SPR_ic_novaconversa, SPR_ic_prompt};

static const char *passive_names[P_COUNT] = {"Café", "Manual", "Colinha", "Ergonomia", "Crachá", "Plano de Saúde", "Network", "Hora Extra"};
static const char *passive_descriptions[P_COUNT] = {
    "Recarga mais rápida.", "Mais área nos poderes.", "Mais XP dos chips.", "Mais velocidade.",
    "Menos dano recebido.", "Mais vida máxima.", "Pega os itens de mais longe.", "Poderes duram mais."};
static const SvSpriteId passive_icons[P_COUNT] = {SPR_ps_cafe, SPR_ps_manual, SPR_ps_colinha, SPR_ps_ergonomia, SPR_ps_cracha, SPR_ps_saude, SPR_ps_network, SPR_ps_horaextra};

static const char *boss_names[BOSS_COUNT] = {"Impressora de Rescisões", "Modelo Enviesado", "Diego, o Crachá Ativo", "Renê, na cabeceira", "IARA, a Mãe-d'Água"};
static const char *act_names[SV_ACT_COUNT] = {"ATO I", "ATO II", "ATO III", "ATO IV", "ATO V"};

const char *sv_weapon_name(int weapon) { return weapon_names[weapon]; }
const char *sv_weapon_description(int weapon) { return weapon_descriptions[weapon]; }
SvSpriteId sv_weapon_icon(int weapon) { return weapon_icons[weapon]; }
const char *sv_passive_name(int passive) { return passive_names[passive]; }
const char *sv_passive_description(int passive) { return passive_descriptions[passive]; }
SvSpriteId sv_passive_icon(int passive) { return passive_icons[passive]; }
const char *sv_boss_name(int boss) { return boss_names[boss]; }
const char *sv_act_name(int act) { return act_names[act]; }

// Mapa: área de cada célula (ver o catálogo). OPE alterna entre os dois desenhos.
enum { A_OPE, A_COP, A_REU, A_DIR, A_DAT, A_TRE, A_FIN, A_ARQ, A_SUP, A_RHH, A_CAT, A_REC, A_ALA, A_FUN };

static const int area_grid[SV_ROWS][SV_COLS] = {
    {A_COP, A_COP, A_REU, A_REU, A_DIR, A_DIR, A_DAT, A_DAT},
    {A_FIN, A_FIN, A_REU, A_OPE, A_OPE, A_DIR, A_DAT, A_TRE},
    {A_FIN, A_ARQ, A_OPE, A_OPE, A_OPE, A_SUP, A_SUP, A_TRE},
    {A_ARQ, A_ARQ, A_OPE, A_OPE, A_OPE, A_SUP, A_SUP, A_TRE},
    {A_RHH, A_RHH, A_CAT, A_REC, A_REC, A_ALA, A_FUN, A_FUN},
    {A_RHH, A_RHH, A_CAT, A_REC, A_REC, A_ALA, A_ALA, A_FUN},
};

// Ambiente sonoro de cada área, na ordem de sv_audio.c.
static const int area_ambient[14] = {0, 1, 2, 3, 4, 5, 0, 6, 7, 8, 9, 10, 11, 12};


int sv_area_ambient(int area)
{
    return area_ambient[area];
}

int sv_area_room(int row, int col)
{
    int area = area_grid[row][col];

    (void)area;
    return SPR_fl_0_0 + row * SV_COLS + col;
}

// -----------------------------------------------------------------------------
// Utilidades
// -----------------------------------------------------------------------------

float sv_rand(void)
{
    unsigned x = sv_run.rng;

    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    sv_run.rng = x;
    return (float)(x & 0xFFFFFF) / 16777216.0f;
}

float sv_rand_range(float lo, float hi)
{
    return lo + (hi - lo) * sv_rand();
}

Vector2 sv_player_pos(void)
{
    return sv_run.player.pos;
}

void sv_banner(const char *text, float seconds)
{
    sv_run.banner_text = text;
    sv_run.banner = seconds;
}

static Vector2 clamp_to_world(Vector2 p, float margin)
{
    p.x = Clamp(p.x, margin, SV_WORLD_W - margin);
    p.y = Clamp(p.y, margin, SV_WORLD_H - margin);
    return p;
}

static Vector2 direction_of(Vector2 from, Vector2 to)
{
    Vector2 d = Vector2Subtract(to, from);
    float length = Vector2Length(d);

    if (length < 0.001f) return (Vector2){1.0f, 0.0f};
    return Vector2Scale(d, 1.0f / length);
}

// Direção de 8 lados do personagem (EAST, SOUTH, ...) para um vetor de movimento
static int direction_index(Vector2 v)
{
    float degrees = atan2f(v.y, v.x) * RAD2DEG;
    int sector = (int)roundf(degrees / 45.0f);

    switch (sector)
    {
    case 0: return EAST;
    case 1: return SOUTHEAST;
    case 2: return SOUTH;
    case 3: return SOUTHWEST;
    case -1: return NORTHEAST;
    case -2: return NORTH;
    case -3: return NORTHWEST;
    default: return WEST;
    }
}

static int passive_level(int id)
{
    for (int i = 0; i < sv_run.player.passive_count; i++)
    {
        if (sv_run.player.passives[i].id == id) return sv_run.player.passives[i].level;
    }
    return 0;
}

static SvWeapon *find_weapon(int id)
{
    for (int i = 0; i < sv_run.player.weapon_count; i++)
    {
        if (sv_run.player.weapons[i].id == id) return &sv_run.player.weapons[i];
    }
    return NULL;
}

static void recompute_stats(void)
{
    SvPlayer *p = &sv_run.player;
    float old_max = p->max_hp;
    const int *u = sv_meta.upgrades;

    p->max_hp = SV_BASE_HP + 10.0f * u[UPG_VIDA] + 12.0f * passive_level(P_SAUDE);
    p->dmg_mult = 1.0f + 0.05f * u[UPG_DANO];
    p->speed_mult = 1.0f + 0.04f * u[UPG_VELOCIDADE] + 0.06f * passive_level(P_ERGONOMIA);
    p->cooldown_mult = (1.0f - 0.04f * u[UPG_RECARGA]) * (1.0f - 0.06f * passive_level(P_CAFE));
    if (p->cooldown_mult < 0.4f) p->cooldown_mult = 0.4f;
    p->area_mult = 1.0f + 0.05f * u[UPG_AREA] + 0.08f * passive_level(P_MANUAL);
    p->magnet_range = 45.0f * (1.0f + 0.15f * u[UPG_IMA] + 0.2f * passive_level(P_NETWORK));
    p->xp_mult = 1.0f + 0.05f * u[UPG_XP] + 0.10f * passive_level(P_COLINHA);
    p->armor = (float)u[UPG_ARMADURA] + (float)passive_level(P_CRACHA);
    p->duration_mult = 1.0f + 0.10f * passive_level(P_HORAEXTRA);

    if (old_max > 0.0f) p->hp += p->max_hp - old_max;
    if (p->hp > p->max_hp) p->hp = p->max_hp;
}

// -----------------------------------------------------------------------------
// Efeitos, itens e inimigos
// -----------------------------------------------------------------------------

void sv_spawn_fx(int kind, Vector2 pos, float life, float scale)
{
    for (int i = 0; i < SV_MAX_FX; i++)
    {
        SvFx *fx = &sv_run.fx[i];

        if (fx->alive) continue;
        memset(fx, 0, sizeof(*fx));
        fx->alive = true;
        fx->kind = kind;
        fx->pos = pos;
        fx->life = life;
        fx->scale = scale;
        return;
    }
}

static void spawn_link(Vector2 a, Vector2 b)
{
    for (int i = 0; i < SV_MAX_FX; i++)
    {
        SvFx *fx = &sv_run.fx[i];

        if (fx->alive) continue;
        memset(fx, 0, sizeof(*fx));
        fx->alive = true;
        fx->kind = FX_LINK;
        fx->pos = a;
        fx->pos2 = b;
        fx->life = 0.25f;
        fx->scale = 1.0f;
        return;
    }
}

void sv_add_gem(Vector2 pos, int kind, float value)
{
    for (int i = 0; i < SV_MAX_GEMS; i++)
    {
        SvGem *gem = &sv_run.gems[i];

        if (gem->alive) continue;
        gem->alive = true;
        gem->pulled = false;
        gem->kind = kind;
        gem->value = value;
        gem->pos = pos;
        return;
    }
}

void sv_spawn_enemy_at(int type, Vector2 pos, bool fake)
{
    float scale = sv_enemy_hp_scale(sv_run.time);

    for (int i = 0; i < SV_MAX_ENEMIES; i++)
    {
        SvEnemy *e = &sv_run.enemies[i];

        if (e->alive) continue;
        memset(e, 0, sizeof(*e));
        e->alive = true;
        e->type = type;
        e->pos = clamp_to_world(pos, 20.0f);
        e->max_hp = enemy_stats[type].hp * scale;
        if (type == EN_AUDITOR) e->max_hp *= 1.0f;
        e->fake = fake;
        if (fake) e->max_hp = 1.0f;
        e->hp = e->max_hp;
        e->id = ++sv_run.next_enemy_id;
        e->timer = sv_rand() * 4.0f;
        sv_run.enemy_count++;
        return;
    }
}

static void spawn_ring_position(float distance, Vector2 *out)
{
    float angle = sv_rand() * 2.0f * PI;
    Vector2 p = Vector2Add(sv_run.player.pos, (Vector2){cosf(angle) * distance, sinf(angle) * distance});

    // Se o ponto cai fora do mapa, tenta o lado oposto
    if (p.x < 20 || p.y < 20 || p.x > SV_WORLD_W - 20 || p.y > SV_WORLD_H - 20)
    {
        p = Vector2Add(sv_run.player.pos, (Vector2){-cosf(angle) * distance, -sinf(angle) * distance});
    }
    *out = clamp_to_world(p, 20.0f);
}

static int pick_enemy_type(void)
{
    const SpawnWeight *table = act_spawns[sv_run.act];
    int total = 0;
    int roll;

    for (int i = 0; table[i].type >= 0; i++) total += table[i].weight;
    roll = (int)(sv_rand() * total);
    for (int i = 0; table[i].type >= 0; i++)
    {
        if (roll < table[i].weight) return table[i].type;
        roll -= table[i].weight;
    }
    return EN_CHAMADO;
}

// Faz nascer um evento de inimigos e devolve quantos entraram.
static int spawn_event(void)
{
    int type = pick_enemy_type();
    Vector2 origin;
    int count = 1;

    spawn_ring_position(520.0f + sv_rand() * 60.0f, &origin);

    if (type == EN_CHAMADO)
    {
        count = 1 + (int)(sv_rand() * 4.0f);
        for (int i = 0; i < count; i++)
        {
            sv_spawn_enemy_at(type, Vector2Add(origin, (Vector2){sv_rand_range(-24, 24), sv_rand_range(-24, 24)}), false);
        }
    }
    else if (type == EN_DADO)
    {
        // Parede de dados que cruza o mapa na direção do jogador
        Vector2 toward = direction_of(origin, sv_run.player.pos);
        Vector2 side = {-toward.y, toward.x};
        float angle = atan2f(toward.y, toward.x);

        count = 7;
        for (int i = 0; i < count; i++)
        {
            float offset = ((float)i - 3.0f) * 36.0f;
            SvEnemy *last;

            sv_spawn_enemy_at(type, Vector2Add(origin, Vector2Scale(side, offset)), false);
            last = NULL;
            for (int j = SV_MAX_ENEMIES - 1; j >= 0; j--)
            {
                if (sv_run.enemies[j].alive && sv_run.enemies[j].id == sv_run.next_enemy_id) { last = &sv_run.enemies[j]; break; }
            }
            if (last)
            {
                last->wall = true;
                last->aux_angle = angle;
            }
        }
    }
    else if (type == EN_ALUCINACAO)
    {
        // Quatro rostos iguais, e só um é de verdade
        int real = (int)(sv_rand() * 4.0f);

        count = 4;
        for (int i = 0; i < count; i++)
        {
            sv_spawn_enemy_at(type, Vector2Add(origin, (Vector2){sv_rand_range(-40, 40), sv_rand_range(-40, 40)}), i != real);
        }
    }
    else
    {
        sv_spawn_enemy_at(type, origin, false);
    }

    return count;
}

static void kill_enemy(SvEnemy *e)
{
    int coin_chance = 4 + sv_run.act;

    e->alive = false;
    sv_run.enemy_count--;
    sv_run.kills++;

    sv_spawn_fx(FX_DEATH, e->pos, 0.35f, 1.0f);

    if (e->fake) return;

    if (e->type == EN_CHAMADO) sv_sfx(SND_chamadoMorre);
    sv_add_gem(e->pos, 0, (float)enemy_stats[e->type].xp);

    if (e->type == EN_AUTORESPOSTA)
    {
        sv_sfx(SND_autoRespostaDivide);
        for (int i = 0; i < 2; i++) sv_spawn_enemy_at(EN_MINI, Vector2Add(e->pos, (Vector2){i ? 12.0f : -12.0f, 0.0f}), false);
    }

    if (e->type == EN_AUDITOR)
    {
        sv_add_gem(Vector2Add(e->pos, (Vector2){10, 0}), 3, 1.0f);
        sv_add_gem(Vector2Add(e->pos, (Vector2){-10, 0}), 1, 1.0f);
    }
    else if ((int)(sv_rand() * 100.0f) < coin_chance)
    {
        sv_add_gem(Vector2Add(e->pos, (Vector2){8, 4}), 1, 1.0f);
    }
    else if ((int)(sv_rand() * 1000.0f) < 9)
    {
        sv_add_gem(Vector2Add(e->pos, (Vector2){-8, 4}), 2, 25.0f);
    }
}

static void damage_enemy(SvEnemy *e, float damage, Vector2 from, float knockback)
{
    if (!e->alive) return;

    if (e->fake)
    {
        sv_sfx(SND_alucinacaoSome);
        kill_enemy(e);
        return;
    }

    e->hp -= damage * sv_run.player.dmg_mult;
    e->hurt_flash = 0.1f;
    sv_sfx(SND_acertaInimigo);

    if (e->type != EN_AUDITOR && knockback > 0.0f)
    {
        e->vel = Vector2Add(e->vel, Vector2Scale(direction_of(from, e->pos), knockback));
    }

    if (e->hp <= 0.0f) kill_enemy(e);
}

// -----------------------------------------------------------------------------
// Jogador
// -----------------------------------------------------------------------------

void sv_hurt_player(float amount, int effect, float effect_time)
{
    SvPlayer *p = &sv_run.player;
    float damage;

    if (p->invulnerable > 0.0f || sv_run.phase != SV_PLAYING) return;

    damage = amount - p->armor;
    if (damage < amount * 0.4f) damage = amount * 0.4f;
    if (damage < 1.0f) damage = 1.0f;

    p->hp -= damage;
    p->invulnerable = 0.45f;
    p->hurt_flash = 0.25f;
    sv_run.shake = 0.15f;
    sv_sfx(SND_dano);

    if (effect == EFF_SLOW && effect_time > p->slow) p->slow = effect_time;
    if (effect == EFF_ROOT && effect_time > p->root) p->root = effect_time;
    if (effect == EFF_INVERT && effect_time > p->invert) p->invert = effect_time;
}

void sv_apply_status(int effect, float effect_time)
{
    SvPlayer *p = &sv_run.player;

    if (effect == EFF_SLOW && effect_time > p->slow) p->slow = effect_time;
    if (effect == EFF_ROOT && effect_time > p->root) p->root = effect_time;
    if (effect == EFF_INVERT && effect_time > p->invert) p->invert = effect_time;
}

// -----------------------------------------------------------------------------
// Alvos
// -----------------------------------------------------------------------------

typedef struct
{
    bool found;
    bool is_boss;
    SvEnemy *enemy;
    Vector2 pos;
} Target;

static bool id_listed(unsigned id, const unsigned *list, int count)
{
    for (int i = 0; i < count; i++)
    {
        if (list[i] == id) return true;
    }
    return false;
}

static Target nearest_target(Vector2 from, float max_distance, const unsigned *skip, int skip_count)
{
    Target best = {0};
    float best_distance = max_distance;

    for (int i = 0; i < SV_MAX_ENEMIES; i++)
    {
        SvEnemy *e = &sv_run.enemies[i];
        float d;

        if (!e->alive || id_listed(e->id, skip, skip_count)) continue;
        d = Vector2Distance(from, e->pos);
        if (d < best_distance)
        {
            best_distance = d;
            best.found = true;
            best.is_boss = false;
            best.enemy = e;
            best.pos = e->pos;
        }
    }

    if (sv_run.boss.active && !id_listed(0xB055u, skip, skip_count))
    {
        float d = Vector2Distance(from, sv_run.boss.pos) - sv_run.boss.radius;

        if (d < best_distance)
        {
            best.found = true;
            best.is_boss = true;
            best.enemy = NULL;
            best.pos = sv_run.boss.pos;
        }
    }

    return best;
}

static void damage_target(const Target *target, float damage, Vector2 from, float knockback)
{
    if (!target->found) return;
    if (target->is_boss) sv_damage_boss(damage * sv_run.player.dmg_mult);
    else damage_enemy(target->enemy, damage, from, knockback);
}

static Vector2 random_enemy_position(float max_distance, bool *found)
{
    int candidates[SV_MAX_ENEMIES];
    int count = 0;

    for (int i = 0; i < SV_MAX_ENEMIES; i++)
    {
        SvEnemy *e = &sv_run.enemies[i];

        if (e->alive && Vector2Distance(e->pos, sv_run.player.pos) < max_distance) candidates[count++] = i;
    }

    if (count == 0)
    {
        *found = false;
        return Vector2Add(sv_run.player.pos, (Vector2){sv_rand_range(-120, 120), sv_rand_range(-120, 120)});
    }

    *found = true;
    return sv_run.enemies[candidates[(int)(sv_rand() * count) % count]].pos;
}

// -----------------------------------------------------------------------------
// Armas
// -----------------------------------------------------------------------------

static SvShot *new_shot(void)
{
    for (int i = 0; i < SV_MAX_SHOTS; i++)
    {
        if (!sv_run.shots[i].alive)
        {
            memset(&sv_run.shots[i], 0, sizeof(SvShot));
            sv_run.shots[i].alive = true;
            return &sv_run.shots[i];
        }
    }
    return NULL;
}

static SvZone *new_zone(void)
{
    for (int i = 0; i < SV_MAX_ZONES; i++)
    {
        if (!sv_run.zones[i].alive)
        {
            memset(&sv_run.zones[i], 0, sizeof(SvZone));
            sv_run.zones[i].alive = true;
            return &sv_run.zones[i];
        }
    }
    return NULL;
}

static Vector2 aim_vector(float max_distance)
{
    Target target = nearest_target(sv_run.player.pos, max_distance, NULL, 0);

    if (target.found) return direction_of(sv_run.player.pos, target.pos);
    return sv_run.player.facing_vector;
}

static void add_strike_zone(Vector2 origin, float angle, float length, float half_width, float damage, float confuse, int kind)
{
    SvZone *z = new_zone();

    if (!z) return;
    z->shape = ZONE_RECT;
    z->kind = kind;
    z->pos = origin;
    z->angle = angle;
    z->length = length;
    z->radius = half_width;
    z->damage = damage;
    z->confuse = confuse;
    z->delay = 0.0f;
    z->life = z->max_life = 0.3f;
}

static void fire_weapon(SvWeapon *w)
{
    SvPlayer *p = &sv_run.player;
    int level = w->level;
    float base_cd = 1.0f;
    float angle;

    switch (w->id)
    {
    case W_GRAMPEADOR:
    {
        int count = 1 + (level >= 3) + (level >= 5);
        Vector2 aim = aim_vector(380.0f);
        float base_angle = atan2f(aim.y, aim.x);

        base_cd = 0.75f - 0.06f * level;
        for (int i = 0; i < count; i++)
        {
            SvShot *s = new_shot();
            float a = base_angle + ((float)i - (count - 1) / 2.0f) * 0.16f;

            if (!s) break;
            s->pos = p->pos;
            s->vel = (Vector2){cosf(a) * 380.0f, sinf(a) * 380.0f};
            s->damage = 7.0f + 3.0f * level;
            s->radius = 7.0f;
            s->life = 1.3f * p->duration_mult;
            s->pierce = level >= 4 ? 2 : 1;
        }
        sv_sfx(SND_grampeador);
        break;
    }
    case W_PONTEIRO:
    {
        float reach = (56.0f + 10.0f * level) * p->area_mult;
        float width = (10.0f + 1.5f * level) * p->area_mult;

        base_cd = 1.7f - 0.13f * level;
        angle = atan2f(p->facing_vector.y, p->facing_vector.x) * RAD2DEG;
        add_strike_zone(p->pos, angle, reach, width, 12.0f + 5.0f * level, 0.0f, ZK_PONTEIRO);
        add_strike_zone(p->pos, angle + 180.0f, reach, width, 12.0f + 5.0f * level, 0.0f, ZK_PONTEIRO);
        sv_sfx(SND_ponteiro);
        break;
    }
    case W_CANECA:
    {
        int count = 1 + (level >= 3) + (level >= 5);

        base_cd = 2.3f - 0.15f * level;
        for (int i = 0; i < count; i++)
        {
            bool found;
            Vector2 target = random_enemy_position(300.0f, &found);
            SvZone *z = new_zone();

            if (!z) break;
            z->shape = ZONE_CIRCLE;
            z->kind = ZK_CANECA;
            z->pos = target;
            z->radius = (36.0f + 5.0f * level) * p->area_mult;
            z->damage = 14.0f + 6.0f * level;
            z->delay = 0.65f;
            z->life = z->max_life = 0.65f + 0.35f;
        }
        sv_sfx(SND_caneca);
        break;
    }
    case W_DEPLOY:
    {
        int count = 1 + level / 2;

        base_cd = 2.7f - 0.2f * level;
        for (int i = 0; i < count; i++)
        {
            bool found;
            Vector2 target = random_enemy_position(420.0f, &found);
            SvZone *z = new_zone();

            if (!z) break;
            z->shape = ZONE_CIRCLE;
            z->kind = ZK_DEPLOY;
            z->pos = target;
            z->radius = (26.0f + 3.0f * level) * p->area_mult;
            z->damage = 20.0f + 8.0f * level;
            z->delay = 0.4f;
            z->life = z->max_life = 0.4f + 0.45f;
        }
        break;
    }
    case W_TARJA:
    {
        float length = (110.0f + 14.0f * level) * p->area_mult;

        base_cd = 3.3f - 0.2f * level;
        angle = atan2f(p->facing_vector.y, p->facing_vector.x) * RAD2DEG;
        add_strike_zone(p->pos, angle, length, 17.0f * p->area_mult, 5.0f + 2.0f * level, 2.0f + 0.3f * level, ZK_TARJA);
        sv_sfx(SND_tarja);
        break;
    }
    case W_MODUS:
    {
        int hops = 2 + level;
        unsigned hit[8];
        int hit_count = 0;
        Vector2 from = p->pos;
        float range = 300.0f;
        float damage = 10.0f + 4.0f * level;

        base_cd = 1.9f - 0.14f * level;
        for (int i = 0; i < hops && hit_count < 8; i++)
        {
            Target t = nearest_target(from, range, hit, hit_count);

            if (!t.found) break;
            hit[hit_count++] = t.is_boss ? 0xB055u : t.enemy->id;
            spawn_link(from, t.pos);
            damage_target(&t, damage, from, 20.0f);
            from = t.pos;
            range = 170.0f;
        }
        if (hit_count > 0) sv_sfx(SND_modus);
        else base_cd = 0.3f;
        break;
    }
    case W_PROMPT:
    {
        Vector2 aim = aim_vector(420.0f);

        base_cd = 7.0f - 0.6f * level;
        angle = atan2f(aim.y, aim.x) * RAD2DEG;
        add_strike_zone(p->pos, angle, 280.0f * p->area_mult, 14.0f * p->area_mult, 40.0f + 15.0f * level, 0.0f, ZK_PROMPT);
        sv_sfx(SND_prompt);
        break;
    }
    case W_NOVACONVERSA:
    {
        float radius = 260.0f * p->area_mult;

        base_cd = 15.0f - 1.5f * level;
        for (int i = 0; i < SV_MAX_ENEMIES; i++)
        {
            SvEnemy *e = &sv_run.enemies[i];

            if (e->alive && Vector2Distance(e->pos, p->pos) < radius) damage_enemy(e, 6.0f + 2.0f * level, p->pos, 520.0f);
        }
        // Apaga os tiros inimigos que estão ao alcance
        for (int i = 0; i < SV_MAX_HAZARDS; i++)
        {
            SvHazard *h = &sv_run.hazards[i];

            if (h->alive && h->kind == HZ_PROJ && Vector2Distance(h->pos, p->pos) < radius) h->alive = false;
        }
        sv_spawn_fx(FX_WAVE, p->pos, 0.5f, radius / 36.0f);
        sv_sfx(SND_novaconversa);
        break;
    }
    default:
        break;
    }

    w->cooldown = base_cd * p->cooldown_mult;
}

static void update_continuous_weapon(SvWeapon *w, float dt)
{
    SvPlayer *p = &sv_run.player;
    int level = w->level;

    if (w->id == W_POSTITS)
    {
        float radius = (50.0f + 4.0f * level) * p->area_mult;
        float damage = 5.0f + 2.0f * level;

        w->angle += dt * 2.6f;
        for (int i = 0; i < SV_MAX_ENEMIES; i++)
        {
            SvEnemy *e = &sv_run.enemies[i];
            float d;

            if (!e->alive || e->wcd[W_POSTITS] > 0.0f) continue;
            d = Vector2Distance(e->pos, p->pos);
            if (fabsf(d - radius) < 13.0f + enemy_stats[e->type].radius)
            {
                e->wcd[W_POSTITS] = 0.45f;
                sv_sfx(SND_postits);
                damage_enemy(e, damage, p->pos, 40.0f);
            }
        }
        if (sv_run.boss.active && fabsf(Vector2Distance(sv_run.boss.pos, p->pos) - radius) < 13.0f + sv_run.boss.radius)
        {
            if (w->cooldown <= 0.0f)
            {
                w->cooldown = 0.45f;
                sv_damage_boss(damage * p->dmg_mult);
            }
        }
        if (w->cooldown > 0.0f) w->cooldown -= dt;
    }
    else if (w->id == W_NAOPERTURBE)
    {
        float radius = (52.0f + 8.0f * level) * p->area_mult;

        w->angle += dt;
        if (w->cooldown > 0.0f)
        {
            w->cooldown -= dt;
            return;
        }
        w->cooldown = 0.5f * p->cooldown_mult;
        for (int i = 0; i < SV_MAX_ENEMIES; i++)
        {
            SvEnemy *e = &sv_run.enemies[i];

            if (e->alive && Vector2Distance(e->pos, p->pos) < radius + enemy_stats[e->type].radius)
            {
                damage_enemy(e, 3.0f + 2.0f * level, p->pos, 90.0f);
            }
        }
        if (sv_run.boss.active && Vector2Distance(sv_run.boss.pos, p->pos) < radius + sv_run.boss.radius)
        {
            sv_damage_boss((3.0f + 2.0f * level) * p->dmg_mult);
        }
        sv_sfx(SND_naoperturbe);
    }
    else if (w->id == W_TABELA)
    {
        float radius = (46.0f + 6.0f * level) * p->area_mult;

        w->angle += dt;
        if (w->cooldown > 0.0f)
        {
            w->cooldown -= dt;
            return;
        }
        w->cooldown = 0.6f * p->cooldown_mult;
        w->level = level;
        {
            // As casas acendem em xadrez e trocam de cor a cada pulso
            int phase = (int)(w->angle / 0.6f) % 2;

            for (int i = 0; i < SV_MAX_ENEMIES; i++)
            {
                SvEnemy *e = &sv_run.enemies[i];
                int cell;

                if (!e->alive || fabsf(e->pos.x - p->pos.x) > radius || fabsf(e->pos.y - p->pos.y) > radius) continue;
                cell = (int)floorf(e->pos.x / 18.0f) + (int)floorf(e->pos.y / 18.0f);
                if (((cell % 2) + 2) % 2 == phase) damage_enemy(e, 4.0f + 2.0f * level, p->pos, 0.0f);
            }
            if (sv_run.boss.active && fabsf(sv_run.boss.pos.x - p->pos.x) < radius && fabsf(sv_run.boss.pos.y - p->pos.y) < radius)
            {
                sv_damage_boss((4.0f + 2.0f * level) * p->dmg_mult);
            }
        }
        sv_sfx(SND_tabela);
    }
    else if (w->id == W_COLEGA)
    {
        int count = 1 + (level >= 3) + (level >= 5);
        bool attacked = false;

        w->angle += dt;
        if (w->cooldown > 0.0f) w->cooldown -= dt;

        for (int c = 0; c < count; c++)
        {
            Vector2 *pos = &sv_run.colega_pos[c];
            float a = (float)c * 2.4f + w->angle * 0.4f;
            Vector2 target = Vector2Add(p->pos, (Vector2){cosf(a) * 34.0f, sinf(a) * 24.0f});
            Target near_enemy = nearest_target(*pos, 170.0f, NULL, 0);

            if (near_enemy.found) target = near_enemy.pos;
            if (Vector2Distance(*pos, p->pos) > 420.0f) *pos = p->pos;
            if (Vector2Distance(*pos, target) > 14.0f)
            {
                float speed = near_enemy.found ? 130.0f : 150.0f + Vector2Distance(*pos, p->pos);

                *pos = Vector2Add(*pos, Vector2Scale(Vector2Normalize(Vector2Subtract(target, *pos)), speed * dt));
            }
            sv_map_collide(pos, 6.0f, 8.0f);

            if (w->cooldown <= 0.0f)
            {
                for (int i = 0; i < SV_MAX_ENEMIES; i++)
                {
                    SvEnemy *e = &sv_run.enemies[i];

                    if (e->alive && Vector2Distance(e->pos, *pos) < 26.0f)
                    {
                        damage_enemy(e, 8.0f + 4.0f * level, *pos, 60.0f);
                        attacked = true;
                    }
                }
                if (sv_run.boss.active && Vector2Distance(sv_run.boss.pos, *pos) < 26.0f + sv_run.boss.radius)
                {
                    sv_damage_boss((8.0f + 4.0f * level) * p->dmg_mult);
                    attacked = true;
                }
            }
        }

        if (attacked)
        {
            w->cooldown = 0.7f * p->cooldown_mult;
            sv_sfx(SND_colega);
        }
    }
}

static void update_weapons(float dt)
{
    SvPlayer *p = &sv_run.player;

    for (int i = 0; i < p->weapon_count; i++)
    {
        SvWeapon *w = &p->weapons[i];

        if (w->id == W_POSTITS || w->id == W_NAOPERTURBE || w->id == W_TABELA || w->id == W_COLEGA)
        {
            update_continuous_weapon(w, dt);
            continue;
        }

        w->cooldown -= dt;
        if (w->cooldown <= 0.0f) fire_weapon(w);
    }
}

static void apply_zone(SvZone *z)
{
    float angle = z->angle * DEG2RAD;
    Vector2 direction = {cosf(angle), sinf(angle)};
    Vector2 perpendicular = {-direction.y, direction.x};

    for (int i = 0; i < SV_MAX_ENEMIES; i++)
    {
        SvEnemy *e = &sv_run.enemies[i];
        bool inside = false;
        float enemy_radius;

        if (!e->alive) continue;
        enemy_radius = enemy_stats[e->type].radius;

        if (z->shape == ZONE_CIRCLE)
        {
            inside = Vector2Distance(e->pos, z->pos) < z->radius + enemy_radius;
        }
        else
        {
            Vector2 rel = Vector2Subtract(e->pos, z->pos);
            float along = Vector2DotProduct(rel, direction);
            float across = Vector2DotProduct(rel, perpendicular);

            inside = along >= -enemy_radius && along <= z->length + enemy_radius && fabsf(across) <= z->radius + enemy_radius;
        }

        if (!inside) continue;
        if (z->confuse > 0.0f) e->confused = z->confuse;
        damage_enemy(e, z->damage, z->pos, z->kind == ZK_CANECA ? 70.0f : 30.0f);
    }

    if (sv_run.boss.active)
    {
        bool inside;

        if (z->shape == ZONE_CIRCLE)
        {
            inside = Vector2Distance(sv_run.boss.pos, z->pos) < z->radius + sv_run.boss.radius;
        }
        else
        {
            Vector2 rel = Vector2Subtract(sv_run.boss.pos, z->pos);
            float along = Vector2DotProduct(rel, direction);
            float across = Vector2DotProduct(rel, perpendicular);

            inside = along >= -sv_run.boss.radius && along <= z->length + sv_run.boss.radius && fabsf(across) <= z->radius + sv_run.boss.radius;
        }
        if (inside) sv_damage_boss(z->damage * sv_run.player.dmg_mult);
    }
}

static void update_zones(float dt)
{
    for (int i = 0; i < SV_MAX_ZONES; i++)
    {
        SvZone *z = &sv_run.zones[i];

        if (!z->alive) continue;

        if (z->delay > 0.0f)
        {
            z->delay -= dt;
            z->life -= dt;
            if (z->delay > 0.0f) continue;
        }

        if (!z->fired)
        {
            z->fired = true;
            apply_zone(z);
            if (z->kind == ZK_CANECA)
            {
                sv_spawn_fx(FX_EXPLOSION, z->pos, 0.35f, z->radius / 28.0f);
            }
            else if (z->kind == ZK_DEPLOY)
            {
                sv_sfx(SND_deploy);
            }
        }

        z->life -= dt;
        if (z->life <= 0.0f) z->alive = false;
    }
}

static void update_shots(float dt)
{
    for (int i = 0; i < SV_MAX_SHOTS; i++)
    {
        SvShot *s = &sv_run.shots[i];

        if (!s->alive) continue;

        s->pos = Vector2Add(s->pos, Vector2Scale(s->vel, dt));
        if (sv_map_point_blocked(s->pos, 2.0f))
        {
            s->alive = false;
            continue;
        }
        s->life -= dt;
        if (s->life <= 0.0f)
        {
            s->alive = false;
            continue;
        }

        for (int j = 0; j < SV_MAX_ENEMIES && s->alive; j++)
        {
            SvEnemy *e = &sv_run.enemies[j];

            if (!e->alive || id_listed(e->id, s->hit_ids, 4)) continue;
            if (Vector2Distance(s->pos, e->pos) < s->radius + enemy_stats[e->type].radius)
            {
                memmove(&s->hit_ids[1], &s->hit_ids[0], sizeof(unsigned) * 3);
                s->hit_ids[0] = e->id;
                damage_enemy(e, s->damage, Vector2Subtract(s->pos, s->vel), 30.0f);
                if (--s->pierce <= 0) s->alive = false;
            }
        }

        if (s->alive && sv_run.boss.active && !id_listed(0xB055u, s->hit_ids, 4))
        {
            if (Vector2Distance(s->pos, sv_run.boss.pos) < s->radius + sv_run.boss.radius)
            {
                memmove(&s->hit_ids[1], &s->hit_ids[0], sizeof(unsigned) * 3);
                s->hit_ids[0] = 0xB055u;
                sv_damage_boss(s->damage * sv_run.player.dmg_mult);
                if (--s->pierce <= 0) s->alive = false;
            }
        }
    }
}

// -----------------------------------------------------------------------------
// Inimigos
// -----------------------------------------------------------------------------

static void update_enemies(float dt)
{
    SvPlayer *p = &sv_run.player;
    float danger_scale = 1.0f + sv_run.time / 600.0f * 0.8f;

    for (int i = 0; i < SV_MAX_ENEMIES; i++)
    {
        SvEnemy *e = &sv_run.enemies[i];
        const EnemyStats *stats;
        Vector2 toward, desired;
        float distance;

        if (!e->alive) continue;
        stats = &enemy_stats[e->type];

        e->anim += dt;
        e->timer += dt;
        if (e->hurt_flash > 0.0f) e->hurt_flash -= dt;
        if (e->confused > 0.0f) e->confused -= dt;
        for (int w = 0; w < W_COUNT; w++)
        {
            if (e->wcd[w] > 0.0f) e->wcd[w] -= dt;
        }

        toward = direction_of(e->pos, p->pos);
        distance = Vector2Distance(e->pos, p->pos);

        if (e->wall) desired = (Vector2){cosf(e->aux_angle), sinf(e->aux_angle)};
        else if (e->confused > 0.0f) desired = Vector2Scale((Vector2){-toward.x + sinf(e->timer * 9.0f) * 0.8f, -toward.y + cosf(e->timer * 7.0f) * 0.8f}, 0.7f);
        else desired = toward;

        if (e->type == EN_NOTIFICACAO && !e->wall && e->confused <= 0.0f)
        {
            Vector2 perpendicular = {-toward.y, toward.x};

            desired = Vector2Add(desired, Vector2Scale(perpendicular, sinf(e->timer * 6.0f) * 0.9f));
        }

        // Bateu em um objeto há pouco: contorna por um lado em vez de empurrar de frente
        if (e->detour > 0.0f)
        {
            Vector2 side = {-toward.y * (float)e->detour_sign, toward.x * (float)e->detour_sign};

            e->detour -= dt;
            desired = Vector2Normalize(Vector2Add(Vector2Scale(desired, 0.4f), Vector2Scale(side, 0.9f)));
        }

        e->pos = Vector2Add(e->pos, Vector2Scale(desired, stats->speed * dt));
        e->pos = Vector2Add(e->pos, Vector2Scale(e->vel, dt));
        e->vel = Vector2Scale(e->vel, expf(-8.0f * dt));

        // O Dado Enviesado e o Crachá Fantasma atravessam objetos; os outros batem neles
        if (!e->wall && e->type != EN_CRACHA)
        {
            float pushed = sv_map_collide(&e->pos, stats->radius * 0.7f, 10.0f);

            if (pushed > 0.4f && e->detour <= 0.0f)
            {
                e->detour = 0.6f;
                e->detour_sign = (e->id & 1u) ? 1 : -1;
            }
        }

        if (fabsf(desired.x) > fabsf(desired.y))
        {
            e->facing = 1;
            e->flip = desired.x < 0.0f;
        }
        else
        {
            e->facing = desired.y > 0.0f ? 0 : 2;
        }

        // Quem ficou longe demais volta para a borda da tela, como em todo jogo do gênero
        if (distance > 900.0f)
        {
            if (e->wall)
            {
                e->alive = false;
                sv_run.enemy_count--;
                continue;
            }
            spawn_ring_position(560.0f, &e->pos);
            distance = Vector2Distance(e->pos, p->pos);
        }

        // Comportamentos próprios
        if (e->type == EN_AFOGADO && distance < 280.0f && e->timer > 3.2f)
        {
            SvHazard *h = NULL;

            e->timer = 0.0f;
            for (int j = 0; j < SV_MAX_HAZARDS; j++)
            {
                if (!sv_run.hazards[j].alive) { h = &sv_run.hazards[j]; break; }
            }
            if (h)
            {
                memset(h, 0, sizeof(*h));
                h->alive = true;
                h->kind = HZ_PROJ;
                h->sprite = -2;
                h->pos = e->pos;
                h->vel = Vector2Scale(toward, 130.0f);
                h->radius = 6.0f;
                h->life = h->max_life = 3.0f;
                h->damage = 8.0f * danger_scale;
                sv_sfx(SND_afogadoCospe);
            }
        }
        else if (e->type == EN_ECO && distance < 300.0f)
        {
            p->pos = Vector2Add(p->pos, Vector2Scale(toward, -32.0f * dt));
            if (e->timer > 4.0f)
            {
                e->timer = 0.0f;
                sv_sfx(SND_ecoSussurro);
            }
        }
        else if (e->type == EN_AUDITOR && distance < 320.0f && e->timer > 1.3f)
        {
            e->timer = 0.0f;
            sv_sfx(SND_auditorPassos);
        }
        else if (e->type == EN_NOTIFICACAO && e->timer > 2.5f && distance < 300.0f)
        {
            e->timer = 0.0f;
            sv_sfx(SND_notificacaoPing);
        }
        else if (e->type == EN_CRACHA && e->timer > 3.0f && distance < 260.0f)
        {
            e->timer = 0.0f;
            sv_sfx(SND_crachaAtravessa);
        }

        // Contato com o jogador
        if (distance < stats->radius + 8.0f)
        {
            if (e->fake)
            {
                sv_sfx(SND_alucinacaoSome);
                kill_enemy(e);
                continue;
            }
            sv_hurt_player(stats->damage * danger_scale, EFF_NONE, 0.0f);
        }
    }

    // Separação: ninguém fica em cima do outro
    for (int i = 0; i < SV_MAX_ENEMIES; i++)
    {
        SvEnemy *a = &sv_run.enemies[i];

        if (!a->alive || a->wall) continue;
        for (int j = i + 1; j < SV_MAX_ENEMIES; j++)
        {
            SvEnemy *b = &sv_run.enemies[j];
            float dx, dy, d2, minimum;

            if (!b->alive || b->wall) continue;
            dx = a->pos.x - b->pos.x;
            if (dx > 18.0f || dx < -18.0f) continue;
            dy = a->pos.y - b->pos.y;
            d2 = dx * dx + dy * dy;
            minimum = (enemy_stats[a->type].radius + enemy_stats[b->type].radius) * 0.7f;
            if (d2 < minimum * minimum && d2 > 0.01f)
            {
                float d = sqrtf(d2);
                float push = (minimum - d) * 0.5f;

                a->pos.x += dx / d * push;
                a->pos.y += dy / d * push;
                b->pos.x -= dx / d * push;
                b->pos.y -= dy / d * push;
            }
        }
    }
}

// -----------------------------------------------------------------------------
// Itens
// -----------------------------------------------------------------------------

static void update_gems(float dt)
{
    SvPlayer *p = &sv_run.player;

    for (int i = 0; i < SV_MAX_GEMS; i++)
    {
        SvGem *g = &sv_run.gems[i];
        float d;

        if (!g->alive) continue;
        d = Vector2Distance(g->pos, p->pos);

        if (g->kind == 3 || g->kind == 2) { if (d < p->magnet_range * 0.5f) g->pulled = true; }
        else if (d < p->magnet_range) g->pulled = true;

        if (g->pulled) g->pos = Vector2Add(g->pos, Vector2Scale(direction_of(g->pos, p->pos), 300.0f * dt));

        if (d < 11.0f)
        {
            g->alive = false;
            switch (g->kind)
            {
            case 0:
                p->xp += g->value * p->xp_mult;
                sv_sfx(SND_coletaXp);
                break;
            case 1:
                sv_run.coins++;
                sv_sfx(SND_coletaMoeda);
                break;
            case 2:
                p->hp += g->value;
                if (p->hp > p->max_hp) p->hp = p->max_hp;
                sv_sfx(SND_marmita);
                break;
            case 3:
                p->pending_levels++;
                sv_sfx(SND_bau);
                break;
            }
        }
    }
}

// -----------------------------------------------------------------------------
// Nível e escolhas
// -----------------------------------------------------------------------------

static void build_choices(void)
{
    SvChoice pool[32];
    int count = 0;
    SvPlayer *p = &sv_run.player;

    for (int w = 0; w < W_COUNT; w++)
    {
        SvWeapon *owned = find_weapon(w);

        if (owned && owned->level < SV_WEAPON_MAX_LEVEL) pool[count++] = (SvChoice){1, w};
        else if (!owned && p->weapon_count < SV_MAX_WEAPONS) pool[count++] = (SvChoice){0, w};
    }

    for (int i = 0; i < P_COUNT; i++)
    {
        int level = passive_level(i);

        if (level > 0 && level < SV_PASSIVE_MAX_LEVEL) pool[count++] = (SvChoice){3, i};
        else if (level == 0 && p->passive_count < SV_MAX_PASSIVES) pool[count++] = (SvChoice){2, i};
    }

    // Embaralha
    for (int i = count - 1; i > 0; i--)
    {
        int j = (int)(sv_rand() * (i + 1));
        SvChoice tmp = pool[i];

        pool[i] = pool[j];
        pool[j] = tmp;
    }

    sv_run.choice_count = 0;
    for (int i = 0; i < count && sv_run.choice_count < 3; i++) sv_run.choices[sv_run.choice_count++] = pool[i];
    if (sv_run.choice_count < 3) sv_run.choices[sv_run.choice_count++] = (SvChoice){4, 0};
    if (sv_run.choice_count < 3) sv_run.choices[sv_run.choice_count++] = (SvChoice){5, 0};
    if (sv_run.choice_count < 3) sv_run.choices[sv_run.choice_count++] = (SvChoice){6, 0};
}

const char *sv_choice_title(const SvChoice *c)
{
    switch (c->kind)
    {
    case 0: case 1: return weapon_names[c->id];
    case 2: case 3: return passive_names[c->id];
    case 4: return "Marmita";
    case 6: return "Chip de XP";
    default: return "Banco de Horas";
    }
}

const char *sv_choice_text(const SvChoice *c)
{
    switch (c->kind)
    {
    case 0: case 1: return weapon_descriptions[c->id];
    case 2: case 3: return passive_descriptions[c->id];
    case 4: return "Recupera 40% da vida.";
    case 6: return "Ganha metade do XP do próximo nível.";
    default: return "Ganha 15 de Banco de Horas.";
    }
}

SvSpriteId sv_choice_icon(const SvChoice *c)
{
    switch (c->kind)
    {
    case 0: case 1: return weapon_icons[c->id];
    case 2: case 3: return passive_icons[c->id];
    case 4: return SPR_it_marmita;
    case 6: return SPR_it_xp;
    default: return SPR_it_moeda;
    }
}

// Nível que a escolha leva a arma ou o passivo (0 se não for um deles)
int sv_choice_level(const SvChoice *c)
{
    switch (c->kind)
    {
    case 0: case 2: return 1;
    case 1:
    {
        SvWeapon *w = find_weapon(c->id);
        return w ? w->level + 1 : 1;
    }
    case 3: return passive_level(c->id) + 1;
    default: return 0;
    }
}

void sv_game_choose(int index)
{
    SvPlayer *p = &sv_run.player;
    SvChoice c;

    if (sv_run.phase != SV_LEVELUP || index < 0 || index >= sv_run.choice_count) return;
    c = sv_run.choices[index];

    switch (c.kind)
    {
    case 0:
        p->weapons[p->weapon_count++] = (SvWeapon){c.id, 1, 0.3f, 0.0f};
        break;
    case 1:
    {
        SvWeapon *w = find_weapon(c.id);

        if (w) w->level++;
        if (w && w->level == SV_WEAPON_MAX_LEVEL) sv_sfx(SND_evolucao);
        break;
    }
    case 2:
        p->passives[p->passive_count++] = (SvPassive){c.id, 1};
        break;
    case 3:
        for (int i = 0; i < p->passive_count; i++)
        {
            if (p->passives[i].id == c.id) p->passives[i].level++;
        }
        break;
    case 4:
        p->hp += p->max_hp * 0.4f;
        if (p->hp > p->max_hp) p->hp = p->max_hp;
        sv_sfx(SND_marmita);
        break;
    case 6:
        p->xp += (float)p->xp_needed * 0.5f;
        sv_sfx(SND_coletaXp);
        break;
    default:
        sv_run.coins += 15;
        sv_sfx(SND_coletaMoeda);
        break;
    }

    recompute_stats();
    sv_sfx(SND_confirma);
    sv_run.phase = SV_PLAYING;
}

static void check_level_up(void)
{
    SvPlayer *p = &sv_run.player;

    while (p->xp >= (float)p->xp_needed)
    {
        p->xp -= (float)p->xp_needed;
        p->level++;
        p->xp_needed = sv_xp_to_next(p->level);
        p->pending_levels++;
    }

    if (p->pending_levels > 0 && sv_run.phase == SV_PLAYING)
    {
        p->pending_levels--;
        build_choices();
        sv_run.phase = SV_LEVELUP;
        sv_sfx(SND_subirNivel);
    }
}

// -----------------------------------------------------------------------------
// Partida
// -----------------------------------------------------------------------------

void sv_game_start(unsigned seed)
{
    SvPlayer *p;

    sv_map_init();
    memset(&sv_run, 0, sizeof(sv_run));
    finished_flag = false;
    sv_run.rng = seed ? seed : 2463534242u;
    sv_run.phase = SV_PLAYING;
    sv_run.next_enemy_id = 0;

    p = &sv_run.player;
    p->pos = (Vector2){3.5f * SV_CELL_W, 2.5f * SV_CELL_H};
    p->facing_vector = (Vector2){0.0f, 1.0f};
    p->direction = SOUTH;
    p->level = 1;
    p->xp_needed = sv_xp_to_next(1);
    p->weapons[0] = (SvWeapon){W_GRAMPEADOR, 1, 0.5f, 0.0f};
    p->weapon_count = 1;
    p->hp = 0.0f;
    p->max_hp = 0.0f;
    recompute_stats();
    p->hp = p->max_hp;
    p->revive_ready = sv_meta.upgrades[UPG_RECOMECO] > 0;

    sv_run.act = 0;
    sv_banner("ATO I", 3.0f);
    sv_music_play(MUS_ATO1);
}

static void update_music_and_ambient(void)
{
    int col = (int)(sv_run.player.pos.x / SV_CELL_W);
    int row = (int)(sv_run.player.pos.y / SV_CELL_H);

    if (col < 0) col = 0;
    if (col >= SV_COLS) col = SV_COLS - 1;
    if (row < 0) row = 0;
    if (row >= SV_ROWS) row = SV_ROWS - 1;

    sv_run.area = area_grid[row][col];
    sv_ambient_play(area_ambient[sv_run.area]);

    if (sv_run.boss.active)
    {
        static const SvMusic boss_tracks[BOSS_COUNT] = {MUS_IMPRESSORA, MUS_MODELO, MUS_DIEGO, MUS_RENE, MUS_IARA};
        SvMusic track = boss_tracks[sv_run.boss.id];

        if (sv_run.boss.id == BOSS_IARA && sv_run.boss.hp < sv_run.boss.max_hp * 0.3f) track = MUS_HORAEXTRA;
        sv_music_play(track);
    }
    else
    {
        sv_music_play((SvMusic)(MUS_ATO1 + sv_run.act));
    }
}

static void update_player(float dt, Vector2 move)
{
    SvPlayer *p = &sv_run.player;
    float length = Vector2Length(move);
    float speed = 78.0f * p->speed_mult;

    if (p->invert > 0.0f) move = Vector2Negate(move);
    if (length > 1.0f) move = Vector2Scale(move, 1.0f / length);
    if (p->slow > 0.0f) speed *= 0.5f;
    if (p->root > 0.0f) speed = 0.0f;

    p->slow = p->slow > dt ? p->slow - dt : 0.0f;
    p->root = p->root > dt ? p->root - dt : 0.0f;
    p->invert = p->invert > dt ? p->invert - dt : 0.0f;
    if (p->invulnerable > 0.0f) p->invulnerable -= dt;
    if (p->hurt_flash > 0.0f) p->hurt_flash -= dt;

    if (length > 0.05f)
    {
        Vector2 unit = Vector2Normalize(move);

        p->pos = Vector2Add(p->pos, Vector2Scale(move, speed * dt));
        p->facing_vector = unit;
        p->direction = direction_index(unit);
        p->walk_time += dt;
        p->step_timer -= dt;
        if (p->step_timer <= 0.0f)
        {
            sv_sfx(SND_passo);
            p->step_timer = 0.34f;
        }
    }
    else
    {
        p->walk_time = 0.0f;
    }

    p->pos = clamp_to_world(p->pos, 18.0f);
    sv_map_collide(&p->pos, 7.0f, 10.0f);

    // Rastro dos últimos passos (o Modelo Enviesado o usa)
    sv_run.trail_timer -= dt;
    if (sv_run.trail_timer <= 0.0f)
    {
        sv_run.trail_timer = 0.25f;
        memmove(&sv_run.trail[1], &sv_run.trail[0], sizeof(Vector2) * (SV_TRAIL - 1));
        sv_run.trail[0] = p->pos;
    }
}

static void update_fx(float dt)
{
    for (int i = 0; i < SV_MAX_FX; i++)
    {
        SvFx *fx = &sv_run.fx[i];

        if (!fx->alive) continue;
        fx->t += dt;
        if (fx->t >= fx->life) fx->alive = false;
    }
}

static void check_death(void)
{
    SvPlayer *p = &sv_run.player;

    if (p->hp > 0.0f) return;

    if (p->revive_ready)
    {
        // Ctrl+Z: volta com metade da vida e empurra todo mundo para longe
        p->revive_ready = false;
        p->hp = p->max_hp * 0.5f;
        p->invulnerable = 2.5f;
        for (int i = 0; i < SV_MAX_ENEMIES; i++)
        {
            SvEnemy *e = &sv_run.enemies[i];

            if (e->alive && Vector2Distance(e->pos, p->pos) < 320.0f && e->type != EN_AUDITOR) kill_enemy(e);
        }
        sv_boss_clear_hazards();
        sv_spawn_fx(FX_EXPLOSION, p->pos, 0.5f, 3.0f);
        sv_sfx(SND_evolucao);
        sv_banner("CTRL+Z", 2.0f);
        return;
    }

    sv_run.phase = SV_DEAD;
    sv_sfx(SND_morte);
    sv_music_play(MUS_GAMEOVER);
}

void sv_game_update(float dt, Vector2 move)
{
    int budget;

    if (sv_run.phase != SV_PLAYING) return;
    if (dt > 0.05f) dt = 0.05f;

    sv_run.time += dt;
    if (sv_run.banner > 0.0f) sv_run.banner -= dt;
    if (sv_run.shake > 0.0f) sv_run.shake -= dt;

    // Novo ato
    {
        int act = sv_act_at(sv_run.time);

        if (act != sv_run.act)
        {
            sv_run.act = act;
            sv_banner(act_names[act], 3.0f);
            sv_sfx(SND_novoAto);
        }
    }

    // Chefes
    if (sv_run.next_boss < BOSS_COUNT && !sv_run.boss.active && sv_run.time >= sv_boss_spawn_time(sv_run.next_boss))
    {
        sv_boss_spawn(sv_run.next_boss++);
    }
    if (sv_run.time >= SV_RUN_SECONDS && !sv_run.overtime)
    {
        sv_run.overtime = true;
        sv_sfx(SND_horaExtra);
    }

    update_player(dt, move);

    // Nascimentos
    sv_run.spawn_accumulator += sv_spawn_rate(sv_run.time) * dt * (sv_run.boss.active ? 0.5f : 1.0f);
    budget = 6;
    while (sv_run.spawn_accumulator >= 1.0f && budget-- > 0 && sv_run.enemy_count < SV_MAX_ENEMIES - 20)
    {
        sv_run.spawn_accumulator -= (float)spawn_event();
    }
    if (sv_run.spawn_accumulator > 8.0f) sv_run.spawn_accumulator = 8.0f;

    update_weapons(dt);
    update_shots(dt);
    update_zones(dt);
    update_enemies(dt);
    sv_boss_update(dt);
    sv_hazards_update(dt);
    update_gems(dt);
    update_fx(dt);
    update_music_and_ambient();

    check_level_up();
    check_death();
}

int sv_game_finish(void)
{
    int earned;

    if (finished_flag) return 0;
    finished_flag = true;

    earned = sv_run.coins + (int)(sv_run.time / 10.0f) + sv_run.bosses_defeated * 30 + (sv_run.phase == SV_WON ? 300 : 0);
    sv_meta.coins += earned;
    sv_meta.runs++;
    sv_meta.kills += sv_run.kills;
    if (sv_run.phase == SV_WON) sv_meta.wins++;
    if ((int)sv_run.time > sv_meta.best_seconds) sv_meta.best_seconds = (int)sv_run.time;
    sv_meta_save(SV_SAVE_PATH);

    return earned;
}
