#include <math.h>
#include <stdio.h>
#include <string.h>
#include "raymath.h"
#include "characters.h"
#include "fonts.h"
#include "helper.h"
#include "macros.h"
#include "sv_render.h"
#include "sv_meta.h"

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
            else sv_draw_stretched(SPR_fx_ponteiro, (int)(progress * 4.0f), center, length, 13.0f, z->angle, WHITE);
            continue;
        }

        if (z->kind == ZK_CANECA)
        {
            if (z->delay > 0.0f)
            {
                float t = z->delay / 0.65f;

                DrawCircleLinesV(z->pos, z->radius, (Color){255, 200, 120, 120});
                sv_draw(SPR_fx_caneca, t > 0.5f ? 0 : 1, (Vector2){z->pos.x, z->pos.y - t * 90.0f}, 1.0f, 0.0f, false, WHITE);
            }
        }
        else if (z->kind == ZK_DEPLOY)
        {
            Vector2 top = {z->pos.x, z->pos.y - 30.0f};

            if (z->delay > 0.0f)
            {
                DrawCircleLinesV(z->pos, z->radius, (Color){255, 230, 90, 140});
                sv_draw(SPR_fx_deploy, 0, top, 1.0f, 0.0f, false, WHITE);
                sv_draw(SPR_fx_deploy, 3, z->pos, 1.0f, 0.0f, false, WHITE);
            }
            else
            {
                progress = 1.0f - z->life / z->max_life;
                sv_draw(SPR_fx_deploy, progress < 0.5f ? 1 : 2, top, 1.0f, 0.0f, false, WHITE);
                sv_draw(SPR_fx_deploy, 3, z->pos, 1.0f, 0.0f, false, WHITE);
            }
        }
    }
}

static void draw_enemies(void)
{
    for (int i = 0; i < SV_MAX_ENEMIES; i++)
    {
        SvEnemy *e = &sv_run.enemies[i];
        Color tint = WHITE;
        int frame;

        if (!e->alive) continue;
        if (e->hurt_flash > 0.0f) tint = (Color){255, 140, 140, 255};
        else if (e->confused > 0.0f) tint = (Color){190, 150, 255, 255};

        frame = e->facing * 4 + anim_frame(e->anim, 6.0f) % 4;
        draw_shadow(e->pos, 9.0f);
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
            float radius = (52.0f + 8.0f * w->level) * p->area_mult;

            sv_draw(SPR_fx_naoperturbe, anim_frame(sv_run.time, 8.0f) % 4, p->pos, radius * 2.0f / 64.0f, 0.0f, false, (Color){255, 255, 255, 110});
        }
    }

    draw_shadow(p->pos, 10.0f);
    sv_draw_player(p->direction, frame, p->pos, 1.0f, tint);

    for (int i = 0; i < p->weapon_count; i++)
    {
        SvWeapon *w = &p->weapons[i];

        if (w->id == W_POSTITS)
        {
            float radius = (50.0f + 4.0f * w->level) * p->area_mult;

            sv_draw(SPR_fx_postits, anim_frame(sv_run.time, 6.0f) % 4, p->pos, radius / 22.0f, w->angle * RAD2DEG, false, WHITE);
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

    camera.zoom = GetScreenHeight() / 540.0f;
    if (camera.zoom < 1.0f) camera.zoom = 1.0f;
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
    draw_enemies();
    draw_boss();
    draw_player();
    draw_shots();
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
    float card_w = 330.0f * ui, card_h = 400.0f * ui, gap = 36.0f * ui;
    float total = card_w * 3.0f + gap * 2.0f;
    float x = (GetScreenWidth() - total) / 2.0f + (float)index * (card_w + gap);
    float y = GetScreenHeight() / 2.0f - card_h / 2.0f + 30.0f * ui;

    return (Rectangle){x, y, card_w, card_h};
}

static void draw_wrapped(const char *text, Rectangle area, float size, Color color)
{
    char line[200] = {0};
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
    sv_text_centered("NOVO NÍVEL", w / 2.0f, GetScreenHeight() / 2.0f - 300.0f * ui, 70.0f * ui, (Color){230, 40, 50, 255});
    sv_text_centered("Escolha um poder. Setas ou 1, 2, 3 e Enter.", w / 2.0f, GetScreenHeight() / 2.0f - 220.0f * ui, 28.0f * ui, (Color){210, 200, 210, 255});

    for (int i = 0; i < sv_run.choice_count; i++)
    {
        Rectangle card = sv_levelup_card(i);
        SvChoice *c = &sv_run.choices[i];
        bool is_selected = i == selected;
        int level = sv_choice_level(c);
        char text[64];

        DrawRectangleRec(card, is_selected ? (Color){60, 14, 24, 245} : (Color){24, 12, 28, 235});
        DrawRectangleLinesEx(card, is_selected ? 5.0f * ui : 2.0f * ui, is_selected ? (Color){240, 50, 60, 255} : (Color){110, 70, 90, 255});

        sv_draw(sv_choice_icon(c), anim_frame(GetTime(), 4.0f) % 4, (Vector2){card.x + card.width / 2.0f, card.y + 90.0f * ui}, 4.0f * ui, 0.0f, false, WHITE);
        sv_text_centered(sv_choice_title(c), card.x + card.width / 2.0f, card.y + 160.0f * ui, 32.0f * ui, WHITE);

        if (c->kind == 0 || c->kind == 2) snprintf(text, sizeof(text), "NOVO");
        else if (level > 0) snprintf(text, sizeof(text), "NÍVEL %d", level);
        else snprintf(text, sizeof(text), "BÔNUS");
        sv_text_centered(text, card.x + card.width / 2.0f, card.y + 205.0f * ui, 26.0f * ui, (Color){255, 214, 80, 255});

        draw_wrapped(sv_choice_text(c), (Rectangle){card.x + 20.0f * ui, card.y + 250.0f * ui, card.width - 40.0f * ui, 120.0f * ui}, 26.0f * ui, (Color){200, 195, 205, 255});

        snprintf(text, sizeof(text), "%d", i + 1);
        sv_text(text, card.x + 12.0f * ui, card.y + 6.0f * ui, 28.0f * ui, (Color){150, 130, 150, 255});
    }
}
