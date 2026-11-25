#include <math.h>
#include "raylib.h"
#include "sv_achievements.h"
#include "sv_assets.h"
#include "sv_audio.h"
#include "sv_game.h"
#include "sv_render.h"
#include "characters.h"
#include "macros.h"

#define TOAST_SECONDS 5.0f     // Quanto cada notificação fica na tela
#define TOAST_SLIDE 0.35f      // Segundos para entrar e para sair

static bool known[SV_ACH_COUNT];
static int queue[SV_ACH_COUNT];
static int queued = 0;
static float shown = 0.0f;     // Há quanto tempo a primeira da fila está na tela
static bool announced = false; // O som dela já tocou

static bool unlocked(int id, SvProgress p)
{
    if (id < CH_COUNT) return sv_character_unlocked(id, p);
    return sv_weapon_unlocked(sv_locked_power(id - CH_COUNT), p);
}

void sv_ach_reset(SvProgress progress)
{
    for (int i = 0; i < SV_ACH_COUNT; i++) known[i] = unlocked(i, progress);
    queued = 0;
    shown = 0.0f;
    announced = false;
}

int sv_ach_check(SvProgress progress)
{
    int added = 0;

    for (int i = 0; i < SV_ACH_COUNT; i++)
    {
        if (known[i] || !unlocked(i, progress)) continue;
        known[i] = true;
        if (queued < SV_ACH_COUNT) queue[queued++] = i;
        added++;
    }
    return added;
}

int sv_ach_current(void)
{
    return queued > 0 ? queue[0] : -1;
}

int sv_ach_pending(void)
{
    return queued;
}

void sv_ach_draw(float dt)
{
    const char *title, *objective, *power;
    float ui, w, h, x, y, in, text_x, widest;
    int id, weapon = -1;

    if (queued == 0) return;
    if (!announced)
    {
        sv_sfx(SND_conquista);
        announced = true;
    }
    shown += dt > 0.1f ? 0.1f : dt; // Um quadro travado não engole a notificação
    if (shown >= TOAST_SECONDS)
    {
        for (int i = 1; i < queued; i++) queue[i - 1] = queue[i];
        queued--;
        shown = 0.0f;
        announced = false;
        return;
    }

    id = queue[0];
    ui = sv_ui();
    if (id < CH_COUNT)
    {
        const SvCharacter *c = sv_character(id);

        title = c->name;
        objective = c->unlock_text;
        power = TextFormat("Poder liberado nas cartas: %s", sv_weapon_name(c->start_weapon));
    }
    else
    {
        weapon = sv_locked_power(id - CH_COUNT);
        title = sv_weapon_name(weapon);
        objective = sv_weapon_unlock_text(weapon);
        power = "Novo poder liberado nas cartas";
    }
    // A caixa acompanha o texto mais largo: em janelas pequenas a letra não encolhe junto com ela
    widest = fmaxf(sv_text_width(objective, 26.0f * ui), sv_text_width(power, 23.0f * ui));
    widest = fmaxf(widest, fmaxf(sv_text_width(title, 48.0f * ui), sv_text_width("CONQUISTA DESBLOQUEADA", 22.0f * ui)));
    w = fmaxf(640.0f * ui, 200.0f * ui + widest);
    h = 168.0f * ui;
    in = fminf(1.0f, fminf(shown, TOAST_SECONDS - shown) / TOAST_SLIDE);
    in = 1.0f - (1.0f - in) * (1.0f - in);
    x = (GetScreenWidth() - w) / 2.0f;
    y = GetScreenHeight() - (h + 96.0f * ui) * in + (1.0f - in) * 20.0f * ui;

    DrawRectangleRounded((Rectangle){x, y, w, h}, 0.16f, 8, (Color){20, 12, 26, 240});
    DrawRectangleRoundedLinesEx((Rectangle){x, y, w, h}, 0.16f, 8, 3.0f * ui, (Color){255, 204, 72, 255});
    DrawRectangleRounded((Rectangle){x + 14.0f * ui, y + 14.0f * ui, 140.0f * ui, 140.0f * ui}, 0.18f, 8, (Color){58, 44, 70, 255});
    if (weapon >= 0) sv_draw(sv_weapon_icon(weapon), 0, (Vector2){x + 84.0f * ui, y + 84.0f * ui}, 5.0f * ui, 0.0f, false, WHITE);
    else
    {
        Texture2D portrait = sv_character_frame(id, SOUTH, 0);

        if (portrait.id)
        {
            DrawTexturePro(portrait, (Rectangle){0, 0, (float)portrait.width, (float)portrait.height},
                           (Rectangle){x + 18.0f * ui, y + 16.0f * ui, 132.0f * ui, 132.0f * ui}, (Vector2){0, 0}, 0.0f, WHITE);
        }
    }

    text_x = x + 176.0f * ui;
    sv_text("CONQUISTA DESBLOQUEADA", text_x, y + 14.0f * ui, 22.0f * ui, (Color){255, 204, 72, 255});
    sv_text(title, text_x, y + 40.0f * ui, 48.0f * ui, WHITE);
    sv_text(objective, text_x, y + 94.0f * ui, 26.0f * ui, (Color){206, 196, 214, 255});
    sv_text(power, text_x, y + 128.0f * ui, 23.0f * ui, (Color){110, 232, 210, 255});
}
