#include <stddef.h> // Define a constante NULL
#include <math.h>   // roundf() do passo do brilho

#include "raylib.h"
#include "accessibility.h"

// Filtro em tempo real: aumenta o contraste de luminância, que é o que continua
// perceptível com daltonismo, sem depender da matiz. Vale para todas as telas.
static const char *HIGH_CONTRAST_FRAGMENT_SHADER =
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "in vec4 fragColor;\n"
    "uniform sampler2D texture0;\n"
    "uniform vec4 colDiffuse;\n"
    "out vec4 finalColor;\n"
    "void main()\n"
    "{\n"
    "    vec4 color = texture(texture0, fragTexCoord) * colDiffuse * fragColor;\n"
    "    float luminance = dot(color.rgb, vec3(0.299, 0.587, 0.114));\n"
    "    vec3 saturated = mix(vec3(luminance), color.rgb, 1.25);\n"
    "    vec3 contrasted = (saturated - 0.4) * 1.7 + 0.45;\n"
    "    finalColor = vec4(clamp(contrasted, 0.0, 1.0), color.a);\n"
    "}\n";

static bool high_contrast_enabled = false;
static bool filter_loaded = false;
static bool filter_active = false;
static Shader high_contrast_shader;
static float game_brightness = 1.0f;

void toggle_high_contrast(void)
{
    high_contrast_enabled = !high_contrast_enabled;

    // Só compila o filtro para quem usa o modo
    if (high_contrast_enabled && !filter_loaded)
    {
        high_contrast_shader = LoadShaderFromMemory(NULL, HIGH_CONTRAST_FRAGMENT_SHADER);
        filter_loaded = true;
    }
}

bool is_high_contrast_enabled(void)
{
    return high_contrast_enabled;
}

bool is_high_contrast_filter_ready(void)
{
    return filter_loaded && IsShaderValid(high_contrast_shader);
}

void begin_high_contrast_filter(void)
{
    if (high_contrast_enabled && !filter_active)
    {
        BeginShaderMode(high_contrast_shader);
        filter_active = true;
    }
}

void end_high_contrast_filter(void)
{
    if (filter_active)
    {
        EndShaderMode();
        filter_active = false;
    }
}

void set_game_brightness(float brightness)
{
    if (brightness < BRIGHTNESS_MIN) brightness = BRIGHTNESS_MIN;
    if (brightness > BRIGHTNESS_MAX) brightness = BRIGHTNESS_MAX;

    game_brightness = roundf(brightness / BRIGHTNESS_STEP) * BRIGHTNESS_STEP;
}

float get_game_brightness(void)
{
    return game_brightness;
}

void draw_brightness_overlay(void)
{
    int width = GetScreenWidth();
    int height = GetScreenHeight();

    if (game_brightness < 1.0f)
    {
        // Escurece: preto translúcido sobre a tela
        DrawRectangle(0, 0, width, height, Fade(BLACK, 1.0f - game_brightness));
    }
    else if (game_brightness > 1.0f)
    {
        // Clareia: soma branco às cores, de forma mais suave que o escurecimento
        BeginBlendMode(BLEND_ADDITIVE);
        DrawRectangle(0, 0, width, height, Fade(WHITE, (game_brightness - 1.0f) * 0.5f));
        EndBlendMode();
    }
}

void begin_game_drawing(void)
{
    BeginDrawing();
    begin_high_contrast_filter();
}

void end_game_drawing(void)
{
    end_high_contrast_filter();
    draw_brightness_overlay();
    EndDrawing();
}

void unload_accessibility(void)
{
    if (filter_loaded)
    {
        UnloadShader(high_contrast_shader);
        filter_loaded = false;
    }
}
