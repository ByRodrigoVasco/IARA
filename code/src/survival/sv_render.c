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
#include "sv_light.h"
#include "sv_meta.h"
#include "sv_map.h"
#include "sv_debug.h"
#include "sv_duo.h"

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
            // Moldura da janela (fx_terminal, gerada por tools/sprites/gerar_prompt.py) com sombra embaixo
            DrawRectangle((int)wx + 2, (int)wy + 2, (int)ww, (int)wh, (Color){0, 0, 0, 90});
            sv_draw(SPR_fx_terminal, 0, (Vector2){wx + ww / 2.0f, wy + wh / 2.0f}, 1.0f, 0.0f, false, WHITE);
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

                // No Enter a moldura acende (quadro 1) e depois some junto com o comando
                sv_draw(SPR_fx_terminal, q < 0.15f ? 1 : 0, (Vector2){wx + ww / 2.0f, wy + wh / 2.0f}, 1.0f, 0.0f, false, (Color){255, 255, 255, alpha});
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


// Mapa de Calor (Prof. Baudel): quadradinhos coloridos como num teste de usabilidade,
// azul na borda e vermelho no centro, tremendo de leve; aparece e some suavemente
static Color heat_color(float heat)
{
    static const Color ramp[5] = {{40, 70, 220, 255}, {40, 200, 230, 255}, {70, 210, 90, 255}, {245, 220, 60, 255}, {235, 45, 40, 255}};
    float x = heat * 4.0f;
    int k = (int)x;

    if (k >= 4) return ramp[4];
    if (k < 0) return ramp[0];
    float t = x - (float)k;
    return (Color){(unsigned char)(ramp[k].r + (ramp[k + 1].r - ramp[k].r) * t), (unsigned char)(ramp[k].g + (ramp[k + 1].g - ramp[k].g) * t),
                   (unsigned char)(ramp[k].b + (ramp[k + 1].b - ramp[k].b) * t), 255};
}

static void draw_heatmap(const SvZone *z, float strength)
{
    const float cell = 6.0f, fade = 0.35f;
    float age = z->max_life - z->life;
    float alpha = fminf(1.0f, fminf(age / fade, z->life / fade));
    float t = sv_run.time;
    float x0 = floorf((z->pos.x - z->radius) / cell) * cell, y0 = floorf((z->pos.y - z->radius) / cell) * cell;

    if (alpha <= 0.0f) return;
    for (float y = y0; y <= z->pos.y + z->radius; y += cell)
    {
        for (float x = x0; x <= z->pos.x + z->radius; x += cell)
        {
            float d = Vector2Distance((Vector2){x + cell / 2.0f, y + cell / 2.0f}, z->pos) / z->radius;
            float wobble, heat;
            Color c;

            if (d > 1.0f) continue;
            wobble = 0.14f * sinf(x * 0.11f + t * 2.3f) * cosf(y * 0.09f - t * 1.7f);
            heat = 1.0f - d * 0.95f + wobble + 0.06f * sinf(t * 5.0f);
            if (heat < 0.0f) heat = 0.0f;
            if (heat > 1.0f) heat = 1.0f;
            c = heat_color(heat);
            c.a = (unsigned char)(130.0f * strength * alpha * fminf(1.0f, (1.0f - d) * 5.0f));
            DrawRectangle((int)x, (int)y, (int)cell, (int)cell, c);
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
            if (z->kind == ZK_FIBRA) sv_draw_stretched(SPR_fx_fibra, (int)(progress * 4.0f), center, length, z->radius * 2.0f + 10.0f, z->angle, WHITE);
            else sv_draw_stretched(SPR_fx_ponteiro, (int)(progress * 4.0f), center, length, 13.0f, z->angle, WHITE);
            continue;
        }

        if (z->kind == ZK_CALOR)
        {
            draw_heatmap(z, 1.0f);
            continue;
        }

        if (z->kind == ZK_NEGACAO)
        {
            // Negação: o ¬ cai (com o círculo de aviso no chão) e, no impacto, carimba o chão e solta um anel
            if (z->delay > 0.0f)
            {
                float t = z->delay / 0.5f;

                DrawCircleV(z->pos, z->radius, (Color){170, 120, 255, 22});
                DrawCircleLinesV(z->pos, z->radius, (Color){190, 150, 255, 150});
                sv_draw(SPR_fx_negacao, 0, (Vector2){z->pos.x, z->pos.y - t * 110.0f}, 1.5f, 0.0f, false, WHITE);
            }
            else
            {
                float t = 1.0f - z->life / 0.45f;
                unsigned char fade = (unsigned char)(255.0f * (1.0f - t));

                DrawCircleV(z->pos, z->radius * (0.4f + 0.6f * t), (Color){170, 120, 255, (unsigned char)(fade / 4)});
                DrawCircleLinesV(z->pos, z->radius * (0.4f + 0.6f * t), (Color){220, 200, 255, fade});
                sv_draw(SPR_fx_negacao, 1, z->pos, 1.5f + 0.4f * t, 0.0f, false, (Color){255, 255, 255, fade});
            }
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

        // Negado (confuso): um ¬ pequeno em cima da cabeça
        if (e->confused > 0.0f)
        {
            int x = (int)e->pos.x - 4, y = (int)e->pos.y - 24 + (int)(sinf(e->timer * 6.0f) * 1.5f);

            DrawRectangle(x - 1, y - 1, 10, 4, (Color){20, 10, 30, 220});
            DrawRectangle(x + 6, y - 1, 3, 6, (Color){20, 10, 30, 220});
            DrawRectangle(x, y, 8, 2, (Color){210, 180, 255, 255});
            DrawRectangle(x + 6, y, 2, 4, (Color){210, 180, 255, 255});
        }
    }
}

// Dupla: cor de cada jogador (marcador sobre a cabeça e no HUD)
static const Color duo_colors[SV_DUO_PLAYERS] = {{255, 90, 100, 255}, {90, 170, 255, 255}};

// Dupla: quem caiu fica deitado e cinza, com o anel de quanto falta para levantar
static void draw_fallen_player(SvPlayer *p, int index)
{
    float progress = sv_duo_revive_progress(index);
    char label[8];

    draw_shadow(p->pos, 12.0f);
    sv_draw_player(p->direction, 0, p->pos, 1.0f, (Color){110, 105, 120, 220});
    DrawCircleLinesV(p->pos, 20.0f, (Color){duo_colors[index].r, duo_colors[index].g, duo_colors[index].b, 120});
    if (progress > 0.0f) DrawRing(p->pos, 18.0f, 22.0f, -90.0f, -90.0f + 360.0f * progress, 32, (Color){120, 230, 160, 255});
    snprintf(label, sizeof(label), "J%d", index + 1);
    sv_text_centered(label, p->pos.x, p->pos.y - 40.0f, 12.0f, duo_colors[index]);
    sv_text_centered(progress > 0.0f ? "LEVANTANDO" : "FIQUE AO LADO", p->pos.x, p->pos.y + 18.0f, 9.0f, (Color){230, 230, 235, 230});
}

// Dupla: marcador J1 / J2 sobre a cabeça, para cada um achar o seu personagem
static void draw_duo_marker(SvPlayer *p, int index)
{
    Vector2 tip = {p->pos.x, p->pos.y - 24.0f};
    char label[8];

    DrawTriangle((Vector2){tip.x - 4.0f, tip.y - 6.0f}, tip, (Vector2){tip.x + 4.0f, tip.y - 6.0f}, duo_colors[index]);
    snprintf(label, sizeof(label), "J%d", index + 1);
    sv_text_centered(label, tip.x, tip.y - 20.0f, 11.0f, duo_colors[index]);
}

static void draw_player(void)
{
    SvPlayer *p = &sv_run.player;
    Color tint = WHITE;
    int frame = p->walk_time > 0.0f ? get_walk_frame(p->walk_time) : 0;

    if (sv_run.duo)
    {
        sv_use_player_slot(sv_run.active);
        if (p->down)
        {
            draw_fallen_player(p, sv_run.active);
            sv_use_player_slot(0);
            return;
        }
    }

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
    if (p->pure_flash > 0.0f) sv_text_centered("sem efeito colateral", p->pos.x, p->pos.y - 46.0f - (0.9f - p->pure_flash) * 14.0f, 13.0f, (Color){150, 245, 180, (unsigned char)(255.0f * fminf(1.0f, p->pure_flash * 2.5f))});
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

    if (sv_run.duo)
    {
        draw_duo_marker(p, sv_run.active);
        sv_use_player_slot(0);
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
    int was = sv_run.active;

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

    // Na dupla, cada jogador entra na lista com os seus colegas (index = jogador * SV_MAX_COLEGAS + colega)
    for (int k = 0; k < sv_duo_count(); k++)
    {
        sv_duo_use(k);
        list[count++] = (Drawable){player->pos.y + 10.0f, DR_PLAYER, k};

        if (player->down) continue;
        for (int i = 0; i < player->weapon_count; i++)
        {
            if (player->weapons[i].id == W_COLEGA)
            {
                int n = sv_colega_count();

                for (int c = 0; c < n; c++) list[count++] = (Drawable){sv_run.colega_pos[c].y + 8.0f, DR_COLEGA, k * SV_MAX_COLEGAS + c};
            }
        }
    }
    sv_duo_use(was);

    qsort(list, (size_t)count, sizeof(Drawable), compare_drawables);

    for (int i = 0; i < count; i++)
    {
        switch (list[i].kind)
        {
        case DR_PROP: draw_prop(sv_map_prop(list[i].index)); break;
        case DR_ENEMY: draw_enemy(&sv_run.enemies[list[i].index]); break;
        case DR_BOSS: draw_boss(); break;
        case DR_PLAYER: sv_duo_use(list[i].index); draw_player(); break;
        default: sv_duo_use(list[i].index / SV_MAX_COLEGAS); draw_colega(list[i].index % SV_MAX_COLEGAS); break;
        }
    }
    sv_duo_use(was);
}

// Depois da escuridão: os olhos e o sangue dos monstros voltam com a cor cheia, como se brilhassem
static void draw_glows(Camera2D camera)
{
    float half_w = GetScreenWidth() / 2.0f / camera.zoom + 40.0f, half_h = GetScreenHeight() / 2.0f / camera.zoom + 40.0f;

    BeginMode2D(camera);
    // O Mapa de Calor é um dado na tela do teste de usabilidade: brilha também no escuro, mais fraco
    for (int i = 0; i < SV_MAX_ZONES; i++)
    {
        if (sv_run.zones[i].alive && sv_run.zones[i].kind == ZK_CALOR) draw_heatmap(&sv_run.zones[i], 0.5f);
    }
    for (int i = 0; i < SV_MAX_ENEMIES; i++)
    {
        SvEnemy *e = &sv_run.enemies[i];

        if (!e->alive || fabsf(e->pos.x - camera.target.x) > half_w || fabsf(e->pos.y - camera.target.y) > half_h) continue;
        sv_draw_glow(sv_enemy_sprite(e->type), e->facing * 4 + anim_frame(e->anim, 6.0f) % 4, e->pos, e->facing == 1 && e->flip, (Color){255, 255, 255, e->type == EN_CRACHA ? 190 : 230});
    }
    if (sv_run.boss.active)
    {
        SvBoss *b = &sv_run.boss;

        sv_draw_glow(boss_sprites[b->id], (b->attack_anim > 0.0f ? 12 : 0) + b->facing * 4 + anim_frame(b->timer, 6.0f) % 4, b->pos, b->facing == 1 && b->flip, WHITE);
    }
    EndMode2D();
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
        if (s->kind == 2)
        {
            // Nota fiscal: uma folha branca com linhas e o carimbo verde, girando e deixando um rastro
            Vector2 tail = Vector2Subtract(s->pos, Vector2Scale(Vector2Normalize(s->vel), 16.0f));

            DrawLineEx(tail, s->pos, 5.0f, (Color){240, 242, 236, 70});
            DrawRectanglePro((Rectangle){s->pos.x, s->pos.y + 2.0f, 15.0f, 19.0f}, (Vector2){7.5f, 9.5f}, s->spin, (Color){0, 0, 0, 70});
            DrawRectanglePro((Rectangle){s->pos.x, s->pos.y, 15.0f, 19.0f}, (Vector2){7.5f, 9.5f}, s->spin, (Color){10, 10, 13, 255});
            DrawRectanglePro((Rectangle){s->pos.x, s->pos.y, 13.0f, 17.0f}, (Vector2){6.5f, 8.5f}, s->spin, (Color){240, 242, 236, 255});
            for (int line = 0; line < 3; line++)
            {
                float offset = -4.0f + (float)line * 3.5f;
                Vector2 rotated = Vector2Rotate((Vector2){0.0f, offset}, s->spin * DEG2RAD);

                DrawRectanglePro((Rectangle){s->pos.x + rotated.x, s->pos.y + rotated.y, 8.0f, 1.5f}, (Vector2){4.0f, 0.75f}, s->spin, (Color){122, 130, 138, 255});
            }
            {
                Vector2 stamp = Vector2Rotate((Vector2){3.0f, 5.0f}, s->spin * DEG2RAD);

                DrawRectanglePro((Rectangle){s->pos.x + stamp.x, s->pos.y + stamp.y, 5.0f, 4.0f}, (Vector2){2.5f, 2.0f}, s->spin, (Color){64, 176, 104, 255});
            }
            continue;
        }
        if (s->kind == 3)
        {
            // Recursão: o f() voa reto, com um rastro verde; cada cópia sai menor que a chamada anterior
            float size = s->radius / 7.0f;   // O f() da primeira chamada sai um pouco maior que o sprite
            Vector2 tail = Vector2Subtract(s->pos, Vector2Scale(Vector2Normalize(s->vel), 14.0f * size));

            DrawLineEx(tail, s->pos, 4.0f * size, (Color){60, 190, 110, 90});
            sv_draw(SPR_fx_recursao, 0, s->pos, size, 0.0f, false, WHITE);
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
            // Canto da IARA: o olho de sonar com anéis ciano se abrindo
            sv_draw(SPR_bp_nota, anim_frame(sv_run.time, 8.0f) % 4, h->pos, 1.0f, 0.0f, false, WHITE);
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

            int frame = anim_frame(sv_run.time, 10.0f) % sprite->frames;

            if (h->sprite == SPR_bp_falsa)
            {
                // Resposta Confiante: parece segura até machucar pela primeira vez (1 s), depois mostra a mentira
                float age = h->max_life - h->life;

                if (age < 1.0f) frame = anim_frame(sv_run.time, 3.0f) % 2;
                else frame = age < 1.25f ? 2 : 2 + anim_frame(sv_run.time, 3.0f) % 2;
            }
            if (h->tick > 0.0f || h->damage == 0.0f) DrawCircleV(h->pos, h->radius, (Color){60, 220, 120, 40});
            sv_draw((SvSpriteId)h->sprite, frame, h->pos, scale < 1.0f ? 1.0f : scale, 0.0f, false, WHITE);
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

// Dupla: quem caiu e está fora da tela ganha uma seta na borda, na cor dele, com "J1" ou "J2",
// para o parceiro saber para onde voltar e levantá-lo
static void draw_offscreen_partner_arrow(Camera2D camera)
{
    float ui = sv_ui();
    float margin = 46.0f * ui;
    float w = (float)GetScreenWidth(), h = (float)GetScreenHeight();

    if (!sv_run.duo) return;

    for (int i = 0; i < SV_DUO_PLAYERS; i++)
    {
        SvPlayer *p = sv_duo_player(i);
        Vector2 screen;
        char label[8];

        if (!p->down) continue;
        screen = GetWorldToScreen2D(p->pos, camera);
        if (screen.x > 0 && screen.x < w && screen.y > 0 && screen.y < h) continue;

        {
            Vector2 center = {w / 2.0f, h / 2.0f};
            Vector2 direction = Vector2Normalize(Vector2Subtract(screen, center));
            Vector2 arrow = {Clamp(center.x + direction.x * w, margin, w - margin), Clamp(center.y + direction.y * h, margin, h - margin)};
            float angle = atan2f(direction.y, direction.x);
            float size = 18.0f * ui;
            Vector2 a = {arrow.x + cosf(angle) * size, arrow.y + sinf(angle) * size};
            Vector2 c = {arrow.x + cosf(angle + 2.5f) * size, arrow.y + sinf(angle + 2.5f) * size};
            Vector2 d = {arrow.x + cosf(angle - 2.5f) * size, arrow.y + sinf(angle - 2.5f) * size};

            DrawTriangle(a, d, c, duo_colors[i]);
            snprintf(label, sizeof(label), "J%d", i + 1);
            sv_text_centered(label, arrow.x - cosf(angle) * size * 1.6f, arrow.y - sinf(angle) * size * 1.6f - 12.0f * ui, 24.0f * ui, duo_colors[i]);
        }
    }
}

// O botão fica no canto de baixo à direita: é a única parte da tela que o HUD deixa livre
// (vida no alto à esquerda, tempo no centro, Banco de Horas no alto à direita, poderes embaixo à esquerda).
// A largura vem do maior texto que ele mostra, então o texto nunca passa da borda.
#define SPEED_BUTTON_TEXT_SIZE 24.0f

Rectangle sv_speed_button_rect(void)
{
    float ui = sv_ui();
    float pad = 18.0f * ui;
    float width = sv_text_width("TEMPO  x3  [TAB]", SPEED_BUTTON_TEXT_SIZE * ui) + 36.0f * ui;
    float height = 44.0f * ui;

    return (Rectangle){(float)GetScreenWidth() - pad - width, (float)GetScreenHeight() - pad - height, width, height};
}

// Dupla: vida, XP, nível e poderes de um jogador, no lado esquerdo (jogador 1) ou direito (jogador 2)
static void draw_duo_panel(int index)
{
    SvPlayer *p = sv_duo_player(index);
    bool right = index == 1;
    float ui = sv_ui();
    float w = (float)GetScreenWidth(), h = (float)GetScreenHeight();
    float pad = 18.0f * ui;
    float bar_w = 300.0f * ui;
    float x = right ? w - pad - bar_w : pad;
    float icon = 24.0f * 2.0f * ui;
    char text[96];

    snprintf(text, sizeof(text), "J%d", index + 1);
    sv_text(text, right ? x - 46.0f * ui : x + bar_w + 12.0f * ui, pad - 6.0f * ui, 34.0f * ui, duo_colors[index]);

    draw_bar(x, pad, bar_w, 20.0f * ui, p->down ? 0.0f : p->hp / p->max_hp, COLOR_HP);
    if (p->down) snprintf(text, sizeof(text), "CAIU");
    else snprintf(text, sizeof(text), "%d / %d", (int)ceilf(p->hp > 0 ? p->hp : 0), (int)p->max_hp);
    sv_text(text, x + 8.0f * ui, pad - 2.0f * ui, 22.0f * ui, WHITE);

    draw_bar(x, pad + 30.0f * ui, bar_w, 12.0f * ui, p->xp / (float)p->xp_needed, COLOR_XP);
    snprintf(text, sizeof(text), "NÍVEL %d", p->level);
    sv_text(text, right ? x + bar_w - sv_text_width(text, 24.0f * ui) : x, pad + 46.0f * ui, 24.0f * ui, COLOR_XP);

    // Poderes embaixo: o jogador 1 a partir da esquerda, o jogador 2 a partir da direita, acima do botão de tempo
    {
        float y = h - pad - icon - (right ? 54.0f * ui : 0.0f);
        float step = icon + 8.0f * ui;
        float small = icon * 0.75f;

        x = right ? w - pad - icon : pad;
        for (int i = 0; i < p->weapon_count; i++)
        {
            SvWeapon *wp = &p->weapons[i];

            DrawRectangle((int)x - 2, (int)y - 2, (int)icon + 4, (int)icon + 4, (Color){0, 0, 0, 180});
            sv_draw(sv_weapon_icon(wp->id), anim_frame(sv_run.time, 4.0f) % 4, (Vector2){x + icon / 2.0f, y + icon / 2.0f}, 2.0f * ui, 0.0f, false, WHITE);
            snprintf(text, sizeof(text), "%d", wp->level);
            sv_text(text, x + icon - 14.0f * ui, y + icon - 20.0f * ui, 20.0f * ui, (Color){255, 220, 120, 255});
            x += right ? -step : step;
        }

        x = right ? w - pad - small : pad;
        y -= icon + 10.0f * ui;
        for (int i = 0; i < p->passive_count; i++)
        {
            SvPassive *ps = &p->passives[i];

            DrawRectangle((int)x - 2, (int)y - 2, (int)small + 4, (int)small + 4, (Color){0, 0, 0, 180});
            sv_draw(sv_passive_icon(ps->id), 0, (Vector2){x + small / 2.0f, y + small / 2.0f}, 1.5f * ui, 0.0f, false, WHITE);
            snprintf(text, sizeof(text), "%d", ps->level);
            sv_text(text, x + small - 12.0f * ui, y + small - 18.0f * ui, 18.0f * ui, (Color){170, 230, 255, 255});
            x += right ? -(small + 8.0f * ui) : small + 8.0f * ui;
        }
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

    // Vida e XP (na dupla, um painel para cada jogador)
    if (sv_run.duo)
    {
        draw_duo_panel(0);
        draw_duo_panel(1);
    }
    else
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

    // Botão de velocidade do tempo
    if (sv_run.phase == SV_PLAYING)
    {
        Rectangle button = sv_speed_button_rect();
        bool hot = sv_time_scale > 1;
        bool hover = CheckCollisionPointRec(GetMousePosition(), button);

        DrawRectangleRec(button, hot ? (Color){120, 40, 40, 230} : (Color){20, 20, 28, hover ? 230 : 170});
        DrawRectangleLinesEx(button, 2.0f * ui, hot ? (Color){255, 120, 100, 255} : (Color){160, 160, 175, 255});
        snprintf(text, sizeof(text), "TEMPO  x%d  [TAB]", sv_time_scale);
        sv_text_centered(text, button.x + button.width / 2.0f, button.y + (button.height - SPEED_BUTTON_TEXT_SIZE * ui) / 2.0f - 2.0f * ui, SPEED_BUTTON_TEXT_SIZE * ui, WHITE);
    }

    // Banco de Horas e abates (na dupla o alto à direita é do jogador 2, então eles descem para o centro, embaixo)
    if (sv_run.duo)
    {
        snprintf(text, sizeof(text), "BANCO DE HORAS %d    ABATES %d", sv_run.coins, sv_run.kills);
        sv_text_centered(text, w / 2.0f, h - pad - font, font * 0.8f, (Color){255, 214, 80, 255});
    }
    else
    {
        snprintf(text, sizeof(text), "BANCO DE HORAS %d", sv_run.coins);
        sv_text(text, w - pad - sv_text_width(text, font), pad - 2.0f * ui, font, (Color){255, 214, 80, 255});
        snprintf(text, sizeof(text), "ABATES %d", sv_run.kills);
        sv_text(text, w - pad - sv_text_width(text, font * 0.8f), pad + 32.0f * ui, font * 0.8f, (Color){220, 220, 220, 255});
    }

    // Poderes e passivos
    if (!sv_run.duo)
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

    // Barra do chefe: logo abaixo do ato (que termina em 78), sem encostar em nada do HUD
    if (sv_run.boss.active)
    {
        SvBoss *b = &sv_run.boss;
        float bar_w = fminf(w * 0.5f, 900.0f * ui);

        sv_text_centered(sv_boss_name(b->id), w / 2.0f, 92.0f * ui, 28.0f * ui, (Color){255, 120, 110, 255});
        draw_bar(w / 2.0f - bar_w / 2.0f, 128.0f * ui, bar_w, 14.0f * ui, b->hp / b->max_hp, (Color){180, 20, 40, 255});
        if (b->id == BOSS_RENE)
        {
            snprintf(text, sizeof(text), "ESCUDOS %d", b->shields);
            sv_text_centered(text, w / 2.0f, 148.0f * ui, 20.0f * ui, (Color){190, 200, 255, 255});
        }
        draw_offscreen_boss_arrow(camera);
    }
    draw_offscreen_partner_arrow(camera);

    // Aviso grande
    if (sv_run.banner > 0.0f && sv_run.banner_text)
    {
        float alpha = sv_run.banner > 0.6f ? 1.0f : sv_run.banner / 0.6f;

        sv_text_centered(sv_run.banner_text, w / 2.0f, h * 0.28f, 90.0f * ui, (Color){230, 40, 50, (unsigned char)(255 * alpha)});
    }

    // Efeitos de estado
    if (sv_run.duo)
    {
        for (int k = 0; k < SV_DUO_PLAYERS; k++)
        {
            SvPlayer *q = sv_duo_player(k);

            if (q->invert > 0.0f) sv_text_centered(k == 0 ? "J1: CONTROLES INVERTIDOS" : "J2: CONTROLES INVERTIDOS", w / 2.0f, h * 0.2f + (float)k * 80.0f * ui, 34.0f * ui, (Color){255, 120, 255, 255});
            if (q->root > 0.0f) sv_text_centered(k == 0 ? "J1: PRESO" : "J2: PRESO", w / 2.0f, h * 0.2f + 40.0f * ui + (float)k * 80.0f * ui, 34.0f * ui, (Color){150, 170, 255, 255});
        }
    }
    else
    {
        if (p->invert > 0.0f) sv_text_centered("CONTROLES INVERTIDOS", w / 2.0f, h * 0.2f, 34.0f * ui, (Color){255, 120, 255, 255});
        if (p->root > 0.0f) sv_text_centered("PRESO", w / 2.0f, h * 0.2f + 40.0f * ui, 34.0f * ui, (Color){150, 170, 255, 255});
    }

    // Pouca vida: a borda da tela fica vermelha (na dupla, se alguém de pé estiver mal ou tiver caído)
    if (sv_run.duo)
    {
        bool danger = false;

        for (int k = 0; k < SV_DUO_PLAYERS; k++)
        {
            SvPlayer *q = sv_duo_player(k);

            if (q->down || q->hp < q->max_hp * 0.3f) danger = true;
        }
        if (danger)
        {
            unsigned char alpha = (unsigned char)(40 + 30 * sinf(sv_run.time * 6.0f));

            DrawRectangleLinesEx((Rectangle){0, 0, w, h}, 18.0f * ui, (Color){200, 0, 20, alpha});
        }
    }
    else if (p->hp < p->max_hp * 0.3f)
    {
        unsigned char alpha = (unsigned char)(40 + 30 * sinf(sv_run.time * 6.0f));

        DrawRectangleLinesEx((Rectangle){0, 0, w, h}, 18.0f * ui, (Color){200, 0, 20, alpha});
    }
}

// Dupla: quando a câmera troca de alvo (o meio dos dois <-> quem ficou de pé), ela desliza até o alvo
// novo em vez de pular. Guarda a distância que falta e a desconta aos poucos; fora das trocas ela
// segue o alvo exatamente, como antes.
#define CAMERA_SWITCH_SPEED 6.0f   // Quanto maior, mais rápido ela chega (fração por segundo, em escala exponencial)

static Vector2 duo_camera_focus(void)
{
    static Vector2 last_target = {0};
    static Vector2 pending = {0};
    static int last_mode = -2;
    static float last_time = -1.0f;
    Vector2 focus = sv_duo_focus();
    int mode = sv_duo_focus_player();
    float dt = GetFrameTime();

    // Solo ou partida nova: sem deslize
    if (!sv_run.duo || sv_run.time < last_time || last_mode == -2)
    {
        pending = (Vector2){0};
        last_mode = mode;
    }
    else if (mode != last_mode)
    {
        pending = Vector2Subtract(last_target, focus);   // Começa de onde a câmera estava
        last_mode = mode;
    }

    pending = Vector2Scale(pending, expf(-CAMERA_SWITCH_SPEED * dt));
    if (Vector2Length(pending) < 0.5f) pending = (Vector2){0};

    last_time = sv_run.time;
    last_target = Vector2Add(focus, pending);
    return last_target;
}

void sv_render_game(void)
{
    Camera2D camera = {0};
    float shake = sv_run.shake > 0.0f ? sv_run.shake * 18.0f : 0.0f;

    // Câmera mais aberta: aparecem 720 unidades de mundo na altura
    camera.zoom = GetScreenHeight() / 720.0f;
    if (camera.zoom < 0.75f) camera.zoom = 0.75f;
    camera.offset = (Vector2){GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
    {
        // No solo, a posição do jogador; na dupla, o meio entre os dois, ou quem ficou de pé se o outro caiu
        Vector2 focus = duo_camera_focus();

        camera.target = (Vector2){focus.x + sinf(sv_run.time * 90.0f) * shake, focus.y + cosf(sv_run.time * 80.0f) * shake};
    }

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
    if (sv_debug_mode) sv_debug_draw_world();
    EndMode2D();

    sv_light_draw(camera);
    draw_glows(camera);
    draw_hud(camera);
    if (sv_debug_mode) sv_debug_draw_hud();
}

// -----------------------------------------------------------------------------
// Escolha de nível
// -----------------------------------------------------------------------------

Rectangle sv_levelup_card(int index)
{
    float ui = sv_ui();
    float card_w = SV_CARD_W * ui, card_h = SV_CARD_H * ui, gap = 30.0f * ui;
    float total = card_w * 3.0f + gap * 2.0f;
    float x = (GetScreenWidth() - total) / 2.0f + (float)index * (card_w + gap);
    float y = GetScreenHeight() / 2.0f - card_h / 2.0f + 60.0f * ui;

    return (Rectangle){x, y, card_w, card_h};
}

// Quebra o texto em linhas que cabem em width (mesma unidade de size). Devolve quantas linhas o texto tem,
// e preenche até max_lines delas.
static int wrap_text(const char *text, float size, float width, char lines[][SV_CARD_LINE_CHARS], int max_lines)
{
    int count = 0;
    char line[SV_CARD_LINE_CHARS] = {0};
    const char *cursor = text;

    while (*cursor)
    {
        char word[48];
        char trial[SV_CARD_LINE_CHARS];
        int wl = 0;

        while (*cursor == ' ') cursor++;
        while (*cursor && *cursor != ' ' && wl < 47) word[wl++] = *cursor++;
        word[wl] = '\0';
        if (wl == 0) break;

        snprintf(trial, sizeof(trial), line[0] ? "%s %s" : "%s%s", line, word);
        if (line[0] && sv_text_width(trial, size) > width)
        {
            if (count < max_lines) snprintf(lines[count], SV_CARD_LINE_CHARS, "%s", line);
            count++;
            snprintf(line, sizeof(line), "%s", word);
        }
        else
        {
            snprintf(line, sizeof(line), "%s", trial);
        }
    }
    if (line[0])
    {
        if (count < max_lines) snprintf(lines[count], SV_CARD_LINE_CHARS, "%s", line);
        count++;
    }
    return count;
}

// Diminui o texto até ele caber na largura, sem passar de um mínimo.
static float fit_text_size(const char *text, float size, float width, float min_size)
{
    while (size > min_size && sv_text_width(text, size) > width) size -= 1.0f;
    return size;
}

void sv_levelup_layout(const SvChoice *choice, SvCardLayout *layout)
{
    char changes[SV_CHOICE_LINES][96];
    char lines[SV_CARD_MAX_DESC_LINES][SV_CARD_LINE_CHARS];
    int change_count = sv_choice_lines(choice, changes);
    float desc_width = SV_CARD_W - 2.0f * SV_CARD_DESC_MARGIN;

    layout->title_size = fit_text_size(sv_choice_title(choice), SV_CARD_TITLE_SIZE, SV_CARD_W - 2.0f * SV_CARD_TEXT_MARGIN, SV_CARD_TITLE_MIN);
    layout->change_count = change_count;
    layout->change_size = SV_CARD_CHANGE_SIZE;
    layout->desc_size = SV_CARD_DESC_SIZE;

    // A descrição encolhe até ela e as linhas de "de -> para" caberem juntas dentro da carta
    for (;;)
    {
        layout->desc_lines = wrap_text(sv_choice_text(choice), layout->desc_size, desc_width, lines, SV_CARD_MAX_DESC_LINES);
        layout->divider_y = SV_CARD_DESC_Y + layout->desc_lines * layout->desc_size * 1.15f + SV_CARD_GAP;
        layout->changes_y = layout->divider_y + SV_CARD_GAP;
        layout->bottom = layout->changes_y + change_count * SV_CARD_CHANGE_STEP;
        if (layout->bottom <= SV_CARD_H - SV_CARD_BOTTOM_MARGIN || layout->desc_size <= SV_CARD_DESC_MIN) break;
        layout->desc_size -= 1.0f;
    }
}

void sv_render_levelup(int selected)
{
    float ui = sv_ui();
    float w = (float)GetScreenWidth();

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){4, 0, 8, 200});
    if (sv_run.duo) sv_text_centered(sv_run.chooser == 0 ? "NOVO NÍVEL: JOGADOR 1 ESCOLHE" : "NOVO NÍVEL: JOGADOR 2 ESCOLHE", w / 2.0f, GetScreenHeight() / 2.0f - 340.0f * ui, 70.0f * ui, duo_colors[sv_run.chooser]);
    else sv_text_centered("NOVO NÍVEL", w / 2.0f, GetScreenHeight() / 2.0f - 340.0f * ui, 70.0f * ui, (Color){230, 40, 50, 255});
    sv_text_centered("Escolha um poder. Setas ou 1, 2, 3 e Enter.", w / 2.0f, GetScreenHeight() / 2.0f - 260.0f * ui, 28.0f * ui, (Color){210, 200, 210, 255});

    for (int i = 0; i < sv_run.choice_count; i++)
    {
        Rectangle card = sv_levelup_card(i);
        SvChoice *c = &sv_run.choices[i];
        SvCardLayout layout;
        bool is_selected = i == selected;
        int level = sv_choice_level(c);
        float center = card.x + card.width / 2.0f;
        char text[64];

        sv_levelup_layout(c, &layout);

        DrawRectangleRec(card, is_selected ? (Color){60, 14, 24, 245} : (Color){24, 12, 28, 235});
        DrawRectangleLinesEx(card, is_selected ? 5.0f * ui : 2.0f * ui, is_selected ? (Color){240, 50, 60, 255} : (Color){110, 70, 90, 255});

        sv_draw(sv_choice_icon(c), anim_frame(GetTime(), 4.0f) % 4, (Vector2){center, card.y + SV_CARD_ICON_Y * ui}, 4.0f * ui, 0.0f, false, WHITE);
        sv_text_centered(sv_choice_title(c), center, card.y + SV_CARD_TITLE_Y * ui, layout.title_size * ui, WHITE);

        if (c->kind == 0 || c->kind == 2) snprintf(text, sizeof(text), "NOVO");
        else if (level > 0) snprintf(text, sizeof(text), "NÍVEL %d", level);
        else snprintf(text, sizeof(text), "BÔNUS");
        sv_text_centered(text, center, card.y + SV_CARD_TAG_Y * ui, SV_CARD_TAG_SIZE * ui, (Color){255, 214, 80, 255});

        // Descrição, linha por linha
        {
            char lines[SV_CARD_MAX_DESC_LINES][SV_CARD_LINE_CHARS];
            int count = wrap_text(sv_choice_text(c), layout.desc_size * ui, (SV_CARD_W - 2.0f * SV_CARD_DESC_MARGIN) * ui, lines, SV_CARD_MAX_DESC_LINES);

            if (count > SV_CARD_MAX_DESC_LINES) count = SV_CARD_MAX_DESC_LINES;
            for (int k = 0; k < count; k++)
            {
                sv_text_centered(lines[k], center, card.y + (SV_CARD_DESC_Y + (float)k * layout.desc_size * 1.15f) * ui, layout.desc_size * ui, (Color){170, 165, 178, 255});
            }
        }

        // O que muda: atributo, de -> para
        {
            char changes[SV_CHOICE_LINES][96];
            int n = sv_choice_lines(c, changes);

            DrawRectangle((int)(card.x + 20.0f * ui), (int)(card.y + layout.divider_y * ui), (int)(card.width - 40.0f * ui), (int)(2.0f * ui + 0.5f), (Color){110, 70, 90, 255});
            for (int k = 0; k < n; k++)
            {
                float size = fit_text_size(changes[k], layout.change_size * ui, card.width - 2.0f * SV_CARD_TEXT_MARGIN * ui, SV_CARD_CHANGE_MIN * ui);

                sv_text_centered(changes[k], center, card.y + (layout.changes_y + (float)k * SV_CARD_CHANGE_STEP) * ui, size, (Color){255, 226, 120, 255});
            }
        }

        snprintf(text, sizeof(text), "%d", i + 1);
        sv_text(text, card.x + 12.0f * ui, card.y + 6.0f * ui, 28.0f * ui, (Color){150, 130, 150, 255});
    }
}
