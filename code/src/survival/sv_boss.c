#include <math.h>
#include <string.h>
#include "raymath.h"
#include "sv_game.h"
#include "sv_audio.h"
#include "sv_map.h"

/*
 * Chefes e perigos. Todo golpe de chefe passa por sv_boss_hit_damage(), que o limita a 30% da vida base,
 * e cada um tem um aviso antes de causar dano (a área aparece e vai enchendo).
 */

typedef struct
{
    float hold_distance;   // Distância que o chefe tenta manter do jogador
    float speed;
    float radius;
} BossStats;

static const BossStats boss_stats[BOSS_COUNT] = {
    {170.0f, 34.0f, 26.0f},
    {210.0f, 38.0f, 28.0f},
    {110.0f, 58.0f, 22.0f},
    {0.0f, 0.0f, 24.0f},
    {230.0f, 42.0f, 38.0f},
};

// Recarga de cada golpe, em ordem de prioridade.
static const float pattern_cooldown[BOSS_COUNT][4] = {
    {14.0f, 9.0f, 4.0f, 0.0f},    // justa causa, papelada, leque
    {15.0f, 11.0f, 8.0f, 0.0f},   // sobreajuste, olhar, rastro
    {18.0f, 13.0f, 10.0f, 6.0f},  // crachás, porta, acesso, garras
    {8.0f, 10.0f, 7.0f, 5.0f},    // martelo, sombra, corrente, contrato
    {16.0f, 12.0f, 9.0f, 7.0f},   // resposta confiante, canto, onda, mãos
};

static SvHazard *new_hazard(void)
{
    for (int i = 0; i < SV_MAX_HAZARDS; i++)
    {
        if (!sv_run.hazards[i].alive)
        {
            memset(&sv_run.hazards[i], 0, sizeof(SvHazard));
            sv_run.hazards[i].alive = true;
            return &sv_run.hazards[i];
        }
    }
    return NULL;
}

// Os primeiros chefes batem mais leve, para o jogador sem upgrades chegar ao fim do ato
static const float boss_damage_scale[BOSS_COUNT] = {0.65f, 0.7f, 0.55f, 0.95f, 1.1f};

static float boss_hit(float percent)
{
    int id = sv_run.boss.id;

    if (id < 0 || id >= BOSS_COUNT) id = BOSS_COUNT - 1;
    float damage = sv_boss_hit_damage(percent) * boss_damage_scale[id];

    // Ajustar a dificuldade nunca fura o teto de 30% da vida base
    if (damage > SV_BASE_HP * SV_BOSS_HIT_CAP) damage = SV_BASE_HP * SV_BOSS_HIT_CAP;
    return damage;
}

static SvHazard *add_circle(Vector2 pos, float radius, float warn, float life, float percent, int effect, float effect_time, int sprite)
{
    SvHazard *h = new_hazard();

    if (!h) return NULL;
    h->kind = HZ_CIRCLE;
    h->sprite = sprite;
    h->pos = pos;
    h->radius = radius;
    h->warn = h->warn_total = warn;
    h->life = h->max_life = life;
    h->damage = boss_hit(percent);
    h->effect = effect;
    h->effect_time = effect_time;
    return h;
}

static SvHazard *add_projectile(Vector2 pos, Vector2 velocity, float radius, float life, float percent, int sprite)
{
    SvHazard *h = new_hazard();

    if (!h) return NULL;
    h->kind = HZ_PROJ;
    h->sprite = sprite;
    h->pos = pos;
    h->vel = velocity;
    h->radius = radius;
    h->life = h->max_life = life;
    h->damage = boss_hit(percent);
    h->angle = atan2f(velocity.y, velocity.x) * RAD2DEG;
    return h;
}

static SvHazard *add_beam(Vector2 pos, float angle, float length, float half_width, float warn, float life, float percent, float tick, int sprite)
{
    SvHazard *h = new_hazard();

    if (!h) return NULL;
    h->kind = HZ_BEAM;
    h->sprite = sprite;
    h->pos = pos;
    h->angle = angle;
    h->length = length;
    h->radius = half_width;
    h->warn = h->warn_total = warn;
    h->life = h->max_life = life;
    h->damage = boss_hit(percent);
    h->tick = tick;
    return h;
}

void sv_boss_clear_hazards(void)
{
    for (int i = 0; i < SV_MAX_HAZARDS; i++) sv_run.hazards[i].alive = false;
}

static Vector2 unit_toward(Vector2 from, Vector2 to)
{
    Vector2 d = Vector2Subtract(to, from);
    float length = Vector2Length(d);

    if (length < 0.001f) return (Vector2){1.0f, 0.0f};
    return Vector2Scale(d, 1.0f / length);
}

static Vector2 around_player(float min_distance, float max_distance)
{
    float angle = sv_rand() * 2.0f * PI;
    float distance = sv_rand_range(min_distance, max_distance);

    return Vector2Add(sv_player_pos(), (Vector2){cosf(angle) * distance, sinf(angle) * distance});
}

static Vector2 clamp_world(Vector2 p)
{
    p.x = Clamp(p.x, 30.0f, SV_WORLD_W - 30.0f);
    p.y = Clamp(p.y, 30.0f, SV_WORLD_H - 30.0f);
    return p;
}

// -----------------------------------------------------------------------------
// Chefe
// -----------------------------------------------------------------------------

void sv_boss_spawn(int id)
{
    SvBoss *b = &sv_run.boss;
    float angle = sv_rand() * 2.0f * PI;

    memset(b, 0, sizeof(*b));
    b->active = true;
    b->id = id;
    b->max_hp = sv_boss_hp(id);
    b->hp = b->max_hp;
    b->radius = boss_stats[id].radius;
    b->shields = id == BOSS_RENE ? 3 : 0;
    b->pos = clamp_world(Vector2Add(sv_player_pos(), (Vector2){cosf(angle) * 320.0f, sinf(angle) * 320.0f}));
    for (int k = 0; k < 4; k++) b->cooldown[k] = 2.0f + 1.5f * (float)k;

    sv_banner(sv_boss_name(id), 3.5f);
    sv_sfx(SND_chefeSurge);
}

static void spawn_defeat_rewards(SvBoss *b)
{
    int coins = 6 + 3 * b->id;

    sv_spawn_fx(FX_EXPLOSION, b->pos, 0.6f, 2.0f);
    sv_spawn_fx(FX_EXPLOSION, Vector2Add(b->pos, (Vector2){-22, 10}), 0.6f, 1.4f);
    sv_spawn_fx(FX_EXPLOSION, Vector2Add(b->pos, (Vector2){22, -10}), 0.6f, 1.4f);

    sv_add_gem(b->pos, 3, 1.0f);
    sv_add_gem(Vector2Add(b->pos, (Vector2){18, 12}), 0, 40.0f * (float)(b->id + 1));
    for (int i = 0; i < coins; i++)
    {
        sv_add_gem(Vector2Add(b->pos, (Vector2){sv_rand_range(-40, 40), sv_rand_range(-40, 40)}), 1, 1.0f);
    }
}

bool sv_damage_boss(float amount)
{
    SvBoss *b = &sv_run.boss;

    if (!b->active || b->invulnerable > 0.0f) return false;

    if (sv_run.cheats[CHEAT_ONE_HIT]) amount *= 1000.0f;
    b->hp -= amount;
    b->hurt_flash = 0.08f;
    sv_sfx(SND_acertaInimigo);

    if (b->id == BOSS_RENE)
    {
        float fraction = b->hp / b->max_hp;
        int shields = fraction > 0.75f ? 3 : fraction > 0.5f ? 2 : fraction > 0.25f ? 1 : 0;

        if (shields < b->shields)
        {
            b->shields = shields;
            b->invulnerable = 1.0f;
            sv_sfx(SND_reneEscudoCai);
            sv_banner("ESCUDO PERDIDO", 1.5f);
        }
    }

    if (b->hp <= 0.0f)
    {
        b->active = false;
        sv_run.bosses_defeated++;
        sv_sfx(SND_chefeMorre);
        spawn_defeat_rewards(b);
        sv_boss_clear_hazards();

        if (b->id == BOSS_IARA)
        {
            sv_run.phase = SV_WON;
            sv_music_play(MUS_VITORIA);
        }
        else
        {
            sv_banner("CHEFE DERROTADO", 2.5f);
        }
        return true;
    }

    return false;
}

static void cast_pattern(SvBoss *b, int pattern)
{
    Vector2 player = sv_player_pos();
    Vector2 aim = unit_toward(b->pos, player);
    float aim_degrees = atan2f(aim.y, aim.x) * RAD2DEG;

    switch (b->id)
    {
    case BOSS_IMPRESSORA:
        if (pattern == 0) // Justa Causa
        {
            add_circle(player, 55.0f, 1.5f, 0.3f, 20.0f, EFF_NONE, 0.0f, SPR_bp_justa);
            sv_sfx(SND_impressoraJustaCausa);
        }
        else if (pattern == 1) // Papelada
        {
            for (int i = 0; i < 4; i++) add_circle(around_player(60.0f, 150.0f), 40.0f, 0.7f, 5.0f, 0.0f, EFF_SLOW, 0.5f, SPR_bp_papelada);
            sv_sfx(SND_impressoraPapelada);
        }
        else // Leque de Rescisões
        {
            for (int i = -2; i <= 2; i++)
            {
                float a = (aim_degrees + 12.0f * (float)i) * DEG2RAD;

                add_projectile(b->pos, (Vector2){cosf(a) * 150.0f, sinf(a) * 150.0f}, 8.0f, 4.0f, 8.0f, -1);
            }
            sv_sfx(SND_impressoraLeque);
        }
        break;

    case BOSS_MODELO:
        if (pattern == 0) // Sobreajuste
        {
            for (int i = 0; i < 3; i++)
            {
                float a = (aim_degrees + 120.0f * (float)i) * DEG2RAD;
                SvHazard *h = add_projectile(b->pos, (Vector2){cosf(a) * 70.0f, sinf(a) * 70.0f}, 9.0f, 7.0f, 5.0f, SPR_bp_sobreajuste);

                if (h) h->homing = true;
            }
            sv_sfx(SND_modeloClones);
        }
        else if (pattern == 1) // Olhar Treinado
        {
            add_beam(b->pos, aim_degrees, 460.0f, 14.0f, 1.0f, 1.5f, 2.5f, 0.25f, SPR_bp_olhar);
            sv_sfx(SND_modeloOlhar);
        }
        else // Viés: marca o caminho percorrido
        {
            for (int i = 1; i < SV_TRAIL; i += 3) add_circle(sv_run.trail[i], 26.0f, 1.5f, 0.3f, 12.0f, EFF_NONE, 0.0f, SPR_bp_rastro);
            sv_sfx(SND_modeloMarca);
        }
        break;

    case BOSS_DIEGO:
        if (pattern == 0) // Crachás Fantasmas
        {
            for (int i = 0; i < 3; i++) sv_spawn_enemy_at(EN_CRACHA, Vector2Add(b->pos, (Vector2){cosf(PI * 0.667f * (float)i) * 40.0f, sinf(PI * 0.667f * (float)i) * 40.0f}), false);
            sv_sfx(SND_diegoFantasmas);
        }
        else if (pattern == 1) // Porta das 03:12
        {
            SvHazard *h = add_circle(clamp_world(around_player(90.0f, 130.0f)), 64.0f, 1.6f, 0.4f, 22.0f, EFF_NONE, 0.0f, SPR_bp_porta);

            if (h) h->teleport_boss = true;
            sv_sfx(SND_diegoPorta);
        }
        else if (pattern == 2) // Acesso Negado
        {
            add_circle(player, 80.0f, 1.0f, 0.4f, 6.0f, EFF_ROOT, 3.0f, SPR_bp_acesso);
            sv_sfx(SND_diegoNegado);
        }
        else // Pelas Costas: reaparece atrás e ataca
        {
            b->scheduled = 0.4f;
            b->scheduled_id = 1;
            sv_sfx(SND_diegoGarras);
        }
        break;

    case BOSS_RENE:
        if (pattern == 0) // Decisão Final
        {
            add_circle(player, 62.0f, 2.0f, 0.3f, 30.0f, EFF_NONE, 0.0f, SPR_bp_martelo);
            sv_sfx(SND_reneDecisao);
        }
        else if (pattern == 1) // Premissa H: a sombra
        {
            add_circle(around_player(60.0f, 100.0f), 30.0f, 0.8f, 0.3f, 14.0f, EFF_NONE, 0.0f, SPR_bp_sombra);
            sv_sfx(SND_reneSombra);
        }
        else if (pattern == 2) // Premissa D → C: a corrente
        {
            add_beam(b->pos, aim_degrees, 400.0f, 8.0f, 0.8f, 0.6f, 12.0f, 0.0f, SPR_bp_corrente);
            sv_sfx(SND_reneCorrente);
        }
        else // Premissa D: o contrato
        {
            SvHazard *h = add_projectile(b->pos, Vector2Scale(aim, 85.0f), 10.0f, 6.0f, 10.0f, SPR_bp_contrato);

            if (h) h->homing = true;
            sv_sfx(SND_reneCarimbo);
        }
        break;

    default: // IARA
        if (pattern == 0) // Resposta Confiante
        {
            // Parece uma zona segura, mas machuca a cada segundo
            SvHazard *h = add_circle(clamp_world(around_player(40.0f, 120.0f)), 70.0f, 2.0f, 4.0f, 15.0f, EFF_NONE, 0.0f, SPR_bp_falsa);

            if (h) h->tick = 1.0f;
            sv_sfx(SND_iaraMentira);
        }
        else if (pattern == 1) // Canto
        {
            b->scheduled = 1.0f;
            b->scheduled_id = 2;
            sv_sfx(SND_iaraCanto);
        }
        else if (pattern == 2) // Onda
        {
            bool horizontal = sv_rand() < 0.5f;
            float side = sv_rand() < 0.5f ? -1.0f : 1.0f;
            Vector2 origin = horizontal ? (Vector2){player.x + side * 460.0f, player.y} : (Vector2){player.x, player.y + side * 330.0f};

            b->sched_pos = origin;
            b->sched_dir = horizontal ? (Vector2){-side, 0.0f} : (Vector2){0.0f, -side};
            b->scheduled = 1.5f;
            b->scheduled_id = 3;

            // Linha de aviso no caminho da onda
            add_beam(origin, atan2f(b->sched_dir.y, b->sched_dir.x) * RAD2DEG, 900.0f, 4.0f, 1.5f, 0.05f, 0.0f, 0.0f, -4);
            sv_sfx(SND_iaraOnda);
        }
        else // Mãos d'Água
        {
            add_circle(player, 36.0f, 0.8f, 0.4f, 6.0f, EFF_ROOT, 1.0f, SPR_bp_maos);
            sv_sfx(SND_iaraMaos);
        }
        break;
    }
}

static void run_scheduled(SvBoss *b)
{
    Vector2 player = sv_player_pos();

    if (b->id == BOSS_DIEGO && b->scheduled_id == 1)
    {
        // Pelas Costas
        Vector2 behind = clamp_world(Vector2Subtract(player, Vector2Scale(sv_run.player.facing_vector, 70.0f)));
        Vector2 aim;

        b->pos = behind;
        aim = unit_toward(b->pos, player);
        for (int i = -1; i <= 1; i++)
        {
            float a = atan2f(aim.y, aim.x) + 0.3f * (float)i;

            add_projectile(b->pos, (Vector2){cosf(a) * 280.0f, sinf(a) * 280.0f}, 10.0f, 1.2f, 10.0f, SPR_bp_garras);
        }
        sv_spawn_fx(FX_FLASH, b->pos, 0.25f, 1.0f);
    }
    else if (b->id == BOSS_IARA && b->scheduled_id == 2)
    {
        // Canto: anel de som com um vão
        int gap = (int)(sv_rand() * 18.0f);

        for (int i = 0; i < 18; i++)
        {
            float a;
            SvHazard *h;

            if (i == gap || i == (gap + 1) % 18 || i == (gap + 2) % 18) continue;
            a = (float)i / 18.0f * 2.0f * PI;
            h = add_projectile(b->pos, (Vector2){cosf(a) * 115.0f, sinf(a) * 115.0f}, 9.0f, 4.5f, 8.0f, -3);
            if (h)
            {
                h->effect = EFF_INVERT;
                h->effect_time = 1.5f;
            }
        }
    }
    else if (b->id == BOSS_IARA && b->scheduled_id == 3)
    {
        // Onda com rostos: uma fileira de projetis com um vão
        bool horizontal = fabsf(b->sched_dir.x) > 0.5f;
        float span = horizontal ? 280.0f : 400.0f;
        float gap_center = sv_rand_range(-span * 0.6f, span * 0.6f);

        for (float offset = -span; offset <= span; offset += 26.0f)
        {
            Vector2 pos = horizontal ? (Vector2){b->sched_pos.x, b->sched_pos.y + offset} : (Vector2){b->sched_pos.x + offset, b->sched_pos.y};

            if (fabsf(offset - gap_center) < 60.0f) continue;
            add_projectile(pos, Vector2Scale(b->sched_dir, 165.0f), 14.0f, 7.0f, 18.0f, SPR_bp_onda);
        }
    }
}

void sv_boss_update(float dt)
{
    SvBoss *b = &sv_run.boss;
    const BossStats *stats;
    Vector2 player = sv_player_pos();
    Vector2 toward;
    float distance;

    if (!b->active) return;
    stats = &boss_stats[b->id];

    b->timer += dt;
    if (b->hurt_flash > 0.0f) b->hurt_flash -= dt;
    if (b->invulnerable > 0.0f) b->invulnerable -= dt;
    if (b->attack_anim > 0.0f) b->attack_anim -= dt;

    toward = unit_toward(b->pos, player);
    distance = Vector2Distance(b->pos, player);

    // Movimento: aproxima até a distância preferida, e recua se o jogador chegar perto demais
    if (stats->speed > 0.0f && b->attack_anim <= 0.0f)
    {
        if (distance > stats->hold_distance + 12.0f) b->pos = Vector2Add(b->pos, Vector2Scale(toward, stats->speed * dt));
        else if (distance < stats->hold_distance - 30.0f && b->id != BOSS_DIEGO) b->pos = Vector2Subtract(b->pos, Vector2Scale(toward, stats->speed * 0.6f * dt));
    }
    b->pos = clamp_world(b->pos);
    sv_map_collide(&b->pos, 12.0f, 18.0f);

    if (fabsf(toward.x) > fabsf(toward.y))
    {
        b->facing = 1;
        b->flip = toward.x < 0.0f;
    }
    else
    {
        b->facing = toward.y > 0.0f ? 0 : 2;
    }

    // Toque no jogador
    if (distance < b->radius + 8.0f) sv_hurt_player(sv_boss_hit_damage(10.0f), EFF_NONE, 0.0f);

    // Ação marcada
    if (b->scheduled > 0.0f)
    {
        b->scheduled -= dt;
        if (b->scheduled <= 0.0f)
        {
            run_scheduled(b);
            b->scheduled = 0.0f;
        }
    }

    // O chefe só escolhe um golpe novo quando acabou o anterior
    for (int k = 0; k < 4; k++)
    {
        if (b->cooldown[k] > 0.0f) b->cooldown[k] -= dt;
    }

    if (b->attack_anim <= 0.0f && b->scheduled <= 0.0f)
    {
        for (int k = 0; k < 4; k++)
        {
            bool available = pattern_cooldown[b->id][k] > 0.0f && b->cooldown[k] <= 0.0f;

            // Cada escudo do Renê libera um golpe; sem escudo, só a decisão final
            if (b->id == BOSS_RENE)
            {
                if (k == 0) available = available && b->shields == 0;
                else available = available && b->shields >= k;
            }

            if (!available) continue;

            cast_pattern(b, k);
            b->cooldown[k] = pattern_cooldown[b->id][k];
            b->attack_anim = 0.8f;
            break;
        }
    }

    if (b->id == BOSS_IARA && b->timer > 8.0f)
    {
        b->timer = 0.0f;
        sv_sfx(SND_iaraRiso);
    }
}

// -----------------------------------------------------------------------------
// Perigos
// -----------------------------------------------------------------------------

// Distância do ponto ao segmento (origem, origem + direção * comprimento)
static float distance_to_segment(Vector2 point, Vector2 origin, float angle_degrees, float length)
{
    float angle = angle_degrees * DEG2RAD;
    Vector2 direction = {cosf(angle), sinf(angle)};
    Vector2 rel = Vector2Subtract(point, origin);
    float along = Clamp(Vector2DotProduct(rel, direction), 0.0f, length);
    Vector2 closest = Vector2Add(origin, Vector2Scale(direction, along));

    return Vector2Distance(point, closest);
}

void sv_hazards_update(float dt)
{
    Vector2 player = sv_player_pos();

    for (int i = 0; i < SV_MAX_HAZARDS; i++)
    {
        SvHazard *h = &sv_run.hazards[i];

        if (!h->alive) continue;

        if (h->kind == HZ_PROJ)
        {
            if (h->homing)
            {
                Vector2 desired = Vector2Scale(unit_toward(h->pos, player), Vector2Length(h->vel));

                h->vel = Vector2Lerp(h->vel, desired, 1.5f * dt);
            }
            h->pos = Vector2Add(h->pos, Vector2Scale(h->vel, dt));
            h->angle = atan2f(h->vel.y, h->vel.x) * RAD2DEG;
            h->life -= dt;

            if (Vector2Distance(h->pos, player) < h->radius + 8.0f)
            {
                sv_hurt_player(h->damage, h->effect, h->effect_time);
                h->alive = false;
            }
            else if (h->life <= 0.0f || sv_map_point_blocked(h->pos, h->radius * 0.5f) || h->pos.x < -50 || h->pos.y < -50 || h->pos.x > SV_WORLD_W + 50 || h->pos.y > SV_WORLD_H + 50)
            {
                h->alive = false;
            }
            continue;
        }

        if (h->warn > 0.0f)
        {
            h->warn -= dt;
            if (h->warn > 0.0f) continue;

            // Aviso acabou: o perigo começa
            if (h->teleport_boss && sv_run.boss.active)
            {
                sv_run.boss.pos = h->pos;
                sv_spawn_fx(FX_FLASH, h->pos, 0.25f, 1.0f);
            }
        }

        h->life -= dt;

        if (h->kind == HZ_CIRCLE)
        {
            bool inside = Vector2Distance(player, h->pos) < h->radius + 6.0f;

            if (inside)
            {
                if (h->tick > 0.0f)
                {
                    h->tick_timer -= dt;
                    if (h->tick_timer <= 0.0f)
                    {
                        h->tick_timer = h->tick;
                        sv_hurt_player(h->damage, h->effect, h->effect_time);
                    }
                }
                else if (h->damage > 0.0f)
                {
                    if (!h->applied)
                    {
                        h->applied = true;
                        sv_hurt_player(h->damage, h->effect, h->effect_time);
                    }
                }
                else if (h->effect != EFF_NONE)
                {
                    sv_apply_status(h->effect, h->effect_time);
                }
            }
        }
        else if (h->kind == HZ_BEAM && h->damage > 0.0f)
        {
            bool inside = distance_to_segment(player, h->pos, h->angle, h->length) < h->radius + 6.0f;

            if (inside)
            {
                if (h->tick > 0.0f)
                {
                    h->tick_timer -= dt;
                    if (h->tick_timer <= 0.0f)
                    {
                        h->tick_timer = h->tick;
                        sv_hurt_player(h->damage, EFF_NONE, 0.0f);
                    }
                }
                else if (!h->applied)
                {
                    h->applied = true;
                    sv_hurt_player(h->damage, EFF_NONE, 0.0f);
                }
            }
        }

        if (h->life <= 0.0f) h->alive = false;
    }
}
