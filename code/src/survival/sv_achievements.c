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

static bool known[CH_COUNT];
static int queue[CH_COUNT];
static int queued = 0;
static float shown = 0.0f;     // Há quanto tempo a primeira da fila está na tela
static bool announced = false; // O som dela já tocou

static bool unlocked(int id, SvProgress p)
{
    return sv_character_unlocked(id, p.runs, p.wins, p.best_seconds, p.kills);
}

void sv_ach_reset(SvProgress progress)
{
    for (int i = 0; i < CH_COUNT; i++) known[i] = unlocked(i, progress);
    queued = 0;
    shown = 0.0f;
    announced = false;
}

int sv_ach_check(SvProgress progress)
{
    int added = 0;

    for (int i = 0; i < CH_COUNT; i++)
    {
        if (known[i] || !unlocked(i, progress)) continue;
        known[i] = true;
        if (queued < CH_COUNT) queue[queued++] = i;
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
    const SvCharacter *c;
    float ui, w, h, x, y, in, text_x, widest;
    const char *power;

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

    c = sv_character(queue[0]);
    ui = sv_ui();
    // A caixa acompanha o texto mais largo: em janelas pequenas a letra não encolhe junto com ela
    power = TextFormat("Poder liberado nas cartas: %s", sv_weapon_name(c->start_weapon));
    widest = fmaxf(sv_text_width(c->unlock_text, 26.0f * ui), sv_text_width(power, 23.0f * ui));
    widest = fmaxf(widest, fmaxf(sv_text_width(c->name, 48.0f * ui), sv_text_width("CONQUISTA DESBLOQUEADA", 22.0f * ui)));
    w = fmaxf(640.0f * ui, 200.0f * ui + widest);
    h = 168.0f * ui;
    in = fminf(1.0f, fminf(shown, TOAST_SECONDS - shown) / TOAST_SLIDE);
    in = 1.0f - (1.0f - in) * (1.0f - in);
    x = (GetScreenWidth() - w) / 2.0f;
    y = GetScreenHeight() - (h + 96.0f * ui) * in + (1.0f - in) * 20.0f * ui;

    DrawRectangleRounded((Rectangle){x, y, w, h}, 0.16f, 8, (Color){20, 12, 26, 240});
    DrawRectangleRoundedLinesEx((Rectangle){x, y, w, h}, 0.16f, 8, 3.0f * ui, (Color){255, 204, 72, 255});
    DrawRectangleRounded((Rectangle){x + 14.0f * ui, y + 14.0f * ui, 140.0f * ui, 140.0f * ui}, 0.18f, 8, (Color){58, 44, 70, 255});
    {
        Texture2D portrait = sv_character_frame(queue[0], SOUTH, 0);

        if (portrait.id)
        {
            DrawTexturePro(portrait, (Rectangle){0, 0, (float)portrait.width, (float)portrait.height},
                           (Rectangle){x + 18.0f * ui, y + 16.0f * ui, 132.0f * ui, 132.0f * ui}, (Vector2){0, 0}, 0.0f, WHITE);
        }
    }

    text_x = x + 176.0f * ui;
    sv_text("CONQUISTA DESBLOQUEADA", text_x, y + 14.0f * ui, 22.0f * ui, (Color){255, 204, 72, 255});
    sv_text(c->name, text_x, y + 40.0f * ui, 48.0f * ui, WHITE);
    sv_text(c->unlock_text, text_x, y + 94.0f * ui, 26.0f * ui, (Color){206, 196, 214, 255});
    sv_text(power, text_x, y + 128.0f * ui, 23.0f * ui, (Color){110, 232, 210, 255});
}
