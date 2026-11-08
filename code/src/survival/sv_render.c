#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "raymath.h"
#include "characters.h"
#include "fonts.h"
#include "helper.h"
#include "macros.h"
#include "sv_render.h"
#include "sv_meta.h"
#include "sv_map.h"

static const SvSpriteId boss_sprites[BOSS_COUNT] = {SPR_bo_impressora, SPR_bo_modelo, SPR_bo_diego, SPR_bo_rene, SPR_bo_iara};

#define COLOR_HP (Color){200, 40, 48, 255}
#define COLOR_XP (Color){70, 200, 220, 255}
#define COLOR_BAR_BACK (Color){10, 6, 14, 200}

float sv_ui(void)
{
    return get_ui_scale();
}

// -----------------------------------------------------------------------------
// Texto
// -----------------------------------------------------------------------------

float sv_text_width(const char *text, float size)
{
    return MeasureTextEx(default_game_font, text, size, size / 12.0f).x;
}

void sv_text(const char *text, float x, float y, float size, Color color)
{
    // Sombra escura para o texto ler bem sobre qualquer sala
    DrawTextEx(default_game_font, text, (Vector2){x + size / 14.0f, y + size / 14.0f}, size, size / 12.0f, (Color){0, 0, 0, (unsigned char)(color.a * 0.8f)});
    DrawTextEx(default_game_font, text, (Vector2){x, y}, size, size / 12.0f, color);
}

void sv_text_centered(const char *text, float center_x, float y, float size, Color color)
{
    sv_text(text, center_x - sv_text_width(text, size) / 2.0f, y, size, color);
}

// -----------------------------------------------------------------------------
// Mundo
// -----------------------------------------------------------------------------

static int anim_frame(float t, float rate)
{
    return (int)(t * rate);
}

static void draw_shadow(Vector2 pos, float radius)
{
    DrawEllipse((int)pos.x, (int)(pos.y + 12.0f), radius, radius * 0.4f, (Color){0, 0, 0, 70});
}

static void draw_background(Camera2D camera)
{
    float half_w = GetScreenWidth() / 2.0f / camera.zoom;
    float half_h = GetScreenHeight() / 2.0f / camera.zoom;
    int first_col = (int)((camera.target.x - half_w) / SV_CELL_W);
    int last_col = (int)((camera.target.x + half_w) / SV_CELL_W);
    int first_row = (int)((camera.target.y - half_h) / SV_CELL_H);
    int last_row = (int)((camera.target.y + half_h) / SV_CELL_H);

    if (first_col < 0) first_col = 0;
    if (first_row < 0) first_row = 0;
    if (last_col >= SV_COLS) last_col = SV_COLS - 1;
    if (last_row >= SV_ROWS) last_row = SV_ROWS - 1;

    for (int row = first_row; row <= last_row; row++)
    {
        for (int col = first_col; col <= last_col; col++)
        {
            Vector2 center = {col * SV_CELL_W + SV_CELL_W / 2.0f, row * SV_CELL_H + SV_CELL_H / 2.0f};

            sv_draw((SvSpriteId)sv_area_room(row, col), 0, center, 1.0f, 0.0f, false, WHITE);
        }
    }

    // Borda do mapa
    DrawRectangleLinesEx((Rectangle){0, 0, SV_WORLD_W, SV_WORLD_H}, 6.0f, (Color){120, 10, 18, 255});
}

static void draw_gems(void)
{
    for (int i = 0; i < SV_MAX_GEMS; i++)
    {
        SvGem *g = &sv_run.gems[i];
        SvSpriteId sprite;

        if (!g->alive) continue;
        sprite = g->kind == 0 ? SPR_it_xp : g->kind == 1 ? SPR_it_moeda : g->kind == 2 ? SPR_it_marmita : SPR_it_bau;
        if (g->kind == 0) sv_draw(sprite, 0, g->pos, 0.6f, 0.0f, false, WHITE);
        else sv_draw(sprite, 0, (Vector2){g->pos.x, g->pos.y + sinf(sv_run.time * 4.0f + g->pos.x) * 1.5f}, 0.9f, 0.0f, false, WHITE);
    }
}

static float spark_hash(unsigned n);

// Prompt da IARA: a janela de terminal digita um comando sobre o alvo e, ao dar Enter, solta uma chuva de letras
static const char *prompt_commands[8] = {"> demitir todos", "> sudo matar", "> rm -rf inimigo", "> exit(0);", "> encerrar tudo", "> apagar chamado", "> negar acesso", "> ignorar erro"};

static void draw_prompt_windows(void)
{
    static const char glyphs[] = "01{}<>/?!#;=";

    for (int i = 0; i < SV_MAX_ZONES; i++)
    {
        SvZone *z = &sv_run.zones[i];
        const char *full;
        float wx, wy;
        const float ww = 120.0f, wh = 36.0f;

        if (!z->alive || z->kind != ZK_PROMPT) continue;
        full = prompt_commands[((z->text % 8) + 8) % 8];
        wx = z->pos.x - ww / 2.0f;
        wy = z->pos.y - 84.0f;

        if (z->delay > 0.0f)
        {
            // Digitando: aparece uma letra por vez, com o cursor piscando
            float t = 1.0f - z->delay / SV_PROMPT_TYPING;
            int length = (int)strlen(full);
            int shown = (int)(t * (float)(length + 5));
            char text[24];
            bool blink = ((int)(sv_run.time * 6.0f) % 2) == 0;

            if (shown > length) shown = length;
            memcpy(text, full, (size_t)shown);
            text[shown] = 0;

            for (int k = 0; k < 3; k++) DrawRectangle((int)z->pos.x - 1, (int)(wy + wh + 3.0f + (float)k * 8.0f), 2, 4, (Color){56, 240, 228, 150});
            DrawRectangle((int)wx + 2, (int)wy + 2, (int)ww, (int)wh, (Color){0, 0, 0, 90});
            DrawRectangle((int)wx, (int)wy, (int)ww, (int)wh, (Color){8, 22, 30, 240});
            DrawRectangle((int)wx, (int)wy, (int)ww, 9, (Color){18, 124, 128, 255});
            for (int k = 0; k < 3; k++) DrawRectangle((int)wx + 4 + k * 6, (int)wy + 3, 3, 3, k == 0 ? (Color){200, 12, 20, 255} : (Color){232, 194, 74, 255});
            sv_text("IARA", wx + ww - 30.0f, wy - 1.0f, 8.0f, (Color){224, 255, 248, 210});
            DrawRectangleLines((int)wx, (int)wy, (int)ww, (int)wh, (Color){56, 240, 228, 255});
            sv_text(text, wx + 6.0f, wy + 15.0f, 11.0f, (Color){120, 255, 240, 255});
            if (blink) DrawRectangle((int)(wx + 7.0f + sv_text_width(text, 11.0f)), (int)wy + 16, 5, 9, (Color){224, 255, 248, 255});
        }
        else
        {
            // Enter: a janela pisca e a resposta sai em letras que se espalham pela área
            float q = 1.0f - z->life / 0.4f;
            unsigned char fade = (unsigned char)(255.0f * (1.0f - q));

            if (q < 0.6f)
            {
                // A janela fica um instante com o comando completo, pisca em branco e some
                unsigned char alpha = (unsigned char)(255.0f * (1.0f - q / 0.6f));

                DrawRectangle((int)wx, (int)wy, (int)ww, (int)wh, (Color){8, 22, 30, alpha});
                DrawRectangle((int)wx, (int)wy, (int)ww, 9, (Color){18, 124, 128, alpha});
                DrawRectangleLines((int)wx, (int)wy, (int)ww, (int)wh, (Color){180, 255, 248, alpha});
                sv_text(full, wx + 6.0f, wy + 15.0f, 11.0f, (Color){120, 255, 240, alpha});
                if (q < 0.15f) DrawRectangle((int)wx, (int)wy, (int)ww, (int)wh, (Color){255, 255, 255, (unsigned char)(170.0f * (1.0f - q / 0.15f))});
            }
            DrawCircleV(z->pos, z->radius * (0.3f + 0.7f * q), (Color){56, 240, 228, (unsigned char)(fade / 5)});
            DrawCircleLinesV(z->pos, z->radius * (0.3f + 0.7f * q), (Color){160, 255, 245, fade});
            for (int k = 0; k < 18; k++)
            {
                float angle = ((float)k + spark_hash((unsigned)(z->text * 31 + k))) * (PI * 2.0f / 18.0f);
                float distance = z->radius * (0.15f + 0.9f * q) * (0.6f + 0.4f * spark_hash((unsigned)(k * 7 + 3)));
                char glyph[2] = {glyphs[k % (int)(sizeof(glyphs) - 1)], 0};

                sv_text(glyph, z->pos.x + cosf(angle) * distance - 3.0f, z->pos.y + sinf(angle) * distance * 0.8f - 6.0f, 12.0f, k % 3 ? (Color){120, 255, 240, fade} : (Color){255, 255, 255, fade});
            }
        }
    }
}

// Raio do Deploy em Produção. O desenho muda de forma nos primeiros instantes para parecer elétrico e depois some devagar
#define DEPLOY_WARN 0.4f

static float spark_hash(unsigned n)
{
    n ^= n << 13;
    n ^= n >> 17;
    n ^= n << 5;
    n *= 2654435761u;
    return (float)((n >> 8) & 0xFFFF) / 65535.0f;
}

static void draw_bolt_path(Vector2 from, Vector2 to, unsigned seed, float spread, int parts, float width, Color glow, Color core)
{
    Vector2 previous = from;
    Vector2 direction = Vector2Normalize(Vector2Subtract(to, from));
    Vector2 side = {-direction.y, direction.x};

    for (int i = 1; i <= parts; i++)
    {
        float t = (float)i / (float)parts;
        Vector2 point = Vector2Lerp(from, to, t);

        // Os extremos ficam fixos, o meio zigue-zague
        if (i < parts) point = Vector2Add(point, Vector2Scale(side, (spark_hash(seed + (unsigned)i * 977u) - 0.5f) * 2.0f * spread));
        DrawLineEx(previous, point, width * 6.0f, (Color){120, 170, 255, (unsigned char)(glow.a * 0.85f)});
        DrawLineEx(previous, point, width * 3.0f, glow);
        DrawLineEx(previous, point, width, core);
        previous = point;
    }
}

static void draw_lightning(void)
{
    for (int i = 0; i < SV_MAX_ZONES; i++)
    {
        SvZone *z = &sv_run.zones[i];
        unsigned base_seed;
        float t, flicker, alpha;
        Vector2 top;

        if (!z->alive || z->kind != ZK_DEPLOY) continue;
        base_seed = (unsigned)(z->pos.x * 31.0f) * 7919u + (unsigned)(z->pos.y * 17.0f);
        top = (Vector2){z->pos.x + (spark_hash(base_seed) - 0.5f) * 90.0f, z->pos.y - 440.0f};

        if (z->delay > 0.0f)
        {
            // Pouco antes de cair, um fio fino pisca no céu e avisa onde vai bater
            if (z->delay < 0.14f && ((int)(sv_run.time * 40.0f) % 2) == 0) draw_bolt_path(top, z->pos, base_seed, 7.0f, 7, 1.0f, (Color){255, 230, 120, 40}, (Color){255, 255, 255, 120});
            continue;
        }

        t = 1.0f - z->life / z->max_life;                     // 0 a 1 durante os 0,45 s do raio
        flicker = fminf(3.0f, floorf(t * 22.0f));             // a forma troca 3 vezes e depois se firma
        alpha = t < 0.25f ? 1.0f : fmaxf(0.0f, 1.0f - (t - 0.25f) / 0.6f);

        if (alpha > 0.0f)
        {
            unsigned seed = base_seed + (unsigned)flicker * 4099u;
            Color glow = {255, 225, 90, (unsigned char)(130.0f * alpha)};
            Color core = {255, 255, 235, (unsigned char)(255.0f * alpha)};
            Vector2 fork_from = Vector2Lerp(top, z->pos, 0.45f + 0.2f * spark_hash(seed + 5u));
            Vector2 fork_to = Vector2Add(fork_from, (Vector2){(spark_hash(seed + 6u) - 0.5f) * 150.0f, 90.0f + 40.0f * spark_hash(seed + 7u)});

            draw_bolt_path(top, z->pos, seed, 26.0f, 11, 4.4f, glow, core);
            draw_bolt_path(fork_from, fork_to, seed + 31u, 12.0f, 4, 2.0f, (Color){255, 225, 90, (unsigned char)(90.0f * alpha)}, (Color){255, 250, 210, (unsigned char)(220.0f * alpha)});
        }

        // Clarão e faíscas no ponto de impacto
        if (t < 0.45f)
        {
            float k = t / 0.45f;
            float reach = z->radius + 16.0f;

            DrawCircleV(z->pos, reach * (0.5f + 0.9f * k), (Color){255, 245, 200, (unsigned char)(230.0f * (1.0f - k))});
            DrawCircleLinesV(z->pos, reach * (0.7f + 0.9f * k), (Color){255, 220, 90, (unsigned char)(240.0f * (1.0f - k))});
            DrawCircleLinesV(z->pos, reach * (0.5f + 1.3f * k), (Color){140, 185, 255, (unsigned char)(200.0f * (1.0f - k))});
            for (int s = 0; s < 8; s++)
            {
                float angle = ((float)s + spark_hash(base_seed + (unsigned)s)) * (PI * 2.0f / 8.0f);
                Vector2 a = Vector2Add(z->pos, (Vector2){cosf(angle) * (6.0f + 16.0f * k), sinf(angle) * (3.0f + 8.0f * k)});
                Vector2 b = Vector2Add(z->pos, (Vector2){cosf(angle) * (12.0f + 30.0f * k), sinf(angle) * (6.0f + 15.0f * k)});

                DrawLineEx(a, b, 1.6f, (Color){255, 240, 160, (unsigned char)(255.0f * (1.0f - k))});
            }
        }
    }
}

static void draw_zones(void)
{
    for (int i = 0; i < SV_MAX_ZONES; i++)
    {
        SvZone *z = &sv_run.zones[i];
        float progress;

        if (!z->alive) continue;

        if (z->shape == ZONE_RECT)
        {
            float length = z->length;
            float angle = z->angle * DEG2RAD;
            Vector2 center = Vector2Add(z->pos, (Vector2){cosf(angle) * length / 2.0f, sinf(angle) * length / 2.0f});

            progress = 1.0f - z->life / z->max_life;
            if (z->kind == ZK_TARJA) sv_draw_stretched(SPR_fx_tarja, (int)(progress * 4.0f), center, length, z->radius * 2.0f, z->angle, WHITE);
            else if (z->kind == ZK_FIBRA) sv_draw_stretched(SPR_fx_fibra, (int)(progress * 4.0f), center, length, z->radius * 2.0f + 10.0f, z->angle, WHITE);
            else sv_draw_stretched(SPR_fx_ponteiro, (int)(progress * 4.0f), center, length, 13.0f, z->angle, WHITE);
            continue;
        }

        if (z->kind == ZK_CANECA)
        {
            if (z->delay > 0.0f)
            {
                float t = z->delay / 0.5f;

                DrawCircleLinesV(z->pos, z->radius, (Color){255, 200, 120, 120});
                sv_draw(SPR_fx_caneca, t > 0.5f ? 0 : 1, (Vector2){z->pos.x, z->pos.y - t * 90.0f}, 1.0f, 0.0f, false, WHITE);
            }
            else
            {
                // Respingo de café: a poça cresce no impacto e depois espirra para os lados. O desenho tem 38 px de largura
                float t = 1.0f - z->life / 0.35f;
                float scale = z->radius * 2.0f / 38.0f;
                Color tint = WHITE;

                if (t > 0.6f) tint.a = (unsigned char)(255.0f * (1.0f - (t - 0.6f) / 0.4f));
                sv_draw(SPR_fx_caneca, t < 0.35f ? 2 : 3, z->pos, scale, 0.0f, false, tint);
            }
        }
        else if (z->kind == ZK_PROMPT)
        {
            // No chão: o anel mostra até onde a onda do comando vai. A janela e as letras são desenhadas por cima, em draw_prompt_windows
            if (z->delay > 0.0f)
            {
                float pulse = 0.5f + 0.5f * sinf(sv_run.time * 18.0f);

                DrawCircleV(z->pos, z->radius, (Color){56, 240, 228, 18});
                DrawCircleLinesV(z->pos, z->radius, (Color){56, 240, 228, (unsigned char)(70 + 80 * pulse)});
            }
        }
        else if (z->kind == ZK_VERA)
        {
            // Onda de "falso": o anel abre a partir de quem foi atingido e some
            if (z->delay <= 0.0f)
            {
                float t = 1.0f - z->life / 0.3f;
                float radius = z->radius * (0.35f + 0.65f * t);
                unsigned char fade = (unsigned char)(255.0f * (1.0f - t));

                DrawCircleV(z->pos, radius, (Color){90, 210, 255, (unsigned char)(fade / 6)});
                DrawCircleLinesV(z->pos, radius, (Color){170, 240, 255, fade});
                DrawCircleLinesV(z->pos, radius * 0.82f, (Color){90, 210, 255, (unsigned char)(fade / 2)});
            }
        }
        else if (z->kind == ZK_DEPLOY)
        {
            // No chão: a marca que carrega antes do raio e a queimadura que fica depois. O raio em si é desenhado por cima, em draw_lightning
            if (z->delay > 0.0f)
            {
                float charge = 1.0f - z->delay / DEPLOY_WARN;
                float pulse = 0.5f + 0.5f * sinf(sv_run.time * 24.0f);

                DrawCircleV(z->pos, z->radius, (Color){255, 220, 70, (unsigned char)(18 + 40 * charge)});
                DrawCircleLinesV(z->pos, z->radius, (Color){255, 230, 90, (unsigned char)(110 + 100 * pulse)});
                DrawCircleLinesV(z->pos, z->radius * (1.0f - 0.85f * charge), (Color){255, 255, 230, (unsigned char)(80 + 120 * charge)});
            }
            else
            {
                float t = 1.0f - z->life / z->max_life;
                unsigned char fade = (unsigned char)(200.0f * (1.0f - t));

                DrawEllipse((int)z->pos.x, (int)z->pos.y + 2, z->radius * 0.9f, z->radius * 0.45f, (Color){18, 10, 2, fade});
                DrawEllipse((int)z->pos.x, (int)z->pos.y + 2, z->radius * 0.5f, z->radius * 0.25f, (Color){255, 190, 50, (unsigned char)(fade / 2)});
                sv_draw(SPR_fx_deploy, 3, z->pos, 1.0f, 0.0f, false, (Color){255, 255, 255, fade});
            }
        }
    }
}

static void draw_enemy(SvEnemy *e)
{
    {
        Color tint = WHITE;
        int frame;

        if (e->hurt_flash > 0.0f) tint = (Color){255, 140, 140, 255};
        else if (e->confused > 0.0f) tint = (Color){190, 150, 255, 255};

        frame = e->facing * 4 + anim_frame(e->anim, 6.0f) % 4;
        draw_shadow(e->pos, 9.0f);
        if (e->type == EN_CRACHA) tint.a = 190; // atravessa objetos
        sv_draw(sv_enemy_sprite(e->type), frame, e->pos, 1.0f, 0.0f, e->facing == 1 && e->flip, tint);
    }
}

static void draw_player(void)
{
    SvPlayer *p = &sv_run.player;
    Color tint = WHITE;
    int frame = p->walk_time > 0.0f ? get_walk_frame(p->walk_time) : 0;

    if (p->hurt_flash > 0.0f) tint = (Color){255, 120, 120, 255};
    else if (p->invulnerable > 0.0f && ((int)(sv_run.time * 20.0f) % 2)) tint = (Color){255, 255, 255, 120};
    if (p->root > 0.0f) tint = (Color){150, 150, 255, 255};

    // Auras e satélites ficam atrás da Estagiária
    for (int i = 0; i < p->weapon_count; i++)
    {
        SvWeapon *w = &p->weapons[i];

        if (w->id == W_NAOPERTURBE)
        {
            float radius = sv_weapon_stat(W_NAOPERTURBE, WSTAT_AREA, w->level) * p->area_mult;

            sv_draw(SPR_fx_naoperturbe, anim_frame(sv_run.time, 8.0f) % 4, p->pos, radius * 2.0f / 64.0f, 0.0f, false, (Color){255, 255, 255, 110});
        }
        else if (w->id == W_TABELA)
        {
            float radius = sv_weapon_stat(W_TABELA, WSTAT_AREA, w->level) * p->area_mult;

            sv_draw(SPR_fx_tabela, anim_frame(sv_run.time, 3.0f) % 4, p->pos, radius * 2.0f / 61.0f, 0.0f, false, (Color){255, 255, 255, 150});
        }
    }

    draw_shadow(p->pos, 10.0f);
    sv_draw_player(p->direction, frame, p->pos, 1.0f, tint);

    // Vera Fischer: o anel mostra que ela está armada e o próximo golpe será anulado; o aviso aparece quando acontece
    for (int i = 0; i < p->weapon_count; i++)
    {
        if (p->weapons[i].id == W_VERA && p->weapons[i].cooldown <= 0.0f) DrawCircleLinesV(p->pos, 16.0f + sinf(sv_run.time * 5.0f), (Color){120, 230, 255, 170});
    }
    if (p->negate_flash > 0.0f) sv_text_centered("V -> F = F", p->pos.x, p->pos.y - 46.0f - (0.9f - p->negate_flash) * 14.0f, 13.0f, (Color){160, 240, 255, (unsigned char)(255.0f * fminf(1.0f, p->negate_flash * 2.5f))});

    for (int i = 0; i < p->weapon_count; i++)
    {
        SvWeapon *w = &p->weapons[i];

        if (w->id == W_POSTITS)
        {
            float radius = sv_weapon_stat(W_POSTITS, WSTAT_AREA, w->level) * p->area_mult;

            sv_draw(SPR_fx_postits, anim_frame(sv_run.time, 6.0f) % 4, p->pos, radius / 22.0f, w->angle * RAD2DEG, false, WHITE);
        }
    }
}

static void draw_boss(void);
// Cada colega tem a sua roupa e a sua cor: Segurança (azul), Suporte de TI (verde), Enfermeira do RH (branco e vermelho) e Entregador (laranja)
static const SvSpriteId colega_sprites[SV_MAX_COLEGAS] = {SPR_en_colega_seguranca, SPR_en_colega_suporte, SPR_en_colega_enfermeira, SPR_en_colega_entregador};
static const Color colega_colors[SV_MAX_COLEGAS] = {{110, 160, 255, 255}, {110, 235, 160, 255}, {255, 130, 150, 255}, {255, 200, 80, 255}};

static void draw_colega(int c)
{
    Vector2 pos = sv_run.colega_pos[c];
    Vector2 dir = sv_run.colega_dir[c];
    bool moving = sv_run.colega_moving[c];
    float attack = sv_run.colega_attack[c];
    float bob = moving ? 0.0f : sinf(sv_run.time * 3.0f + (float)c * 1.7f);
    int frame;
    bool flip = dir.x < 0.0f;
    Color color = colega_colors[c];

    // Quadros: 0 a 3 de frente, 4 a 7 de lado, 8 a 11 de costas e 12 a 15 de lado dando o golpe
    if (attack > 0.0f)
    {
        frame = 12 + (int)fminf(3.0f, (1.0f - attack / 0.25f) * 4.0f);
    }
    else
    {
        int base = fabsf(dir.x) >= fabsf(dir.y) ? 4 : dir.y > 0.0f ? 0 : 8;

        if (base != 4) flip = false;
        frame = base + (moving ? (int)(sv_run.colega_walk[c] * 9.0f) % 4 : 0);
    }

    draw_shadow(pos, 9.0f);
    if (attack > 0.0f && c != 1)
    {
        float t = 1.0f - attack / 0.25f;

        DrawCircleLinesV(pos, 12.0f + 18.0f * t, (Color){color.r, color.g, color.b, (unsigned char)(200.0f * (1.0f - t))});
    }
    sv_draw(colega_sprites[c], frame, (Vector2){pos.x, pos.y + bob}, 1.0f, 0.0f, flip, WHITE);
    DrawRectanglePro((Rectangle){pos.x, pos.y - 25.0f + bob, 4.0f, 4.0f}, (Vector2){2.0f, 2.0f}, 45.0f, color);

    if (c == 2 && sv_run.colega_heal_flash > 0.0f)
    {
        // A cura da enfermeira: uma cruz verde sobe do jogador
        float t = 1.0f - sv_run.colega_heal_flash / 0.7f;

        DrawCircleLinesV(sv_run.player.pos, 12.0f + 14.0f * t, (Color){110, 235, 160, (unsigned char)(220.0f * (1.0f - t))});
        sv_text("+", sv_run.player.pos.x - 4.0f, sv_run.player.pos.y - 30.0f - 18.0f * t, 16.0f, (Color){110, 235, 160, (unsigned char)(255.0f * (1.0f - t))});
    }
}

static void draw_prop(const SvMapProp *p)
{
    SvSprite *sprite = &sv_sprites[p->sprite];

    sv_draw((SvSpriteId)p->sprite, 0, (Vector2){p->x, p->y - sprite->frame_height / 2.0f}, 1.0f, 0.0f, false, WHITE);
}

// Tudo o que tem altura (objetos, inimigos, chefe, jogador e colegas) é desenhado de cima para baixo pela base
typedef struct { float y; int kind; int index; } Drawable;
enum { DR_PROP, DR_ENEMY, DR_BOSS, DR_PLAYER, DR_COLEGA };

static int compare_drawables(const void *a, const void *b)
{
    float d = ((const Drawable *)a)->y - ((const Drawable *)b)->y;

    return d < 0 ? -1 : (d > 0 ? 1 : 0);
}

static void draw_world_objects(Camera2D camera)
{
    static Drawable list[1200];
    int count = 0;
    float half_w = GetScreenWidth() / 2.0f / camera.zoom + 120.0f;
    float half_h = GetScreenHeight() / 2.0f / camera.zoom + 120.0f;
    SvPlayer *player = &sv_run.player;

    for (int i = 0; i < sv_map_prop_count() && count < 1100; i++)
    {
        const SvMapProp *p = sv_map_prop(i);

        if (fabsf(p->x - camera.target.x) < half_w + 100.0f && p->y > camera.target.y - half_h && p->y < camera.target.y + half_h + 120.0f) list[count++] = (Drawable){p->y, DR_PROP, i};
    }

    for (int i = 0; i < SV_MAX_ENEMIES && count < 1190; i++)
    {
        SvEnemy *e = &sv_run.enemies[i];

        if (e->alive) list[count++] = (Drawable){e->pos.y + 10.0f, DR_ENEMY, i};
    }

    if (sv_run.boss.active) list[count++] = (Drawable){sv_run.boss.pos.y + 18.0f, DR_BOSS, 0};
    list[count++] = (Drawable){player->pos.y + 10.0f, DR_PLAYER, 0};

    for (int i = 0; i < player->weapon_count; i++)
    {
        if (player->weapons[i].id == W_COLEGA)
        {
            int n = sv_colega_count();

            for (int c = 0; c < n; c++) list[count++] = (Drawable){sv_run.colega_pos[c].y + 8.0f, DR_COLEGA, c};
        }
    }

    qsort(list, (size_t)count, sizeof(Drawable), compare_drawables);

    for (int i = 0; i < count; i++)
    {
        switch (list[i].kind)
        {
        case DR_PROP: draw_prop(sv_map_prop(list[i].index)); break;
        case DR_ENEMY: draw_enemy(&sv_run.enemies[list[i].index]); break;
        case DR_BOSS: draw_boss(); break;
        case DR_PLAYER: draw_player(); break;
        default: draw_colega(list[i].index); break;
        }
    }
}

static void draw_boss(void)
{
    SvBoss *b = &sv_run.boss;
    Color tint = WHITE;
    int frame;

    if (!b->active) return;
    if (b->hurt_flash > 0.0f) tint = (Color){255, 150, 150, 255};
    if (b->invulnerable > 0.0f && ((int)(sv_run.time * 16.0f) % 2)) tint = (Color){150, 200, 255, 255};

    frame = (b->attack_anim > 0.0f ? 12 : 0) + b->facing * 4 + anim_frame(b->timer, 6.0f) % 4;
    draw_shadow(b->pos, b->radius);
    sv_draw(boss_sprites[b->id], frame, b->pos, 1.0f, 0.0f, b->facing == 1 && b->flip, tint);
}

static void draw_shots(void)
{
    for (int i = 0; i < SV_MAX_SHOTS; i++)
    {
        SvShot *s = &sv_run.shots[i];

        if (!s->alive) continue;
        if (s->kind == 1)
        {
            // Pacote de dados do Suporte de TI: um tijolinho verde com rastro
            float angle = atan2f(s->vel.y, s->vel.x) * RAD2DEG;
            Vector2 tail = Vector2Subtract(s->pos, Vector2Scale(Vector2Normalize(s->vel), 11.0f));

            DrawLineEx(tail, s->pos, 3.0f, (Color){90, 230, 140, 120});
            DrawRectanglePro((Rectangle){s->pos.x, s->pos.y, 8.0f, 6.0f}, (Vector2){4.0f, 3.0f}, angle, (Color){20, 70, 50, 255});
            DrawRectanglePro((Rectangle){s->pos.x, s->pos.y, 6.0f, 4.0f}, (Vector2){3.0f, 2.0f}, angle, (Color){120, 240, 170, 255});
            DrawRectanglePro((Rectangle){s->pos.x, s->pos.y, 2.0f, 2.0f}, (Vector2){1.0f, 1.0f}, angle, (Color){240, 255, 245, 255});
            continue;
        }
        sv_draw(SPR_fx_grampeador, anim_frame(s->life, 12.0f) % 4, s->pos, 1.0f, atan2f(s->vel.y, s->vel.x) * RAD2DEG, false, WHITE);
    }
}

static void draw_hazard(SvHazard *h)
{
    float pulse = 0.5f + 0.5f * sinf(sv_run.time * 14.0f);

    if (h->kind == HZ_PROJ)
    {
        if (h->sprite == -1)
        {
            // Cartão de rescisão
            DrawRectanglePro((Rectangle){h->pos.x, h->pos.y, 14, 18}, (Vector2){7, 9}, h->angle + 90.0f, (Color){232, 228, 210, 255});
            DrawRectanglePro((Rectangle){h->pos.x, h->pos.y, 8, 3}, (Vector2){4, 1.5f}, h->angle + 90.0f, (Color){200, 12, 20, 255});
        }
        else if (h->sprite == -2)
        {
            DrawCircleV(h->pos, 6.0f, (Color){20, 60, 90, 255});
            DrawCircleV((Vector2){h->pos.x - 1.5f, h->pos.y - 1.5f}, 2.0f, (Color){160, 200, 210, 255});
        }
        else if (h->sprite == -3)
        {
            DrawCircleLinesV(h->pos, 9.0f, (Color){255, 80, 70, 255});
            DrawCircleLinesV(h->pos, 6.0f, WHITE);
        }
        else if (h->sprite == SPR_bp_onda)
        {
            sv_draw(SPR_bp_onda, anim_frame(sv_run.time, 8.0f) % 4, h->pos, 0.5f, 0.0f, false, WHITE);
        }
        else
        {
            sv_draw((SvSpriteId)h->sprite, anim_frame(sv_run.time, 10.0f) % 4, h->pos, 1.0f, h->sprite == SPR_bp_sobreajuste || h->sprite == SPR_bp_contrato ? 0.0f : h->angle, false, WHITE);
        }
        return;
    }

    if (h->kind == HZ_CIRCLE)
    {
        if (h->warn > 0.0f)
        {
            float fill = 1.0f - h->warn / h->warn_total;
            Color danger = h->damage > 0.0f ? (Color){230, 30, 40, 255} : (Color){230, 190, 60, 255};

            DrawCircleV(h->pos, h->radius, (Color){danger.r, danger.g, danger.b, (unsigned char)(30 + 25 * pulse)});
            DrawCircleV(h->pos, h->radius * fill, (Color){danger.r, danger.g, danger.b, 70});
            DrawCircleLinesV(h->pos, h->radius, danger);
        }
        else
        {
            SvSprite *sprite = &sv_sprites[h->sprite];
            float scale = h->radius * 2.0f / (float)(sprite->frame_width > sprite->frame_height ? sprite->frame_width : sprite->frame_height);

            if (h->tick > 0.0f || h->damage == 0.0f) DrawCircleV(h->pos, h->radius, (Color){60, 220, 120, 40});
            sv_draw((SvSpriteId)h->sprite, anim_frame(sv_run.time, 10.0f) % sprite->frames, h->pos, scale < 1.0f ? 1.0f : scale, 0.0f, false, WHITE);
        }
        return;
    }

    // Feixe
    {
        float angle = h->angle * DEG2RAD;
        Vector2 center = Vector2Add(h->pos, (Vector2){cosf(angle) * h->length / 2.0f, sinf(angle) * h->length / 2.0f});

        if (h->warn > 0.0f)
        {
            float width = h->radius * 2.0f;

            if (h->damage <= 0.0f) width = 3.0f;
            DrawRectanglePro((Rectangle){center.x, center.y, h->length, width}, (Vector2){h->length / 2.0f, width / 2.0f}, h->angle, (Color){230, 30, 40, (unsigned char)(50 + 40 * pulse)});
        }
        else if (h->sprite >= 0)
        {
            sv_draw_stretched((SvSpriteId)h->sprite, anim_frame(sv_run.time, 12.0f) % 4, center, h->length, h->radius * 2.0f + 8.0f, h->angle, WHITE);
        }
    }
}

static void draw_fx(void)
{
    for (int i = 0; i < SV_MAX_FX; i++)
    {
        SvFx *fx = &sv_run.fx[i];
        float progress;

        if (!fx->alive) continue;
        progress = fx->t / fx->life;

        switch (fx->kind)
        {
        case FX_DEATH:
            sv_draw(SPR_cm_morte, (int)(progress * 4.0f), fx->pos, fx->scale, 0.0f, false, WHITE);
            break;
        case FX_EXPLOSION:
            sv_draw(SPR_cm_explosao, (int)(progress * 6.0f), fx->pos, fx->scale, 0.0f, false, WHITE);
            break;
        case FX_LINK:
        {
            Vector2 mid = Vector2Scale(Vector2Add(fx->pos, fx->pos2), 0.5f);
            float length = Vector2Distance(fx->pos, fx->pos2);
            float angle = atan2f(fx->pos2.y - fx->pos.y, fx->pos2.x - fx->pos.x) * RAD2DEG;

            sv_draw_stretched(SPR_fx_modus, (int)(progress * 8.0f) % 4, mid, length, 20.0f, angle, WHITE);
            break;
        }
        case FX_WAVE:
            sv_draw(SPR_fx_novaconversa, (int)(progress * 4.0f) % 4, fx->pos, fx->scale * (0.3f + 0.7f * progress), 0.0f, false, (Color){255, 255, 255, (unsigned char)(230 * (1.0f - progress))});
            break;
        case FX_FLASH:
            DrawCircleV(fx->pos, 26.0f * (1.0f - progress), (Color){255, 255, 255, (unsigned char)(200 * (1.0f - progress))});
            break;
        default:
            break;
        }
    }
}

// -----------------------------------------------------------------------------
// Interface
// -----------------------------------------------------------------------------

static void draw_bar(float x, float y, float w, float h, float fraction, Color fill)
{
    if (fraction < 0.0f) fraction = 0.0f;
    if (fraction > 1.0f) fraction = 1.0f;
    DrawRectangle((int)x - 2, (int)y - 2, (int)w + 4, (int)h + 4, (Color){0, 0, 0, 220});
    DrawRectangle((int)x, (int)y, (int)w, (int)h, COLOR_BAR_BACK);
    DrawRectangle((int)x, (int)y, (int)(w * fraction), (int)h, fill);
}

static void draw_offscreen_boss_arrow(Camera2D camera)
{
    SvBoss *b = &sv_run.boss;
    Vector2 screen;
    float margin = 30.0f * sv_ui();
    float w = (float)GetScreenWidth(), h = (float)GetScreenHeight();

    if (!b->active) return;
    screen = GetWorldToScreen2D(b->pos, camera);
    if (screen.x > 0 && screen.x < w && screen.y > 0 && screen.y < h) return;

    {
        Vector2 center = {w / 2.0f, h / 2.0f};
        Vector2 direction = Vector2Normalize(Vector2Subtract(screen, center));
        Vector2 arrow = {Clamp(center.x + direction.x * w, margin, w - margin), Clamp(center.y + direction.y * h, margin, h - margin)};
        float angle = atan2f(direction.y, direction.x);
        float size = 16.0f * sv_ui();
        Vector2 a = {arrow.x + cosf(angle) * size, arrow.y + sinf(angle) * size};
        Vector2 c = {arrow.x + cosf(angle + 2.5f) * size, arrow.y + sinf(angle + 2.5f) * size};
        Vector2 d = {arrow.x + cosf(angle - 2.5f) * size, arrow.y + sinf(angle - 2.5f) * size};

        DrawTriangle(a, d, c, (Color){230, 30, 40, 255});
    }
}

static void draw_hud(Camera2D camera)
{
    SvPlayer *p = &sv_run.player;
    float ui = sv_ui();
    float w = (float)GetScreenWidth(), h = (float)GetScreenHeight();
    float pad = 18.0f * ui;
    float font = 30.0f * ui;
    int seconds = (int)sv_run.time;
    char text[96];

    // Vida e XP
    {
        float bar_w = 300.0f * ui;

        draw_bar(pad, pad, bar_w, 20.0f * ui, p->hp / p->max_hp, COLOR_HP);
        snprintf(text, sizeof(text), "%d / %d", (int)ceilf(p->hp > 0 ? p->hp : 0), (int)p->max_hp);
        sv_text(text, pad + 8.0f * ui, pad - 2.0f * ui, 22.0f * ui, WHITE);

        draw_bar(pad, pad + 30.0f * ui, bar_w, 12.0f * ui, p->xp / (float)p->xp_needed, COLOR_XP);
        snprintf(text, sizeof(text), "NÍVEL %d", p->level);
        sv_text(text, pad, pad + 46.0f * ui, 24.0f * ui, COLOR_XP);
    }

    // Tempo e ato
    {
        float remaining = SV_RUN_SECONDS - sv_run.time;

        if (remaining < 0.0f) snprintf(text, sizeof(text), "HORA EXTRA +%d:%02d", (int)(-remaining) / 60, (int)(-remaining) % 60);
        else snprintf(text, sizeof(text), "%d:%02d", seconds / 60, seconds % 60);
        sv_text_centered(text, w / 2.0f, pad - 4.0f * ui, 44.0f * ui, remaining < 0.0f ? (Color){230, 60, 60, 255} : WHITE);
        sv_text_centered(sv_act_name(sv_run.act), w / 2.0f, pad + 38.0f * ui, 22.0f * ui, (Color){200, 170, 170, 255});
    }

    // Banco de Horas e abates
    snprintf(text, sizeof(text), "BANCO DE HORAS %d", sv_run.coins);
    sv_text(text, w - pad - sv_text_width(text, font), pad - 2.0f * ui, font, (Color){255, 214, 80, 255});
    snprintf(text, sizeof(text), "ABATES %d", sv_run.kills);
    sv_text(text, w - pad - sv_text_width(text, font * 0.8f), pad + 32.0f * ui, font * 0.8f, (Color){220, 220, 220, 255});

    // Poderes e passivos
    {
        float icon = 24.0f * 2.0f * ui;
        float x = pad;
        float y = h - pad - icon;

        for (int i = 0; i < p->weapon_count; i++)
        {
            SvWeapon *wp = &p->weapons[i];

            DrawRectangle((int)x - 2, (int)y - 2, (int)icon + 4, (int)icon + 4, (Color){0, 0, 0, 180});
            sv_draw(sv_weapon_icon(wp->id), anim_frame(sv_run.time, 4.0f) % 4, (Vector2){x + icon / 2.0f, y + icon / 2.0f}, 2.0f * ui, 0.0f, false, WHITE);
            snprintf(text, sizeof(text), "%d", wp->level);
            sv_text(text, x + icon - 14.0f * ui, y + icon - 20.0f * ui, 20.0f * ui, (Color){255, 220, 120, 255});
            x += icon + 8.0f * ui;
        }

        x = pad;
        y -= icon + 10.0f * ui;
        for (int i = 0; i < p->passive_count; i++)
        {
            SvPassive *ps = &p->passives[i];

            DrawRectangle((int)x - 2, (int)y - 2, (int)(icon * 0.75f) + 4, (int)(icon * 0.75f) + 4, (Color){0, 0, 0, 180});
            sv_draw(sv_passive_icon(ps->id), 0, (Vector2){x + icon * 0.375f, y + icon * 0.375f}, 1.5f * ui, 0.0f, false, WHITE);
            snprintf(text, sizeof(text), "%d", ps->level);
            sv_text(text, x + icon * 0.75f - 12.0f * ui, y + icon * 0.75f - 18.0f * ui, 18.0f * ui, (Color){170, 230, 255, 255});
            x += icon * 0.75f + 8.0f * ui;
        }
    }

    // Barra do chefe
    if (sv_run.boss.active)
    {
        SvBoss *b = &sv_run.boss;
        float bar_w = w * 0.5f;

        sv_text_centered(sv_boss_name(b->id), w / 2.0f, 76.0f * ui, 28.0f * ui, (Color){255, 120, 110, 255});
        draw_bar(w / 2.0f - bar_w / 2.0f, 118.0f * ui, bar_w, 14.0f * ui, b->hp / b->max_hp, (Color){180, 20, 40, 255});
        if (b->id == BOSS_RENE)
        {
            snprintf(text, sizeof(text), "ESCUDOS %d", b->shields);
            sv_text_centered(text, w / 2.0f, 138.0f * ui, 20.0f * ui, (Color){190, 200, 255, 255});
        }
        draw_offscreen_boss_arrow(camera);
    }

    // Aviso grande
    if (sv_run.banner > 0.0f && sv_run.banner_text)
    {
        float alpha = sv_run.banner > 0.6f ? 1.0f : sv_run.banner / 0.6f;

        sv_text_centered(sv_run.banner_text, w / 2.0f, h * 0.28f, 90.0f * ui, (Color){230, 40, 50, (unsigned char)(255 * alpha)});
    }

    // Efeitos de estado
    if (p->invert > 0.0f) sv_text_centered("CONTROLES INVERTIDOS", w / 2.0f, h * 0.2f, 34.0f * ui, (Color){255, 120, 255, 255});
    if (p->root > 0.0f) sv_text_centered("PRESO", w / 2.0f, h * 0.2f + 40.0f * ui, 34.0f * ui, (Color){150, 170, 255, 255});

    // Pouca vida: a borda da tela fica vermelha
    if (p->hp < p->max_hp * 0.3f)
    {
        unsigned char alpha = (unsigned char)(40 + 30 * sinf(sv_run.time * 6.0f));

        DrawRectangleLinesEx((Rectangle){0, 0, w, h}, 18.0f * ui, (Color){200, 0, 20, alpha});
    }
}

void sv_render_game(void)
{
    Camera2D camera = {0};
    float shake = sv_run.shake > 0.0f ? sv_run.shake * 18.0f : 0.0f;

    // Câmera mais aberta: aparecem 720 unidades de mundo na altura
    camera.zoom = GetScreenHeight() / 720.0f;
    if (camera.zoom < 0.75f) camera.zoom = 0.75f;
    camera.offset = (Vector2){GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
    camera.target = (Vector2){sv_run.player.pos.x + sinf(sv_run.time * 90.0f) * shake, sv_run.player.pos.y + cosf(sv_run.time * 80.0f) * shake};

    // A câmera não mostra o vazio fora do mapa
    {
        float half_w = GetScreenWidth() / 2.0f / camera.zoom;
        float half_h = GetScreenHeight() / 2.0f / camera.zoom;

        if (SV_WORLD_W > half_w * 2.0f) camera.target.x = Clamp(camera.target.x, half_w, SV_WORLD_W - half_w);
        if (SV_WORLD_H > half_h * 2.0f) camera.target.y = Clamp(camera.target.y, half_h, SV_WORLD_H - half_h);
    }

    ClearBackground((Color){6, 3, 8, 255});

    BeginMode2D(camera);
    draw_background(camera);
    draw_gems();
    draw_zones();
    for (int i = 0; i < SV_MAX_HAZARDS; i++)
    {
        if (sv_run.hazards[i].alive && sv_run.hazards[i].kind != HZ_PROJ) draw_hazard(&sv_run.hazards[i]);
    }
    draw_world_objects(camera);
    draw_shots();
    draw_lightning();
    draw_prompt_windows();
    for (int i = 0; i < SV_MAX_HAZARDS; i++)
    {
        if (sv_run.hazards[i].alive && sv_run.hazards[i].kind == HZ_PROJ) draw_hazard(&sv_run.hazards[i]);
    }
    draw_fx();
    EndMode2D();

    draw_hud(camera);
}

// -----------------------------------------------------------------------------
// Escolha de nível
// -----------------------------------------------------------------------------

Rectangle sv_levelup_card(int index)
{
    float ui = sv_ui();
    float card_w = 360.0f * ui, card_h = 500.0f * ui, gap = 30.0f * ui;
    float total = card_w * 3.0f + gap * 2.0f;
    float x = (GetScreenWidth() - total) / 2.0f + (float)index * (card_w + gap);
    float y = GetScreenHeight() / 2.0f - card_h / 2.0f + 70.0f * ui;

    return (Rectangle){x, y, card_w, card_h};
}

static void draw_wrapped(const char *text, Rectangle area, float size, Color color)
{
    char line[260] = {0};
    int length = 0;
    float y = area.y;
    char word[48];
    const char *cursor = text;

    while (*cursor)
    {
        int wl = 0;

        while (*cursor == ' ') cursor++;
        while (*cursor && *cursor != ' ' && wl < 47) word[wl++] = *cursor++;
        word[wl] = '\0';
        if (wl == 0) break;

        {
            char trial[256];

            snprintf(trial, sizeof(trial), length ? "%s %s" : "%s%s", line, word);
            if (length && sv_text_width(trial, size) > area.width)
            {
                sv_text_centered(line, area.x + area.width / 2.0f, y, size, color);
                y += size * 1.15f;
                snprintf(line, sizeof(line), "%s", word);
                length = (int)strlen(line);
            }
            else
            {
                snprintf(line, sizeof(line), "%s", trial);
                length = (int)strlen(line);
            }
        }
    }
    if (length) sv_text_centered(line, area.x + area.width / 2.0f, y, size, color);
}

void sv_render_levelup(int selected)
{
    float ui = sv_ui();
    float w = (float)GetScreenWidth();

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){4, 0, 8, 200});
    sv_text_centered("NOVO NÍVEL", w / 2.0f, GetScreenHeight() / 2.0f - 330.0f * ui, 70.0f * ui, (Color){230, 40, 50, 255});
    sv_text_centered("Escolha um poder. Setas ou 1, 2, 3 e Enter.", w / 2.0f, GetScreenHeight() / 2.0f - 250.0f * ui, 28.0f * ui, (Color){210, 200, 210, 255});

    for (int i = 0; i < sv_run.choice_count; i++)
    {
        Rectangle card = sv_levelup_card(i);
        SvChoice *c = &sv_run.choices[i];
        bool is_selected = i == selected;
        int level = sv_choice_level(c);
        char text[64];

        DrawRectangleRec(card, is_selected ? (Color){60, 14, 24, 245} : (Color){24, 12, 28, 235});
        DrawRectangleLinesEx(card, is_selected ? 5.0f * ui : 2.0f * ui, is_selected ? (Color){240, 50, 60, 255} : (Color){110, 70, 90, 255});

        sv_draw(sv_choice_icon(c), anim_frame(GetTime(), 4.0f) % 4, (Vector2){card.x + card.width / 2.0f, card.y + 80.0f * ui}, 4.0f * ui, 0.0f, false, WHITE);
        sv_text_centered(sv_choice_title(c), card.x + card.width / 2.0f, card.y + 145.0f * ui, 31.0f * ui, WHITE);

        if (c->kind == 0 || c->kind == 2) snprintf(text, sizeof(text), "NOVO");
        else if (level > 0) snprintf(text, sizeof(text), "NÍVEL %d", level);
        else snprintf(text, sizeof(text), "BÔNUS");
        sv_text_centered(text, card.x + card.width / 2.0f, card.y + 192.0f * ui, 25.0f * ui, (Color){255, 214, 80, 255});

        draw_wrapped(sv_choice_text(c), (Rectangle){card.x + 16.0f * ui, card.y + 240.0f * ui, card.width - 32.0f * ui, 70.0f * ui}, 21.0f * ui, (Color){170, 165, 178, 255});

        // O que muda: atributo, de -> para
        {
            char changes[SV_CHOICE_LINES][96];
            int n = sv_choice_lines(c, changes);

            DrawRectangle((int)(card.x + 20.0f * ui), (int)(card.y + 318.0f * ui), (int)(card.width - 40.0f * ui), (int)(2.0f * ui), (Color){110, 70, 90, 255});
            for (int k = 0; k < n; k++)
            {
                float size = 23.0f * ui;

                while (size > 14.0f * ui && sv_text_width(changes[k], size) > card.width - 24.0f * ui) size -= 1.0f * ui;
                sv_text_centered(changes[k], card.x + card.width / 2.0f, card.y + (332.0f + 28.0f * (float)k) * ui, size, (Color){255, 226, 120, 255});
            }
        }

        snprintf(text, sizeof(text), "%d", i + 1);
        sv_text(text, card.x + 12.0f * ui, card.y + 6.0f * ui, 28.0f * ui, (Color){150, 130, 150, 255});
    }
}
