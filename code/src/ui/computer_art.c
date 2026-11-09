#include <math.h>
#include <stddef.h>

#include "raylib.h"
#include "computer_art.h"
#include "fonts.h"

#define ICON_GRID 16

// Paleta dos ícones: cada letra do mapa de pixels vira uma cor.
static Color icon_color(char mark)
{
    switch (mark)
    {
    case 'k': return (Color){10, 10, 13, 255};      // contorno
    case 'g': return (Color){178, 184, 190, 255};   // cinza claro
    case 'd': return (Color){118, 126, 134, 255};   // cinza escuro
    case 'y': return (Color){228, 190, 80, 255};    // amarelo
    case 'o': return (Color){196, 144, 52, 255};    // amarelo escuro
    case 'b': return (Color){70, 140, 200, 255};    // azul
    case 'w': return (Color){236, 238, 232, 255};   // branco
    case 'r': return (Color){190, 50, 50, 255};     // vermelho
    case 'c': return (Color){222, 206, 170, 255};   // creme
    default:  return BLANK;
    }
}

static const char *const icon_maps[4][ICON_GRID] = {
    [DESKTOP_ICON_TRASH] = {
        "................",
        "......kkkk......",
        ".....kddddk.....",
        "..kkkkkkkkkkkk..",
        "..kggggggggggk..",
        "..kkkkkkkkkkkk..",
        "...kggdggdggk...",
        "...kggdggdggk...",
        "...kggdggdggk...",
        "...kggdggdggk...",
        "...kggdggdggk...",
        "...kggdggdggk...",
        "...kggdggdggk...",
        "...kkkkkkkkkk...",
        "................",
        "................"},
    [DESKTOP_ICON_TICKETS] = {
        "................",
        "................",
        "kkkkkkkkkkkkkkkk",
        "kyyyyyyyyyyyyyyk",
        "kkyyyyyyyyyyyykk",
        "kykkyyyyyyyykkyk",
        "kyyykkyyyyykkyyk",
        "kyyyyykkkkkyyyyk",
        "kyyyyyyyyyyyyyyk",
        "kyoyyyyyyyyyyoyk",
        "kyyyyyyyyyyyyyyk",
        "kyyyyyyyyyyyyyyk",
        "kkkkkkkkkkkkkkkk",
        "................",
        "................",
        "................"},
    [DESKTOP_ICON_CLOCK] = {
        "................",
        ".....kkkkkk.....",
        "...kkbbbbbbkk...",
        "..kbbbbbbbbbbk..",
        ".kbbwwwwwwwwbbk.",
        ".kbwwwwkwwwwwbk.",
        "kbbwwwwkwwwwwbbk",
        "kbwwwwwkwwwwwwbk",
        "kbwwwwwkkkkwwwbk",
        "kbwwwwwwwwwwwwbk",
        "kbbwwwwwwwwwwbbk",
        ".kbwwwwwwwwwwbk.",
        ".kbbwwwwwwwwbbk.",
        "..kbbbbbbbbbbk..",
        "...kkbbbbbbkk...",
        ".....kkkkkk....."},
    [DESKTOP_ICON_MANUAL] = {
        "................",
        "..kkkkkkkkkkkk..",
        "..krrwwwwwwwwk..",
        "..krrwwwwwwwwk..",
        "..krrwkkkkkkwk..",
        "..krrwwwwwwwwk..",
        "..krrwkkkkkwwk..",
        "..krrwwwwwwwwk..",
        "..krrwkkkkkkwk..",
        "..krrwwwwwwwwk..",
        "..krrwwwwwwwwk..",
        "..krrwwwwwwwwk..",
        "..kkkkkkkkkkkk..",
        "..kcccccccccck..",
        "..kkkkkkkkkkkk..",
        "................"}};

void computer_art_draw_icon(DesktopIcon icon, Rectangle bounds)
{
    float cell = bounds.width / ICON_GRID;

    if (icon < 0 || icon > DESKTOP_ICON_MANUAL) return;

    for (int row = 0; row < ICON_GRID; row++)
    {
        for (int column = 0; column < ICON_GRID; column++)
        {
            Color color = icon_color(icon_maps[icon][row][column]);
            if (color.a == 0) continue;

            // As bordas arredondadas evitam frestas entre os pixels em qualquer tamanho
            float left = roundf(bounds.x + (column * cell));
            float right = roundf(bounds.x + ((column + 1) * cell));
            float top = roundf(bounds.y + (row * cell));
            float bottom = roundf(bounds.y + ((row + 1) * cell));

            DrawRectangleRec((Rectangle){left, top, right - left, bottom - top}, color);
        }
    }
}

#define WINDOW_TITLE_COLOR (Color){0, 110, 120, 255}
#define WINDOW_TITLE_LIGHT_COLOR (Color){70, 170, 176, 255}
#define WINDOW_BODY_COLOR (Color){248, 249, 246, 255}
#define WINDOW_OUTLINE_COLOR (Color){10, 10, 13, 255}
#define WINDOW_SHADOW (Color){0, 0, 0, 90}

float computer_art_window_buttons_width(float title_height)
{
    float button = title_height * 0.46f;

    return (title_height * 0.45f) + (button * 3.0f) + (button * 0.55f * 2.0f);
}

void computer_art_draw_window(Rectangle window, float title_height)
{
    float outline = fmaxf(2.0f, window.height * 0.006f);
    float border = fmaxf(3.0f, window.width * 0.006f);
    float shadow = window.width * 0.01f;

    // Sombra, contorno e moldura em verde-azulado, igual ao botão CISSA da barra de tarefas
    DrawRectangleRec((Rectangle){window.x + shadow, window.y + shadow, window.width, window.height}, WINDOW_SHADOW);
    DrawRectangleRec(window, WINDOW_OUTLINE_COLOR);
    Rectangle frame = {window.x + outline, window.y + outline, window.width - (outline * 2.0f), window.height - (outline * 2.0f)};
    DrawRectangleRec(frame, WINDOW_TITLE_COLOR);

    // Filete claro no alto da barra de título, como um relevo
    DrawRectangleRec((Rectangle){frame.x, frame.y, frame.width, fmaxf(1.0f, outline * 0.6f)}, WINDOW_TITLE_LIGHT_COLOR);

    // Corpo claro, com contorno fino
    Rectangle body = {frame.x + border, window.y + title_height, frame.width - (border * 2.0f), frame.height - (title_height - outline) - border};
    DrawRectangleRec((Rectangle){body.x - 1.0f, body.y - 1.0f, body.width + 2.0f, body.height + 2.0f}, WINDOW_OUTLINE_COLOR);
    DrawRectangleRec(body, WINDOW_BODY_COLOR);

    // Três botões quadrados da barra: fechar, minimizar e ampliar
    float button = title_height * 0.46f;
    float gap = button * 0.55f;
    float x = window.x + (title_height * 0.45f);
    float y = window.y + ((title_height - button) / 2.0f);
    static const Color button_colors[3] = {{222, 74, 62, 255}, {232, 190, 72, 255}, {82, 190, 112, 255}};

    for (int i = 0; i < 3; i++)
    {
        Rectangle square = {roundf(x + (i * (button + gap))), roundf(y), roundf(button), roundf(button)};

        DrawRectangleRec((Rectangle){square.x - 1.0f, square.y - 1.0f, square.width + 2.0f, square.height + 2.0f}, WINDOW_OUTLINE_COLOR);
        DrawRectangleRec(square, button_colors[i]);
        DrawRectangleRec((Rectangle){square.x, square.y, square.width, fmaxf(1.0f, square.height * 0.25f)}, Fade(WHITE, 0.35f));
    }
}

// ---------------------------------------------------------------------------
// Teclado
// ---------------------------------------------------------------------------

#define KEY_ROW_COUNT 5
#define KEY_ROW_UNITS 14.0f
#define KEY_STATE_COUNT 350

typedef struct {
    int key;          // Código da tecla do raylib (0 = decorativa)
    float width;      // Largura em unidades de tecla
    const char *label;
} KeycapDefinition;

static const KeycapDefinition keyboard_row_0[] = {
    {KEY_ONE, 1, "1"}, {KEY_TWO, 1, "2"}, {KEY_THREE, 1, "3"}, {KEY_FOUR, 1, "4"}, {KEY_FIVE, 1, "5"},
    {KEY_SIX, 1, "6"}, {KEY_SEVEN, 1, "7"}, {KEY_EIGHT, 1, "8"}, {KEY_NINE, 1, "9"}, {KEY_ZERO, 1, "0"},
    {KEY_MINUS, 1, "-"}, {KEY_EQUAL, 1, "="}, {KEY_BACKSPACE, 2, "<-"}, {0, 0, NULL}};
static const KeycapDefinition keyboard_row_1[] = {
    {KEY_TAB, 2, "Tab"}, {KEY_Q, 1, "Q"}, {KEY_W, 1, "W"}, {KEY_E, 1, "E"}, {KEY_R, 1, "R"}, {KEY_T, 1, "T"},
    {KEY_Y, 1, "Y"}, {KEY_U, 1, "U"}, {KEY_I, 1, "I"}, {KEY_O, 1, "O"}, {KEY_P, 1, "P"},
    {KEY_LEFT_BRACKET, 1, "["}, {KEY_RIGHT_BRACKET, 1, "]"}, {0, 0, NULL}};
static const KeycapDefinition keyboard_row_2[] = {
    {KEY_CAPS_LOCK, 2, "Caps"}, {KEY_A, 1, "A"}, {KEY_S, 1, "S"}, {KEY_D, 1, "D"}, {KEY_F, 1, "F"},
    {KEY_G, 1, "G"}, {KEY_H, 1, "H"}, {KEY_J, 1, "J"}, {KEY_K, 1, "K"}, {KEY_L, 1, "L"},
    {KEY_SEMICOLON, 1, ";"}, {KEY_ENTER, 2, "Enter"}, {0, 0, NULL}};
static const KeycapDefinition keyboard_row_3[] = {
    {KEY_LEFT_SHIFT, 2.5f, "Shift"}, {KEY_Z, 1, "Z"}, {KEY_X, 1, "X"}, {KEY_C, 1, "C"}, {KEY_V, 1, "V"},
    {KEY_B, 1, "B"}, {KEY_N, 1, "N"}, {KEY_M, 1, "M"}, {KEY_COMMA, 1, ","}, {KEY_PERIOD, 1, "."},
    {KEY_RIGHT_SHIFT, 2.5f, "Shift"}, {0, 0, NULL}};
static const KeycapDefinition keyboard_row_4[] = {
    {KEY_LEFT_CONTROL, 1.5f, "Ctrl"}, {KEY_LEFT_ALT, 1.5f, "Alt"}, {KEY_SPACE, 7, ""},
    {KEY_RIGHT_ALT, 1.5f, "Alt"}, {KEY_RIGHT_CONTROL, 1.5f, "Ctrl"}, {0, 0, NULL}};

static const KeycapDefinition *const keyboard_rows[KEY_ROW_COUNT] = {
    keyboard_row_0, keyboard_row_1, keyboard_row_2, keyboard_row_3, keyboard_row_4};

static float key_depth[KEY_STATE_COUNT];   // 0 = solta, 1 = afundada: anima a descida e a subida

static float approach(float value, float target, float step)
{
    if (value < target) return fminf(target, value + step);
    return fmaxf(target, value - step);
}

void computer_art_draw_keyboard(Rectangle bounds)
{
    float outline = fmaxf(2.0f, bounds.height * 0.03f);
    float padding = bounds.height * 0.07f;
    float row_height = (bounds.height - (padding * 2.0f)) / KEY_ROW_COUNT;
    float unit = (bounds.width - (padding * 2.0f)) / KEY_ROW_UNITS;
    float gap = fmaxf(1.5f, unit * 0.07f);
    float dt = GetFrameTime();
    Color body = {26, 27, 33, 255};
    Color outline_color = {10, 10, 13, 255};

    // Corpo do teclado com uma sombra por baixo e a borda superior mais clara
    DrawRectangleRec((Rectangle){bounds.x, bounds.y + (bounds.height * 0.06f), bounds.width, bounds.height}, (Color){0, 0, 0, 70});
    DrawRectangleRec(bounds, outline_color);
    DrawRectangleRec((Rectangle){bounds.x + outline, bounds.y + outline, bounds.width - (outline * 2.0f), bounds.height - (outline * 2.0f)}, body);
    DrawRectangleRec((Rectangle){bounds.x + outline, bounds.y + outline, bounds.width - (outline * 2.0f), fmaxf(1.0f, outline * 0.5f)}, (Color){60, 63, 74, 255});

    for (int row = 0; row < KEY_ROW_COUNT; row++)
    {
        float x = bounds.x + padding;
        float y = bounds.y + padding + (row * row_height);

        for (const KeycapDefinition *cap = keyboard_rows[row]; cap->width > 0.0f; cap++)
        {
            Rectangle slot = {x, y, (cap->width * unit) - gap, row_height - gap};
            bool pressed = cap->key > 0 && cap->key < KEY_STATE_COUNT && IsKeyDown(cap->key);
            float *depth = (cap->key > 0 && cap->key < KEY_STATE_COUNT) ? &key_depth[cap->key] : NULL;

            if (depth != NULL) *depth = approach(*depth, pressed ? 1.0f : 0.0f, dt * (pressed ? 16.0f : 9.0f));

            float sink = (depth != NULL ? *depth : 0.0f) * (row_height * 0.16f);
            float lit = depth != NULL ? *depth : 0.0f;
            Color side = (Color){22, 23, 28, 255};
            Color face = {(unsigned char)(62 + (lit * 16.0f)), (unsigned char)(65 + (lit * 120.0f)), (unsigned char)(76 + (lit * 126.0f)), 255};
            Color top_edge = {(unsigned char)(88 + (lit * 40.0f)), (unsigned char)(92 + (lit * 110.0f)), (unsigned char)(106 + (lit * 110.0f)), 255};

            // A lateral da tecla (altura) some conforme ela afunda
            float side_height = fmaxf(1.0f, slot.height * 0.2f) - sink * 0.9f;
            if (side_height > 0.0f)
            {
                DrawRectangleRec((Rectangle){slot.x, slot.y + slot.height - side_height, slot.width, side_height}, side);
            }

            Rectangle cap_face = {slot.x, slot.y + sink, slot.width, slot.height - fmaxf(1.0f, slot.height * 0.2f)};
            DrawRectangleRec(cap_face, face);
            DrawRectangleRec((Rectangle){cap_face.x, cap_face.y, cap_face.width, fmaxf(1.0f, cap_face.height * 0.14f)}, top_edge);

            if (cap->label != NULL && cap->label[0] != '\0')
            {
                float font = cap_face.height * (cap->width > 1.0f ? 0.52f : 0.74f);
                Vector2 size = MeasureTextEx(default_game_font, cap->label, font, 0.0f);

                DrawTextEx(default_game_font, cap->label,
                           (Vector2){cap_face.x + ((cap_face.width - size.x) / 2.0f), cap_face.y + ((cap_face.height - size.y) / 2.0f)},
                           font, 0.0f, (Color){(unsigned char)(196 + (lit * 50.0f)), (unsigned char)(200 + (lit * 50.0f)), (unsigned char)(210 + (lit * 40.0f)), 255});
            }

            x += cap->width * unit;
        }
    }
}

// ---------------------------------------------------------------------------
// Mouse da mesa
// ---------------------------------------------------------------------------

static float mouse_left_depth = 0.0f;
static float mouse_right_depth = 0.0f;

void computer_art_draw_mouse(Rectangle bounds)
{
    float dt = GetFrameTime();
    bool left = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    bool right = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
    float outline = fmaxf(2.0f, bounds.width * 0.07f);
    Color outline_color = {10, 10, 13, 255};
    Color body = {44, 45, 53, 255};

    mouse_left_depth = approach(mouse_left_depth, left ? 1.0f : 0.0f, dt * (left ? 18.0f : 8.0f));
    mouse_right_depth = approach(mouse_right_depth, right ? 1.0f : 0.0f, dt * (right ? 18.0f : 8.0f));

    // Cabo que sobe até a parte de trás do monitor
    Vector2 cable_start = {bounds.x + (bounds.width / 2.0f), bounds.y + 2.0f};
    Vector2 cable_end = {bounds.x + (bounds.width * 0.1f), bounds.y - (bounds.height * 0.7f)};

    DrawLineBezier(cable_start, cable_end, fmaxf(2.0f, bounds.width * 0.06f), (Color){18, 18, 22, 255});

    // Sombra e corpo arredondado
    DrawRectangleRounded((Rectangle){bounds.x, bounds.y + (bounds.height * 0.07f), bounds.width, bounds.height}, 0.45f, 8, (Color){0, 0, 0, 70});
    DrawRectangleRounded(bounds, 0.45f, 8, outline_color);
    Rectangle inner = {bounds.x + outline, bounds.y + outline, bounds.width - (outline * 2.0f), bounds.height - (outline * 2.0f)};
    DrawRectangleRounded(inner, 0.42f, 8, body);

    // Os dois botões ocupam a metade de cima; o que é clicado afunda e acende
    float button_height = inner.height * 0.44f;
    float half = inner.width / 2.0f;
    Rectangle left_button = {inner.x + 1.0f, inner.y + 1.0f + (mouse_left_depth * 1.5f), half - 1.5f, button_height - (mouse_left_depth * 1.5f)};
    Rectangle right_button = {inner.x + half + 0.5f, inner.y + 1.0f + (mouse_right_depth * 1.5f), half - 1.5f, button_height - (mouse_right_depth * 1.5f)};

    DrawRectangleRounded(left_button, 0.35f, 6, (Color){(unsigned char)(62 + (mouse_left_depth * 20.0f)), (unsigned char)(64 + (mouse_left_depth * 120.0f)), (unsigned char)(74 + (mouse_left_depth * 126.0f)), 255});
    DrawRectangleRounded(right_button, 0.35f, 6, (Color){(unsigned char)(62 + (mouse_right_depth * 20.0f)), (unsigned char)(64 + (mouse_right_depth * 120.0f)), (unsigned char)(74 + (mouse_right_depth * 126.0f)), 255});

    // Rodinha no meio, entre os botões
    float wheel_width = fmaxf(2.0f, inner.width * 0.14f);
    DrawRectangleRec((Rectangle){inner.x + ((inner.width - wheel_width) / 2.0f), inner.y + (button_height * 0.18f), wheel_width, button_height * 0.62f}, (Color){120, 124, 136, 255});
    DrawRectangleRec((Rectangle){inner.x + (inner.width / 2.0f) - 0.5f, inner.y + button_height, 1.0f, inner.height * 0.12f}, outline_color);
}

// ---------------------------------------------------------------------------
// Ondas de clique na tela
// ---------------------------------------------------------------------------

#define RIPPLE_COUNT 8
#define RIPPLE_DURATION 0.38f

typedef struct {
    Vector2 position;
    float age;
    bool active;
    bool right_button;
} ClickRipple;

static ClickRipple ripples[RIPPLE_COUNT];

void computer_art_reset(void)
{
    for (int i = 0; i < RIPPLE_COUNT; i++) ripples[i] = (ClickRipple){0};
    for (int i = 0; i < KEY_STATE_COUNT; i++) key_depth[i] = 0.0f;
    mouse_left_depth = 0.0f;
    mouse_right_depth = 0.0f;
}

void computer_art_update_click_ripples(Rectangle area)
{
    float dt = GetFrameTime();
    Vector2 mouse = GetMousePosition();
    bool left = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    bool right = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);

    for (int i = 0; i < RIPPLE_COUNT; i++)
    {
        if (!ripples[i].active) continue;

        ripples[i].age += dt;
        if (ripples[i].age >= RIPPLE_DURATION) ripples[i].active = false;
    }

    if ((!left && !right) || !CheckCollisionPointRec(mouse, area)) return;

    for (int i = 0; i < RIPPLE_COUNT; i++)
    {
        if (ripples[i].active) continue;

        ripples[i] = (ClickRipple){mouse, 0.0f, true, !left && right};
        return;
    }
}

void computer_art_draw_click_ripples(float scale)
{
    for (int i = 0; i < RIPPLE_COUNT; i++)
    {
        if (!ripples[i].active) continue;

        float progress = ripples[i].age / RIPPLE_DURATION;
        float eased = 1.0f - ((1.0f - progress) * (1.0f - progress));
        float radius = (5.0f + (eased * 26.0f)) * scale;
        unsigned char alpha = (unsigned char)(220.0f * (1.0f - progress));
        Color color = ripples[i].right_button ? (Color){255, 176, 80, alpha} : (Color){160, 236, 244, alpha};

        DrawRing(ripples[i].position, radius - (2.2f * scale), radius, 0.0f, 360.0f, 28, color);
        DrawCircleV(ripples[i].position, (1.0f - progress) * 4.0f * scale, Fade(color, 0.9f));
    }
}
