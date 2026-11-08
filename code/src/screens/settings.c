#include "raylib.h"
#include "settings.h"
#include "macros.h"
#include "fonts.h"
#include "helper.h"
#include "accessibility.h"

// Medidas em 1920x1080, multiplicadas por get_ui_scale().
#define PANEL_WIDTH 1000.0f
#define PANEL_HEIGHT 570.0f
#define PANEL_PADDING 50.0f
#define ROW_HEIGHT 95.0f
#define ROWS_TOP 150.0f
#define TITLE_FONT 80.0f
#define ROW_FONT 52.0f
#define CHECKBOX_SIZE 56.0f
#define CHECK_LINE 8.0f
#define SLIDER_WIDTH 340.0f
#define SLIDER_HEIGHT 16.0f
#define SLIDER_HANDLE_WIDTH 28.0f
#define SLIDER_HANDLE_HEIGHT 56.0f
#define SLIDER_VALUE_GAP 24.0f
#define SELECTION_BAR_WIDTH 10.0f

#define LETTER_SPACING_RATIO 0.06f

#define PANEL_COLOR (Color){20, 11, 21, 235}
#define ROW_SELECTED_COLOR (Color){96, 64, 96, 255}
#define CONTROL_COLOR (Color){48, 32, 48, 255}

static Texture2D settings_background;
static int selected_option = SETTINGS_CONTRAST;

// Retângulo de uma linha, centralizado na janela.
static Rectangle get_row_rectangle(int option)
{
    float scale = get_ui_scale();
    float panel_x = (GetScreenWidth() - (PANEL_WIDTH * scale)) / 2.0f;
    float panel_y = (GetScreenHeight() - (PANEL_HEIGHT * scale)) / 2.0f;

    return (Rectangle){
        panel_x + (PANEL_PADDING * scale),
        panel_y + ((ROWS_TOP + (option * ROW_HEIGHT)) * scale),
        (PANEL_WIDTH - (PANEL_PADDING * 2.0f)) * scale,
        (ROW_HEIGHT - 10.0f) * scale};
}

// Barra do brilho, alinhada à direita da linha, deixando espaço para a porcentagem.
static Rectangle get_slider_rectangle(void)
{
    float scale = get_ui_scale();
    Rectangle row = get_row_rectangle(SETTINGS_BRIGHTNESS);
    float value_width = 130.0f * scale;
    float width = SLIDER_WIDTH * scale;
    float height = SLIDER_HEIGHT * scale;

    return (Rectangle){
        row.x + row.width - width - value_width - (SLIDER_VALUE_GAP * scale),
        row.y + (row.height - height) / 2.0f,
        width,
        height};
}

static bool is_option_checked(int option)
{
    return (option == SETTINGS_CONTRAST) ? is_high_contrast_enabled() : is_fullscreen_enabled();
}

bool activate_settings_option(int option)
{
    switch (option)
    {
    case SETTINGS_CONTRAST:
        toggle_high_contrast();
        return false;

    case SETTINGS_FULLSCREEN:
        toggle_fullscreen();
        return false;

    case SETTINGS_BACK:
        return true;

    default:
        return false;
    }
}

// Teclado: W/S ou setas escolhem a linha, A/D ou setas ajustam o brilho, Enter ou Espaço acionam e ESC volta ao menu.
static bool handle_settings_keyboard(void)
{
    if (IsKeyPressed(KEY_ESCAPE)) return true;

    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
    {
        selected_option = (selected_option + 1) % SETTINGS_OPTION_COUNT;
    }

    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
    {
        selected_option = (selected_option + SETTINGS_OPTION_COUNT - 1) % SETTINGS_OPTION_COUNT;
    }

    if (selected_option == SETTINGS_BRIGHTNESS)
    {
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) set_game_brightness(get_game_brightness() + BRIGHTNESS_STEP);
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) set_game_brightness(get_game_brightness() - BRIGHTNESS_STEP);
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
    {
        return activate_settings_option(selected_option);
    }

    return false;
}

// Mouse: mover destaca a linha, clicar aciona e arrastar na barra ajusta o brilho.
static bool handle_settings_mouse(void)
{
    if (!IsWindowFocused()) return false;

    Vector2 mouse = GetMousePosition();
    Vector2 movement = GetMouseDelta();
    bool mouse_moved = (movement.x != 0 || movement.y != 0);
    int hovered = -1;

    for (int option = 0; option < SETTINGS_OPTION_COUNT; option++)
    {
        if (CheckCollisionPointRec(mouse, get_row_rectangle(option))) hovered = option;
    }

    SetMouseCursor((hovered == -1) ? MOUSE_CURSOR_ARROW : MOUSE_CURSOR_POINTING_HAND);

    if (hovered == -1) return false;

    if (mouse_moved || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) selected_option = hovered;

    if (hovered == SETTINGS_BRIGHTNESS)
    {
        Rectangle slider = get_slider_rectangle();
        float reach = SLIDER_HANDLE_WIDTH * get_ui_scale();
        bool on_slider = mouse.x >= slider.x - reach && mouse.x <= slider.x + slider.width + reach;

        if (on_slider && IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            float progress = (mouse.x - slider.x) / slider.width;

            set_game_brightness(BRIGHTNESS_MIN + (progress * (BRIGHTNESS_MAX - BRIGHTNESS_MIN)));
        }

        return false;
    }

    return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && activate_settings_option(hovered);
}

static void draw_row_text(const char *text, Rectangle row, float font_size, Color color)
{
    float letter_spacing = font_size * LETTER_SPACING_RATIO;
    Vector2 size = MeasureTextEx(default_game_font, text, font_size, letter_spacing);

    DrawTextEx(default_game_font, text, (Vector2){row.x + (SELECTION_BAR_WIDTH * 3.0f * get_ui_scale()), row.y + (row.height - size.y) / 2.0f},
               font_size, letter_spacing, color);
}

// Caixa de seleção: marcada, mostra um X, então o estado não depende só da cor.
static void draw_checkbox(Rectangle row, bool checked)
{
    float scale = get_ui_scale();
    float size = CHECKBOX_SIZE * scale;
    float line = CHECK_LINE * scale;
    Rectangle box = {row.x + row.width - size - (SELECTION_BAR_WIDTH * 2.0f * scale), row.y + (row.height - size) / 2.0f, size, size};

    DrawRectangleRec(box, CONTROL_COLOR);
    DrawRectangleLinesEx(box, line / 2.0f, WHITE);

    if (checked)
    {
        float inset = size * 0.22f;

        DrawLineEx((Vector2){box.x + inset, box.y + inset}, (Vector2){box.x + size - inset, box.y + size - inset}, line, WHITE);
        DrawLineEx((Vector2){box.x + size - inset, box.y + inset}, (Vector2){box.x + inset, box.y + size - inset}, line, WHITE);
    }
}

static void draw_brightness_slider(Rectangle row, float font_size)
{
    float scale = get_ui_scale();
    Rectangle slider = get_slider_rectangle();
    float progress = (get_game_brightness() - BRIGHTNESS_MIN) / (BRIGHTNESS_MAX - BRIGHTNESS_MIN);
    float handle_w = SLIDER_HANDLE_WIDTH * scale;
    float handle_h = SLIDER_HANDLE_HEIGHT * scale;
    float handle_x = slider.x + (progress * slider.width) - (handle_w / 2.0f);
    float handle_y = row.y + (row.height - handle_h) / 2.0f;

    DrawRectangleRec(slider, CONTROL_COLOR);
    DrawRectangleLinesEx(slider, 2.0f, WHITE);
    DrawRectangleRec((Rectangle){handle_x, handle_y, handle_w, handle_h}, WHITE);

    const char *value = TextFormat("%d%%", (int)((get_game_brightness() * 100.0f) + 0.5f));
    float letter_spacing = font_size * LETTER_SPACING_RATIO;
    Vector2 size = MeasureTextEx(default_game_font, value, font_size, letter_spacing);

    DrawTextEx(default_game_font, value,
               (Vector2){row.x + row.width - size.x - (SELECTION_BAR_WIDTH * 2.0f * scale), row.y + (row.height - size.y) / 2.0f},
               font_size, letter_spacing, WHITE);
}

void initialize_settings_screen(void)
{
    // O menu já mantém o escritório na memória: abrir as configurações não decodifica a imagem de novo
    settings_background = acquire_smooth_texture(ESCRITORIO_TELA_PNG_PATH);
    selected_option = SETTINGS_CONTRAST;
}

bool render_settings_screen(void)
{
    const char *labels[SETTINGS_OPTION_COUNT] = {"Alto contraste", "Brilho", "Tela cheia", "VOLTAR AO MENU"};

    bool go_back = handle_settings_keyboard();
    go_back = handle_settings_mouse() || go_back;

    float scale = get_ui_scale();
    float window_w = (float)GetScreenWidth();
    float window_h = (float)GetScreenHeight();
    float row_font = ROW_FONT * scale;
    Rectangle panel = {(window_w - (PANEL_WIDTH * scale)) / 2.0f, (window_h - (PANEL_HEIGHT * scale)) / 2.0f, PANEL_WIDTH * scale, PANEL_HEIGHT * scale};

    begin_game_drawing();
    ClearBackground(BG_COLOR);

    // A arte fica ao fundo para mostrar na hora o efeito do brilho e do contraste
    draw_texture_cover(settings_background, (Rectangle){0, 0, window_w, window_h}, WHITE);
    DrawRectangleRec(panel, PANEL_COLOR);
    DrawRectangleLinesEx(panel, 2.0f, WHITE);

    float title_font = TITLE_FONT * scale;
    float title_spacing = title_font * LETTER_SPACING_RATIO;
    const char *title = "CONFIGURAÇÕES";
    Vector2 title_size = MeasureTextEx(default_game_font, title, title_font, title_spacing);

    DrawTextEx(default_game_font, title, (Vector2){(window_w - title_size.x) / 2.0f, panel.y + (PANEL_PADDING * scale)}, title_font, title_spacing, WHITE);

    for (int option = 0; option < SETTINGS_OPTION_COUNT; option++)
    {
        Rectangle row = get_row_rectangle(option);

        // A linha selecionada tem fundo e uma barra lateral, para não depender só da cor
        if (option == selected_option)
        {
            DrawRectangleRec(row, ROW_SELECTED_COLOR);
            DrawRectangleRec((Rectangle){row.x, row.y, SELECTION_BAR_WIDTH * scale, row.height}, WHITE);
        }

        draw_row_text(labels[option], row, row_font, WHITE);

        if (option == SETTINGS_BRIGHTNESS)
        {
            draw_brightness_slider(row, row_font);
        }
        else if (option != SETTINGS_BACK)
        {
            draw_checkbox(row, is_option_checked(option));
        }
    }

    end_game_drawing();

    return go_back;
}

void unload_settings_screen(void)
{
    SetMouseCursor(MOUSE_CURSOR_ARROW);
    release_smooth_texture(settings_background);
    settings_background = (Texture2D){0};
}
