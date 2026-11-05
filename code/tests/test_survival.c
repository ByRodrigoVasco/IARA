// Testes do Modo Sobrevivência: regras puras, progresso salvo e uma partida simulada sem janela.
// Compilar e rodar: tools/tests/run_tests.ps1
//   ou: gcc code/tests/test_survival.c code/src/survival/sv_rules.c code/src/survival/sv_meta.c code/src/survival/sv_game.c code/src/survival/sv_boss.c code/src/survival/sv_audio.c -Icode/src/survival -Icode/src/game -o test_survival.exe -lraylib -lopengl32 -lgdi32 -lwinmm && ./test_survival.exe
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "raymath.h"
#include "sv_game.h"
#include "sv_meta.h"
#include "sv_rules.h"

#define TEST_META_PATH "test_iara.survival"

static int checks = 0;

static void check(const char *id, const char *text, bool ok)
{
    checks++;
    printf("  [%s] %s: %s\n", ok ? "OK" : "FALHA", id, text);
    if (!ok) exit(1);
}

static void test_rules(void)
{
    printf("\nRegras\n");

    check("SV-01", "uma partida dura 10 minutos", SV_RUN_SECONDS == 600.0f);
    check("SV-02", "os 5 atos terminam nos chefes",
          sv_act_at(0) == 0 && sv_act_at(119) == 0 && sv_act_at(120) == 1 && sv_act_at(270) == 2 && sv_act_at(420) == 3 && sv_act_at(540) == 4 && sv_act_at(599) == 4);

    {
        bool rising = true;

        for (int t = 1; t < 700; t++)
        {
            if (sv_spawn_rate((float)t) < sv_spawn_rate((float)t - 1)) rising = false;
        }
        check("SV-03", "a dificuldade só sobe com o tempo", rising && sv_spawn_rate(600.0f) > 2.0f * sv_spawn_rate(0.0f));
    }

    {
        bool ok = true;

        for (int l = 1; l < 60; l++)
        {
            if (sv_xp_to_next(l + 1) <= sv_xp_to_next(l)) ok = false;
        }
        check("SV-04", "cada nível pede mais XP que o anterior", ok);
    }

    check("SV-05", "os chefes surgem em ordem, o último aos 10:00",
          sv_boss_spawn_time(BOSS_IMPRESSORA) < sv_boss_spawn_time(BOSS_MODELO) && sv_boss_spawn_time(BOSS_MODELO) < sv_boss_spawn_time(BOSS_DIEGO) &&
          sv_boss_spawn_time(BOSS_DIEGO) < sv_boss_spawn_time(BOSS_RENE) && sv_boss_spawn_time(BOSS_IARA) == SV_RUN_SECONDS);

    check("SV-06", "nenhum golpe de chefe passa de 30% da vida base",
          fabsf(sv_boss_hit_damage(30.0f) - 30.0f) < 0.01f && fabsf(sv_boss_hit_damage(80.0f) - 30.0f) < 0.01f && fabsf(sv_boss_hit_damage(8.0f) - 8.0f) < 0.01f && sv_boss_hit_damage(-5.0f) == 0.0f);

    {
        bool ok = true;

        for (int u = 0; u < UPG_COUNT; u++)
        {
            int cost_first = sv_upgrade_cost(u, 0);

            if (cost_first <= 0 || sv_upgrade_cost(u, sv_upgrade_max(u)) != -1) ok = false;
            if (sv_upgrade_max(u) > 1 && sv_upgrade_cost(u, 1) <= cost_first) ok = false;
        }
        check("SV-07", "upgrade custa mais a cada nível e para no máximo", ok);
    }
}

static void test_meta(void)
{
    printf("\nProgresso permanente\n");

    remove(TEST_META_PATH);
    check("SV-08", "sem arquivo, começa zerado", !sv_meta_load(TEST_META_PATH) && sv_meta.coins == 0 && sv_meta.upgrades[UPG_VIDA] == 0);

    sv_meta.coins = 500;
    check("SV-09", "compra desconta o saldo e sobe o nível", sv_meta_buy(UPG_VIDA) && sv_meta.upgrades[UPG_VIDA] == 1 && sv_meta.coins == 500 - sv_upgrade_cost(UPG_VIDA, 0));

    sv_meta.coins = 1;
    check("SV-10", "sem saldo a compra falha e nada muda", !sv_meta_buy(UPG_DANO) && sv_meta.coins == 1 && sv_meta.upgrades[UPG_DANO] == 0);

    sv_meta.coins = 777;
    sv_meta.runs = 4;
    sv_meta.wins = 1;
    sv_meta.best_seconds = 321;
    sv_meta.upgrades[UPG_RECOMECO] = 1;
    check("SV-11", "grava", sv_meta_save(TEST_META_PATH));
    sv_meta_reset();
    check("SV-12", "carrega igual ao que gravou",
          sv_meta_load(TEST_META_PATH) && sv_meta.coins == 777 && sv_meta.runs == 4 && sv_meta.wins == 1 && sv_meta.best_seconds == 321 && sv_meta.upgrades[UPG_RECOMECO] == 1 && sv_meta.upgrades[UPG_VIDA] == 1);

    {
        FILE *file = fopen(TEST_META_PATH, "w");

        fputs("coins=abc\nvida=999\nlixo sem igual\n", file);
        fclose(file);
        sv_meta_reset();
        check("SV-13", "arquivo com lixo não trava e limita o nível ao máximo", sv_meta_load(TEST_META_PATH) && sv_meta.upgrades[UPG_VIDA] == sv_upgrade_max(UPG_VIDA));
    }

    remove(TEST_META_PATH);
    sv_meta_reset();
}

// Um jogador automático: foge do que está perto e escolhe poderes ao acaso.
static Vector2 bot_move(void)
{
    Vector2 flee = {0, 0};
    SvPlayer *p = &sv_run.player;

    for (int i = 0; i < SV_MAX_ENEMIES; i++)
    {
        SvEnemy *e = &sv_run.enemies[i];
        float d;

        if (!e->alive) continue;
        d = Vector2Distance(e->pos, p->pos);
        if (d < 70.0f && d > 0.1f) flee = Vector2Add(flee, Vector2Scale(Vector2Subtract(p->pos, e->pos), 1.0f / (d * d)));
    }

    for (int i = 0; i < SV_MAX_HAZARDS; i++)
    {
        SvHazard *h = &sv_run.hazards[i];
        float d;

        if (!h->alive || h->kind == HZ_BEAM) continue;
        d = Vector2Distance(h->pos, p->pos);
        if (d < h->radius + 90.0f && d > 0.1f) flee = Vector2Add(flee, Vector2Scale(Vector2Subtract(p->pos, h->pos), 3.0f / (d * d)));
    }

    // Vai atrás do item mais próximo quando não há perigo logo em cima
    if (Vector2Length(flee) < 0.0012f)
    {
        float best = 320.0f;
        Vector2 target = {0, 0};
        bool found = false;

        for (int i = 0; i < SV_MAX_GEMS; i++)
        {
            SvGem *g = &sv_run.gems[i];
            float d;

            if (!g->alive) continue;
            d = Vector2Distance(g->pos, p->pos);
            if (d < best) { best = d; target = g->pos; found = true; }
        }
        if (found) flee = Vector2Add(flee, Vector2Scale(Vector2Normalize(Vector2Subtract(target, p->pos)), 0.0012f));
    }

    // Puxa de volta ao centro do mapa quando chega nas bordas
    flee = Vector2Add(flee, Vector2Scale(Vector2Subtract((Vector2){SV_WORLD_W / 2.0f, SV_WORLD_H / 2.0f}, p->pos), 0.0000008f));

    if (Vector2Length(flee) < 0.00001f) return (Vector2){(float)cos(sv_run.time * 0.7), (float)sin(sv_run.time * 0.9)};
    return Vector2Normalize(flee);
}

typedef struct
{
    float seconds;
    int level, kills, bosses, coins;
    int peak_enemies;
    SvPhase phase;
    float min_hp_fraction;
} RunResult;

static RunResult simulate(unsigned seed, float max_seconds)
{
    RunResult result = {0};
    float dt = 1.0f / 60.0f;

    sv_game_start(seed);
    result.min_hp_fraction = 1.0f;

    while (sv_run.time < max_seconds && sv_run.phase != SV_DEAD && sv_run.phase != SV_WON)
    {
        if (sv_run.phase == SV_LEVELUP)
        {
            int pick = 0, best_score = -1;

            for (int c = 0; c < sv_run.choice_count; c++)
            {
                int kind = sv_run.choices[c].kind;
                int score = kind == 1 ? 5 : kind == 0 ? 4 : kind == 3 ? 3 : kind == 2 ? 2 : 1;

                if (score > best_score) { best_score = score; pick = c; }
            }
            sv_game_choose(pick);
            continue;
        }

        sv_game_update(dt, bot_move());
        if (getenv("SV_TRACE") && (int)(sv_run.time * 60.0f) % 600 == 0)
        {
            printf("      t=%3.0f nivel %2d vida %3.0f/%3.0f inimigos %3d abates %4d armas %d boss %d\n", sv_run.time, sv_run.player.level, sv_run.player.hp, sv_run.player.max_hp, sv_run.enemy_count, sv_run.kills, sv_run.player.weapon_count, sv_run.boss.active);
        }

        if (sv_run.enemy_count > result.peak_enemies) result.peak_enemies = sv_run.enemy_count;
        {
            float fraction = sv_run.player.hp / sv_run.player.max_hp;

            if (fraction < result.min_hp_fraction) result.min_hp_fraction = fraction;
        }

        // Invariantes: nada sai do mapa nem fica com número inválido
        assert(isfinite(sv_run.player.pos.x) && isfinite(sv_run.player.pos.y) && isfinite(sv_run.player.hp));
        assert(sv_run.player.pos.x >= 0 && sv_run.player.pos.x <= SV_WORLD_W && sv_run.player.pos.y >= 0 && sv_run.player.pos.y <= SV_WORLD_H);
        assert(sv_run.enemy_count >= 0 && sv_run.enemy_count <= SV_MAX_ENEMIES);
    }

    result.seconds = sv_run.time;
    result.level = sv_run.player.level;
    result.kills = sv_run.kills;
    result.bosses = sv_run.bosses_defeated;
    result.coins = sv_run.coins;
    result.phase = sv_run.phase;
    return result;
}

static void test_simulation(void)
{
    RunResult run;

    printf("\nPartida simulada (sem janela)\n");

    sv_meta_reset();
    sv_game_start(7);
    check("SV-14", "a partida começa com a Estagiária, o Grampeador e vida cheia",
          sv_run.phase == SV_PLAYING && sv_run.player.weapon_count == 1 && sv_run.player.weapons[0].id == W_GRAMPEADOR && sv_run.player.hp == sv_run.player.max_hp && sv_run.player.level == 1);

    sv_meta.upgrades[UPG_VIDA] = 3;
    sv_meta.upgrades[UPG_RECOMECO] = 1;
    sv_game_start(7);
    check("SV-15", "os upgrades permanentes valem na partida", sv_run.player.max_hp == SV_BASE_HP + 30.0f && sv_run.player.revive_ready);
    sv_meta_reset();

    run = simulate(12345, SV_RUN_SECONDS + 120.0f);
    printf("      resultado: %.0fs, nível %d, %d abates, %d chefes, %d moedas, pico de %d inimigos, vida mínima %.0f%%\n",
           run.seconds, run.level, run.kills, run.bosses, run.coins, run.peak_enemies, run.min_hp_fraction * 100.0f);
    check("SV-16", "uma partida inteira roda sem travar nem sair do mapa", run.seconds > 30.0f);
    check("SV-17", "inimigos nascem e são abatidos", run.kills > 10 && run.peak_enemies > 5);

    // Chefe: surge na hora, a barra existe e ele pode ser derrotado
    sv_meta_reset();
    sv_game_start(99);
    sv_run.player.hp = sv_run.player.max_hp = 100000.0f;
    sv_run.time = sv_boss_spawn_time(BOSS_IMPRESSORA) - 0.05f;
    for (int i = 0; i < 10; i++) sv_game_update(1.0f / 60.0f, (Vector2){0, 0});
    check("SV-18", "a Impressora surge aos 2:00", sv_run.boss.active && sv_run.boss.id == BOSS_IMPRESSORA);
    check("SV-19", "o chefe perde vida e cai", sv_damage_boss(sv_run.boss.max_hp * 2.0f) && !sv_run.boss.active && sv_run.bosses_defeated == 1);

    // Derrotar a IARA vence a partida
    sv_game_start(5);
    sv_run.player.hp = sv_run.player.max_hp = 100000.0f;
    sv_boss_spawn(BOSS_IARA);
    sv_damage_boss(sv_run.boss.max_hp * 2.0f);
    check("SV-20", "derrotar a IARA vence a partida", sv_run.phase == SV_WON);

    // Morte acaba a partida; o Banco de Horas é pago uma vez só
    sv_meta_reset();
    sv_game_start(3);
    sv_run.coins = 12;
    sv_run.time = 100.0f;
    sv_run.player.hp = 0.0f;
    sv_game_update(1.0f / 60.0f, (Vector2){0, 0});
    check("SV-21", "vida zerada encerra a partida", sv_run.phase == SV_DEAD);
    {
        int first = sv_game_finish();
        int second = sv_game_finish();

        remove(SV_SAVE_PATH);
        check("SV-22", "o Banco de Horas é pago uma vez (moedas + tempo)", first == 12 + 10 && second == 0 && sv_meta.coins == 22 && sv_meta.runs == 1);
    }

    // Ctrl+Z: o upgrade Recomeço devolve uma vida
    sv_meta_reset();
    sv_meta.upgrades[UPG_RECOMECO] = 1;
    sv_game_start(4);
    sv_run.player.hp = 0.0f;
    sv_game_update(1.0f / 60.0f, (Vector2){0, 0});
    check("SV-23", "Ctrl+Z revive uma vez com metade da vida", sv_run.phase == SV_PLAYING && sv_run.player.hp > 0 && !sv_run.player.revive_ready);
    sv_run.player.hp = 0.0f;
    sv_game_update(1.0f / 60.0f, (Vector2){0, 0});
    check("SV-24", "e a segunda morte é definitiva", sv_run.phase == SV_DEAD);
    sv_meta_reset();
}

static void test_level_up(void)
{
    printf("\nSubir de nível\n");

    sv_meta_reset();
    sv_game_start(21);
    sv_run.player.xp = (float)sv_run.player.xp_needed;
    sv_game_update(1.0f / 60.0f, (Vector2){0, 0});
    check("SV-25", "XP suficiente abre a escolha com 3 opções", sv_run.phase == SV_LEVELUP && sv_run.choice_count == 3 && sv_run.player.level == 2);

    {
        bool distinct = !(sv_run.choices[0].kind == sv_run.choices[1].kind && sv_run.choices[0].id == sv_run.choices[1].id);

        check("SV-26", "as opções não se repetem", distinct);
    }

    {
        int before = sv_run.player.weapon_count + sv_run.player.passive_count;

        sv_run.choices[0] = (SvChoice){2, P_SAUDE};
        sv_game_choose(0);
        check("SV-27", "escolher um passivo o aplica e retoma a partida",
              sv_run.phase == SV_PLAYING && sv_run.player.passive_count == 1 && before == 1 && sv_run.player.max_hp == SV_BASE_HP + 12.0f);
    }

    // Todos os poderes no nível máximo: sobra só marmita e Banco de Horas
    for (int w = 0; w < W_COUNT; w++)
    {
        if (w == W_GRAMPEADOR) continue;
        if (sv_run.player.weapon_count < SV_MAX_WEAPONS) sv_run.player.weapons[sv_run.player.weapon_count++] = (SvWeapon){w, SV_WEAPON_MAX_LEVEL, 0.0f, 0.0f};
    }
    sv_run.player.weapons[0].level = SV_WEAPON_MAX_LEVEL;
    for (int i = 0; i < SV_MAX_PASSIVES; i++) sv_run.player.passives[i] = (SvPassive){i, SV_PASSIVE_MAX_LEVEL};
    sv_run.player.passive_count = SV_MAX_PASSIVES;
    sv_run.player.xp = (float)sv_run.player.xp_needed;
    sv_game_update(1.0f / 60.0f, (Vector2){0, 0});
    check("SV-28", "com tudo no máximo, oferece marmita e Banco de Horas", sv_run.phase == SV_LEVELUP && sv_run.choice_count == 3 && sv_run.choices[sv_run.choice_count - 1].kind >= 4);
    sv_meta_reset();

    // Cada poder dispara sem travar e causa dano: todos os 12, dois grupos de 6, no nível máximo
    for (int group = 0; group < 2; group++)
    {
        int kills_before;
        bool damaged;

        sv_game_start(60 + (unsigned)group);
        sv_run.player.hp = sv_run.player.max_hp = 1000000.0f;
        sv_run.player.weapon_count = 0;
        for (int i = 0; i < 6; i++) sv_run.player.weapons[sv_run.player.weapon_count++] = (SvWeapon){group * 6 + i, SV_WEAPON_MAX_LEVEL, 0.0f, 0.0f};
        sv_run.time = 100.0f;
        for (int i = 0; i < 30; i++) sv_spawn_enemy_at(EN_ESVAZIADO, Vector2Add(sv_run.player.pos, (Vector2){(float)(i % 6 - 3) * 25.0f, (float)(i / 6 - 2) * 25.0f}), false);
        kills_before = sv_run.kills;
        for (int frame = 0; frame < 60 * 20; frame++)
        {
            if (sv_run.phase == SV_LEVELUP) sv_game_choose(0);
            sv_game_update(1.0f / 60.0f, (Vector2){0, 0});
        }
        damaged = sv_run.kills > kills_before;
        check(group == 0 ? "SV-29" : "SV-30", "o grupo de poderes mata inimigos e não trava", damaged && isfinite(sv_run.player.hp));
    }
    sv_meta_reset();
}

int main(void)
{
    printf("Testes do Modo Sobrevivencia\n");

    test_rules();
    test_meta();
    test_simulation();
    test_level_up();

    printf("\n%d verificacoes, todas passaram.\n", checks);
    return 0;
}
