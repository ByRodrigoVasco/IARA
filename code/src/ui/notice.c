#include <stdio.h>

#include "raylib.h"
#include "notice.h"
#include "ui_theme.h"
#include "fonts.h"
#include "helper.h"

#define NOTICE_SECONDS 2.6
#define NOTICE_ERROR_SECONDS 7.0
#define NOTICE_FADE_SECONDS 0.5

static char notice_text[200] = "";
static notice_kind notice_current_kind = NOTICE_INFO;
static double notice_started_at = 0.0;
static double notice_duration = 0.0;

void notice_show(const char *text, notice_kind kind)
{
    snprintf(notice_text, sizeof(notice_text), "%s", text != NULL ? text : "");
    notice_current_kind = kind;
    notice_started_at = GetTime();
    notice_duration = kind == NOTICE_ERROR ? NOTICE_ERROR_SECONDS : NOTICE_SECONDS;
}

bool notice_is_active(void)
{
    return notice_text[0] != '\0' && GetTime() < notice_started_at + notice_duration;
}

const char *notice_text_now(void)
{
    return notice_is_active() ? notice_text : "";
}

void notice_clear(void)
{
    notice_text[0] = '\0';
}

void notice_draw(void)
{
    double remaining;
    float scale;
    float size;
    float padding;
    float max_width;
    float alpha = 1.0f;
    float text_width;
    Vector2 measure;
    Rectangle box;
    Color accent;

    if (!notice_is_active()) return;

    scale = get_ui_scale();
    size = ui_font_size(1.0f);
    padding = 14.0f * scale;
    max_width = GetScreenWidth() * 0.8f;
    measure = MeasureTextEx(default_game_font, notice_text, size, size * 0.06f);
    text_width = measure.x;
    if (text_width > max_width - padding * 2.0f) text_width = max_width - padding * 2.0f;
    box = (Rectangle){(GetScreenWidth() - text_width) / 2.0f - padding, 18.0f * scale, text_width + padding * 2.0f, ui_measure_wrapped(notice_text, text_width + 1.0f, size) + padding * 2.0f - 6.0f * scale};

    remaining = notice_started_at + notice_duration - GetTime();
    if (remaining < NOTICE_FADE_SECONDS) alpha = (float)(remaining / NOTICE_FADE_SECONDS);

    accent = notice_current_kind == NOTICE_ERROR ? UI_WARNING_COLOR : (notice_current_kind == NOTICE_SUCCESS ? UI_SUCCESS_COLOR : UI_BORDER_COLOR);

    DrawRectangleRec(box, Fade((Color){14, 8, 18, 255}, 0.92f * alpha));
    DrawRectangleLinesEx(box, 2.0f * scale, Fade(accent, alpha));
    ui_draw_wrapped(notice_text, box.x + padding, box.y + padding, text_width + 1.0f, size, Fade(UI_TEXT_COLOR, alpha), true);
}
