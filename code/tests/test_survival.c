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
#include "sv_map.h"
#include "sv_rules.h"
#include "sv_ranking.h"
#include "sv_duo.h"

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
          sv_act_at(0) == 0 && sv_act_at(179) == 0 && sv_act_at(180) == 1 && sv_act_at(285) == 2 && sv_act_at(390) == 3 && sv_act_at(495) == 4 && sv_act_at(599) == 4);

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

    check("SV-05", "o primeiro chefe vem aos 3:00, há mais de 1:30 entre eles e o último surge aos 10:00",
          sv_boss_spawn_time(BOSS_IMPRESSORA) >= 180.0f && sv_boss_spawn_time(BOSS_MODELO) - sv_boss_spawn_time(BOSS_IMPRESSORA) >= 100.0f &&
          sv_boss_spawn_time(BOSS_DIEGO) - sv_boss_spawn_time(BOSS_MODELO) >= 100.0f && sv_boss_spawn_time(BOSS_RENE) - sv_boss_spawn_time(BOSS_DIEGO) >= 100.0f &&
          sv_boss_spawn_time(BOSS_IARA) - sv_boss_spawn_time(BOSS_RENE) >= 100.0f &&
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
    check("SV-18", "a Impressora surge aos 3:00", sv_run.boss.active && sv_run.boss.id == BOSS_IMPRESSORA);
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

static void test_map(void)
{
    printf("\nMapa e colisao\n");

    sv_map_init();
    check("SV-31", "o mapa tem objetos soltos", sv_map_prop_count() > 200);

    // Nenhuma passagem falsa: o vão entre duas caixas é zero (encostadas) ou largo o bastante para andar
    {
        int narrow = 0;

        for (int i = 0; i < sv_map_prop_count(); i++)
        {
            for (int j = i + 1; j < sv_map_prop_count(); j++)
            {
                const SvMapProp *a = sv_map_prop(i), *b = sv_map_prop(j);
                float dx = fmaxf(0.0f, fmaxf(a->rx - (b->rx + b->rw), b->rx - (a->rx + a->rw)));
                float dy = fmaxf(0.0f, fmaxf(a->ry - (b->ry + b->rh), b->ry - (a->ry + a->rh)));
                float gap = sqrtf(dx * dx + dy * dy);

                if (gap > 0.5f && gap < 20.0f) narrow++;
            }
        }
        check("SV-32", "não há vão estreito entre objetos (só encostados ou passagem larga)", narrow == 0);
    }

    // O jogador empurrado contra cada objeto nunca entra nele
    {
        bool inside = false;

        for (int i = 0; i < sv_map_prop_count() && !inside; i += 7)
        {
            const SvMapProp *p = sv_map_prop(i);
            Vector2 pos = {p->rx + p->rw / 2.0f, p->ry - 12.0f};

            sv_game_start(31);
            sv_run.player.pos = (Vector2){pos.x, pos.y - 10.0f};
            for (int f = 0; f < 90; f++) sv_game_update(1.0f / 60.0f, (Vector2){0.0f, 1.0f});
            {
                float fx = sv_run.player.pos.x, fy = sv_run.player.pos.y + 10.0f;

                for (int k = 0; k < sv_map_prop_count(); k++)
                {
                    const SvMapProp *q = sv_map_prop(k);

                    if (fx > q->rx + 1.0f && fx < q->rx + q->rw - 1.0f && fy > q->ry + 1.0f && fy < q->ry + q->rh - 1.0f) inside = true;
                }
            }
        }
        check("SV-33", "andar contra os objetos não atravessa nenhum", !inside);
    }

    // Todo espaço livre é alcançável a partir do ponto de partida (sem bolsões fechados)
    {
        enum { STEP = 12, GW = SV_WORLD_W / STEP, GH = SV_WORLD_H / STEP };
        static unsigned char free_cell[GW * GH], seen[GW * GH];
        static int queue[GW * GH];
        int head = 0, tail = 0, free_count = 0, reached = 0;
        int start = ((int)(3.5f * SV_CELL_H / STEP)) * 0 + (int)(2.5f * SV_CELL_H / STEP) * GW + (int)(3.5f * SV_CELL_W / STEP);

        for (int y = 2; y < GH - 2; y++)
        {
            for (int x = 2; x < GW - 2; x++)
            {
                Vector2 p = {(float)x * STEP, (float)y * STEP - 10.0f};

                free_cell[y * GW + x] = sv_map_collide(&p, 8.0f, 10.0f) == 0.0f;
                if (free_cell[y * GW + x]) free_count++;
            }
        }

        seen[start] = 1;
        queue[tail++] = start;
        while (head < tail)
        {
            int cell = queue[head++], x = cell % GW, y = cell / GW;
            int next[4] = {cell - 1, cell + 1, cell - GW, cell + GW};

            reached++;
            for (int n = 0; n < 4; n++)
            {
                if (next[n] < 0 || next[n] >= GW * GH || (n < 2 && next[n] / GW != y) || !free_cell[next[n]] || seen[next[n]]) continue;
                seen[next[n]] = 1;
                queue[tail++] = next[n];
            }
            (void)x;
        }
        printf("      %d de %d casas livres alcançáveis\n", reached, free_count);
        check("SV-34", "o espaço livre é um só (sem bolsões fechados)", free_count > 0 && reached >= free_count * 99 / 100);
    }
    sv_meta_reset();
}

static void test_choices(void)
{
    char lines[SV_CHOICE_LINES][96];
    int n;

    printf("\nO que cada escolha muda\n");

    sv_meta_reset();
    sv_game_start(41);

    // Evoluir o Grampeador do nível 1 para o 2: dano igual, recarga e perfuração mudam
    {
        SvChoice up = {1, W_GRAMPEADOR};

        n = sv_choice_lines(&up, lines);
        check("SV-35", "evoluir um poder mostra só o que muda, de -> para",
              n == 2 && strstr(lines[0], "Recarga: 0,7 s -> 0,6 s") && strstr(lines[1], "Perfuração: 1 -> 2"));
    }

    {
        SvChoice fresh = {0, W_TABELA};

        n = sv_choice_lines(&fresh, lines);
        check("SV-36", "poder novo mostra os atributos do nível 1", n >= 3 && strstr(lines[0], "Dano: 6") && !strstr(lines[0], "->"));
    }

    {
        SvChoice pas = {2, P_SAUDE};
        SvChoice pas_up = {3, P_SAUDE};

        n = sv_choice_lines(&pas, lines);
        check("SV-37", "passivo novo mostra o ganho do nível 1", n == 1 && strstr(lines[0], "Vida máxima: +12"));
        sv_run.player.passives[sv_run.player.passive_count++] = (SvPassive){P_SAUDE, 1};
        n = sv_choice_lines(&pas_up, lines);
        check("SV-38", "evoluir passivo mostra de -> para", n == 1 && strstr(lines[0], "+12 -> +24"));
    }

    // Os números mostrados são os que valem: com 3 níveis de Foco o dano mostrado já inclui o bônus
    {
        SvChoice up = {1, W_GRAMPEADOR};

        sv_run.player.passives[sv_run.player.passive_count++] = (SvPassive){P_FOCO, 3};
        sv_run.player.weapons[0].level = 2;
        sv_run.player.dmg_mult = 1.18f;
        n = sv_choice_lines(&up, lines);
        check("SV-39", "os valores da carta incluem os bônus do jogador", n >= 1 && strstr(lines[0], "Dano: 11,8 -> 16,5"));
    }

    // Cada poder muda atributos diferentes ao longo dos 5 níveis e todos melhoram
    {
        bool ok = true;

        for (int w = 0; w < W_COUNT; w++)
        {
            for (int level = 1; level < SV_WEAPON_MAX_LEVEL; level++)
            {
                bool changed = false;

                for (int stat = 0; stat < 5; stat++)
                {
                    float a = sv_weapon_stat(w, stat, level), b = sv_weapon_stat(w, stat, level + 1);

                    if (stat == WSTAT_COOLDOWN ? b > a : b < a) ok = false;
                    if (a != b) changed = true;
                }
                if (!changed) ok = false;
            }
        }
        check("SV-40", "cada nível de cada poder melhora algum atributo e nenhum piora", ok);
    }
    sv_meta_reset();
}

static void unlock_everything(void)
{
    sv_meta.runs = 99;
    sv_meta.wins = 9;
    sv_meta.best_seconds = 600;
    sv_meta.kills = 99999;
}

static void test_characters(void)
{
    printf("\nPersonagens\n");

    {
        bool distinct = true, weapons_ok = true;

        for (int a = 0; a < CH_COUNT; a++)
        {
            const SvCharacter *ca = sv_character(a);

            if (ca->start_weapon < 0 || ca->start_weapon >= W_COUNT) weapons_ok = false;
            for (int b = a + 1; b < CH_COUNT; b++)
            {
                const SvCharacter *cb = sv_character(b);

                if (!strcmp(ca->folder, cb->folder) || !strcmp(ca->name, cb->name) || ca->start_weapon == cb->start_weapon) distinct = false;
            }
        }
        check("SV-41", "os 12 personagens têm pasta, nome e poder inicial próprios", CH_COUNT == 12 && distinct && weapons_ok);
    }

    check("SV-42", "só o Estagiário vem liberado; os outros abrem com o progresso",
          sv_character_unlocked(CH_ESTAGIARIO, 0, 0, 0, 0) && !sv_character_unlocked(CH_LAURA, 0, 0, 0, 0) &&
          !sv_character_unlocked(CH_LAURA, 0, 0, 179, 0) && sv_character_unlocked(CH_LAURA, 0, 0, 180, 0) &&
          sv_character_unlocked(CH_ANDERSON, 0, 0, 0, 500) && sv_character_unlocked(CH_JULIO, 5, 0, 0, 0) &&
          sv_character_unlocked(CH_NICOLLAS, 0, 0, 390, 0) && sv_character_unlocked(CH_VASCO, 0, 0, 0, 3000) &&
          sv_character_unlocked(CH_ZITO, 15, 0, 0, 0) && !sv_character_unlocked(CH_JV, 99, 0, 600, 99999) && sv_character_unlocked(CH_JV, 0, 1, 0, 0));

    {
        bool all_open = true;

        for (int i = 0; i < CH_COUNT; i++) all_open = all_open && sv_character_unlocked(i, 99, 9, 600, 99999);
        check("SV-43", "com o progresso máximo todos estão liberados; id inválido cai no Estagiário", all_open && sv_character(-1) == sv_character(CH_ESTAGIARIO) && sv_character(99) == sv_character(CH_ESTAGIARIO));
    }

    sv_meta_reset();
    sv_game_start(7);
    {
        float base_hp = sv_run.player.max_hp, base_speed = sv_run.player.speed_mult, base_magnet = sv_run.player.magnet_range;

        check("SV-44", "o Estagiário começa com o Grampeador e sem bônus de vida", sv_run.player.weapons[0].id == W_GRAMPEADOR && base_hp == SV_BASE_HP && sv_run.player.character == CH_ESTAGIARIO);

        sv_meta_reset();
        unlock_everything();
        sv_meta.character = CH_ANDERSON;
        sv_game_start(7);
        check("SV-45", "Anderson: +60 de vida, +2 de armadura, mais lento, poder inicial próprio",
              sv_run.player.max_hp == base_hp + 60.0f && sv_run.player.armor == 2.0f && sv_run.player.speed_mult < base_speed && sv_run.player.weapons[0].id == sv_character(CH_ANDERSON)->start_weapon);

        sv_meta.character = CH_JULIO;
        sv_game_start(7);
        check("SV-46", "Julio: mais rápido e com menos vida", sv_run.player.speed_mult > base_speed * 1.17f && sv_run.player.max_hp == base_hp - 10.0f);

        sv_meta.character = CH_VASCO;
        sv_game_start(7);
        check("SV-47", "Vasco: coleta maior, 15% a mais de XP e 1 colega a mais", sv_run.player.magnet_range > base_magnet * 1.7f && sv_run.player.xp_mult > 1.14f && sv_run.player.xp_mult < 1.16f && sv_run.player.extra_colleagues == 1);

        sv_meta.character = CH_NICOLLAS;
        sv_game_start(7);
        check("SV-48", "Nicollas: recarga menor e recupera vida a cada 20 abates", sv_run.player.cooldown_mult < 0.9f && sv_run.player.kill_heal_every == 20 && sv_run.player.kill_heal > 3.0f);

        sv_meta.character = CH_LAURA;
        sv_game_start(7);
        check("SV-49", "Laura: chance de crítico", sv_run.player.crit_chance > 0.14f && sv_run.player.max_hp == base_hp - 10.0f);

        sv_meta.character = CH_JV;
        sv_game_start(7);
        check("SV-50", "JV começa com o Ctrl+Z ativo", sv_run.player.revive_ready);

        sv_meta.character = CH_ZITO;
        sv_game_start(7);
        sv_run.coins = 100;
        sv_run.time = 0.0f;
        {
            int before = sv_meta.coins;
            int earned = sv_game_finish();

            check("SV-51", "Zito ganha 25% a mais de Banco de Horas ao fim da partida", earned == 125 && sv_meta.coins == before + 125);
        }
    }

    // Personagem bloqueado salvo no arquivo volta para o Estagiário
    sv_meta_reset();
    sv_meta.character = CH_JV;
    sv_game_start(7);
    check("SV-52", "personagem bloqueado não entra na partida", sv_run.player.character == CH_ESTAGIARIO);

    sv_meta_reset();
    unlock_everything();
    sv_meta.character = CH_VASCO;
    check("SV-53", "o personagem escolhido é salvo e lido de volta", sv_meta_save(TEST_META_PATH));
    sv_meta_reset();
    check("SV-54", "o arquivo devolve o mesmo personagem", sv_meta_load(TEST_META_PATH) && sv_meta.character == CH_VASCO);
    remove(TEST_META_PATH);

    {
        bool survived = true;

        for (int ch = 0; ch < CH_COUNT; ch++)
        {
            sv_meta_reset();
            unlock_everything();
            sv_meta.character = ch;
            sv_game_start(100 + (unsigned)ch);
            for (int frame = 0; frame < 60 * 90; frame++)
            {
                float angle = (float)frame * 0.02f;

                sv_game_update(1.0f / 60.0f, (Vector2){cosf(angle), sinf(angle)});
                if (sv_run.phase == SV_LEVELUP) sv_game_choose(0);
                if (sv_run.phase != SV_PLAYING && sv_run.phase != SV_LEVELUP) break;
                if (!isfinite(sv_run.player.pos.x) || !isfinite(sv_run.player.hp)) survived = false;
            }
        }
        check("SV-55", "os 8 personagens jogam 90 segundos sem valores inválidos", survived);
    }

    // Vera Fischer (V -> F = F): o Anderson anula um golpe e só repete depois do intervalo
    {
        SvPlayer *p;
        float hp;

        sv_meta_reset();
        unlock_everything();
        sv_meta.character = CH_ANDERSON;
        sv_game_start(9);
        p = &sv_run.player;
        p->weapons[0].cooldown = 0.0f;
        hp = p->hp;
        sv_hurt_player(20.0f, EFF_NONE, 0.0f);
        check("SV-58", "Vera Fischer anula o golpe, solta a onda e recomeça a recarga", p->hp == hp && p->weapons[0].cooldown > 2.0f && p->invulnerable > 0.4f);
        p->invulnerable = 0.0f;
        sv_hurt_player(20.0f, EFF_NONE, 0.0f);
        check("SV-59", "o golpe seguinte, ainda na recarga, causa dano", p->hp < hp);
        p->invulnerable = 0.0f;
        p->weapons[0].cooldown = 0.0f;
        hp = p->hp;
        sv_hurt_player(20.0f, EFF_NONE, 0.0f);
        check("SV-60", "recarregada, a Vera Fischer anula de novo", p->hp == hp);

        sv_meta.character = CH_ESTAGIARIO;
        sv_game_start(9);
        hp = sv_run.player.hp;
        sv_hurt_player(20.0f, EFF_NONE, 0.0f);
        check("SV-61", "quem não tem a Vera Fischer leva o golpe", sv_run.player.hp < hp);

        sv_meta.character = CH_ANDERSON;
        sv_game_start(10);
        sv_run.player.weapons[0].cooldown = 0.0f;
        sv_spawn_enemy_at(EN_CHAMADO, (Vector2){sv_run.player.pos.x + 30.0f, sv_run.player.pos.y}, false);
        sv_game_update(1.0f / 60.0f, (Vector2){0.0f, 0.0f});
        sv_game_update(1.0f / 60.0f, (Vector2){0.0f, 0.0f});
        check("SV-62", "a Vera Fischer dispara sozinha quando um inimigo chega perto", sv_run.kills >= 1 && sv_run.player.weapons[0].cooldown > 2.0f);
    }

    // Julio: recarrega mais rápido andando, e o Deploy cai sobre quem se aproxima
    {
        float standing, walking;

        sv_meta_reset();
        unlock_everything();
        sv_meta.character = CH_JULIO;
        sv_game_start(12);
        sv_run.player.weapons[0].cooldown = 10.0f;
        sv_game_update(0.05f, (Vector2){0.0f, 0.0f});
        standing = 10.0f - sv_run.player.weapons[0].cooldown;
        sv_run.player.weapons[0].cooldown = 10.0f;
        sv_game_update(0.05f, (Vector2){1.0f, 0.0f});
        walking = 10.0f - sv_run.player.weapons[0].cooldown;
        check("SV-65", "o Julio recarrega os poderes mais rápido enquanto anda", walking > standing * 1.1f);

        sv_game_start(13);
        sv_spawn_enemy_at(EN_CHAMADO, (Vector2){sv_run.player.pos.x + 150.0f, sv_run.player.pos.y}, false);
        for (int frame = 0; frame < 60 * 4 && sv_run.kills == 0; frame++) sv_game_update(1.0f / 60.0f, (Vector2){0.0f, 0.0f});
        check("SV-66", "o Deploy em Produção acerta o inimigo que se aproxima", sv_run.kills >= 1);
    }

    // Nicollas: cada 20 abates devolvem 4 de vida, e o Ponteiro mira o inimigo mais próximo
    {
        float hp;

        sv_meta_reset();
        unlock_everything();
        sv_meta.character = CH_NICOLLAS;
        sv_game_start(14);
        sv_run.player.hp = sv_run.player.max_hp - 30.0f;
        hp = sv_run.player.hp;
        sv_run.player.dmg_mult = 50.0f;
        sv_run.player.invulnerable = 30.0f;
        // 20 chamados enfileirados na linha do Ponteiro, que derruba quase todos num golpe só
        for (int i = 0; i < 20; i++) sv_spawn_enemy_at(EN_CHAMADO, (Vector2){sv_run.player.pos.x + 55.0f + (float)i * 2.5f, sv_run.player.pos.y}, false);
        for (int frame = 0; frame < 60 * 6 && sv_run.kills < 20; frame++)
        {
            if (sv_run.phase == SV_LEVELUP) sv_game_choose(0);
            sv_game_update(1.0f / 60.0f, (Vector2){0.0f, 0.0f});
        }
        check("SV-67", "a cada 20 inimigos derrotados a Nicollas recupera 4 de vida", sv_run.kills >= 20 && sv_run.player.hp > hp + 3.9f && sv_run.player.hp < hp + 4.1f);

        sv_game_start(15);
        sv_spawn_enemy_at(EN_CHAMADO, (Vector2){sv_run.player.pos.x + 70.0f, sv_run.player.pos.y}, false);
        for (int frame = 0; frame < 60 * 3 && sv_run.kills == 0; frame++) sv_game_update(1.0f / 60.0f, (Vector2){0.0f, 0.0f});
        check("SV-68", "o Ponteiro acerta o inimigo mesmo fora da direção em que o jogador olha", sv_run.kills >= 1);
    }

    // Colegas de Plantão: o Vasco traz um a mais, e cada função faz a sua parte
    {
        float hp;
        bool shot_seen = false;

        sv_meta_reset();
        unlock_everything();
        sv_meta.character = CH_VASCO;
        sv_game_start(16);
        sv_run.player.weapons[sv_run.player.weapon_count++] = (SvWeapon){W_COLEGA, 1, 0.0f, 0.0f};
        check("SV-69", "o Vasco tem 2 colegas de plantão já no nível 1", sv_colega_count() == 2);

        sv_meta.character = CH_ESTAGIARIO;
        sv_game_start(16);
        check("SV-70", "sem o poder não há colegas", sv_colega_count() == 0);
        sv_run.player.weapons[sv_run.player.weapon_count++] = (SvWeapon){W_COLEGA, 1, 0.0f, 0.0f};
        check("SV-71", "os outros personagens começam com 1 colega", sv_colega_count() == 1);

        // A enfermeira (3º colega) cura quem está ferido e espera 6 s para curar de novo
        sv_meta.character = CH_VASCO;
        sv_game_start(17);
        sv_run.player.weapons[sv_run.player.weapon_count++] = (SvWeapon){W_COLEGA, 3, 0.0f, 0.0f};
        sv_run.player.hp = sv_run.player.max_hp - 20.0f;
        hp = sv_run.player.hp;
        sv_game_update(0.05f, (Vector2){0.0f, 0.0f});
        check("SV-72", "a enfermeira do RH cura o jogador ferido", sv_run.player.hp > hp + 1.9f);
        hp = sv_run.player.hp;
        for (int frame = 0; frame < 60 * 3; frame++) sv_game_update(1.0f / 60.0f, (Vector2){0.0f, 0.0f});
        check("SV-73", "e só cura de novo depois do intervalo", sv_run.player.hp < hp + 0.1f);

        // O Suporte de TI (2º colega) atira pacotes de dados de longe
        sv_game_start(18);
        sv_run.player.weapons[sv_run.player.weapon_count++] = (SvWeapon){W_COLEGA, 3, 0.0f, 0.0f};
        sv_spawn_enemy_at(EN_ESVAZIADO, (Vector2){sv_run.player.pos.x + 220.0f, sv_run.player.pos.y}, false);
        for (int frame = 0; frame < 60 * 4 && !shot_seen; frame++)
        {
            sv_game_update(1.0f / 60.0f, (Vector2){0.0f, 0.0f});
            for (int i = 0; i < SV_MAX_SHOTS; i++) if (sv_run.shots[i].alive && sv_run.shots[i].kind == 1) shot_seen = true;
        }
        check("SV-74", "o Suporte de TI atira pacotes de dados no inimigo distante", shot_seen);
    }

    // Prompt da IARA e Fibra Óptica
    {
        unsigned strongest_id = 0;
        bool targeted = false;

        sv_meta_reset();
        unlock_everything();
        sv_meta.character = CH_ESTAGIARIO;
        sv_game_start(19);
        sv_run.player.weapons[0] = (SvWeapon){W_PROMPT, 1, 0.0f, 0.0f};
        sv_spawn_enemy_at(EN_CHAMADO, (Vector2){sv_run.player.pos.x + 60.0f, sv_run.player.pos.y}, false);
        sv_spawn_enemy_at(EN_ESVAZIADO, (Vector2){sv_run.player.pos.x, sv_run.player.pos.y + 200.0f}, false);
        for (int i = 0; i < SV_MAX_ENEMIES; i++) if (sv_run.enemies[i].alive && sv_run.enemies[i].type == EN_ESVAZIADO) strongest_id = sv_run.enemies[i].id;
        sv_game_update(1.0f / 60.0f, (Vector2){0.0f, 0.0f});
        for (int i = 0; i < SV_MAX_ZONES; i++) if (sv_run.zones[i].alive && sv_run.zones[i].kind == ZK_PROMPT && sv_run.zones[i].target_id == strongest_id) targeted = true;
        check("SV-75", "o Prompt da IARA digita o comando no inimigo mais forte", targeted);
        for (int frame = 0; frame < 60 * 3 && sv_run.kills < 2; frame++) sv_game_update(1.0f / 60.0f, (Vector2){0.0f, 0.0f});
        check("SV-76", "ao executar, derruba o alvo e quem está em volta dele", sv_run.kills >= 1);

        sv_game_start(20);
        sv_run.player.weapons[0] = (SvWeapon){W_FIBRA, 1, 0.0f, 0.0f};
        for (int i = 0; i < 5; i++) sv_spawn_enemy_at(EN_CHAMADO, (Vector2){sv_run.player.pos.x + 100.0f + (float)i * 14.0f, sv_run.player.pos.y}, false);
        for (int frame = 0; frame < 60 && sv_run.kills < 3; frame++) sv_game_update(1.0f / 60.0f, (Vector2){0.0f, 0.0f});
        check("SV-77", "a Fibra Óptica atravessa vários inimigos em linha", sv_run.kills >= 3);

        check("SV-78", "o Não Perturbe tem área inicial maior e bate menos", sv_weapon_stat(W_NAOPERTURBE, WSTAT_AREA, 1) > 60.0f && sv_weapon_stat(W_NAOPERTURBE, WSTAT_DAMAGE, 1) < 5.0f);
    }

    // Caneca: cai sobre o inimigo mais próximo, em vez de um ponto qualquer
    {
        sv_meta_reset();
        unlock_everything();
        sv_meta.character = CH_ANDERSON;
        sv_game_start(11);
        sv_spawn_enemy_at(EN_CHAMADO, (Vector2){sv_run.player.pos.x + 150.0f, sv_run.player.pos.y}, false);
        for (int frame = 0; frame < 60 * 3 && sv_run.kills == 0; frame++) sv_game_update(1.0f / 60.0f, (Vector2){0.0f, 0.0f});
        check("SV-63", "a Caneca acerta o inimigo que se aproxima", sv_run.kills >= 1);
    }
    sv_meta_reset();
}

// Guarda a posição de cada inimigo e mede depois quanto o que mais andou se deslocou
static Vector2 saved_enemy_pos[SV_MAX_ENEMIES];

static void save_enemy_positions(void)
{
    for (int i = 0; i < SV_MAX_ENEMIES; i++) saved_enemy_pos[i] = sv_run.enemies[i].pos;
}

static float enemy_max_displacement(void)
{
    float largest = 0.0f;

    for (int i = 0; i < SV_MAX_ENEMIES; i++)
    {
        float moved;

        if (!sv_run.enemies[i].alive) continue;
        moved = Vector2Distance(sv_run.enemies[i].pos, saved_enemy_pos[i]);
        if (moved > largest) largest = moved;
    }
    return largest;
}

static void test_debug_cheats(void)
{
    int before;

    // Poderes e passivos
    sv_meta_reset();
    sv_game_start(401);
    check("SV-79", "dar um poder pelo cheat o coloca no nível pedido e marca a partida", sv_cheat_set_weapon(W_PROMPT, 3) && sv_cheat_weapon_level(W_PROMPT) == 3 && sv_run.cheated);
    check("SV-80", "dar de novo muda o nível, e o nível 0 tira o poder", sv_cheat_set_weapon(W_PROMPT, 5) && sv_cheat_weapon_level(W_PROMPT) == 5 && sv_cheat_set_weapon(W_PROMPT, 0) && sv_cheat_weapon_level(W_PROMPT) == 0);
    {
        bool refused_a_new_one = false;

        for (int w = 0; w < W_COUNT; w++) sv_cheat_set_weapon(w, 1);
        for (int w = 0; w < W_COUNT; w++)
        {
            if (sv_cheat_weapon_level(w) == 0 && !sv_cheat_set_weapon(w, 1)) refused_a_new_one = true;
        }
        check("SV-81", "o limite de 6 poderes vale também para o cheat", sv_run.player.weapon_count == SV_MAX_WEAPONS && refused_a_new_one);
    }
    check("SV-82", "passivo dado pelo cheat vale na hora", sv_cheat_set_passive(P_SAUDE, 2) && sv_cheat_passive_level(P_SAUDE) == 2 && sv_run.player.max_hp == SV_BASE_HP + 24.0f);

    // Chefes e inimigos
    sv_game_start(402);
    sv_cheat_spawn_boss(BOSS_DIEGO);
    check("SV-83", "chamar um chefe pelo cheat o coloca na partida", sv_run.boss.active && sv_run.boss.id == BOSS_DIEGO);
    before = sv_run.enemy_count;
    sv_cheat_spawn_enemies(EN_AUDITOR, 5);
    check("SV-84", "chamar inimigos pelo cheat os coloca na partida", sv_run.enemy_count == before + 5);
    sv_cheat_clear_enemies();
    check("SV-85", "limpar os inimigos remove todos", sv_run.enemy_count == 0);

    // Invulnerável, um golpe, congelar, nascimentos e tempo
    sv_game_start(403);
    sv_cheat_toggle(CHEAT_GOD);
    {
        float hp = sv_run.player.hp;

        sv_hurt_player(50.0f, EFF_NONE, 0.0f);
        check("SV-86", "invulnerável: o jogador não perde vida", sv_run.player.hp == hp);
    }
    sv_cheat_toggle(CHEAT_GOD);
    sv_cheat_toggle(CHEAT_NO_SPAWN);
    for (int frame = 0; frame < 60 * 6; frame++) sv_game_update(1.0f / 60.0f, (Vector2){0.0f, 0.0f});
    check("SV-87", "sem nascimentos: nenhum inimigo novo aparece", sv_run.enemy_count == 0);
    // Sem armas, para que nada morra durante a medição; os nascimentos seguem parados, para só contar os 3 chamados
    while (sv_run.player.weapon_count > 0) sv_cheat_set_weapon(sv_run.player.weapons[0].id, 0);
    sv_cheat_spawn_enemies(EN_CHAMADO, 3);
    sv_cheat_toggle(CHEAT_FREEZE);
    save_enemy_positions();
    for (int frame = 0; frame < 60 * 2; frame++) sv_game_update(1.0f / 60.0f, (Vector2){0.0f, 0.0f});
    {
        bool stayed = sv_run.enemy_count == 3 && enemy_max_displacement() < 0.01f;

        sv_cheat_toggle(CHEAT_FREEZE);
        for (int frame = 0; frame < 60; frame++) sv_game_update(1.0f / 60.0f, (Vector2){0.0f, 0.0f});
        check("SV-88", "congelar: os inimigos ficam parados e voltam a andar quando descongelados", stayed && enemy_max_displacement() > 5.0f);
    }
    sv_cheat_toggle(CHEAT_NO_SPAWN);
    {
        float t = sv_run.time;

        sv_cheat_jump_time(60.0f);
        check("SV-89", "pular o tempo adianta o relógio da partida", sv_run.time >= t + 59.9f);
    }

    // Partida com cheat não conta para o progresso
    sv_meta_reset();
    sv_game_start(404);
    sv_run.coins = 50;
    sv_run.time = 120.0f;
    sv_cheat_heal();
    {
        int earned = sv_game_finish();

        check("SV-90", "partida com cheat não dá Banco de Horas nem altera o progresso", earned == 0 && sv_meta.coins == 0 && sv_meta.runs == 0 && sv_meta.best_seconds == 0);
    }
    sv_meta_reset();
    sv_game_start(405);
    sv_run.coins = 50;
    sv_run.time = 120.0f;
    check("SV-91", "partida sem cheat continua valendo", sv_game_finish() > 0 && sv_meta.runs == 1);

    // O debug libera todos os personagens
    sv_meta_reset();
    sv_meta.character = CH_JV;
    sv_debug_mode = false;
    sv_game_start(406);
    check("SV-92", "sem debug, um personagem bloqueado volta para o Estagiário", sv_run.player.character == CH_ESTAGIARIO);
    sv_debug_mode = true;
    sv_game_start(406);
    check("SV-93", "com o debug ligado, todos os personagens estão liberados", sv_run.player.character == CH_JV);
    sv_debug_mode = false;
    sv_meta_reset();
}

static void test_boss_balance(void)
{
    bool rising = true;

    printf("\nEquilibrio dos chefes\n");
    for (int b = 1; b < BOSS_COUNT; b++)
    {
        if (sv_boss_hp((SvBossId)b) <= sv_boss_hp((SvBossId)(b - 1))) rising = false;
        if (sv_boss_spawn_time((SvBossId)b) - sv_boss_spawn_time((SvBossId)(b - 1)) < 100.0f) rising = false;
    }
    check("SV-56", "cada chefe tem mais vida que o anterior e pelo menos 1:40 de intervalo", rising && sv_boss_spawn_time(BOSS_IMPRESSORA) == 180.0f);
    check("SV-57", "os inimigos comuns ficam mais fortes aos poucos, sem salto", sv_enemy_hp_scale(0.0f) == 1.0f && sv_enemy_hp_scale(600.0f) < 8.0f && sv_spawn_rate(600.0f) < 11.0f);
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

    // Cada poder dispara sem travar e causa dano: todos os 13, em grupos de 6, no nível máximo
    for (int group = 0; group < 3; group++)
    {
        int kills_before;
        bool damaged;

        sv_game_start(60 + (unsigned)group);
        sv_run.player.hp = sv_run.player.max_hp = 1000000.0f;
        sv_run.player.weapon_count = 0;
        for (int i = 0; i < 6 && group * 6 + i < W_COUNT; i++) sv_run.player.weapons[sv_run.player.weapon_count++] = (SvWeapon){group * 6 + i, SV_WEAPON_MAX_LEVEL, 0.0f, 0.0f};
        sv_run.time = 100.0f;
        for (int i = 0; i < 30; i++) sv_spawn_enemy_at(EN_ESVAZIADO, Vector2Add(sv_run.player.pos, (Vector2){(float)(i % 6 - 3) * 25.0f, (float)(i / 6 - 2) * 25.0f}), false);
        kills_before = sv_run.kills;
        for (int frame = 0; frame < 60 * 20; frame++)
        {
            if (sv_run.phase == SV_LEVELUP) sv_game_choose(0);
            sv_game_update(1.0f / 60.0f, (Vector2){0, 0});
        }
        damaged = sv_run.kills > kills_before;
        check(group == 0 ? "SV-29" : group == 1 ? "SV-30" : "SV-64", "o grupo de poderes mata inimigos e não trava", damaged && isfinite(sv_run.player.hp));
    }
    sv_meta_reset();
}

// Quando o disco não deixa gravar, o jogo precisa saber para avisar o jogador (heurística de Nielsen 9)
static void test_save_feedback(void)
{
    printf("\nAviso de falha ao salvar\n");

    sv_meta_reset();
    check("SV-94", "gravar numa pasta que não existe falha e fica registrado", !sv_meta_save("pasta-que-nao-existe-xyz/progresso.survival") && !sv_meta_last_save_ok());
    check("SV-95", "gravar num lugar válido dá certo e limpa o registro de falha", sv_meta_save("build/tests/sv-aviso.survival") && sv_meta_last_save_ok());
    remove("build/tests/sv-aviso.survival");
}

static void test_ajustes_do_teste_com_jogadores(void)
{
    printf("\nAjustes do teste com jogadores\n");

    check("SV-96", "o começo da partida já tem inimigos de sobra (pelo menos 2 por segundo)", sv_spawn_rate(0.0f) >= 2.0f);
    check("SV-97", "acelerar o tempo só vale até os 7:00", SV_SPEEDUP_LIMIT == 420.0f && SV_SPEEDUP_LIMIT < sv_boss_spawn_time(BOSS_IARA));
    check("SV-98", "a partida começa na velocidade normal", sv_time_scale == 1);
    {
        float base_hp, base_cd;

        sv_meta_reset();
        unlock_everything();
        sv_meta.character = CH_ESTAGIARIO;
        sv_game_start(7);
        base_hp = sv_run.player.max_hp;
        base_cd = sv_run.player.cooldown_mult;
        sv_meta.character = CH_LUCAS;
        sv_game_start(7);
        check("SV-99", "Prof. Lucas (Engenharia de Software): abre aos 8:30, começa com o Prompt, recarrega mais rápido e perde 10 de vida",
              !sv_character_unlocked(CH_LUCAS, 99, 9, 509, 99999) && sv_character_unlocked(CH_LUCAS, 0, 0, 510, 0) &&
              sv_run.player.weapons[0].id == W_PROMPT && sv_run.player.max_hp == base_hp - 10.0f && sv_run.player.cooldown_mult < base_cd);
    }
}

// Ranking da feira: cada save é um jogador, que aparece uma vez só, com o recorde dele
static void test_ranking(void)
{
    char name[SV_NAME_BYTES];
    bool record = false;

    printf("\nRanking\n");
    sv_ranking_set_path("teste.ranking");
    remove("teste.ranking");
    sv_ranking_clear();

    check("SV-100", "ranking vazio comeca sem ninguem", sv_ranking_count() == 0 && sv_ranking_get(0) == NULL);

    {
        SvRankEntry e = {0};

        e.kills = 100;
        e.seconds = 60;
        e.bosses = 2;
        e.won = true;
        check("SV-143", "a pontuacao soma abates, tempo, chefes e o bonus de vitoria",
              sv_ranking_score(&e) == 100 * SV_SCORE_PER_KILL + 60 * SV_SCORE_PER_SECOND + 2 * SV_SCORE_PER_BOSS + SV_SCORE_WIN_BONUS);
        e.won = false;
        check("SV-144", "sem vitoria nao tem o bonus", sv_ranking_score(&e) == 100 * SV_SCORE_PER_KILL + 60 * SV_SCORE_PER_SECOND + 2 * SV_SCORE_PER_BOSS);
    }

    sv_ranking_add("Ana", 300, 120, 10, 1, 0, false, NULL);     // 3200 pontos
    sv_ranking_add("Bruno", 420, 40, 12, 1, 1, false, NULL);    // 3000: sobreviveu mais, mas quase não lutou
    {
        int pos = sv_ranking_add("Caio", 280, 200, 9, 1, 2, false, NULL);   // 3900

        check("SV-101", "quem derrotou mais inimigos passa na frente de quem so sobreviveu mais tempo",
              pos == 1 && strcmp(sv_ranking_get(0)->name, "Caio") == 0 && strcmp(sv_ranking_get(1)->name, "Ana") == 0 && strcmp(sv_ranking_get(2)->name, "Bruno") == 0);
    }
    check("SV-102", "desligar a IARA da o bonus de vitoria e leva ao topo", sv_ranking_add("Duda", 600, 10, 20, 5, 3, true, NULL) == 1);
    check("SV-145", "empate de pontos: quem derrotou mais inimigos fica na frente",
          sv_ranking_add("Gabi", 200, 170, 8, 1, 0, false, NULL) == 3 && strcmp(sv_ranking_get(3)->name, "Ana") == 0);   // 3200, igual a Ana
    check("SV-103", "empate total: quem chegou primeiro fica na frente", sv_ranking_add("Eva", 300, 120, 10, 1, 0, false, NULL) == 5 && strcmp(sv_ranking_get(3)->name, "Ana") == 0);
    sv_ranking_remove("Gabi");

    // Mesmo jogador: uma linha só, e só o recorde conta
    check("SV-112", "partida pior do mesmo jogador nao muda o ranking", sv_ranking_add("Bruno", 100, 10, 3, 0, 1, false, &record) == 5 && !record && sv_ranking_count() == 5 && sv_ranking_get(4)->seconds == 420);
    check("SV-113", "recorde novo substitui o antigo e sobe o jogador", sv_ranking_add("bruno", 700, 300, 25, 5, 1, true, &record) == 1 && record && sv_ranking_count() == 5 && sv_ranking_get(0)->seconds == 700);
    check("SV-114", "o nome e o mesmo sem diferenciar maiusculas", sv_ranking_find("BRUNO") == 0 && sv_ranking_find("ninguem") == -1);

    sv_ranking_load();
    check("SV-104", "o ranking volta igual depois de gravar e ler o arquivo", sv_ranking_count() == 5 && sv_ranking_get(0)->won && sv_ranking_get(0)->seconds == 700 && strcmp(sv_ranking_get(1)->name, "Duda") == 0);
    check("SV-105", "a ordem de chegada continua depois de ler o arquivo", sv_ranking_add("Fabi", 300, 120, 10, 1, 0, false, NULL) == 6);

    check("SV-115", "apagar o jogador tira ele do ranking e do arquivo", sv_ranking_remove("Caio") && sv_ranking_find("Caio") == -1 && sv_ranking_load() && sv_ranking_count() == 5 && sv_ranking_find("Caio") == -1);

    sv_ranking_clean_name("  João   da  Silva  ", name, sizeof(name));
    check("SV-106", "nome: tira espacos das pontas e repetidos, mantem acento", strcmp(name, "João da Silva") == 0);
    sv_ranking_clean_name("a|b\nc", name, sizeof(name));
    check("SV-107", "nome: barra e quebra de linha nao entram no arquivo", strchr(name, '|') == NULL && strchr(name, '\n') == NULL);
    sv_ranking_clean_name("ABCDEFGHIJKLMNOPQRST", name, sizeof(name));
    check("SV-108", "nome: corta no limite de letras", (int)strlen(name) == SV_NAME_MAX_CHARS);
    sv_ranking_clean_name("   ", name, sizeof(name));
    check("SV-109", "nome vazio vira Anonimo", strcmp(name, "Anônimo") == 0);

    sv_ranking_clear();
    for (int i = 0; i < SV_RANKING_MAX; i++)
    {
        char n[16];

        snprintf(n, sizeof(n), "J%d", i);
        sv_ranking_add(n, 100 + i, 0, 1, 0, 0, false, NULL);
    }
    check("SV-110", "com o ranking cheio, um jogador novo pior fica de fora", sv_ranking_add("Pior", 1, 0, 1, 0, 0, false, NULL) == 0 && sv_ranking_count() == SV_RANKING_MAX);
    check("SV-111", "com o ranking cheio, um jogador novo melhor entra e o pior sai", sv_ranking_add("Melhor", 9999, 0, 1, 0, 0, false, NULL) == 1 && sv_ranking_count() == SV_RANKING_MAX && sv_ranking_get(SV_RANKING_MAX - 1)->seconds == 101);

    remove("teste.ranking");
    sv_ranking_clear();
    sv_ranking_set_path(NULL);
}

// -----------------------------------------------------------------------------
// Modo dupla (sv_duo.c)
// -----------------------------------------------------------------------------

// Começa uma partida em dupla, sem inimigos nascendo, para testar uma regra de cada vez
static void start_duo(unsigned seed)
{
    sv_meta_reset();
    sv_meta.character2 = CH_ESTAGIARIO;
    sv_duo_mode = true;
    sv_game_start(seed);
    sv_duo_mode = false;
    sv_cheat_toggle(CHEAT_NO_SPAWN);
}

static void clear_enemies(void)
{
    for (int i = 0; i < SV_MAX_ENEMIES; i++) sv_run.enemies[i].alive = false;
    sv_run.enemy_count = 0;
}

static void step_duo(int frames, Vector2 m1, Vector2 m2)
{
    for (int i = 0; i < frames && sv_run.phase == SV_PLAYING; i++) sv_game_update_duo(1.0f / 60.0f, m1, m2);
}

// Partida inteira com dois bots. O jogador 2 usa o mesmo bot, com ele em cena.
static RunResult simulate_duo(unsigned seed, float max_seconds)
{
    RunResult result = {0};
    float dt = 1.0f / 60.0f;

    sv_meta_reset();
    sv_duo_mode = true;
    sv_game_start(seed);
    sv_duo_mode = false;

    while (sv_run.time < max_seconds && sv_run.phase != SV_DEAD && sv_run.phase != SV_WON)
    {
        Vector2 m1, m2;

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

        m1 = bot_move();
        sv_duo_use(1);
        m2 = bot_move();
        sv_duo_use(0);
        sv_game_update_duo(dt, m1, m2);

        if (sv_run.enemy_count > result.peak_enemies) result.peak_enemies = sv_run.enemy_count;
        for (int k = 0; k < SV_DUO_PLAYERS; k++)
        {
            SvPlayer *p = sv_duo_player(k);

            assert(isfinite(p->pos.x) && isfinite(p->pos.y) && isfinite(p->hp));
            assert(p->pos.x >= 0 && p->pos.x <= SV_WORLD_W && p->pos.y >= 0 && p->pos.y <= SV_WORLD_H);
        }
        assert(sv_run.enemy_count >= 0 && sv_run.enemy_count <= SV_MAX_ENEMIES);
        assert(sv_run.phase == SV_LEVELUP || sv_run.active == 0);
    }

    result.seconds = sv_run.time;
    result.level = sv_duo_player(0)->level;
    result.kills = sv_run.kills;
    result.bosses = sv_run.bosses_defeated;
    result.phase = sv_run.phase;
    return result;
}

static void test_duo(void)
{
    printf("\nModo dupla\n");

    // Solo continua sem nada da dupla
    sv_meta_reset();
    sv_game_start(11);
    check("SV-116", "a partida solo não liga a dupla e tem um jogador só", !sv_run.duo && sv_run.active == 0 && sv_duo_count() == 1 && sv_duo_player(0) == &sv_run.player && sv_run.seats[1].player.max_hp == 0.0f);

    sv_meta_reset();
    sv_meta.character = CH_ESTAGIARIO;
    sv_meta.character2 = CH_LAURA;
    sv_meta.runs = 99; sv_meta.wins = 9; sv_meta.best_seconds = 900; sv_meta.kills = 99999;   // Tudo liberado
    sv_duo_mode = true;
    sv_game_start(12);
    sv_duo_mode = false;
    check("SV-117", "a dupla começa com dois jogadores lado a lado, cada um com o seu personagem",
          sv_run.duo && sv_duo_count() == 2 && sv_run.active == 0 && sv_duo_player(0)->character == CH_ESTAGIARIO && sv_duo_player(1)->character == CH_LAURA &&
          fabsf(Vector2Distance(sv_duo_player(0)->pos, sv_duo_player(1)->pos) - SV_DUO_START_GAP) < 0.01f && sv_duo_player(1)->hp > 0.0f && sv_duo_player(1)->weapon_count == 1);

    // Cada um anda com o seu controle
    {
        Vector2 a = sv_duo_player(0)->pos, b = sv_duo_player(1)->pos;

        sv_cheat_toggle(CHEAT_NO_SPAWN);
        clear_enemies();
        step_duo(30, (Vector2){-1, 0}, (Vector2){1, 0});
        check("SV-118", "o jogador 1 e o jogador 2 andam cada um com o seu controle", sv_duo_player(0)->pos.x < a.x - 5.0f && sv_duo_player(1)->pos.x > b.x + 5.0f);
    }

    // Os dois na mesma tela
    start_duo(13);
    clear_enemies();
    sv_duo_player(0)->pos = (Vector2){1200.0f, 1300.0f};
    sv_duo_player(1)->pos = (Vector2){1220.0f, 1300.0f};
    for (int i = 0; i < 60 * 20; i++)
    {
        clear_enemies();
        step_duo(1, (Vector2){-1, 0}, (Vector2){1, 0});
    }
    check("SV-119", "um não consegue se afastar demais do outro", fabsf(sv_duo_player(1)->pos.x - sv_duo_player(0)->pos.x) <= SV_DUO_LEASH_X + 1.0f);

    // XP dividida e escolha de um de cada vez
    start_duo(14);
    clear_enemies();
    sv_add_gem(sv_duo_player(0)->pos, 0, 1.0f);
    step_duo(1, (Vector2){0, 0}, (Vector2){0, 0});
    check("SV-120", "um chip de XP pego por um sobe a barra dos dois", sv_duo_player(0)->xp > 0.0f && fabsf(sv_duo_player(0)->xp - sv_duo_player(1)->xp) < 0.001f);

    sv_duo_player(0)->xp = (float)sv_duo_player(0)->xp_needed;
    sv_duo_player(1)->xp = (float)sv_duo_player(1)->xp_needed;
    step_duo(1, (Vector2){0, 0}, (Vector2){0, 0});
    {
        bool first_is_p1 = sv_run.phase == SV_LEVELUP && sv_run.chooser == 0 && sv_run.active == 0;
        int p1_before = sv_duo_player(0)->weapon_count + sv_duo_player(0)->passive_count;
        int p2_before = sv_duo_player(1)->weapon_count + sv_duo_player(1)->passive_count;
        bool second_is_p2, p1_got, p2_got;

        sv_run.choices[0] = (SvChoice){2, P_CAFE};
        sv_game_choose(0);
        second_is_p2 = sv_run.phase == SV_LEVELUP && sv_run.chooser == 1 && sv_run.active == 1;
        p1_got = sv_duo_player(0)->weapon_count + sv_duo_player(0)->passive_count == p1_before + 1;
        sv_run.choices[0] = (SvChoice){2, P_FOCO};
        sv_game_choose(0);
        p2_got = sv_duo_player(1)->weapon_count + sv_duo_player(1)->passive_count == p2_before + 1;
        check("SV-121", "os dois sobem de nível juntos e escolhem um de cada vez, primeiro o jogador 1",
              first_is_p1 && second_is_p2 && p1_got && p2_got && sv_run.phase == SV_PLAYING && sv_duo_player(0)->level == 2 && sv_duo_player(1)->level == 2 &&
              sv_duo_player(0)->passives[0].id == P_CAFE && sv_duo_player(1)->passives[0].id == P_FOCO);
    }
    step_duo(1, (Vector2){0, 0}, (Vector2){0, 0});
    check("SV-122", "depois da escolha, o jogador 1 volta a ficar em cena", sv_run.active == 0);

    // Cair e levantar
    start_duo(15);
    clear_enemies();
    sv_duo_player(1)->revive_ready = false;
    sv_duo_player(1)->hp = 0.0f;
    step_duo(1, (Vector2){0, 0}, (Vector2){0, 0});
    check("SV-123", "quem fica sem vida cai, e a partida continua com o outro", sv_duo_player(1)->down && !sv_duo_player(0)->down && sv_run.phase == SV_PLAYING);

    {
        Vector2 where = sv_duo_player(1)->pos;

        step_duo(30, (Vector2){0, 0}, (Vector2){1, 0});
        check("SV-124", "quem caiu fica parado no chão", Vector2Distance(where, sv_duo_player(1)->pos) < 0.01f);
    }

    // O parceiro chega perto e fica ao lado
    sv_duo_player(0)->pos = Vector2Add(sv_duo_player(1)->pos, (Vector2){20.0f, 0.0f});
    step_duo((int)(60 * SV_DUO_REVIVE_TIME * 0.5f), (Vector2){0, 0}, (Vector2){0, 0});
    {
        float half = sv_duo_revive_progress(1);

        step_duo((int)(60 * SV_DUO_REVIVE_TIME * 0.5f) + 5, (Vector2){0, 0}, (Vector2){0, 0});
        check("SV-125", "o parceiro levanta quem caiu ficando ao lado, e ele volta com metade da vida",
              half > 0.4f && half < 0.6f && !sv_duo_player(1)->down && fabsf(sv_duo_player(1)->hp - sv_duo_player(1)->max_hp * SV_DUO_REVIVE_HP) < 1.0f);
    }

    // Ctrl+Z vale para cada um antes de cair
    start_duo(16);
    clear_enemies();
    sv_duo_player(1)->revive_ready = true;
    sv_duo_player(1)->hp = 0.0f;
    step_duo(1, (Vector2){0, 0}, (Vector2){0, 0});
    check("SV-126", "com Ctrl+Z, o jogador volta na hora em vez de cair", !sv_duo_player(1)->down && sv_duo_player(1)->hp > 0.0f && !sv_duo_player(1)->revive_ready);

    // Os dois caem: fim
    start_duo(17);
    clear_enemies();
    for (int k = 0; k < SV_DUO_PLAYERS; k++)
    {
        sv_duo_player(k)->revive_ready = false;
        sv_duo_player(k)->hp = 0.0f;
    }
    step_duo(1, (Vector2){0, 0}, (Vector2){0, 0});
    check("SV-127", "a partida acaba quando os dois caem", sv_run.phase == SV_DEAD && sv_run.active == 0);

    // Os inimigos vão atrás de quem está de pé
    start_duo(18);
    clear_enemies();
    sv_duo_player(0)->pos = (Vector2){1500.0f, 1300.0f};
    sv_duo_player(1)->pos = (Vector2){1900.0f, 1300.0f};
    sv_spawn_enemy_at(EN_CHAMADO, (Vector2){1700.0f, 1250.0f}, false);
    {
        float before = Vector2Distance(sv_run.enemies[0].pos, sv_duo_player(1)->pos);

        sv_duo_player(0)->revive_ready = false;
        sv_duo_player(0)->hp = 0.0f;
        sv_duo_player(0)->weapon_count = 0;
        sv_duo_player(1)->weapon_count = 0;
        step_duo(60, (Vector2){0, 0}, (Vector2){0, 0});
        check("SV-128", "o inimigo ignora quem caiu e vai atrás de quem está de pé", sv_duo_player(0)->down && Vector2Distance(sv_run.enemies[0].pos, sv_duo_player(1)->pos) < before - 20.0f);
    }

    // Os tiros de cada um usam os bônus de quem atirou
    start_duo(19);
    clear_enemies();
    sv_duo_player(0)->weapon_count = 0;
    sv_duo_player(1)->weapons[0] = (SvWeapon){W_GRAMPEADOR, 1, 0.0f, 0.0f};
    sv_duo_player(1)->weapon_count = 1;
    sv_duo_player(1)->dmg_mult = 50.0f;
    sv_duo_player(1)->crit_chance = 0.0f;
    sv_spawn_enemy_at(EN_ESVAZIADO, Vector2Add(sv_duo_player(1)->pos, (Vector2){60.0f, 0.0f}), false);
    sv_run.enemies[0].max_hp = sv_run.enemies[0].hp = 400.0f;
    step_duo(40, (Vector2){0, 0}, (Vector2){0, 0});
    check("SV-129", "o tiro do jogador 2 usa o dano do jogador 2", !sv_run.enemies[0].alive && sv_run.kills == 1);

    // Os perigos dos chefes acertam os dois
    start_duo(20);
    clear_enemies();
    sv_duo_player(0)->weapon_count = 0;
    sv_duo_player(1)->weapon_count = 0;
    {
        SvHazard *h = &sv_run.hazards[0];
        float hp1 = sv_duo_player(0)->hp, hp2 = sv_duo_player(1)->hp;

        memset(h, 0, sizeof(*h));
        h->alive = true;
        h->kind = HZ_CIRCLE;
        h->pos = Vector2Scale(Vector2Add(sv_duo_player(0)->pos, sv_duo_player(1)->pos), 0.5f);
        h->radius = 60.0f;
        h->life = h->max_life = 1.0f;
        h->damage = 10.0f;
        step_duo(2, (Vector2){0, 0}, (Vector2){0, 0});
        check("SV-130", "a área de perigo de um chefe acerta os dois jogadores que estão dentro", sv_duo_player(0)->hp < hp1 && sv_duo_player(1)->hp < hp2);
    }

    // Dificuldade
    start_duo(21);
    clear_enemies();
    sv_spawn_enemy_at(EN_CHAMADO, (Vector2){100.0f, 100.0f}, false);
    {
        float duo_hp = sv_run.enemies[0].max_hp;
        float duo_boss;

        sv_run.time = sv_boss_spawn_time(0);
        sv_run.next_boss = 0;
        step_duo(1, (Vector2){0, 0}, (Vector2){0, 0});
        duo_boss = sv_run.boss.max_hp;

        sv_meta_reset();
        sv_game_start(21);
        sv_cheat_toggle(CHEAT_NO_SPAWN);
        clear_enemies();
        sv_spawn_enemy_at(EN_CHAMADO, (Vector2){100.0f, 100.0f}, false);
        check("SV-131", "na dupla os inimigos têm mais vida", fabsf(duo_hp - sv_run.enemies[0].max_hp * SV_DUO_ENEMY_HP_MULT) < 0.01f);
        sv_run.time = sv_boss_spawn_time(0);
        sv_run.next_boss = 0;
        sv_game_update(1.0f / 60.0f, (Vector2){0, 0});
        check("SV-132", "na dupla os chefes têm mais vida", sv_run.boss.active && fabsf(duo_boss - sv_run.boss.max_hp * SV_DUO_BOSS_HP_MULT) < 0.5f);
    }

    // Partida inteira com dois bots
    {
        RunResult r = simulate_duo(77, 660.0f);

        printf("      dupla: %.0f s, nível %d, %d abates, %d chefes, pico de %d inimigos\n", r.seconds, r.level, r.kills, r.bosses, r.peak_enemies);
        check("SV-133", "uma partida inteira em dupla roda sem travar e sem ninguém sair do mapa", r.seconds > 60.0f && r.kills > 0);
    }

    // Save e ranking da dupla
    sv_meta_reset();
    sv_meta.character = CH_LAURA;
    sv_meta.character2 = CH_JULIO;
    sv_meta_save(TEST_META_PATH);
    sv_meta_reset();
    check("SV-134", "o save da dupla guarda o personagem de cada um", sv_meta_load(TEST_META_PATH) && sv_meta.character == CH_LAURA && sv_meta.character2 == CH_JULIO);
    {
        FILE *file;
        char content[512] = {0};

        sv_meta_reset();
        sv_meta.character = CH_LAURA;
        sv_meta_save(TEST_META_PATH);
        file = fopen(TEST_META_PATH, "r");
        if (file) { size_t got = fread(content, 1, sizeof(content) - 1, file); (void)got; fclose(file); }
        check("SV-135", "o save solo continua sem a linha do jogador 2", strstr(content, "character=") != NULL && strstr(content, "character2") == NULL);
    }
    remove(TEST_META_PATH);
    sv_meta_reset();

    sv_ranking_set_path("teste.ranking");
    remove("teste.ranking");
    sv_ranking_clear();
    sv_ranking_add("Solo", 100, 10, 2, 0, 0, false, NULL);
    sv_ranking_set_path("teste_duplas.ranking");
    remove("teste_duplas.ranking");
    sv_ranking_load();
    sv_ranking_add("Ana e Beto", 300, 50, 5, 1, 0, false, NULL);
    sv_ranking_set_path("teste.ranking");
    sv_ranking_load();
    {
        bool solo_ok = sv_ranking_count() == 1 && sv_ranking_find("Ana e Beto") == -1;

        sv_ranking_set_path("teste_duplas.ranking");
        sv_ranking_load();
        check("SV-136", "o ranking das duplas é separado do solo", solo_ok && sv_ranking_count() == 1 && sv_ranking_find("Solo") == -1 && sv_ranking_find("Ana e Beto") == 0);
    }
    remove("teste.ranking");
    remove("teste_duplas.ranking");
    sv_ranking_set_path(NULL);
    sv_ranking_clear();
}

// Correções do bug tracker da Sprint 03: BUG-33 (personagem do cheat) e BUG-37 (cheats na dupla)
static void test_correcoes_do_bug_tracker(void)
{
    int locked = -1;

    printf("\nCorrecoes do bug tracker (BUG-33 e BUG-37)\n");

    // BUG-33: um valor só decide o personagem da partida
    sv_meta_reset();
    sv_debug_mode = false;
    for (int c = 0; c < CH_COUNT && locked < 0; c++)
    {
        if (!sv_character_unlocked(c, sv_meta.runs, sv_meta.wins, sv_meta.best_seconds, sv_meta.kills)) locked = c;
    }
    check("SV-137", "sem o debug, um personagem bloqueado (ou inválido) vale como Estagiário; com o debug, vale ele mesmo",
          locked >= 0 && sv_playable_character(locked) == CH_ESTAGIARIO && sv_playable_character(-1) == CH_ESTAGIARIO &&
          sv_playable_character(CH_COUNT) == CH_ESTAGIARIO && sv_playable_character(CH_ESTAGIARIO) == CH_ESTAGIARIO &&
          (sv_debug_mode = true, sv_playable_character(locked) == locked));

    sv_meta.character = locked;
    sv_game_start(501);
    {
        bool with_cheat = sv_run.player.character == locked;

        sv_debug_mode = false;   // Desligou o cheat no meio da partida
        sv_game_start(502);
        check("SV-138", "desligado o debug, a próxima partida usa o Estagiário, o mesmo valor que o save passa a guardar",
              with_cheat && sv_run.player.character == CH_ESTAGIARIO && sv_playable_character(sv_meta.character) == CH_ESTAGIARIO);
    }

    // BUG-37: na dupla, os cheats de jogador valem para os dois
    sv_meta_reset();
    sv_duo_mode = true;
    sv_game_start(503);
    sv_duo_mode = false;
    sv_duo_player(0)->hp = 10.0f;
    sv_duo_player(1)->hp = 10.0f;
    sv_duo_fall(1);
    sv_cheat_heal();
    check("SV-139", "na dupla, curar enche a vida dos dois e levanta quem caiu",
          sv_duo_player(0)->hp == sv_duo_player(0)->max_hp && sv_duo_player(1)->hp == sv_duo_player(1)->max_hp && !sv_duo_player(1)->down);

    sv_cheat_level_up();
    check("SV-140", "na dupla, subir de nível enche a barra dos dois",
          sv_duo_player(0)->xp >= (float)sv_duo_player(0)->xp_needed && sv_duo_player(1)->xp >= (float)sv_duo_player(1)->xp_needed);

    sv_duo_player(0)->revive_ready = false;
    sv_duo_player(1)->revive_ready = false;
    sv_cheat_revive();
    check("SV-141", "na dupla, o Ctrl+Z de volta fica pronto para os dois", sv_duo_player(0)->revive_ready && sv_duo_player(1)->revive_ready);

    {
        int before_1 = sv_duo_player(0)->weapon_count, before_2 = sv_duo_player(1)->weapon_count;
        int before_p1 = sv_duo_player(0)->passive_count, before_p2 = sv_duo_player(1)->passive_count;
        int weapon = -1, passive = -1;

        for (int w = 0; w < W_COUNT && weapon < 0; w++)
        {
            bool owned = false;

            for (int k = 0; k < SV_DUO_PLAYERS; k++)
            {
                for (int i = 0; i < sv_duo_player(k)->weapon_count; i++) if (sv_duo_player(k)->weapons[i].id == w) owned = true;
            }
            if (!owned) weapon = w;
        }
        for (int q = 0; q < P_COUNT && passive < 0; q++)
        {
            bool owned = false;

            for (int k = 0; k < SV_DUO_PLAYERS; k++)
            {
                for (int i = 0; i < sv_duo_player(k)->passive_count; i++) if (sv_duo_player(k)->passives[i].id == q) owned = true;
            }
            if (!owned) passive = q;
        }
        check("SV-142", "na dupla, dar um poder ou um passivo entrega aos dois, e o jogador em cena continua o mesmo",
              weapon >= 0 && passive >= 0 && sv_cheat_set_weapon(weapon, 2) && sv_cheat_set_passive(passive, 1) &&
              sv_duo_player(0)->weapon_count == before_1 + 1 && sv_duo_player(1)->weapon_count == before_2 + 1 &&
              sv_duo_player(0)->passive_count == before_p1 + 1 && sv_duo_player(1)->passive_count == before_p2 + 1 &&
              sv_run.active == 0);
    }
    sv_debug_mode = false;
}

int main(void)
{
    printf("Testes do Modo Sobrevivencia\n");

    test_rules();
    test_meta();
    test_simulation();
    test_map();
    test_choices();
    test_level_up();
    test_characters();
    test_boss_balance();
    test_debug_cheats();
    test_save_feedback();
    test_ajustes_do_teste_com_jogadores();
    test_ranking();
    test_duo();
    test_correcoes_do_bug_tracker();

    printf("\n%d verificacoes, todas passaram.\n", checks);
    return 0;
}
