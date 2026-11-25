#ifndef SV_RULES_H
#define SV_RULES_H

/*
 * Regras puras do Modo Sobrevivência: números e fórmulas, sem Raylib e sem janela.
 * Ficam separadas para os testes automáticos rodarem sem abrir o jogo.
 */

#include <stdbool.h>

#define SV_RUN_SECONDS 600.0f   // Uma partida dura 10 minutos
#define SV_ACT_COUNT 5          // Atos I a V
#define SV_BASE_HP 100.0f       // "Vida base": o teto de dano dos chefes é percentual dela
#define SV_BOSS_HIT_CAP 0.30f   // Nenhum golpe de chefe passa de 30% da vida base
#define SV_BASE_DAMAGE 0.92f    // Dano dos poderes sem upgrade nenhum
#define SV_BASE_XP 0.95f        // XP ganho sem upgrade nenhum
#define SV_STRONG_ENEMY 3       // Do nível 3 para cima o inimigo sempre solta o chip de XP e pode soltar marmita
#define SV_WEAK_CHIP_PERCENT 80 //Chance de um inimigo de nível 1 ou 2 soltar o chip de XP

// Os 5 chefes, na ordem em que aparecem.
typedef enum { BOSS_IMPRESSORA, BOSS_MODELO, BOSS_DIEGO, BOSS_RENE, BOSS_IARA, BOSS_COUNT } SvBossId;

// Momento (em segundos) em que cada chefe surge.
float sv_boss_spawn_time(SvBossId boss);

// Vida total de cada chefe.
float sv_boss_hp(SvBossId boss);

// Ato (0 a 4) em que a partida está no instante t.
int sv_act_at(float seconds);

// Quantos inimigos por segundo nascem no instante t.
float sv_spawn_rate(float seconds);

// Multiplicador da vida dos inimigos no instante t.
float sv_enemy_hp_scale(float seconds);

// XP necessário para sair do nível indicado (o primeiro nível é 1).
int sv_xp_to_next(int level);

// Upgrades permanentes comprados com o Banco de Horas.
typedef enum
{
    UPG_VIDA, UPG_DANO, UPG_VELOCIDADE, UPG_RECARGA, UPG_AREA,
    UPG_IMA, UPG_XP, UPG_ARMADURA, UPG_RECOMECO, UPG_COUNT
} SvUpgradeId;

int sv_upgrade_max(SvUpgradeId id);

// Preço do próximo nível, ou -1 se já está no máximo.
int sv_upgrade_cost(SvUpgradeId id, int current_level);

// Dano de cada golpe de chefe em pontos de vida: percentual da vida base, nunca acima do teto.
float sv_boss_hit_damage(float percent_of_base_hp);

// Personagens jogáveis. O primeiro está sempre liberado; os outros abrem com o progresso permanente.
typedef enum
{
    CH_ESTAGIARIO, CH_LAURA, CH_ANDERSON, CH_JULIO, CH_NICOLLAS, CH_VASCO, CH_ZITO, CH_JV, CH_LUCAS, CH_BAUDEL, CH_GUILHERME, CH_DANILO, CH_COUNT
} SvCharacterId;

typedef struct
{
    const char *folder;        // Pasta dos sprites em PERSONAGENS
    const char *name;
    const char *title;         // Cargo, em tom de brincadeira
    const char *ability_name;
    const char *ability_text;
    const char *unlock_text;   // Como liberar
    int start_weapon;          // Índice do poder inicial (mesma ordem de SvWeaponId)
    float hp_bonus;            // Soma na vida máxima
    float armor_bonus;         // Soma na armadura
    float speed_mult;          // Multiplicadores (1,0 = sem mudança)
    float cooldown_mult;
    float duration_mult;
    float magnet_mult;
    float xp_mult;
    float damage_mult;
    float crit_chance;         // 0 a 1; o golpe crítico causa o dobro de dano
    float coin_mult;           // Multiplica o Banco de Horas ganho ao fim da partida
    int coin_luck;             // Soma, em pontos percentuais, na chance de um inimigo soltar Banco de Horas
    bool free_revive;          // Começa com o Ctrl+Z já ativo
    float move_recharge;       // Poderes recarregam mais rápido enquanto o personagem anda (0,12 = 12%)
    float kill_heal;           // Vida recuperada a cada kill_heal_every inimigos derrotados
    int kill_heal_every;       // 0 = sem recuperação por abates
    int extra_colleagues;      // Colegas de Plantão a mais quando o personagem tem esse poder
    float pure_every;          // Função Pura: anula um golpe a cada tantos segundos (0 = não tem)
} SvCharacter;

// Dados do personagem. Um id inválido devolve o Estagiário.
const SvCharacter *sv_character(int id);

// O progresso que libera personagens, poderes e conquistas
typedef struct
{
    int runs;          // Partidas jogadas
    int wins;          // Vezes que a IARA foi derrotada
    int best_seconds;  // Maior tempo de sobrevivência
    int kills;         // Inimigos eliminados no total
    int best_level;    // Maior nível alcançado em uma partida
    int run_kills;     // Mais inimigos eliminados em uma partida
    int run_bosses;    // Mais chefes derrotados em uma partida
} SvProgress;

// Se o personagem está liberado para o progresso informado.
bool sv_character_unlocked(int id, SvProgress progress);

// Poderes sem dono que só saem nas cartas depois de uma conquista, do mais fácil para o mais difícil.
#define SV_LOCKED_POWERS 3

// O poder da conquista de índice 0 a SV_LOCKED_POWERS - 1, ou -1.
int sv_locked_power(int index);

// O objetivo que libera o poder, ou NULL se ele não depende de conquista própria.
const char *sv_weapon_unlock_text(int weapon);

// Peso do poder novo no sorteio das cartas: quanto mais alto o tier, menor (veja sv_rules.c).
int sv_weapon_weight(int weapon);

#define SV_OVERTIME_MULT 2.0f       // Hora extra: inimigos, vida, dano, XP e Banco de Horas em dobro
#define SV_OVERTIME_STEP 1.10f      // E a cada minuto completo dela os inimigos ficam 10% mais fortes

// Multiplicador de vida e dano dos inimigos depois de tantos segundos de hora extra.
float sv_overtime_strength(float seconds_in_overtime);

// Chance, em partes por mil, de um inimigo forte soltar uma marmita no instante t. luck é o nível de Sorte.
int sv_heal_drop_chance(float seconds, int luck);

// O personagem que começa com o poder, ou -1 se o poder não é de ninguém.
int sv_weapon_owner(int weapon);

// O poder de um personagem exige o personagem liberado; os de conquista, o objetivo; o resto é livre.
bool sv_weapon_unlocked(int weapon, SvProgress progress);

#endif // SV_RULES_H
