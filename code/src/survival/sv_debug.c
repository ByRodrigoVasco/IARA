#include <math.h>
#include <stdio.h>
#include <string.h>

#include "raylib.h"
#include "raymath.h"
#include "sv_debug.h"
#include "sv_game.h"
#include "sv_render.h"
#include "sv_map.h"
#include "sv_rules.h"

// -----------------------------------------------------------------------------
// Estado
// -----------------------------------------------------------------------------

typedef enum { PAGE_MAIN, PAGE_WEAPONS, PAGE_PASSIVES, PAGE_BOSSES, PAGE_ENEMIES, PAGE_PLAYER, PAGE_MATCH, PAGE_COUNT } DebugPage;

#define MAIN_ITEMS 7
#define MAX_ROWS 48
#define FRAME_HISTORY 120

static bool menu_open = false;
static DebugPage page = PAGE_MAIN;
static int cursor = 0;
static int first_row = 0;
static int target_level = 1;                 // Nível dos poderes e passivos que o menu dá
static int amount_index = 1;                 // Quantos inimigos o menu chama
static const int amounts[5] = {1, 5, 10, 25, 50};
static char message[112];
static double message_until = 0.0;

static float frame_ms[FRAME_HISTORY];
static int frame_head = 0;

static const char *main_labels[MAIN_ITEMS] = {"PODERES", "PASSIVOS", "CHEFES", "INIMIGOS", "JOGADOR", "PARTIDA", "FECHAR"};

static void show_message(const char *text)
{
    snprintf(message, sizeof(message), "%s", text);
    message_until = GetTime() + 3.0;
}

void sv_debug_toggle(void)
{
    sv_debug_mode = !sv_debug_mode;
    if (!sv_debug_mode)
    {
        menu_open = false;
        sv_cheat_clear_toggles();
    }
    else
    {
        show_message("Debug ligado: F2 abre os cheats.");
    }
}

bool sv_debug_menu_open(void)
{
    return menu_open && sv_debug_mode;
}

// -----------------------------------------------------------------------------
// Desenhos do mundo
// -----------------------------------------------------------------------------

static void draw_oriented_rect(Vector2 origin, float angle_degrees, float length, float half_width, Color color, float thickness)
{
    float angle = angle_degrees * DEG2RAD;
    Vector2 dir = {cosf(angle), sinf(angle)};
    Vector2 perp = {-dir.y, dir.x};
    Vector2 a = Vector2Add(origin, Vector2Scale(perp, half_width));
    Vector2 b = Vector2Subtract(origin, Vector2Scale(perp, half_width));
    Vector2 c = Vector2Add(b, Vector2Scale(dir, length));
    Vector2 d = Vector2Add(a, Vector2Scale(dir, length));

    DrawLineEx(a, b, thickness, color);
    DrawLineEx(b, c, thickness, color);
    DrawLineEx(c, d, thickness, color);
    DrawLineEx(d, a, thickness, color);
}

static Color zone_color(int kind)
{
    switch (kind)
    {
    case ZK_CANECA: return (Color){200, 140, 80, 255};
    case ZK_DEPLOY: return (Color){255, 230, 90, 255};
    case ZK_PROMPT: return (Color){56, 240, 228, 255};
    case ZK_VERA: return (Color){120, 190, 255, 255};
    default: return (Color){90, 220, 255, 255};
    }
}

void sv_debug_draw_world(void)
{
    SvPlayer *p = &sv_run.player;
    float zoom = fmaxf(GetScreenHeight() / 720.0f, 0.75f);
    float half_w = GetScreenWidth() / 2.0f / zoom + 160.0f;
    float half_h = GetScreenHeight() / 2.0f / zoom + 160.0f;
    float line = 1.6f / zoom;
    float text = 11.0f;
    char label[48];

    // Obstáculos do mapa: a caixa de colisão (vermelha) e o ponto da base, que ordena o desenho
    for (int i = 0; i < sv_map_prop_count(); i++)
    {
        const SvMapProp *prop = sv_map_prop(i);
        Rectangle box = {prop->rx, prop->ry, prop->rw, prop->rh};

        if (fabsf(prop->x - p->pos.x) > half_w + box.width || fabsf(prop->y - p->pos.y) > half_h + box.height) continue;
        DrawRectangleRec(box, (Color){255, 70, 70, 36});
        DrawRectangleLinesEx(box, line, (Color){255, 70, 70, 230});
        DrawCircleV((Vector2){prop->x, prop->y}, 2.0f / zoom, (Color){255, 200, 60, 255});
    }

    // Onde os inimigos nascem
    DrawCircleLinesV(p->pos, 800.0f, (Color){255, 255, 255, 40});

    // Itens: cruz pequena; os que já estão sendo puxados ganham uma linha até o jogador
    for (int i = 0; i < SV_MAX_GEMS; i++)
    {
        SvGem *g = &sv_run.gems[i];

        if (!g->alive || fabsf(g->pos.x - p->pos.x) > half_w || fabsf(g->pos.y - p->pos.y) > half_h) continue;
        DrawLineEx((Vector2){g->pos.x - 3.0f, g->pos.y}, (Vector2){g->pos.x + 3.0f, g->pos.y}, line, (Color){120, 255, 200, 200});
        DrawLineEx((Vector2){g->pos.x, g->pos.y - 3.0f}, (Vector2){g->pos.x, g->pos.y + 3.0f}, line, (Color){120, 255, 200, 200});
        if (g->pulled) DrawLineEx(g->pos, p->pos, line, (Color){120, 255, 200, 60});
    }

    // Zonas dos poderes e do chefe
    for (int i = 0; i < SV_MAX_ZONES; i++)
    {
        SvZone *z = &sv_run.zones[i];
        Color color;

        if (!z->alive) continue;
        color = zone_color(z->kind);
        if (z->shape == ZONE_CIRCLE) DrawCircleLinesV(z->pos, z->radius, color);
        else draw_oriented_rect(z->pos, z->angle, z->length, z->radius, color, line);
        if (z->delay > 0.0f)
        {
            snprintf(label, sizeof(label), "%.1fs", z->delay);
            sv_text(label, z->pos.x - 10.0f, z->pos.y - 6.0f, text, color);
        }
    }

    // Perigos dos chefes: círculos, feixes e projéteis com a direção
    for (int i = 0; i < SV_MAX_HAZARDS; i++)
    {
        SvHazard *h = &sv_run.hazards[i];
        Color color = h->warn > 0.0f ? (Color){255, 220, 80, 255} : (Color){255, 60, 90, 255};

        if (!h->alive) continue;
        if (h->kind == HZ_CIRCLE) DrawCircleLinesV(h->pos, h->radius, color);
        else if (h->kind == HZ_BEAM) draw_oriented_rect(h->pos, h->angle, h->length, h->radius, color, line);
        else
        {
            DrawCircleLinesV(h->pos, h->radius, color);
            DrawLineEx(h->pos, Vector2Add(h->pos, Vector2Scale(h->vel, 0.6f)), line, color);
        }
        if (h->warn > 0.0f)
        {
            snprintf(label, sizeof(label), "aviso %.1fs", h->warn);
            sv_text(label, h->pos.x - 20.0f, h->pos.y - h->radius - 14.0f, text, color);
        }
    }

    // Tiros: o círculo de acerto e o caminho que ainda vão percorrer
    for (int i = 0; i < SV_MAX_SHOTS; i++)
    {
        SvShot *s = &sv_run.shots[i];
        Vector2 end;

        if (!s->alive) continue;
        end = Vector2Add(s->pos, Vector2Scale(s->vel, s->life));
        DrawLineEx(s->pos, end, line, (Color){255, 230, 120, 140});
        DrawCircleV(end, 2.0f / zoom, (Color){255, 230, 120, 200});
        DrawCircleLinesV(s->pos, s->radius, (Color){255, 230, 120, 255});
    }

    // Inimigos: o círculo de colisão, a barra de vida e a cor do tipo
    for (int i = 0; i < SV_MAX_ENEMIES; i++)
    {
        SvEnemy *e = &sv_run.enemies[i];
        Color color = {110, 255, 130, 230};
        float radius;

        if (!e->alive || fabsf(e->pos.x - p->pos.x) > half_w || fabsf(e->pos.y - p->pos.y) > half_h) continue;
        radius = sv_enemy_radius(e->type);
        if (e->fake) color = (Color){255, 230, 80, 230};
        else if (e->wall) color = (Color){255, 160, 60, 230};
        else if (e->type == EN_AUDITOR || e->type == EN_ECO) color = (Color){230, 100, 255, 230};
        DrawCircleLinesV(e->pos, radius, color);
        if (e->hp < e->max_hp)
        {
            float fraction = e->hp / e->max_hp;

            DrawRectangle((int)(e->pos.x - radius), (int)(e->pos.y - radius - 6.0f), (int)(radius * 2.0f), 2, (Color){60, 0, 0, 220});
            DrawRectangle((int)(e->pos.x - radius), (int)(e->pos.y - radius - 6.0f), (int)(radius * 2.0f * fraction), 2, (Color){255, 80, 80, 255});
        }
    }

    // Chefe
    if (sv_run.boss.active)
    {
        SvBoss *b = &sv_run.boss;

        DrawCircleLinesV(b->pos, b->radius, (Color){255, 80, 255, 255});
        DrawLineEx(b->pos, p->pos, line, (Color){255, 80, 255, 70});
        snprintf(label, sizeof(label), "%.0f / %.0f", b->hp, b->max_hp);
        sv_text(label, b->pos.x - 30.0f, b->pos.y - b->radius - 16.0f, text + 2.0f, (Color){255, 140, 255, 255});
    }

    // Colegas de plantão
    for (int c = 0; c < sv_colega_count(); c++)
    {
        DrawCircleLinesV(sv_run.colega_pos[c], 6.0f, (Color){120, 200, 255, 255});
        DrawLineEx(sv_run.colega_pos[c], p->pos, line, (Color){120, 200, 255, 50});
    }

    // Jogador: acerto, coleta, colisão com o mapa (nos pés) e para onde olha
    DrawCircleLinesV(p->pos, p->magnet_range, (Color){110, 160, 255, 90});
    DrawCircleLinesV((Vector2){p->pos.x, p->pos.y + 10.0f}, 7.0f, (Color){255, 70, 70, 255});
    DrawCircleLinesV(p->pos, 8.0f, p->invulnerable > 0.0f ? (Color){255, 255, 120, 255} : WHITE);
    DrawLineEx(p->pos, Vector2Add(p->pos, Vector2Scale(p->facing_vector, 24.0f)), line * 1.5f, WHITE);
}

// -----------------------------------------------------------------------------
// Painel
// -----------------------------------------------------------------------------

void sv_debug_draw_hud(void)
{
    SvPlayer *p = &sv_run.player;
    float ui = sv_ui();
    float size = 17.0f * ui;
    float step = size * 1.22f;
    float x = 18.0f * ui;
    float y = 150.0f * ui;
    float width = 560.0f * ui;
    char lines[28][140];
    int count = 0;
    float current = GetFrameTime() * 1000.0f;
    float sum = 0.0f, worst = 0.0f;
    char cheats[96] = "";
    int enemies = sv_run.enemy_count, shots = 0, zones = 0, hazards = 0, gems = 0;

    frame_ms[frame_head] = current;
    frame_head = (frame_head + 1) % FRAME_HISTORY;
    for (int i = 0; i < FRAME_HISTORY; i++)
    {
        sum += frame_ms[i];
        if (frame_ms[i] > worst) worst = frame_ms[i];
    }

    for (int i = 0; i < SV_MAX_SHOTS; i++) if (sv_run.shots[i].alive) shots++;
    for (int i = 0; i < SV_MAX_ZONES; i++) if (sv_run.zones[i].alive) zones++;
    for (int i = 0; i < SV_MAX_HAZARDS; i++) if (sv_run.hazards[i].alive) hazards++;
    for (int i = 0; i < SV_MAX_GEMS; i++) if (sv_run.gems[i].alive) gems++;

    snprintf(lines[count++], sizeof(lines[0]), "DEBUG   F1: sair   F2: cheats");
    snprintf(lines[count++], sizeof(lines[0]), "FPS %d   quadro %.1f ms   media %.1f   pior %.1f", GetFPS(), current, sum / FRAME_HISTORY, worst);
    snprintf(lines[count++], sizeof(lines[0]), "tempo %d:%02d   ato %d   nascem %.1f/s   vida dos inimigos x%.2f", (int)sv_run.time / 60, (int)sv_run.time % 60, sv_run.act + 1, sv_spawn_rate(sv_run.time), sv_enemy_hp_scale(sv_run.time));
    snprintf(lines[count++], sizeof(lines[0]), "inimigos %d/%d   tiros %d   zonas %d   perigos %d   itens %d", enemies, SV_MAX_ENEMIES, shots, zones, hazards, gems);
    snprintf(lines[count++], sizeof(lines[0]), "jogador (%.0f, %.0f)   area %d,%d   vida %.0f/%.0f", p->pos.x, p->pos.y, (int)(p->pos.x / SV_CELL_W), (int)(p->pos.y / SV_CELL_H), p->hp, p->max_hp);
    snprintf(lines[count++], sizeof(lines[0]), "dano x%.2f   recarga x%.2f   area x%.2f   velocidade x%.2f", p->dmg_mult, p->cooldown_mult, p->area_mult, p->speed_mult);
    snprintf(lines[count++], sizeof(lines[0]), "armadura %.0f   coleta %.0f   xp x%.2f   critico %.0f%%", p->armor, p->magnet_range, p->xp_mult, p->crit_chance * 100.0f);
    snprintf(lines[count++], sizeof(lines[0]), "nivel %d   xp %.0f/%d   horas %d   abates %d", p->level, p->xp, p->xp_needed, sv_run.coins, sv_run.kills);
    for (int i = 0; i < p->weapon_count && count < 22; i++)
    {
        snprintf(lines[count++], sizeof(lines[0]), "  %s   nv %d   recarga %.1fs", sv_weapon_name(p->weapons[i].id), p->weapons[i].level, p->weapons[i].cooldown > 0.0f ? p->weapons[i].cooldown : 0.0f);
    }
    if (p->passive_count > 0)
    {
        char passives[110] = "";

        for (int i = 0; i < p->passive_count; i++)
        {
            char one[40];

            snprintf(one, sizeof(one), "%s%s %d", i ? ", " : "", sv_passive_name(p->passives[i].id), p->passives[i].level);
            if (strlen(passives) + strlen(one) < sizeof(passives)) strcat(passives, one);
        }
        snprintf(lines[count++], sizeof(lines[0]), "passivos: %s", passives);
    }
    if (sv_cheat_active(CHEAT_GOD)) strcat(cheats, "INVULNERAVEL ");
    if (sv_cheat_active(CHEAT_NO_SPAWN)) strcat(cheats, "SEM NASCIMENTOS ");
    if (sv_cheat_active(CHEAT_ONE_HIT)) strcat(cheats, "UM GOLPE ");
    if (sv_cheat_active(CHEAT_FREEZE)) strcat(cheats, "CONGELADO ");
    snprintf(lines[count++], sizeof(lines[0]), "cheats ligados: %s", cheats[0] ? cheats : "nenhum");
    if (sv_run.cheated) snprintf(lines[count++], sizeof(lines[0]), "PARTIDA COM CHEATS: nao conta para o progresso");

    DrawRectangle((int)(x - 8.0f * ui), (int)(y - 8.0f * ui), (int)width, (int)(step * (float)count + 16.0f * ui), (Color){0, 0, 0, 165});
    for (int i = 0; i < count; i++)
    {
        Color color = i == 0 ? (Color){255, 214, 80, 255} : (sv_run.cheated && i == count - 1) ? (Color){255, 110, 96, 255} : (Color){230, 230, 236, 255};

        sv_text(lines[i], x, y + step * (float)i, size, color);
    }

    // Gráfico dos últimos quadros: verde até 17 ms, amarelo até 33 ms e vermelho acima. Cada barra é um quadro.
    {
        float gy = y + step * (float)count + 22.0f * ui;
        float bar_width = 4.0f * ui;
        float full = 40.0f * ui;

        DrawRectangle((int)(x - 8.0f * ui), (int)(gy - 4.0f * ui), (int)(bar_width * FRAME_HISTORY + 16.0f * ui), (int)(full + 12.0f * ui), (Color){0, 0, 0, 165});
        for (int i = 0; i < FRAME_HISTORY; i++)
        {
            float value = frame_ms[(frame_head + i) % FRAME_HISTORY];
            float height = fminf(value / 50.0f, 1.0f) * full;
            Color color = value <= 17.5f ? (Color){90, 220, 120, 255} : value <= 33.4f ? (Color){240, 210, 80, 255} : (Color){240, 80, 80, 255};

            DrawRectangle((int)(x + bar_width * (float)i), (int)(gy + full - height), (int)fmaxf(bar_width - 1.0f, 1.0f), (int)fmaxf(height, 1.0f), color);
        }
        DrawLine((int)x, (int)(gy + full - full * (16.7f / 50.0f)), (int)(x + bar_width * FRAME_HISTORY), (int)(gy + full - full * (16.7f / 50.0f)), (Color){255, 255, 255, 90});
    }

    if (GetTime() < message_until) sv_text_centered(message, GetScreenWidth() / 2.0f, GetScreenHeight() - 70.0f * ui, 26.0f * ui, (Color){150, 240, 170, 255});
}

// -----------------------------------------------------------------------------
// Menu de cheats
// -----------------------------------------------------------------------------

static char row_label[MAX_ROWS][96];
static char row_value[MAX_ROWS][40];
static int row_count = 0;

static void add_row(const char *label, const char *value)
{
    if (row_count >= MAX_ROWS) return;
    snprintf(row_label[row_count], sizeof(row_label[0]), "%s", label);
    snprintf(row_value[row_count], sizeof(row_value[0]), "%s", value != NULL ? value : "");
    row_count++;
}

static const char *on_off(bool value)
{
    return value ? "LIGADO" : "desligado";
}

static void build_rows(void)
{
    char text[48];

    row_count = 0;
    switch (page)
    {
    case PAGE_MAIN:
        for (int i = 0; i < MAIN_ITEMS; i++) add_row(main_labels[i], "");
        break;
    case PAGE_WEAPONS:
        for (int i = 0; i < W_COUNT; i++)
        {
            int level = sv_cheat_weapon_level(i);

            snprintf(text, sizeof(text), level > 0 ? "nivel %d" : "-", level);
            add_row(sv_weapon_name(i), text);
        }
        break;
    case PAGE_PASSIVES:
        for (int i = 0; i < P_COUNT; i++)
        {
            int level = sv_cheat_passive_level(i);

            snprintf(text, sizeof(text), level > 0 ? "nivel %d" : "-", level);
            add_row(sv_passive_name(i), text);
        }
        break;
    case PAGE_BOSSES:
        for (int i = 0; i < BOSS_COUNT; i++) add_row(sv_boss_name(i), "chamar");
        add_row("Derrotar o chefe que está na tela", sv_run.boss.active ? "ativo" : "nenhum");
        break;
    case PAGE_ENEMIES:
        for (int i = 0; i < EN_COUNT; i++)
        {
            snprintf(text, sizeof(text), "chamar %d", amounts[amount_index]);
            add_row(sv_enemy_name(i), text);
        }
        add_row("Limpar todos os inimigos", "");
        break;
    case PAGE_PLAYER:
        add_row("Invulnerável", on_off(sv_cheat_active(CHEAT_GOD)));
        add_row("Curar tudo", "");
        add_row("Subir de nível agora", "");
        add_row("Um golpe derruba qualquer um (dano x1000)", on_off(sv_cheat_active(CHEAT_ONE_HIT)));
        add_row("Ctrl+Z de volta (reviver uma vez)", sv_run.player.revive_ready ? "pronto" : "");
        break;
    case PAGE_MATCH:
        add_row("Pular 30 segundos", "");
        add_row("Pular 60 segundos", "");
        for (int i = 0; i < BOSS_COUNT; i++)
        {
            snprintf(text, sizeof(text), "%d:%02d", (int)sv_boss_spawn_time((SvBossId)i) / 60, (int)sv_boss_spawn_time((SvBossId)i) % 60);
            {
                char label[96];

                snprintf(label, sizeof(label), "Ir para pouco antes do chefe %d", i + 1);
                add_row(label, text);
            }
        }
        add_row("Parar os nascimentos de inimigos", on_off(sv_cheat_active(CHEAT_NO_SPAWN)));
        add_row("Congelar os inimigos", on_off(sv_cheat_active(CHEAT_FREEZE)));
        add_row("Limpar os perigos dos chefes", "");
        break;
    default:
        break;
    }
}

static void apply_row(void)
{
    char text[96];

    switch (page)
    {
    case PAGE_MAIN:
        if (cursor == MAIN_ITEMS - 1)
        {
            menu_open = false;
        }
        else
        {
            page = (DebugPage)(PAGE_WEAPONS + cursor);
            cursor = 0;
            first_row = 0;
        }
        break;
    case PAGE_WEAPONS:
        if (sv_cheat_set_weapon(cursor, target_level))
        {
            snprintf(text, sizeof(text), "%s no nivel %d.", sv_weapon_name(cursor), target_level);
            show_message(text);
        }
        else
        {
            show_message("Não coube: o limite é de 6 poderes.");
        }
        break;
    case PAGE_PASSIVES:
        if (sv_cheat_set_passive(cursor, target_level))
        {
            snprintf(text, sizeof(text), "%s no nivel %d.", sv_passive_name(cursor), target_level);
            show_message(text);
        }
        else
        {
            show_message("Não coube: o limite é de 6 passivos.");
        }
        break;
    case PAGE_BOSSES:
        if (cursor < BOSS_COUNT)
        {
            sv_cheat_spawn_boss(cursor);
            snprintf(text, sizeof(text), "Chefe chamado: %s.", sv_boss_name(cursor));
            show_message(text);
            menu_open = false;
        }
        else
        {
            sv_cheat_kill_boss();
            show_message("Chefe derrotado.");
        }
        break;
    case PAGE_ENEMIES:
        if (cursor < EN_COUNT)
        {
            sv_cheat_spawn_enemies(cursor, amounts[amount_index]);
            snprintf(text, sizeof(text), "%d x %s.", amounts[amount_index], sv_enemy_name(cursor));
            show_message(text);
        }
        else
        {
            sv_cheat_clear_enemies();
            show_message("Inimigos removidos.");
        }
        break;
    case PAGE_PLAYER:
        if (cursor == 0) sv_cheat_toggle(CHEAT_GOD);
        else if (cursor == 1) { sv_cheat_heal(); show_message("Vida cheia."); }
        else if (cursor == 2) { sv_cheat_level_up(); show_message("Feche o menu: a escolha de nível abre em seguida."); }
        else if (cursor == 3) sv_cheat_toggle(CHEAT_ONE_HIT);
        else { sv_cheat_revive(); show_message("Ctrl+Z pronto."); }
        break;
    case PAGE_MATCH:
        if (cursor == 0) sv_cheat_jump_time(30.0f);
        else if (cursor == 1) sv_cheat_jump_time(60.0f);
        else if (cursor >= 2 && cursor < 2 + BOSS_COUNT)
        {
            float target = sv_boss_spawn_time((SvBossId)(cursor - 2)) - 4.0f;

            if (target > sv_run.time) sv_cheat_jump_time(target - sv_run.time);
            else show_message("Esse momento já passou.");
        }
        else if (cursor == 2 + BOSS_COUNT) sv_cheat_toggle(CHEAT_NO_SPAWN);
        else if (cursor == 3 + BOSS_COUNT) sv_cheat_toggle(CHEAT_FREEZE);
        else { sv_cheat_clear_hazards(); show_message("Perigos removidos."); }
        break;
    default:
        break;
    }
}

static void remove_row(void)
{
    char text[96];

    if (page == PAGE_WEAPONS && sv_cheat_set_weapon(cursor, 0))
    {
        snprintf(text, sizeof(text), "%s removido.", sv_weapon_name(cursor));
        show_message(text);
    }
    else if (page == PAGE_PASSIVES && sv_cheat_set_passive(cursor, 0))
    {
        snprintf(text, sizeof(text), "%s removido.", sv_passive_name(cursor));
        show_message(text);
    }
}

static Rectangle menu_panel(void)
{
    float ui = sv_ui();

    return (Rectangle){18.0f * ui, 110.0f * ui, 640.0f * ui, GetScreenHeight() - 150.0f * ui};
}

static int visible_rows(void)
{
    float ui = sv_ui();

    return (int)((menu_panel().height - 150.0f * ui) / (34.0f * ui));
}

static Rectangle row_area(int visible_index)
{
    float ui = sv_ui();
    Rectangle panel = menu_panel();

    return (Rectangle){panel.x + 14.0f * ui, panel.y + 88.0f * ui + (float)visible_index * 34.0f * ui, panel.width - 28.0f * ui, 32.0f * ui};
}

bool sv_debug_update(void)
{
    bool was_open;
    int visible;

    if (!sv_debug_mode) return false;

    // O quadro em que o menu fecha ainda conta como parado: o Esc que fechou o menu não pode também pausar a partida
    was_open = menu_open;
    if (IsKeyPressed(KEY_F2) && sv_run.phase == SV_PLAYING) menu_open = !menu_open;
    if (sv_run.phase != SV_PLAYING) menu_open = false;
    if (!menu_open) return was_open;

    build_rows();
    visible = visible_rows();
    if (visible < 4) visible = 4;

    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) cursor = (cursor + 1) % row_count;
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) cursor = (cursor + row_count - 1) % row_count;
    if (GetMouseWheelMove() < 0.0f) cursor = (cursor + 1) % row_count;
    if (GetMouseWheelMove() > 0.0f) cursor = (cursor + row_count - 1) % row_count;

    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A))
    {
        if (page == PAGE_WEAPONS || page == PAGE_PASSIVES) target_level = target_level > 1 ? target_level - 1 : 1;
        if (page == PAGE_ENEMIES) amount_index = amount_index > 0 ? amount_index - 1 : 0;
    }
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D))
    {
        if (page == PAGE_WEAPONS || page == PAGE_PASSIVES) target_level = target_level < SV_WEAPON_MAX_LEVEL ? target_level + 1 : SV_WEAPON_MAX_LEVEL;
        if (page == PAGE_ENEMIES) amount_index = amount_index < 4 ? amount_index + 1 : 4;
    }

    if (first_row > cursor) first_row = cursor;
    if (cursor >= first_row + visible) first_row = cursor - visible + 1;

    // Mouse: passar por cima escolhe a linha e clicar aplica
    for (int i = 0; i < visible && first_row + i < row_count; i++)
    {
        if (CheckCollisionPointRec(GetMousePosition(), row_area(i)))
        {
            if (GetMouseDelta().x != 0.0f || GetMouseDelta().y != 0.0f) cursor = first_row + i;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                cursor = first_row + i;
                apply_row();
                return menu_open || was_open;
            }
        }
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_KP_ENTER)) apply_row();
    if (IsKeyPressed(KEY_DELETE)) remove_row();
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE))
    {
        if (page == PAGE_MAIN) menu_open = false;
        else
        {
            page = PAGE_MAIN;
            cursor = 0;
            first_row = 0;
        }
    }

    return menu_open || was_open;
}

void sv_debug_draw_menu(void)
{
    float ui = sv_ui();
    Rectangle panel = menu_panel();
    char text[96];
    int visible = visible_rows();
    const char *titles[PAGE_COUNT] = {"CHEATS", "PODERES", "PASSIVOS", "CHEFES", "INIMIGOS", "JOGADOR", "PARTIDA"};

    if (!sv_debug_menu_open()) return;
    if (visible < 4) visible = 4;
    build_rows();
    if (cursor >= row_count) cursor = row_count - 1;

    DrawRectangleRec(panel, (Color){10, 6, 16, 255});   // Opaco: cobre o painel de números que fica por baixo
    DrawRectangleLinesEx(panel, 3.0f * ui, (Color){200, 12, 20, 255});
    sv_text(titles[page], panel.x + 18.0f * ui, panel.y + 12.0f * ui, 40.0f * ui, (Color){240, 234, 244, 255});

    if (page == PAGE_WEAPONS || page == PAGE_PASSIVES)
    {
        snprintf(text, sizeof(text), "Nivel a dar: %d   (Esq/Dir muda)", target_level);
        sv_text(text, panel.x + 18.0f * ui, panel.y + 56.0f * ui, 22.0f * ui, (Color){255, 214, 80, 255});
    }
    else if (page == PAGE_ENEMIES)
    {
        snprintf(text, sizeof(text), "Quantidade: %d   (Esq/Dir muda)", amounts[amount_index]);
        sv_text(text, panel.x + 18.0f * ui, panel.y + 56.0f * ui, 22.0f * ui, (Color){255, 214, 80, 255});
    }
    else if (page == PAGE_MAIN)
    {
        sv_text("A partida fica parada enquanto o menu está aberto.", panel.x + 18.0f * ui, panel.y + 56.0f * ui, 20.0f * ui, (Color){170, 160, 180, 255});
    }

    for (int i = 0; i < visible && first_row + i < row_count; i++)
    {
        int row = first_row + i;
        Rectangle area = row_area(i);
        bool selected = row == cursor;

        if (selected) DrawRectangleRec(area, (Color){90, 50, 110, 255});
        sv_text(row_label[row], area.x + 10.0f * ui, area.y + 2.0f * ui, 26.0f * ui, selected ? WHITE : (Color){200, 192, 206, 255});
        if (row_value[row][0] != '\0')
        {
            bool on = strcmp(row_value[row], "LIGADO") == 0 || strncmp(row_value[row], "nivel", 5) == 0;

            sv_text(row_value[row], area.x + area.width - sv_text_width(row_value[row], 24.0f * ui) - 10.0f * ui, area.y + 3.0f * ui, 24.0f * ui, on ? (Color){120, 240, 150, 255} : (Color){170, 160, 180, 255});
        }
    }

    {
        const char *hint = page == PAGE_MAIN ? "Setas: escolher    Enter: abrir    F2/Esc: fechar"
                           : (page == PAGE_WEAPONS || page == PAGE_PASSIVES) ? "Enter: dar    Del: tirar    Esc: voltar"
                                                                             : "Enter: aplicar    Esc: voltar";

        sv_text(hint, panel.x + 18.0f * ui, panel.y + panel.height - 40.0f * ui, 22.0f * ui, (Color){170, 160, 180, 255});
    }
    if (GetTime() < message_until) sv_text(message, panel.x + 18.0f * ui, panel.y + panel.height - 74.0f * ui, 22.0f * ui, (Color){150, 240, 170, 255});
}
