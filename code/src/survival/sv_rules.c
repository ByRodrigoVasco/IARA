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

// Ordem dos poderes iniciais: 0 Grampeador, 1 Ponteiro, 2 Caneca, 3 Post-its, 4 Não Perturbe, 5 Deploy, 6 Tarja, 7 Modus
static const SvCharacter characters[CH_COUNT] = {
    [CH_ESTAGIARIO] = {"Estagiarios", "Estagiário", "Aprendiz do departamento", "Aprendiz",
                       "Aprende rápido: ganha 10% a mais de XP.",
                       "Já liberado.",
                       0, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.10f, 1.0f, 0.0f, 1.0f, 0, false},
    [CH_LAURA] = {"Laura", "Laura", "Analista de qualidade", "Olho de Bug",
                  "15% de chance de acertar um golpe crítico, que causa o dobro de dano. Vida máxima -10.",
                  "Sobreviva 3:00 em uma partida.",
                  3, -10.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.15f, 1.0f, 0, false},
    [CH_ANDERSON] = {"Anderson", "Anderson", "Infraestrutura", "Servidor Estável",
                     "Vida máxima +40 e -1 de dano em cada golpe. Velocidade -8%.",
                     "Elimine 500 inimigos no total.",
                     2, 40.0f, 1.0f, 0.92f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0, false},
    [CH_JULIO] = {"Julio", "Julio", "Desenvolvedor back-end", "Deploy Contínuo",
                  "Velocidade +18%. Vida máxima -10.",
                  "Jogue 5 partidas.",
                  5, -10.0f, 0.0f, 1.18f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0, false},
    [CH_NICOLLAS] = {"Nicollas", "Nicollas", "Suporte técnico", "Plantão Estendido",
                     "Poderes recarregam 12% mais rápido e duram 10% mais. Vida máxima -10.",
                     "Sobreviva 6:30 em uma partida.",
                     1, -10.0f, 0.0f, 1.0f, 0.88f, 1.10f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0, false},
    [CH_VASCO] = {"Vasco", "Vasco", "Líder de projeto", "Rede de Contatos",
                  "Alcance de coleta 80% maior e 20% a mais de XP.",
                  "Elimine 3000 inimigos no total.",
                  4, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.80f, 1.20f, 1.0f, 0.0f, 1.0f, 0, false},
    [CH_ZITO] = {"Zito", "Zito", "Financeiro", "Bônus de Produtividade",
                 "Ganha 25% a mais de Banco de Horas ao fim da partida e os inimigos soltam mais moedas.",
                 "Jogue 15 partidas.",
                 6, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.25f, 4, false},
    [CH_JV] = {"JV", "JV", "Gerente de produto", "Segunda Chance",
               "Começa a partida com o Ctrl+Z já ativo e causa 5% a mais de dano.",
               "Derrote a IARA uma vez.",
               7, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.05f, 0.0f, 1.0f, 0, true},
};

const SvCharacter *sv_character(int id)
{
    if (id < 0 || id >= CH_COUNT) return &characters[CH_ESTAGIARIO];
    return &characters[id];
}

bool sv_character_unlocked(int id, int runs, int wins, int best_seconds, int kills)
{
    switch (id)
    {
    case CH_ESTAGIARIO: return true;
    case CH_LAURA: return best_seconds >= 180;
    case CH_ANDERSON: return kills >= 500;
    case CH_JULIO: return runs >= 5;
    case CH_NICOLLAS: return best_seconds >= 390;
    case CH_VASCO: return kills >= 3000;
    case CH_ZITO: return runs >= 15;
    case CH_JV: return wins >= 1;
    default: return false;
    }
}
