#include <stdio.h>
#include <string.h>

#include "raylib.h"
#include "loading.h"
#include "accessibility.h"
#include "screen_style.h"
#include "fonts.h"
#include "helper.h"
#include "macros.h"

static loading_job jobs[LOADING_MAX_JOBS];
static int job_count = 0;
static int current_job = 0;
static float current_progress = 0.0f;   // Andamento do trabalho atual, de 0 a 1
static float shown_percent = 0.0f;      // O que a barra mostra: nunca volta atrás
static char title_text[48] = "CARREGANDO";
static int next_screen_after = 0;
static bool finished = true;

void loading_begin(const char *title, const loading_job *new_jobs, int count, int next_screen)
{
    if (count > LOADING_MAX_JOBS) count = LOADING_MAX_JOBS;
    memcpy(jobs, new_jobs, sizeof(loading_job) * (size_t)count);
    job_count = count;
    current_job = 0;
    current_progress = 0.0f;
    shown_percent = 0.0f;
    next_screen_after = next_screen;
    snprintf(title_text, sizeof(title_text), "%s", title != NULL ? title : "CARREGANDO");
    finished = (count == 0);
}

float loading_percent(void)
{
    float total = 0.0f;
    float done = 0.0f;

    for (int i = 0; i < job_count; i++)
    {
        total += jobs[i].weight;
        if (i < current_job) done += jobs[i].weight;
        else if (i == current_job) done += jobs[i].weight * current_progress;
    }
    if (finished) return 100.0f;
    if (total <= 0.0f) return 0.0f;
    return done / total * 100.0f;
}

bool loading_work(double max_seconds)
{
    double start = GetTime();

    // Sempre faz pelo menos um pedaço por chamada, para o carregamento nunca ficar parado
    do
    {
        if (current_job >= job_count)
        {
            finished = true;
            return false;
        }

        current_progress = jobs[current_job].step();
        if (current_progress >= 1.0f)
        {
            current_job++;
            current_progress = 0.0f;
            if (current_job >= job_count)
            {
                finished = true;
                return false;
            }
        }
    } while (GetTime() - start < max_seconds);

    return true;
}

bool is_loading_finished(void)
{
    return finished;
}

int loading_next_screen(void)
{
    return next_screen_after;
}

void loading_run_all(const loading_job *run_jobs, int count)
{
    loading_begin("CARREGANDO", run_jobs, count, 0);
    while (loading_work(1.0e9)) { }
}

void render_loading(void)
{
    float scale = get_ui_scale();
    float title_size = style_font_size(1.0f);
    Vector2 measure;
    char dots[8];
    char title[72];

    if (!finished) loading_work(LOADING_SLICE_SECONDS);

    // A porcentagem só sobe: um trabalho que se refaz não faz a barra voltar
    if (loading_percent() > shown_percent) shown_percent = loading_percent();

    snprintf(dots, sizeof(dots), "%.*s", (int)(GetTime() * 2.5) % 4, "...");
    snprintf(title, sizeof(title), "%s%s", title_text, dots);

    begin_game_drawing();
    style_backdrop();

    measure = MeasureTextEx(default_game_font, title_text, title_size, title_size * 0.06f);
    DrawTextEx(default_game_font, title, (Vector2){(GetScreenWidth() - measure.x) / 2.0f, GetScreenHeight() * 0.22f}, title_size, title_size * 0.06f, STYLE_TEXT_COLOR);
    style_progress(shown_percent, current_job < job_count ? jobs[current_job].label : "Pronto");
    (void)scale;

    end_game_drawing();
}
