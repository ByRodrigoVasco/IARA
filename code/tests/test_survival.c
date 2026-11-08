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
        check("SV-41", "os 8 personagens têm pasta, nome e poder inicial próprios", CH_COUNT == 8 && distinct && weapons_ok);
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
        check("SV-45", "Anderson: +40 de vida, +1 de armadura, mais lento, poder inicial próprio",
              sv_run.player.max_hp == base_hp + 40.0f && sv_run.player.armor == 1.0f && sv_run.player.speed_mult < base_speed && sv_run.player.weapons[0].id == sv_character(CH_ANDERSON)->start_weapon);

        sv_meta.character = CH_JULIO;
        sv_game_start(7);
        check("SV-46", "Julio: mais rápido e com menos vida", sv_run.player.speed_mult > base_speed * 1.17f && sv_run.player.max_hp == base_hp - 10.0f);

        sv_meta.character = CH_VASCO;
        sv_game_start(7);
        check("SV-47", "Vasco: coleta maior e mais XP", sv_run.player.magnet_range > base_magnet * 1.7f && sv_run.player.xp_mult > 1.19f);

        sv_meta.character = CH_NICOLLAS;
        sv_game_start(7);
        check("SV-48", "Nicollas: recarga menor e poderes mais longos", sv_run.player.cooldown_mult < 0.9f && sv_run.player.duration_mult > 1.09f);

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

    printf("\n%d verificacoes, todas passaram.\n", checks);
    return 0;
}
