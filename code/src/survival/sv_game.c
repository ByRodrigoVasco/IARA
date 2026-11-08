#include <math.h>
#include <stdio.h>
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
    float xp;              // XP do inimigo, proporcional à dificuldade dele; o chip solto vale o dobro
    float radius;
} EnemyStats;

static const EnemyStats enemy_stats[EN_COUNT] = {
    [EN_CHAMADO]      = {7.0f, 52.0f, 4.0f, 0.75f, 9.0f},
    [EN_NOTIFICACAO]  = {10.0f, 80.0f, 5.0f, 1.5f, 9.0f},
    [EN_ESVAZIADO]    = {33.0f, 36.0f, 8.0f, 2.75f, 11.0f},
    [EN_DADO]         = {20.0f, 74.0f, 10.0f, 2.75f, 12.0f},
    [EN_ALUCINACAO]   = {16.0f, 62.0f, 7.0f, 1.75f, 10.0f},
    [EN_AUTORESPOSTA] = {24.0f, 55.0f, 8.0f, 2.5f, 11.0f},
    [EN_CRACHA]       = {28.0f, 78.0f, 11.0f, 4.25f, 10.0f},
    [EN_AFOGADO]      = {50.0f, 50.0f, 12.0f, 6.0f, 11.0f},
    [EN_AUDITOR]      = {260.0f, 60.0f, 18.0f, 40.0f, 13.0f},
    [EN_ECO]          = {90.0f, 60.0f, 14.0f, 13.0f, 11.0f},
    [EN_MINI]         = {8.0f, 92.0f, 4.0f, 0.5f, 7.0f},
};

static const SvSpriteId enemy_sprites[EN_COUNT] = {
    SPR_en_chamado, SPR_en_notificacao, SPR_en_esvaziado, SPR_en_dado, SPR_en_alucinacao,
    SPR_en_autoresposta, SPR_en_cracha, SPR_en_afogado, SPR_en_auditor, SPR_en_eco, SPR_en_autorespostaMini};

SvSpriteId sv_enemy_sprite(int type)
{
    return enemy_sprites[type];
}

// Ponteiro: distância em que ele procura o inimigo para mirar
#define PONTEIRO_RANGE 320.0f

// Não Perturbe: quanto a aura empurra quem está dentro dela
#define NAOPERTURBE_PUSH 125.0f

// Prompt da IARA: alcance para escolher o alvo, tempo de digitação do comando e dano de quem está em volta do alvo
#define PROMPT_RANGE 340.0f
#define PROMPT_TYPING SV_PROMPT_TYPING
#define PROMPT_SPLASH 0.5f
#define PROMPT_TEXTS 8

// Deploy em Produção: alcance de mira
#define DEPLOY_RANGE 300.0f

// Vera Fischer: distância em que um inimigo dispara a onda sozinho
#define VERA_TRIGGER 44.0f

// Caneca Arremessada: alcance de mira e tempo entre o arremesso e o impacto
#define CANECA_RANGE 260.0f
#define CANECA_DELAY 0.5f

// Pesos de nascimento por ato.
typedef struct { int type; int weight; } SpawnWeight;

static const SpawnWeight act_spawns[SV_ACT_COUNT][8] = {
    {{EN_CHAMADO, 70}, {EN_NOTIFICACAO, 20}, {EN_ESVAZIADO, 10}, {-1, 0}},
    {{EN_CHAMADO, 25}, {EN_NOTIFICACAO, 20}, {EN_ESVAZIADO, 15}, {EN_DADO, 20}, {EN_ALUCINACAO, 20}, {-1, 0}},
    {{EN_ESVAZIADO, 10}, {EN_NOTIFICACAO, 15}, {EN_ALUCINACAO, 15}, {EN_AUTORESPOSTA, 25}, {EN_CRACHA, 25}, {EN_DADO, 10}, {-1, 0}},
    {{EN_AFOGADO, 25}, {EN_AUDITOR, 4}, {EN_CRACHA, 15}, {EN_AUTORESPOSTA, 20}, {EN_ESVAZIADO, 15}, {EN_NOTIFICACAO, 10}, {EN_DADO, 10}, {-1, 0}},
    {{EN_ECO, 20}, {EN_AUDITOR, 7}, {EN_AFOGADO, 20}, {EN_CRACHA, 15}, {EN_AUTORESPOSTA, 15}, {EN_ALUCINACAO, 12}, {EN_DADO, 10}, {-1, 0}},
};

static const char *weapon_names[W_COUNT] = {"Grampeador", "Ponteiro", "Caneca Arremessada", "Post-its Orbitais", "Não Perturbe", "Deploy em Produção", "Tarja Preta", "Modus Ponens", "Tabela-Verdade", "Colega de Plantão", "Nova Conversa", "Prompt da IARA", "Vera Fischer", "Fibra Óptica"};
static const char *weapon_descriptions[W_COUNT] = {
    "Tiros retos no inimigo mais próximo.",
    "Golpe em linha, para os dois lados, na direção do inimigo mais próximo.",
    "Cai sobre os inimigos mais próximos e derrama café quente em área.",
    "Giram ao seu redor e protegem de perto.",
    "Aura de dano que afasta quem chega perto.",
    "Raios que caem sobre os inimigos mais próximos.",
    "Faixa que confunde quem atinge.",
    "Se acerta A, salta para B, e assim por diante.",
    "Grade no chão que fere quem pisa nas casas acesas.",
    "Colegas de plantão lutam ao seu lado, cada um com uma função: Segurança, Suporte de TI, Enfermeira do RH e Entregador.",
    "Empurra tudo e apaga tiros, com recarga longa.",
    "Digita um comando na janela de terminal da IARA sobre o inimigo mais forte e o executa: dano alto nele e metade em quem está em volta.",
    "Se um golpe te acerta (V), o dano é falso (F): ele é anulado e solta uma onda. Também dispara sozinha quando alguém chega perto.",
    "Feixe de luz muito forte em linha reta, que atravessa todos os inimigos no caminho."};
static const SvSpriteId weapon_icons[W_COUNT] = {SPR_ic_grampeador, SPR_ic_ponteiro, SPR_ic_caneca, SPR_ic_postits, SPR_ic_naoperturbe, SPR_ic_deploy, SPR_ic_tarja, SPR_ic_modus, SPR_ic_tabela, SPR_ic_colega, SPR_ic_novaconversa, SPR_ic_prompt, SPR_ps_cracha, SPR_ic_fibra};

static const char *passive_names[P_COUNT] = {"Café", "Manual", "Colinha", "Ergonomia", "Crachá", "Plano de Saúde", "Network", "Hora Extra", "Marmita Térmica", "Sorte de Principiante", "Espinhos Corporativos", "Foco"};
static const char *passive_descriptions[P_COUNT] = {
    "Recarga mais rápida.", "Mais área nos poderes.", "Mais XP dos chips.", "Mais velocidade.",
    "Menos dano recebido.", "Mais vida máxima.", "Pega os itens de mais longe.", "Poderes duram mais.",
    "Recupera vida sozinho, aos poucos.", "Mais chance de Banco de Horas e marmita.", "Quem encosta em você leva dano.", "Mais dano em tudo."};
static const SvSpriteId passive_icons[P_COUNT] = {SPR_ps_cafe, SPR_ps_manual, SPR_ps_colinha, SPR_ps_ergonomia, SPR_ps_cracha, SPR_ps_saude, SPR_ps_network, SPR_ps_horaextra, SPR_it_marmita, SPR_it_moeda, SPR_ic_tarja, SPR_ic_prompt};

static const char *boss_names[BOSS_COUNT] = {"Impressora de Rescisões", "Modelo Enviesado", "Diego, o Crachá Ativo", "Renê, na cabeceira", "IARA, a Mãe-d'Água"};
static const char *act_names[SV_ACT_COUNT] = {"ATO I", "ATO II", "ATO III", "ATO IV", "ATO V"};


// -----------------------------------------------------------------------------
// Poderes: cada nível muda atributos próprios (os valores abaixo são os do nível 1 a 5)
// -----------------------------------------------------------------------------

typedef struct
{
    float dmg[5], cd[5], n[5], area[5], extra[5];
} WeaponDef;

typedef struct
{
    const char *cd_name, *n_name, *area_name, *extra_name;
    bool extra_is_time, area_scaled, extra_scaled;
} WeaponInfo;

static const WeaponDef weapon_defs[W_COUNT] = {
    [W_GRAMPEADOR]   = {{10, 10, 14, 14, 20}, {0.70f, 0.60f, 0.60f, 0.50f, 0.45f}, {1, 1, 2, 2, 3}, {0, 0, 0, 0, 0}, {1, 2, 2, 3, 3}},
    [W_PONTEIRO]     = {{14, 14, 20, 20, 28}, {1.60f, 1.45f, 1.45f, 1.25f, 1.10f}, {0, 0, 0, 0, 0}, {85, 95, 95, 110, 125}, {14, 14, 17, 17, 20}},
    [W_CANECA]       = {{20, 20, 26, 26, 36}, {2.20f, 2.00f, 2.00f, 1.80f, 1.60f}, {1, 1, 2, 2, 3}, {40, 46, 46, 52, 58}, {0, 0, 0, 0, 0}},
    [W_POSTITS]      = {{7, 7, 10, 10, 15}, {0.50f, 0.45f, 0.45f, 0.40f, 0.35f}, {0, 0, 0, 0, 0}, {54, 60, 60, 68, 76}, {0, 0, 0, 0, 0}},
    [W_NAOPERTURBE]  = {{4, 4, 6, 6, 10}, {0.55f, 0.50f, 0.50f, 0.45f, 0.40f}, {0, 0, 0, 0, 0}, {64, 72, 72, 84, 96}, {0, 0, 0, 0, 0}},
    [W_DEPLOY]       = {{28, 28, 36, 36, 48}, {2.60f, 2.40f, 2.40f, 2.10f, 1.80f}, {1, 1, 2, 2, 3}, {38, 42, 42, 46, 50}, {0, 0, 0, 0, 0}},
    [W_TARJA]        = {{7, 7, 10, 10, 14}, {3.20f, 3.00f, 3.00f, 2.70f, 2.40f}, {0, 0, 0, 0, 0}, {120, 135, 135, 150, 170}, {2.0f, 2.5f, 2.5f, 3.0f, 3.5f}},
    [W_MODUS]        = {{14, 14, 18, 18, 24}, {1.80f, 1.65f, 1.65f, 1.45f, 1.30f}, {3, 3, 4, 4, 5}, {170, 185, 185, 200, 220}, {0, 0, 0, 0, 0}},
    [W_TABELA]       = {{6, 6, 9, 9, 13}, {0.60f, 0.55f, 0.55f, 0.50f, 0.45f}, {0, 0, 0, 0, 0}, {50, 56, 56, 64, 72}, {0, 0, 0, 0, 0}},
    [W_COLEGA]       = {{13, 13, 18, 18, 26}, {0.75f, 0.65f, 0.65f, 0.55f, 0.50f}, {1, 1, 2, 2, 3}, {28, 30, 30, 34, 38}, {2, 2, 2, 3, 4}},
    [W_NOVACONVERSA] = {{8, 8, 12, 12, 18}, {15.0f, 13.5f, 13.5f, 12.0f, 10.0f}, {0, 0, 0, 0, 0}, {240, 270, 270, 300, 330}, {0, 0, 0, 0, 0}},
    [W_PROMPT]       = {{60, 60, 80, 80, 115}, {3.0f, 2.7f, 2.7f, 2.4f, 2.0f}, {1, 1, 1, 2, 2}, {72, 78, 78, 88, 98}, {0, 0, 0, 0, 0}},
    [W_FIBRA]        = {{34, 34, 48, 48, 70}, {2.9f, 2.6f, 2.6f, 2.3f, 2.0f}, {0, 0, 0, 0, 0}, {300, 320, 320, 350, 380}, {14, 16, 16, 18, 22}},
    [W_VERA]         = {{22, 22, 30, 30, 50}, {2.6f, 2.4f, 2.4f, 2.1f, 1.9f}, {1, 1, 2, 2, 2}, {84, 90, 90, 98, 108}, {0.6f, 0.7f, 0.9f, 1.1f, 1.3f}},
};

static const WeaponInfo weapon_infos[W_COUNT] = {
    [W_GRAMPEADOR]   = {"Recarga", "Projéteis", NULL, "Perfuração", false, false, false},
    [W_PONTEIRO]     = {"Recarga", NULL, "Alcance", "Largura", false, true, true},
    [W_CANECA]       = {"Recarga", "Canecas", "Raio", NULL, false, true, false},
    [W_POSTITS]      = {"Intervalo", NULL, "Raio", NULL, false, true, false},
    [W_NAOPERTURBE]  = {"Intervalo", NULL, "Raio", NULL, false, true, false},
    [W_DEPLOY]       = {"Recarga", "Raios", "Raio", NULL, false, true, false},
    [W_TARJA]        = {"Recarga", NULL, "Comprimento", "Confusão", true, true, false},
    [W_MODUS]        = {"Recarga", "Saltos", "Alcance do salto", NULL, false, false, false},
    [W_TABELA]       = {"Intervalo", NULL, "Lado", NULL, false, true, false},
    [W_COLEGA]       = {"Intervalo", "Colegas", "Alcance", "Cura da enfermeira", false, false, false},
    [W_NOVACONVERSA] = {"Recarga", NULL, "Raio", NULL, false, true, false},
    [W_PROMPT]       = {"Recarga", "Comandos", "Raio da onda", NULL, false, true, false},
    [W_FIBRA]        = {"Recarga", NULL, "Comprimento", "Largura", false, true, true},
    [W_VERA]         = {"Recarga", "Ondas", "Raio", "Imunidade", true, true, false},
};

float sv_weapon_stat(int weapon, int stat, int level)
{
    const WeaponDef *def = &weapon_defs[weapon];

    if (level < 1) level = 1;
    if (level > SV_WEAPON_MAX_LEVEL) level = SV_WEAPON_MAX_LEVEL;
    switch (stat)
    {
    case WSTAT_DAMAGE: return def->dmg[level - 1];
    case WSTAT_COOLDOWN: return def->cd[level - 1];
    case WSTAT_COUNT: return def->n[level - 1];
    case WSTAT_AREA: return def->area[level - 1];
    default: return def->extra[level - 1];
    }
}

#define WS(id, stat, level) sv_weapon_stat((id), (stat), (level))

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
    p->dmg_mult += 0.06f * passive_level(P_FOCO);
    p->regen = 0.4f * passive_level(P_REGEN);

    // Habilidade do personagem escolhido
    {
        const SvCharacter *c = sv_character(p->character);

        p->max_hp += c->hp_bonus;
        p->armor += c->armor_bonus;
        p->speed_mult *= c->speed_mult;
        p->cooldown_mult *= c->cooldown_mult;
        if (p->cooldown_mult < 0.4f) p->cooldown_mult = 0.4f;
        p->duration_mult *= c->duration_mult;
        p->magnet_range *= c->magnet_mult;
        p->xp_mult *= c->xp_mult;
        p->dmg_mult *= c->damage_mult;
        p->crit_chance = c->crit_chance;
        p->coin_mult = c->coin_mult;
        p->coin_luck = c->coin_luck;
        p->move_recharge = c->move_recharge;
        p->kill_heal = c->kill_heal;
        p->kill_heal_every = c->kill_heal_every;
        p->extra_colleagues = c->extra_colleagues;
    }

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

    spawn_ring_position(800.0f + sv_rand() * 60.0f, &origin);

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
    int coin_chance = 4 + sv_run.act + 2 * passive_level(P_SORTE) + sv_run.player.coin_luck;

    e->alive = false;
    sv_run.enemy_count--;
    sv_run.kills++;

    // Nicollas: a cada tantos chamados atendidos, recupera um pouco de vida
    if (!e->fake && sv_run.player.kill_heal_every > 0 && ++sv_run.player.kills_since_heal >= sv_run.player.kill_heal_every)
    {
        sv_run.player.kills_since_heal = 0;
        sv_run.player.hp = fminf(sv_run.player.max_hp, sv_run.player.hp + sv_run.player.kill_heal);
    }

    sv_spawn_fx(FX_DEATH, e->pos, 0.35f, 1.0f);

    if (e->fake) return;

    if (e->type == EN_CHAMADO) sv_sfx(SND_chamadoMorre);
    sv_add_gem(e->pos, 0, (float)(enemy_stats[e->type].xp * 2)); // cada chip vale o dobro do XP do inimigo

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
    else if ((int)(sv_rand() * 1000.0f) < 9 + 3 * passive_level(P_SORTE))
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

    damage *= sv_run.player.dmg_mult;
    if (sv_run.player.crit_chance > 0.0f && sv_rand() < sv_run.player.crit_chance) damage *= 2.0f;
    if (sv_run.cheats[CHEAT_ONE_HIT]) damage *= 1000.0f;
    e->hp -= damage;
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

static void vera_release(SvWeapon *w);

void sv_hurt_player(float amount, int effect, float effect_time)
{
    SvPlayer *p = &sv_run.player;
    float damage;

    if (sv_run.cheats[CHEAT_GOD]) return;
    if (p->invulnerable > 0.0f || sv_run.phase != SV_PLAYING) return;

    // Vera Fischer: o golpe é verdadeiro (V) e o dano é falso (F), então o resultado é falso: V -> F = F
    {
        SvWeapon *vera = find_weapon(W_VERA);

        if (vera && vera->cooldown <= 0.0f)
        {
            vera_release(vera);
            return;
        }
    }

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

// O alvo com mais vida dentro do alcance: o chefe, se estiver perto, ou o inimigo mais resistente
static Target strongest_target(Vector2 from, float max_distance, const unsigned *skip, int skip_count)
{
    Target best = {0};
    float best_hp = -1.0f;

    for (int i = 0; i < SV_MAX_ENEMIES; i++)
    {
        SvEnemy *e = &sv_run.enemies[i];

        if (!e->alive || e->fake || id_listed(e->id, skip, skip_count)) continue;
        if (Vector2Distance(from, e->pos) > max_distance) continue;
        if (e->hp > best_hp)
        {
            best_hp = e->hp;
            best.found = true;
            best.is_boss = false;
            best.enemy = e;
            best.pos = e->pos;
        }
    }

    if (sv_run.boss.active && !id_listed(0xB055u, skip, skip_count) && Vector2Distance(from, sv_run.boss.pos) - sv_run.boss.radius <= max_distance)
    {
        best.found = true;
        best.is_boss = true;
        best.enemy = NULL;
        best.pos = sv_run.boss.pos;
    }

    return best;
}

static void damage_target(const Target *target, float damage, Vector2 from, float knockback)
{
    if (!target->found) return;
    if (target->is_boss) sv_damage_boss(damage * sv_run.player.dmg_mult);
    else damage_enemy(target->enemy, damage, from, knockback);
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

// Vera Fischer: solta as ondas, recomeça a recarga e dá um tempo de imunidade. Vale para o golpe anulado e para o disparo por proximidade
static void vera_release(SvWeapon *w)
{
    SvPlayer *p = &sv_run.player;
    int level = w->level;
    int pulses = (int)WS(W_VERA, WSTAT_COUNT, level);

    for (int i = 0; i < pulses; i++)
    {
        SvZone *z = new_zone();

        if (!z) break;
        z->shape = ZONE_CIRCLE;
        z->kind = ZK_VERA;
        z->pos = p->pos;
        z->radius = WS(W_VERA, WSTAT_AREA, level) * p->area_mult * (1.0f + 0.3f * (float)i);
        z->damage = WS(W_VERA, WSTAT_DAMAGE, level);
        z->delay = 0.3f * (float)i;
        z->life = z->max_life = z->delay + 0.3f;
    }

    w->cooldown = WS(W_VERA, WSTAT_COOLDOWN, level) * p->cooldown_mult;
    if (p->invulnerable < WS(W_VERA, WSTAT_EXTRA, level)) p->invulnerable = WS(W_VERA, WSTAT_EXTRA, level);
    p->negate_flash = 0.9f;
    sv_spawn_fx(FX_FLASH, p->pos, 0.3f, 1.2f);
    sv_sfx(SND_novaconversa);
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
        int count = (int)WS(W_GRAMPEADOR, WSTAT_COUNT, level);
        Vector2 aim = aim_vector(380.0f);
        float base_angle = atan2f(aim.y, aim.x);

        base_cd = WS(W_GRAMPEADOR, WSTAT_COOLDOWN, level);
        for (int i = 0; i < count; i++)
        {
            SvShot *s = new_shot();
            float a = base_angle + ((float)i - (count - 1) / 2.0f) * 0.16f;

            if (!s) break;
            s->pos = p->pos;
            s->vel = (Vector2){cosf(a) * 380.0f, sinf(a) * 380.0f};
            s->damage = WS(W_GRAMPEADOR, WSTAT_DAMAGE, level);
            s->radius = 7.0f;
            s->life = 1.3f * p->duration_mult;
            s->pierce = (int)WS(W_GRAMPEADOR, WSTAT_EXTRA, level);
        }
        sv_sfx(SND_grampeador);
        break;
    }
    case W_PONTEIRO:
    {
        float reach = WS(W_PONTEIRO, WSTAT_AREA, level) * p->area_mult;
        float width = WS(W_PONTEIRO, WSTAT_EXTRA, level) * p->area_mult;

        Vector2 aim = aim_vector(PONTEIRO_RANGE);

        base_cd = WS(W_PONTEIRO, WSTAT_COOLDOWN, level);
        angle = atan2f(aim.y, aim.x) * RAD2DEG;
        add_strike_zone(p->pos, angle, reach, width, WS(W_PONTEIRO, WSTAT_DAMAGE, level), 0.0f, ZK_PONTEIRO);
        add_strike_zone(p->pos, angle + 180.0f, reach, width, WS(W_PONTEIRO, WSTAT_DAMAGE, level), 0.0f, ZK_PONTEIRO);
        sv_sfx(SND_ponteiro);
        break;
    }
    case W_CANECA:
    {
        int count = (int)WS(W_CANECA, WSTAT_COUNT, level);
        unsigned thrown[3];
        int thrown_count = 0;

        base_cd = WS(W_CANECA, WSTAT_COOLDOWN, level);
        for (int i = 0; i < count && i < 3; i++)
        {
            // Cai sobre os inimigos mais próximos, um por caneca, já contando o passo que eles dão até o impacto
            Target t = nearest_target(p->pos, CANECA_RANGE, thrown, thrown_count);
            Vector2 target;
            SvZone *z;

            if (!t.found) break;
            target = t.pos;
            if (t.is_boss) thrown[thrown_count++] = 0xB055u;
            else
            {
                thrown[thrown_count++] = t.enemy->id;
                target = Vector2Add(t.pos, Vector2Scale(direction_of(t.pos, p->pos), enemy_stats[t.enemy->type].speed * CANECA_DELAY));
            }

            z = new_zone();
            if (!z) break;
            z->shape = ZONE_CIRCLE;
            z->kind = ZK_CANECA;
            z->pos = target;
            z->radius = WS(W_CANECA, WSTAT_AREA, level) * p->area_mult;
            z->damage = WS(W_CANECA, WSTAT_DAMAGE, level);
            z->delay = CANECA_DELAY;
            z->life = z->max_life = CANECA_DELAY + 0.35f;
        }
        if (thrown_count == 0) base_cd = 0.25f; // Ninguém por perto: tenta de novo logo, sem gastar a recarga
        else sv_sfx(SND_caneca);
        break;
    }
    case W_DEPLOY:
    {
        int count = (int)WS(W_DEPLOY, WSTAT_COUNT, level);
        unsigned struck[3];
        int struck_count = 0;

        base_cd = WS(W_DEPLOY, WSTAT_COOLDOWN, level);
        for (int i = 0; i < count && i < 3; i++)
        {
            Target t = nearest_target(p->pos, DEPLOY_RANGE, struck, struck_count);
            Vector2 target;
            SvZone *z;

            if (!t.found) break;
            target = t.pos;
            if (t.is_boss) struck[struck_count++] = 0xB055u;
            else
            {
                struck[struck_count++] = t.enemy->id;
                target = Vector2Add(t.pos, Vector2Scale(direction_of(t.pos, p->pos), enemy_stats[t.enemy->type].speed * 0.4f));
            }

            z = new_zone();
            if (!z) break;
            z->shape = ZONE_CIRCLE;
            z->kind = ZK_DEPLOY;
            z->pos = target;
            z->radius = WS(W_DEPLOY, WSTAT_AREA, level) * p->area_mult;
            z->damage = WS(W_DEPLOY, WSTAT_DAMAGE, level);
            z->delay = 0.4f;
            z->life = z->max_life = 0.4f + 0.45f;
        }
        if (struck_count == 0) base_cd = 0.25f; // Ninguém por perto: tenta de novo logo, sem gastar a recarga
        break;
    }
    case W_TARJA:
    {
        float length = WS(W_TARJA, WSTAT_AREA, level) * p->area_mult;

        base_cd = WS(W_TARJA, WSTAT_COOLDOWN, level);
        angle = atan2f(p->facing_vector.y, p->facing_vector.x) * RAD2DEG;
        add_strike_zone(p->pos, angle, length, 17.0f * p->area_mult, WS(W_TARJA, WSTAT_DAMAGE, level), WS(W_TARJA, WSTAT_EXTRA, level), ZK_TARJA);
        sv_sfx(SND_tarja);
        break;
    }
    case W_MODUS:
    {
        int hops = (int)WS(W_MODUS, WSTAT_COUNT, level);
        unsigned hit[8];
        int hit_count = 0;
        Vector2 from = p->pos;
        float range = 300.0f;
        float damage = WS(W_MODUS, WSTAT_DAMAGE, level);

        base_cd = WS(W_MODUS, WSTAT_COOLDOWN, level);
        for (int i = 0; i < hops && hit_count < 8; i++)
        {
            Target t = nearest_target(from, range, hit, hit_count);

            if (!t.found) break;
            hit[hit_count++] = t.is_boss ? 0xB055u : t.enemy->id;
            spawn_link(from, t.pos);
            damage_target(&t, damage, from, 20.0f);
            from = t.pos;
            range = WS(W_MODUS, WSTAT_AREA, level);
        }
        if (hit_count > 0) sv_sfx(SND_modus);
        else base_cd = 0.3f;
        break;
    }
    case W_VERA:
    {
        // Fica armada até alguém chegar perto. Se um golpe vier antes, sv_hurt_player a solta no lugar do dano
        Target near_enemy = nearest_target(p->pos, VERA_TRIGGER, NULL, 0);

        if (near_enemy.found) vera_release(w);
        return;
    }
    case W_PROMPT:
    {
        // A IARA digita um comando sobre o inimigo mais forte (e depois o próximo mais forte) e o executa
        int count = (int)WS(W_PROMPT, WSTAT_COUNT, level);
        unsigned chosen[2];
        int chosen_count = 0;

        base_cd = WS(W_PROMPT, WSTAT_COOLDOWN, level);
        for (int i = 0; i < count && i < 2; i++)
        {
            Target t = strongest_target(p->pos, PROMPT_RANGE, chosen, chosen_count);
            SvZone *z;

            if (!t.found) break;
            chosen[chosen_count++] = t.is_boss ? 0xB055u : t.enemy->id;
            z = new_zone();
            if (!z) break;
            z->shape = ZONE_CIRCLE;
            z->kind = ZK_PROMPT;
            z->pos = t.pos;
            z->radius = WS(W_PROMPT, WSTAT_AREA, level) * p->area_mult;
            z->damage = WS(W_PROMPT, WSTAT_DAMAGE, level);
            z->delay = PROMPT_TYPING;
            z->life = z->max_life = PROMPT_TYPING + 0.4f;
            z->target_id = chosen[chosen_count - 1];
            z->target_boss = t.is_boss;
            z->text = (int)(sv_rand() * (float)PROMPT_TEXTS) % PROMPT_TEXTS;
        }
        if (chosen_count == 0) base_cd = 0.25f; // Ninguém por perto: tenta de novo logo, sem gastar a recarga
        else sv_sfx(SND_escolha);
        break;
    }
    case W_FIBRA:
    {
        Vector2 aim = aim_vector(420.0f);

        base_cd = WS(W_FIBRA, WSTAT_COOLDOWN, level);
        angle = atan2f(aim.y, aim.x) * RAD2DEG;
        add_strike_zone(p->pos, angle, WS(W_FIBRA, WSTAT_AREA, level) * p->area_mult, WS(W_FIBRA, WSTAT_EXTRA, level) * p->area_mult, WS(W_FIBRA, WSTAT_DAMAGE, level), 0.0f, ZK_FIBRA);
        sv_sfx(SND_prompt);
        break;
    }
    case W_NOVACONVERSA:
    {
        float radius = WS(W_NOVACONVERSA, WSTAT_AREA, level) * p->area_mult;

        base_cd = WS(W_NOVACONVERSA, WSTAT_COOLDOWN, level);
        for (int i = 0; i < SV_MAX_ENEMIES; i++)
        {
            SvEnemy *e = &sv_run.enemies[i];

            if (e->alive && Vector2Distance(e->pos, p->pos) < radius) damage_enemy(e, WS(W_NOVACONVERSA, WSTAT_DAMAGE, level), p->pos, 520.0f);
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

// Quantos colegas de plantão estão em campo: os do nível do poder mais os que o personagem traz
int sv_colega_count(void)
{
    for (int i = 0; i < sv_run.player.weapon_count; i++)
    {
        const SvWeapon *w = &sv_run.player.weapons[i];

        if (w->id == W_COLEGA)
        {
            int count = (int)WS(W_COLEGA, WSTAT_COUNT, w->level) + sv_run.player.extra_colleagues;

            return count > SV_MAX_COLEGAS ? SV_MAX_COLEGAS : count;
        }
    }
    return 0;
}

static float recharge_pace(void);

// Funções dos colegas, na ordem em que entram: Segurança, Suporte de TI, Enfermeira do RH e Entregador
enum { COLEGA_SEGURANCA, COLEGA_SUPORTE, COLEGA_ENFERMEIRA, COLEGA_ENTREGADOR };
#define COLEGA_CURA_A_CADA 6.0f

static void update_colegas(SvWeapon *w, float dt)
{
    SvPlayer *p = &sv_run.player;
    int level = w->level;
    int count = sv_colega_count();
    float reach = WS(W_COLEGA, WSTAT_AREA, level);
    float damage = WS(W_COLEGA, WSTAT_DAMAGE, level);
    float interval = WS(W_COLEGA, WSTAT_COOLDOWN, level) * p->cooldown_mult;
    float pace = recharge_pace();

    w->angle += dt;
    if (sv_run.colega_heal_flash > 0.0f) sv_run.colega_heal_flash -= dt;

    for (int c = 0; c < count; c++)
    {
        Vector2 *pos = &sv_run.colega_pos[c];
        Vector2 before = *pos;
        float a = (float)c * 1.9f + w->angle * 0.4f;
        Vector2 target = Vector2Add(p->pos, (Vector2){cosf(a) * 34.0f, sinf(a) * 24.0f});
        float speed = 150.0f + Vector2Distance(*pos, p->pos);
        Target near_enemy = {0};
        Vector2 moved;

        if (sv_run.colega_cd[c] > 0.0f) sv_run.colega_cd[c] -= dt * pace;
        if (sv_run.colega_attack[c] > 0.0f) sv_run.colega_attack[c] -= dt;
        if (Vector2Distance(*pos, p->pos) > 420.0f) *pos = p->pos;

        switch (c)
        {
        case COLEGA_SEGURANCA:
            // Vai ao inimigo mais próximo, mas não se afasta muito de você
            near_enemy = nearest_target(*pos, 170.0f, NULL, 0);
            if (near_enemy.found && Vector2Distance(near_enemy.pos, p->pos) < 260.0f)
            {
                target = near_enemy.pos;
                speed = 130.0f;
            }
            break;
        case COLEGA_SUPORTE:
        {
            // Fica atrás de você, do lado oposto ao perigo, e atira de longe
            Target threat = nearest_target(p->pos, 300.0f, NULL, 0);

            if (threat.found) target = Vector2Add(p->pos, Vector2Scale(direction_of(threat.pos, p->pos), 46.0f));
            near_enemy = nearest_target(*pos, 260.0f, NULL, 0);
            break;
        }
        case COLEGA_ENFERMEIRA:
            // Fica colada em você e protege quem chega perto
            target = Vector2Add(p->pos, (Vector2){cosf(a) * 22.0f, sinf(a) * 16.0f});
            near_enemy = nearest_target(*pos, reach * 1.6f, NULL, 0);
            break;
        default:
            // O Entregador corre atrás de quem estiver mais perto, mesmo longe de você
            near_enemy = nearest_target(*pos, 300.0f, NULL, 0);
            if (near_enemy.found && Vector2Distance(near_enemy.pos, p->pos) < 360.0f)
            {
                target = near_enemy.pos;
                speed = 210.0f;
            }
            break;
        }

        if (Vector2Distance(*pos, target) > 10.0f)
        {
            float step = fminf(speed * dt, Vector2Distance(*pos, target));

            *pos = Vector2Add(*pos, Vector2Scale(direction_of(*pos, target), step));
        }
        sv_map_collide(pos, 6.0f, 8.0f);

        moved = Vector2Subtract(*pos, before);
        sv_run.colega_moving[c] = Vector2Length(moved) > 0.15f;
        if (sv_run.colega_moving[c])
        {
            sv_run.colega_dir[c] = Vector2Normalize(moved);
            sv_run.colega_walk[c] += dt;
        }

        if (c == COLEGA_ENFERMEIRA)
        {
            // A cada 6 s, se você estiver ferido, ela cura um pouco
            if (sv_run.colega_heal > 0.0f) sv_run.colega_heal -= dt;
            else if (p->hp < p->max_hp)
            {
                p->hp = fminf(p->max_hp, p->hp + WS(W_COLEGA, WSTAT_EXTRA, level));
                sv_run.colega_heal = COLEGA_CURA_A_CADA;
                sv_run.colega_heal_flash = 0.7f;
                sv_sfx(SND_marmita);
            }
        }

        if (sv_run.colega_cd[c] > 0.0f) continue;

        if (c == COLEGA_SUPORTE)
        {
            // Pacote de dados: um tiro por vez, mais lento que o golpe do Segurança
            if (near_enemy.found)
            {
                SvShot *s = new_shot();

                if (s)
                {
                    Vector2 dir = direction_of(*pos, near_enemy.pos);

                    s->pos = *pos;
                    s->vel = Vector2Scale(dir, 330.0f);
                    s->damage = damage;
                    s->radius = 6.0f;
                    s->life = 0.9f;
                    s->pierce = 1;
                    s->kind = 1;
                    sv_run.colega_dir[c] = dir;
                    sv_run.colega_cd[c] = interval * 1.6f;
                    sv_run.colega_attack[c] = 0.25f;
                    sv_sfx(SND_colega);
                }
            }
        }
        else
        {
            float strike_reach = c == COLEGA_ENFERMEIRA ? reach * 1.6f : c == COLEGA_ENTREGADOR ? reach * 1.2f : reach;
            float strike_damage = c == COLEGA_ENFERMEIRA ? damage * 0.6f : c == COLEGA_ENTREGADOR ? damage * 0.85f : damage;
            float push = c == COLEGA_ENFERMEIRA ? 130.0f : c == COLEGA_ENTREGADOR ? 140.0f : 90.0f;
            float wait = c == COLEGA_ENFERMEIRA ? interval * 1.6f : c == COLEGA_ENTREGADOR ? interval * 0.9f : interval;
            bool attacked = false;

            for (int i = 0; i < SV_MAX_ENEMIES; i++)
            {
                SvEnemy *e = &sv_run.enemies[i];

                if (e->alive && Vector2Distance(e->pos, *pos) < strike_reach)
                {
                    damage_enemy(e, strike_damage, *pos, push);
                    attacked = true;
                }
            }
            if (sv_run.boss.active && Vector2Distance(sv_run.boss.pos, *pos) < strike_reach + sv_run.boss.radius)
            {
                sv_damage_boss(strike_damage * p->dmg_mult);
                attacked = true;
            }
            if (attacked)
            {
                if (near_enemy.found) sv_run.colega_dir[c] = direction_of(*pos, near_enemy.pos);
                sv_run.colega_cd[c] = wait;
                sv_run.colega_attack[c] = 0.25f;
                sv_sfx(SND_colega);
            }
        }
    }
}

static void update_continuous_weapon(SvWeapon *w, float dt)
{
    SvPlayer *p = &sv_run.player;
    int level = w->level;

    if (w->id == W_POSTITS)
    {
        float radius = WS(W_POSTITS, WSTAT_AREA, level) * p->area_mult;
        float damage = WS(W_POSTITS, WSTAT_DAMAGE, level);
        float tick = WS(W_POSTITS, WSTAT_COOLDOWN, level) * p->cooldown_mult;

        w->angle += dt * 2.6f;
        for (int i = 0; i < SV_MAX_ENEMIES; i++)
        {
            SvEnemy *e = &sv_run.enemies[i];
            float d;

            if (!e->alive || e->wcd[W_POSTITS] > 0.0f) continue;
            d = Vector2Distance(e->pos, p->pos);
            if (fabsf(d - radius) < 13.0f + enemy_stats[e->type].radius)
            {
                e->wcd[W_POSTITS] = tick;
                sv_sfx(SND_postits);
                damage_enemy(e, damage, p->pos, 40.0f);
            }
        }
        if (sv_run.boss.active && fabsf(Vector2Distance(sv_run.boss.pos, p->pos) - radius) < 13.0f + sv_run.boss.radius)
        {
            if (w->cooldown <= 0.0f)
            {
                w->cooldown = tick;
                sv_damage_boss(damage * p->dmg_mult);
            }
        }
        if (w->cooldown > 0.0f) w->cooldown -= dt;
    }
    else if (w->id == W_NAOPERTURBE)
    {
        float radius = WS(W_NAOPERTURBE, WSTAT_AREA, level) * p->area_mult;

        w->angle += dt;
        if (w->cooldown > 0.0f)
        {
            w->cooldown -= dt;
            return;
        }
        w->cooldown = WS(W_NAOPERTURBE, WSTAT_COOLDOWN, level) * p->cooldown_mult;
        for (int i = 0; i < SV_MAX_ENEMIES; i++)
        {
            SvEnemy *e = &sv_run.enemies[i];

            if (e->alive && Vector2Distance(e->pos, p->pos) < radius + enemy_stats[e->type].radius)
            {
                damage_enemy(e, WS(W_NAOPERTURBE, WSTAT_DAMAGE, level), p->pos, NAOPERTURBE_PUSH);
            }
        }
        if (sv_run.boss.active && Vector2Distance(sv_run.boss.pos, p->pos) < radius + sv_run.boss.radius)
        {
            sv_damage_boss(WS(W_NAOPERTURBE, WSTAT_DAMAGE, level) * p->dmg_mult);
        }
        sv_sfx(SND_naoperturbe);
    }
    else if (w->id == W_TABELA)
    {
        float radius = WS(W_TABELA, WSTAT_AREA, level) * p->area_mult;

        w->angle += dt;
        if (w->cooldown > 0.0f)
        {
            w->cooldown -= dt;
            return;
        }
        w->cooldown = WS(W_TABELA, WSTAT_COOLDOWN, level) * p->cooldown_mult;
        w->level = level;
        {
            // As casas acendem em xadrez e trocam de cor a cada pulso
            int phase = (int)(w->angle / WS(W_TABELA, WSTAT_COOLDOWN, level)) % 2;

            for (int i = 0; i < SV_MAX_ENEMIES; i++)
            {
                SvEnemy *e = &sv_run.enemies[i];
                int cell;

                if (!e->alive || fabsf(e->pos.x - p->pos.x) > radius || fabsf(e->pos.y - p->pos.y) > radius) continue;
                cell = (int)floorf(e->pos.x / 18.0f) + (int)floorf(e->pos.y / 18.0f);
                if (((cell % 2) + 2) % 2 == phase) damage_enemy(e, WS(W_TABELA, WSTAT_DAMAGE, level), p->pos, 0.0f);
            }
            if (sv_run.boss.active && fabsf(sv_run.boss.pos.x - p->pos.x) < radius && fabsf(sv_run.boss.pos.y - p->pos.y) < radius)
            {
                sv_damage_boss(WS(W_TABELA, WSTAT_DAMAGE, level) * p->dmg_mult);
            }
        }
        sv_sfx(SND_tabela);
    }
    else if (w->id == W_COLEGA)
    {
        update_colegas(w, dt);
    }
}

// Ritmo das recargas: o Julio recarrega mais rápido enquanto anda
static float recharge_pace(void)
{
    return sv_run.player.moving ? 1.0f + sv_run.player.move_recharge : 1.0f;
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

        w->cooldown -= dt * recharge_pace();
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
        damage_enemy(e, z->kind == ZK_PROMPT && e->id != z->target_id ? z->damage * PROMPT_SPLASH : z->damage, z->pos, z->kind == ZK_VERA ? 130.0f : z->kind == ZK_CANECA ? 70.0f : z->kind == ZK_PROMPT ? 50.0f : 30.0f);
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
        if (inside) sv_damage_boss((z->kind == ZK_PROMPT && !z->target_boss ? z->damage * PROMPT_SPLASH : z->damage) * sv_run.player.dmg_mult);
    }
}

static void update_zones(float dt)
{
    for (int i = 0; i < SV_MAX_ZONES; i++)
    {
        SvZone *z = &sv_run.zones[i];

        if (!z->alive) continue;

        // A janela do Prompt acompanha o alvo enquanto o comando é digitado
        if (z->kind == ZK_PROMPT && z->delay > 0.0f)
        {
            if (z->target_boss)
            {
                if (sv_run.boss.active) z->pos = sv_run.boss.pos;
            }
            else
            {
                for (int j = 0; j < SV_MAX_ENEMIES; j++)
                {
                    if (sv_run.enemies[j].alive && sv_run.enemies[j].id == z->target_id)
                    {
                        z->pos = sv_run.enemies[j].pos;
                        break;
                    }
                }
            }
        }

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
            if (z->kind == ZK_DEPLOY)
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

    if (sv_run.cheats[CHEAT_FREEZE]) return;   // Trapaça: inimigos parados

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
            if (e->wcd[w] > 0.0f) e->wcd[w] -= dt * recharge_pace();
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
        if (distance > 1150.0f)
        {
            if (e->wall)
            {
                e->alive = false;
                sv_run.enemy_count--;
                continue;
            }
            spawn_ring_position(820.0f, &e->pos);
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

            // Espinhos Corporativos: quem encosta também leva dano
            if (passive_level(P_ESPINHOS) > 0 && e->wcd[W_GRAMPEADOR] <= 0.0f)
            {
                e->wcd[W_GRAMPEADOR] = 0.5f;
                damage_enemy(e, 8.0f * (float)passive_level(P_ESPINHOS), p->pos, 80.0f);
            }
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

// -----------------------------------------------------------------------------
// O que cada escolha muda (mostrado na carta: atributo, de -> para)
// -----------------------------------------------------------------------------

typedef struct
{
    const char *label;
    float step;       // quanto cada nível soma
    int kind;         // 0 percentual, 1 inteiro, 2 com uma casa decimal, 3 percentual negativo
} PassiveDef;

static const PassiveDef passive_defs[P_COUNT] = {
    [P_CAFE] = {"Recarga", 6.0f, 3},
    [P_MANUAL] = {"Área dos poderes", 8.0f, 0},
    [P_COLINHA] = {"XP ganho", 10.0f, 0},
    [P_ERGONOMIA] = {"Velocidade", 6.0f, 0},
    [P_CRACHA] = {"Dano a menos por golpe", 1.0f, 1},
    [P_SAUDE] = {"Vida máxima", 12.0f, 1},
    [P_NETWORK] = {"Alcance de coleta", 20.0f, 0},
    [P_HORAEXTRA] = {"Duração dos poderes", 10.0f, 0},
    [P_REGEN] = {"Vida por segundo", 0.4f, 2},
    [P_SORTE] = {"Chance de Banco de Horas", 2.0f, 0},
    [P_ESPINHOS] = {"Dano a quem encosta", 8.0f, 1},
    [P_FOCO] = {"Dano", 6.0f, 0},
};

// Escreve um número com vírgula, sem zeros sobrando
static void format_number(char *out, size_t size, float value)
{
    char text[32];
    char *dot;

    if (fabsf(value - roundf(value)) < 0.05f) snprintf(text, sizeof(text), "%.0f", value);
    else if (fabsf(value * 10.0f - roundf(value * 10.0f)) < 0.05f) snprintf(text, sizeof(text), "%.1f", value);
    else snprintf(text, sizeof(text), "%.2f", value);
    dot = strchr(text, '.');
    if (dot) *dot = ',';
    snprintf(out, size, "%s", text);
}

static void format_passive_value(char *out, size_t size, int passive, int level)
{
    const PassiveDef *def = &passive_defs[passive];
    float value = def->step * (float)level;
    char number[32];

    format_number(number, sizeof(number), value);
    switch (def->kind)
    {
    case 0: snprintf(out, size, "+%s%%", number); break;
    case 3: snprintf(out, size, "-%s%%", number); break;
    case 2: snprintf(out, size, "+%s", number); break;
    default: snprintf(out, size, "+%s", number); break;
    }
}

// Valor efetivo de um atributo do poder, já com os bônus do jogador
static float weapon_effective(int weapon, int stat, int level)
{
    const WeaponInfo *info = &weapon_infos[weapon];
    SvPlayer *p = &sv_run.player;
    float value = sv_weapon_stat(weapon, stat, level);

    switch (stat)
    {
    case WSTAT_DAMAGE: return value * p->dmg_mult;
    case WSTAT_COOLDOWN: return value * p->cooldown_mult;
    case WSTAT_AREA: return info->area_scaled ? value * p->area_mult : value;
    case WSTAT_EXTRA: return info->extra_scaled ? value * p->area_mult : value;
    default: return value;
    }
}

static const char *weapon_stat_label(int weapon, int stat)
{
    const WeaponInfo *info = &weapon_infos[weapon];

    switch (stat)
    {
    case WSTAT_DAMAGE: return "Dano";
    case WSTAT_COOLDOWN: return info->cd_name;
    case WSTAT_COUNT: return info->n_name;
    case WSTAT_AREA: return info->area_name;
    default: return info->extra_name;
    }
}

int sv_choice_lines(const SvChoice *c, char lines[SV_CHOICE_LINES][96])
{
    int count = 0;
    char a[32], b[32];

    if (c->kind == 0 || c->kind == 1)
    {
        SvWeapon *owned = find_weapon(c->id);
        int to = c->kind == 0 ? 1 : (owned ? owned->level + 1 : 1);
        int from = to - 1;

        for (int stat = 0; stat < 5 && count < SV_CHOICE_LINES; stat++)
        {
            const char *label = weapon_stat_label(c->id, stat);
            float now, next;
            bool is_time;

            if (!label) continue;
            next = weapon_effective(c->id, stat, to);
            now = from >= 1 ? weapon_effective(c->id, stat, from) : next;
            is_time = stat == WSTAT_COOLDOWN || (stat == WSTAT_EXTRA && weapon_infos[c->id].extra_is_time);

            // Alcances, raios e contagens aparecem arredondados
            if (stat == WSTAT_AREA || stat == WSTAT_COUNT || (stat == WSTAT_EXTRA && !is_time && weapon_infos[c->id].extra_scaled))
            {
                now = roundf(now);
                next = roundf(next);
            }

            if (from >= 1 && fabsf(now - next) < 0.001f) continue; // só o que muda

            format_number(a, sizeof(a), now);
            format_number(b, sizeof(b), next);
            if (from < 1) snprintf(lines[count++], 96, "%s: %s%s", label, b, is_time ? " s" : "");
            else snprintf(lines[count++], 96, "%.20s: %s%s -> %s%s", label, a, is_time ? " s" : "", b, is_time ? " s" : "");
        }
        return count;
    }

    if (c->kind == 2 || c->kind == 3)
    {
        int to = passive_level(c->id) + 1;
        char text_a[32], text_b[32];

        format_passive_value(text_b, sizeof(text_b), c->id, to);
        if (c->kind == 2) snprintf(lines[count++], 96, "%s: %s", passive_defs[c->id].label, text_b);
        else
        {
            format_passive_value(text_a, sizeof(text_a), c->id, to - 1);
            snprintf(lines[count++], 96, "%s: %s -> %s", passive_defs[c->id].label, text_a, text_b);
        }
        return count;
    }

    if (c->kind == 4)
    {
        SvPlayer *p = &sv_run.player;
        float after = fminf(p->max_hp, p->hp + p->max_hp * 0.4f);

        format_number(a, sizeof(a), ceilf(p->hp));
        format_number(b, sizeof(b), ceilf(after));
        snprintf(lines[count++], 96, "Vida: %s -> %s", a, b);
    }
    else if (c->kind == 5)
    {
        format_number(a, sizeof(a), (float)sv_run.coins);
        format_number(b, sizeof(b), (float)(sv_run.coins + 15));
        snprintf(lines[count++], 96, "Banco de Horas: %s -> %s", a, b);
    }
    else
    {
        SvPlayer *p = &sv_run.player;

        format_number(a, sizeof(a), floorf(p->xp));
        format_number(b, sizeof(b), floorf(p->xp + p->xp_needed * 0.5f));
        snprintf(lines[count++], 96, "XP: %s -> %s", a, b);
    }
    return count;
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

    // Personagem ainda bloqueado volta para o Estagiário
    p->character = (sv_debug_mode || sv_character_unlocked(sv_meta.character, sv_meta.runs, sv_meta.wins, sv_meta.best_seconds, sv_meta.kills)) ? sv_meta.character : CH_ESTAGIARIO;
    p->weapons[0] = (SvWeapon){sv_character(p->character)->start_weapon, 1, 0.5f, 0.0f};
    p->weapon_count = 1;
    p->hp = 0.0f;
    p->max_hp = 0.0f;
    recompute_stats();
    p->hp = p->max_hp;
    p->revive_ready = sv_meta.upgrades[UPG_RECOMECO] > 0 || sv_character(p->character)->free_revive;

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
    if (p->negate_flash > 0.0f) p->negate_flash -= dt;
    if (p->regen > 0.0f && p->hp < p->max_hp) p->hp = fminf(p->max_hp, p->hp + p->regen * dt);
    if (p->hurt_flash > 0.0f) p->hurt_flash -= dt;

    p->moving = length > 0.05f && p->root <= 0.0f;

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
    sv_run.spawn_accumulator += sv_spawn_rate(sv_run.time) * dt * (sv_run.boss.active ? 0.25f : 1.0f) * (sv_run.cheats[CHEAT_NO_SPAWN] ? 0.0f : 1.0f);
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
    if (sv_run.cheated) return 0;   // Partida com trapaça não conta para o progresso do save

    earned = sv_run.coins + (int)(sv_run.time / 10.0f) + sv_run.bosses_defeated * 30 + (sv_run.phase == SV_WON ? 300 : 0);
    if (sv_run.player.coin_mult > 0.0f) earned = (int)((float)earned * sv_run.player.coin_mult + 0.5f);
    sv_meta.coins += earned;
    sv_meta.runs++;
    sv_meta.kills += sv_run.kills;
    if (sv_run.phase == SV_WON) sv_meta.wins++;
    if ((int)sv_run.time > sv_meta.best_seconds) sv_meta.best_seconds = (int)sv_run.time;
    sv_meta_save(sv_meta_path());

    return earned;
}

// -----------------------------------------------------------------------------
// Modo debug: trapaças
// -----------------------------------------------------------------------------

bool sv_debug_mode = false;

bool sv_cheat_active(int cheat)
{
    return cheat >= 0 && cheat < CHEAT_COUNT && sv_run.cheats[cheat];
}

void sv_cheat_toggle(int cheat)
{
    if (cheat < 0 || cheat >= CHEAT_COUNT) return;
    sv_run.cheats[cheat] = !sv_run.cheats[cheat];
    if (sv_run.cheats[cheat]) sv_run.cheated = true;
}

void sv_cheat_clear_toggles(void)
{
    for (int i = 0; i < CHEAT_COUNT; i++) sv_run.cheats[i] = false;
}

int sv_cheat_weapon_level(int weapon)
{
    SvWeapon *owned = find_weapon(weapon);

    return owned ? owned->level : 0;
}

int sv_cheat_passive_level(int passive)
{
    return passive_level(passive);
}

bool sv_cheat_set_weapon(int weapon, int level)
{
    SvPlayer *p = &sv_run.player;
    SvWeapon *owned = find_weapon(weapon);

    if (weapon < 0 || weapon >= W_COUNT) return false;
    if (level > SV_WEAPON_MAX_LEVEL) level = SV_WEAPON_MAX_LEVEL;

    if (level <= 0)
    {
        if (!owned) return false;
        sv_run.cheated = true;
        memmove(owned, owned + 1, sizeof(SvWeapon) * (size_t)(p->weapon_count - 1 - (int)(owned - p->weapons)));
        p->weapon_count--;
    }
    else if (owned)
    {
        sv_run.cheated = true;
        owned->level = level;
    }
    else
    {
        if (p->weapon_count >= SV_MAX_WEAPONS) return false;
        sv_run.cheated = true;
        p->weapons[p->weapon_count++] = (SvWeapon){weapon, level, 0.3f, 0.0f};
    }

    recompute_stats();
    return true;
}

bool sv_cheat_set_passive(int passive, int level)
{
    SvPlayer *p = &sv_run.player;
    SvPassive *owned = NULL;

    if (passive < 0 || passive >= P_COUNT) return false;
    if (level > SV_PASSIVE_MAX_LEVEL) level = SV_PASSIVE_MAX_LEVEL;
    for (int i = 0; i < p->passive_count; i++)
    {
        if (p->passives[i].id == passive) owned = &p->passives[i];
    }

    if (level <= 0)
    {
        if (!owned) return false;
        sv_run.cheated = true;
        memmove(owned, owned + 1, sizeof(SvPassive) * (size_t)(p->passive_count - 1 - (int)(owned - p->passives)));
        p->passive_count--;
    }
    else if (owned)
    {
        sv_run.cheated = true;
        owned->level = level;
    }
    else
    {
        if (p->passive_count >= SV_MAX_PASSIVES) return false;
        sv_run.cheated = true;
        p->passives[p->passive_count++] = (SvPassive){passive, level};
    }

    recompute_stats();
    return true;
}

void sv_cheat_spawn_boss(int boss)
{
    if (boss < 0 || boss >= BOSS_COUNT) return;
    sv_run.cheated = true;
    sv_boss_clear_hazards();
    sv_boss_spawn(boss);
}

void sv_cheat_kill_boss(void)
{
    if (!sv_run.boss.active) return;
    sv_run.cheated = true;
    sv_run.boss.invulnerable = 0.0f;
    sv_damage_boss(sv_run.boss.max_hp * 2.0f);
}

void sv_cheat_spawn_enemies(int type, int count)
{
    if (type < 0 || type >= EN_COUNT) return;
    sv_run.cheated = true;
    for (int i = 0; i < count; i++)
    {
        float angle = sv_rand() * 2.0f * PI;
        float distance = sv_rand_range(190.0f, 330.0f);

        sv_spawn_enemy_at(type, Vector2Add(sv_run.player.pos, (Vector2){cosf(angle) * distance, sinf(angle) * distance}), false);
    }
}

void sv_cheat_level_up(void)
{
    SvPlayer *p = &sv_run.player;

    sv_run.cheated = true;
    p->xp = (float)p->xp_needed;
}

void sv_cheat_heal(void)
{
    sv_run.cheated = true;
    sv_run.player.hp = sv_run.player.max_hp;
}

void sv_cheat_revive(void)
{
    sv_run.cheated = true;
    sv_run.player.revive_ready = true;
}

void sv_cheat_jump_time(float seconds)
{
    if (seconds <= 0.0f) return;
    sv_run.cheated = true;
    sv_run.time += seconds;
}

void sv_cheat_clear_enemies(void)
{
    sv_run.cheated = true;
    for (int i = 0; i < SV_MAX_ENEMIES; i++)
    {
        if (!sv_run.enemies[i].alive) continue;
        sv_run.enemies[i].alive = false;
        sv_run.enemy_count--;
    }
}

void sv_cheat_clear_hazards(void)
{
    sv_run.cheated = true;
    sv_boss_clear_hazards();
}

const char *sv_enemy_name(int type)
{
    static const char *names[EN_COUNT] = {"Chamado", "Notificação", "Esvaziado", "Dado Enviesado", "Alucinação", "Auto-Resposta",
                                          "Crachá Fantasma", "Afogado", "Auditor", "Eco da Iara", "Auto-Resposta menor"};

    return (type >= 0 && type < EN_COUNT) ? names[type] : "?";
}

float sv_enemy_radius(int type)
{
    return (type >= 0 && type < EN_COUNT) ? enemy_stats[type].radius : 10.0f;
}
