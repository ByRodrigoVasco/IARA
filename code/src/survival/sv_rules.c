#include <math.h>
#include "sv_rules.h"

static const float boss_time[BOSS_COUNT] = {180.0f, 285.0f, 390.0f, 495.0f, 600.0f};
static const float boss_hp_table[BOSS_COUNT] = {1100.0f, 2600.0f, 4600.0f, 7600.0f, 13000.0f};

float sv_boss_spawn_time(SvBossId boss)
{
    if (boss < 0 || boss >= BOSS_COUNT) return SV_RUN_SECONDS;
    return boss_time[boss];
}

float sv_boss_hp(SvBossId boss)
{
    if (boss < 0 || boss >= BOSS_COUNT) return 1.0f;
    return boss_hp_table[boss];
}

int sv_act_at(float seconds)
{
    // Os atos terminam nos chefes: 3:00, 4:45, 6:30 e 8:15, com 1:45 entre um e outro
    if (seconds < 180.0f) return 0;
    if (seconds < 285.0f) return 1;
    if (seconds < 390.0f) return 2;
    if (seconds < 495.0f) return 3;
    return 4;
}

float sv_spawn_rate(float seconds)
{
    float minutes = seconds / 60.0f;

    // Começa leve e sobe sem parar: 1,2 por segundo no início e perto de 8 no fim
    return 1.2f + 0.45f * minutes + 0.055f * minutes * minutes;
}

float sv_enemy_hp_scale(float seconds)
{
    return 1.0f + seconds / 75.0f;
}

int sv_xp_to_next(int level)
{
    if (level < 1) level = 1;
    return 4 + level * 5 + (level * level) / 4;
}

static const int upgrade_max_table[UPG_COUNT] = {8, 10, 6, 8, 6, 5, 8, 5, 1};
static const int upgrade_base_cost[UPG_COUNT] = {40, 60, 50, 70, 50, 30, 60, 80, 400};

int sv_upgrade_max(SvUpgradeId id)
{
    if (id < 0 || id >= UPG_COUNT) return 0;
    return upgrade_max_table[id];
}

int sv_upgrade_cost(SvUpgradeId id, int current_level)
{
    if (id < 0 || id >= UPG_COUNT || current_level >= upgrade_max_table[id]) return -1;
    return (int)(upgrade_base_cost[id] * powf(1.0f + 0.55f * (float)current_level, 1.25f));
}

float sv_boss_hit_damage(float percent_of_base_hp)
{
    float fraction = percent_of_base_hp / 100.0f;

    if (fraction > SV_BOSS_HIT_CAP) fraction = SV_BOSS_HIT_CAP;
    if (fraction < 0.0f) fraction = 0.0f;
    return fraction * SV_BASE_HP;
}
