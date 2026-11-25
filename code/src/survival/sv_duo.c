#include <math.h>
#include <string.h>
#include "raymath.h"
#include "sv_duo.h"
#include "sv_audio.h"
#include "sv_map.h"

// Modo dupla. As regras estão explicadas em sv_duo.h.

bool sv_duo_mode = false;

// -----------------------------------------------------------------------------
// Quem está em cena
// -----------------------------------------------------------------------------

static void seat_save(SvSeat *seat)
{
    seat->player = sv_run.player;
    memcpy(seat->trail, sv_run.trail, sizeof(seat->trail));
    seat->trail_timer = sv_run.trail_timer;
    memcpy(seat->colega_pos, sv_run.colega_pos, sizeof(seat->colega_pos));
    memcpy(seat->colega_dir, sv_run.colega_dir, sizeof(seat->colega_dir));
    memcpy(seat->colega_cd, sv_run.colega_cd, sizeof(seat->colega_cd));
    memcpy(seat->colega_attack, sv_run.colega_attack, sizeof(seat->colega_attack));
    memcpy(seat->colega_walk, sv_run.colega_walk, sizeof(seat->colega_walk));
    memcpy(seat->colega_moving, sv_run.colega_moving, sizeof(seat->colega_moving));
    seat->colega_heal = sv_run.colega_heal;
    seat->colega_heal_flash = sv_run.colega_heal_flash;
}

static void seat_load(const SvSeat *seat)
{
    sv_run.player = seat->player;
    memcpy(sv_run.trail, seat->trail, sizeof(sv_run.trail));
    sv_run.trail_timer = seat->trail_timer;
    memcpy(sv_run.colega_pos, seat->colega_pos, sizeof(sv_run.colega_pos));
    memcpy(sv_run.colega_dir, seat->colega_dir, sizeof(sv_run.colega_dir));
    memcpy(sv_run.colega_cd, seat->colega_cd, sizeof(sv_run.colega_cd));
    memcpy(sv_run.colega_attack, seat->colega_attack, sizeof(sv_run.colega_attack));
    memcpy(sv_run.colega_walk, seat->colega_walk, sizeof(sv_run.colega_walk));
    memcpy(sv_run.colega_moving, seat->colega_moving, sizeof(sv_run.colega_moving));
    sv_run.colega_heal = seat->colega_heal;
    sv_run.colega_heal_flash = seat->colega_heal_flash;
}

void sv_duo_use(int index)
{
    if (!sv_run.duo || index < 0 || index >= SV_DUO_PLAYERS || index == sv_run.active) return;
    seat_save(&sv_run.seats[sv_run.active]);
    seat_load(&sv_run.seats[index]);
    sv_run.active = index;
}

SvPlayer *sv_duo_player(int index)
{
    if (!sv_run.duo || index == sv_run.active) return &sv_run.player;
    return &sv_run.seats[index].player;
}

int sv_duo_count(void)
{
    return sv_run.duo ? SV_DUO_PLAYERS : 1;
}

bool sv_duo_standing(int index)
{
    if (index < 0 || index >= sv_duo_count()) return false;
    return !sv_duo_player(index)->down;
}

int sv_duo_nearest(Vector2 pos)
{
    int best = -1;
    float best_distance = 0.0f;

    for (int i = 0; i < sv_duo_count(); i++)
    {
        float d;

        if (!sv_duo_standing(i)) continue;
        d = Vector2DistanceSqr(pos, sv_duo_player(i)->pos);
        if (best < 0 || d < best_distance)
        {
            best = i;
            best_distance = d;
        }
    }
    return best;
}

int sv_duo_focus_player(void)
{
    if (!sv_run.duo) return -1;

    // Com um caído, a câmera é de quem está de pé, como no solo
    if (sv_duo_player(0)->down && !sv_duo_player(1)->down) return 1;
    if (sv_duo_player(1)->down && !sv_duo_player(0)->down) return 0;
    return -1;
}

Vector2 sv_duo_focus(void)
{
    int focused;

    if (!sv_run.duo) return sv_run.player.pos;

    focused = sv_duo_focus_player();
    if (focused >= 0) return sv_duo_player(focused)->pos;

    // Os dois de pé (ou os dois caídos, no fim da partida): o meio entre eles. Como um não se afasta
    // do outro mais que SV_DUO_LEASH_X / SV_DUO_LEASH_Y, os dois cabem na tela
    return Vector2Scale(Vector2Add(sv_duo_player(0)->pos, sv_duo_player(1)->pos), 0.5f);
}

// -----------------------------------------------------------------------------
// Cair e levantar
// -----------------------------------------------------------------------------

static const char *fall_banner[SV_DUO_PLAYERS] = {"JOGADOR 1 CAIU", "JOGADOR 2 CAIU"};
static const char *revive_banner[SV_DUO_PLAYERS] = {"JOGADOR 1 VOLTOU", "JOGADOR 2 VOLTOU"};

void sv_duo_fall(int index)
{
    SvPlayer *p = sv_duo_player(index);

    if (p->down) return;
    p->down = true;
    p->hp = 0.0f;
    p->revive = 0.0f;
    p->moving = false;
    p->walk_time = 0.0f;
    p->slow = p->root = p->invert = 0.0f;
    sv_banner(fall_banner[index], 2.5f);
    sv_sfx(SND_morte);
}

bool sv_duo_all_down(void)
{
    for (int i = 0; i < sv_duo_count(); i++)
    {
        if (sv_duo_standing(i)) return false;
    }
    return true;
}

float sv_duo_revive_progress(int index)
{
    SvPlayer *p;

    if (!sv_run.duo || index < 0 || index >= SV_DUO_PLAYERS) return 0.0f;
    p = sv_duo_player(index);
    return p->down ? Clamp(p->revive / SV_DUO_REVIVE_TIME, 0.0f, 1.0f) : 0.0f;
}

static void update_revive(float dt)
{
    for (int i = 0; i < SV_DUO_PLAYERS; i++)
    {
        SvPlayer *fallen = sv_duo_player(i);
        SvPlayer *partner = sv_duo_player(1 - i);

        if (!fallen->down) continue;

        // O parceiro precisa estar de pé e ao lado. Se sair de perto, o progresso vai voltando
        if (!partner->down && Vector2Distance(fallen->pos, partner->pos) < SV_DUO_REVIVE_RANGE) fallen->revive += dt;
        else fallen->revive = fmaxf(0.0f, fallen->revive - dt * 0.5f);

        if (fallen->revive >= SV_DUO_REVIVE_TIME)
        {
            fallen->down = false;
            fallen->revive = 0.0f;
            fallen->hp = fallen->max_hp * SV_DUO_REVIVE_HP;
            fallen->invulnerable = 2.0f;
            sv_spawn_fx(FX_WAVE, fallen->pos, 0.5f, 2.0f);
            sv_banner(revive_banner[i], 2.0f);
            sv_sfx(SND_evolucao);
        }
    }
}

// -----------------------------------------------------------------------------
// Os dois na mesma tela
// -----------------------------------------------------------------------------

// Se um passou do limite, os dois são puxados para o meio só o que passou. Com um caído não há
// limite: a câmera segue quem está de pé (sv_duo_focus) e uma seta na borda da tela mostra quem caiu.
static void keep_together(void)
{
    SvPlayer *a = sv_duo_player(0);
    SvPlayer *b = sv_duo_player(1);
    float dx = b->pos.x - a->pos.x;
    float dy = b->pos.y - a->pos.y;
    float extra_x = fabsf(dx) - SV_DUO_LEASH_X;
    float extra_y = fabsf(dy) - SV_DUO_LEASH_Y;
    float share_a = 0.5f;
    float share_b = 0.5f;

    if (a->down || b->down) return;

    if (extra_x > 0.0f)
    {
        float sign = dx > 0.0f ? 1.0f : -1.0f;

        a->pos.x += sign * extra_x * share_a;
        b->pos.x -= sign * extra_x * share_b;
    }
    if (extra_y > 0.0f)
    {
        float sign = dy > 0.0f ? 1.0f : -1.0f;

        a->pos.y += sign * extra_y * share_a;
        b->pos.y -= sign * extra_y * share_b;
    }

    // O puxão não pode deixar ninguém dentro de um móvel
    if (extra_x > 0.0f || extra_y > 0.0f)
    {
        sv_map_collide(&a->pos, 7.0f, 10.0f);
        sv_map_collide(&b->pos, 7.0f, 10.0f);
    }
}

void sv_duo_update(float dt)
{
    if (!sv_run.duo) return;
    update_revive(dt);
    keep_together();
}
