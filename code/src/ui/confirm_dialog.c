#include <stdio.h>

#include "raylib.h"
#include "confirm_dialog.h"
#include "ui_theme.h"
#include "helper.h"
#include "fonts.h"

static bool dialog_open = false;
static char dialog_title[64];
static char dialog_message[256];
static char dialog_yes[32];
static char dialog_no[32];
static bool dialog_danger = false;
static int dialog_selected = 0;      // 0 cancela (o padrão), 1 confirma
static int dialog_skip_frames = 0;

void confirm_open(const char *title, const char *message, const char *yes_label, const char *no_label, bool danger)
{
    snprintf(dialog_title, sizeof(dialog_title), "%s", title);
    snprintf(dialog_message, sizeof(dialog_message), "%s", message);
    snprintf(dialog_yes, sizeof(dialog_yes), "%s", yes_label);
    snprintf(dialog_no, sizeof(dialog_no), "%s", no_label);
    dialog_danger = danger;
    dialog_selected = 0;
    dialog_skip_frames = 1;
    dialog_open = true;
}

bool confirm_is_open(void)
{
    return dialog_open;
}

void confirm_close(void)
{
    dialog_open = false;
    SetMouseCursor(MOUSE_CURSOR_ARROW);
}

confirm_result confirm_update_and_draw(void)
{
    float scale;
    float width;
    float height;
    float padding;
    float button_width;
    float button_height;
    Rectangle panel;
    Rectangle no_button;
    Rectangle yes_button;
    confirm_result result = CONFIRM_PENDING;
    bool over_no;
    bool over_yes;

    if (!dialog_open) return CONFIRM_PENDING;

    scale = get_ui_scale();
    width = GetScreenWidth() * 0.6f;
    if (width > 820.0f * scale) width = 820.0f * scale;
    height = 330.0f * scale;
    padding = 34.0f * scale;
    button_width = 250.0f * scale;
    button_height = 62.0f * scale;
    panel = (Rectangle){(GetScreenWidth() - width) / 2.0f, (GetScreenHeight() - height) / 2.0f, width, height};
    no_button = (Rectangle){panel.x + panel.width - padding - button_width * 2.0f - 20.0f * scale, panel.y + panel.height - padding - button_height, button_width, button_height};
    yes_button = (Rectangle){panel.x + panel.width - padding - button_width, no_button.y, button_width, button_height};
    over_no = ui_mouse_over(no_button);
    over_yes = ui_mouse_over(yes_button);

    if (dialog_skip_frames > 0)
    {
        dialog_skip_frames--;
    }
    else
    {
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) dialog_selected = 0;
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) dialog_selected = 1;
        if (IsKeyPressed(KEY_TAB)) dialog_selected = 1 - dialog_selected;
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) result = dialog_selected == 1 ? CONFIRM_YES : CONFIRM_NO;
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) result = CONFIRM_NO;

        if (over_no || over_yes)
        {
            Vector2 delta = GetMouseDelta();

            if (delta.x != 0.0f || delta.y != 0.0f) dialog_selected = over_yes ? 1 : 0;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) result = over_yes ? CONFIRM_YES : CONFIRM_NO;
        }
    }

    SetMouseCursor((over_no || over_yes) ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_ARROW);

    ui_draw_dim();
    ui_draw_panel(panel);
    DrawTextEx(default_game_font, dialog_title, (Vector2){panel.x + padding, panel.y + padding}, ui_font_size(1.6f), ui_font_size(1.6f) * 0.06f, dialog_danger ? UI_WARNING_COLOR : UI_TEXT_COLOR);
    ui_draw_wrapped(dialog_message, panel.x + padding, panel.y + padding + ui_font_size(1.6f) + 22.0f * scale, panel.width - padding * 2.0f, ui_font_size(1.1f), UI_DIM_COLOR, false);
    ui_draw_button(no_button, dialog_no, dialog_selected == 0, false);
    ui_draw_button(yes_button, dialog_yes, dialog_selected == 1, dialog_danger);

    if (result != CONFIRM_PENDING) confirm_close();
    return result;
}
