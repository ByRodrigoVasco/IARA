#include <math.h>
#include "raylib.h"
#include "sv_light.h"
#include "sv_duo.h"
#include "sv_game.h"
#include "sv_map.h"
#include "sv_assets.h"

// -----------------------------------------------------------------------------
// Ajustes (mexer aqui para deixar mais claro ou mais escuro)
// -----------------------------------------------------------------------------

#define VISION_BASE 190.0f         // Raio da luz do jogador sem nenhum bônus
#define LIGHT_RINGS 6              // Degraus da luz (mais degraus = borda mais suave)
#define CEILING_SPACING 290.0f     // Distância entre as lâmpadas do teto
#define CEILING_CHANCE 0           // % dos pontos do teto que têm lâmpada acesa (0 = andar todo apagado)
#define FAULTY_CHANCE 30           // % das lâmpadas que têm defeito e piscam
#define PROP_RADIUS_SCALE 0.35f    // Encolhe a luz dos objetos: só a tela/LED brilha, o chão em volta fica escuro
#define PROP_STRENGTH_SCALE 0.8f
#define MAX_LIGHTS 768
#define MAX_GEM_LIGHTS 160         // Chips de XP que brilham ao mesmo tempo (os mais próximos da câmera)

// Cor do escuro: um roxo bem fechado. É multiplicada pela claridade do ato.
static const Color AMBIENT_TINT = {150, 135, 210, 255};

// Claridade do ambiente em cada ato: praticamente breu. Fora da lanterna só aparecem os olhos dos
// monstros e as telas e LEDs dos objetos; a energia ainda cai um pouco a cada ato
static const float ACT_AMBIENT[SV_ACT_COUNT] = {0.05f, 0.04f, 0.03f, 0.025f, 0.02f};

typedef enum { LK_STEADY, LK_FAULTY, LK_PULSE, LK_TWINKLE } LightKind;

typedef struct
{
    Vector2 pos;
    float radius;
    Color color;
    int kind;
    int seed;
    float strength;        // Força máxima (1 = normal)
} Light;

static Light lights[MAX_LIGHTS];
static int light_count = 0;
static RenderTexture2D layer = {0};
static int layer_w = 0, layer_h = 0;

// -----------------------------------------------------------------------------
// Sorteio determinístico (o mesmo mapa sempre tem as mesmas lâmpadas)
// -----------------------------------------------------------------------------

static unsigned hash_u(unsigned x)
{
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

static float hash_f(int a, int b)
{
    return (float)(hash_u((unsigned)a * 73856093U ^ (unsigned)b * 19349663U) & 0xFFFFu) / 65535.0f;
}

float sv_light_flicker(int seed, float t)
{
    // Cada lâmpada vive ciclos de 6 a 20 s; em cada ciclo ela pisca uma vez, por 0,3 a 1,1 s
    float cycle = 6.0f + 14.0f * hash_f(seed, 1);
    int index = (int)floorf(t / cycle);
    float local = t - (float)index * cycle;
    float start = hash_f(seed, index * 3 + 2) * (cycle - 1.2f);
    float length = 0.3f + 0.8f * hash_f(seed, index * 3 + 3);

    if (local < start || local > start + length) return 1.0f;

    // Dentro do pisca: liga e desliga uns 14 vezes por segundo, às vezes quase apaga
    return hash_f(seed * 31 + index, (int)(t * 14.0f)) < 0.45f ? 0.08f : 0.85f;
}

float sv_light_ambient_for_act(int act)
{
    if (act < 0) act = 0;
    if (act >= SV_ACT_COUNT) act = SV_ACT_COUNT - 1;
    return ACT_AMBIENT[act];
}

float sv_light_vision_radius(void)
{
    return VISION_BASE * sv_run.player.vision_mult;
}

// -----------------------------------------------------------------------------
// Luzes fixas
// -----------------------------------------------------------------------------

static void add_light(Vector2 pos, float radius, Color color, int kind, float strength)
{
    if (light_count >= MAX_LIGHTS) return;
    lights[light_count] = (Light){pos, radius, color, kind, light_count * 7 + 11, strength};
    light_count++;
}

// Luz que cada tipo de objeto solta (raio 0 = não brilha)
static bool prop_light(int sprite, float *radius, Color *color, int *kind, float *strength)
{
    *strength = 1.0f;
    switch (sprite)
    {
    case SPR_pr_rackVivo: *radius = 95.0f; *color = (Color){70, 150, 255, 255}; *kind = LK_TWINKLE; return true;
    case SPR_pr_rack: *radius = 55.0f; *color = (Color){40, 90, 190, 255}; *kind = LK_TWINKLE; return true;
    case SPR_pr_console: *radius = 80.0f; *color = (Color){80, 230, 130, 255}; *kind = LK_STEADY; return true;
    case SPR_pr_bancadaTI: *radius = 75.0f; *color = (Color){80, 230, 130, 255}; *kind = LK_FAULTY; return true;
    case SPR_pr_vending: *radius = 85.0f; *color = (Color){235, 70, 90, 255}; *kind = LK_FAULTY; return true;
    case SPR_pr_geladeira: *radius = 80.0f; *color = (Color){240, 215, 150, 255}; *kind = LK_FAULTY; return true;
    case SPR_pr_impressoraRH: *radius = 90.0f; *color = (Color){235, 60, 70, 255}; *kind = LK_PULSE; return true;
    case SPR_pr_catraca: *radius = 50.0f; *color = (Color){230, 50, 60, 255}; *kind = LK_PULSE; return true;
    case SPR_pr_cofre: *radius = 45.0f; *color = (Color){230, 50, 60, 255}; *kind = LK_PULSE; return true;
    case SPR_pr_nucleo: *radius = 190.0f; *color = (Color){80, 220, 240, 255}; *kind = LK_PULSE; return true;
    case SPR_pr_alga: *radius = 40.0f; *color = (Color){60, 200, 190, 255}; *kind = LK_PULSE; return true;
    case SPR_pr_mesaExec: *radius = 85.0f; *color = (Color){235, 170, 90, 255}; *kind = LK_STEADY; return true;
    case SPR_pr_mesaVerde: *radius = 50.0f; *color = (Color){70, 210, 120, 255}; *kind = LK_STEADY; *strength = 0.45f; return true;
    case SPR_pr_mesaVermelha: *radius = 50.0f; *color = (Color){220, 60, 70, 255}; *kind = LK_FAULTY; *strength = 0.45f; return true;
    case SPR_pr_mesaSec: *radius = 45.0f; *color = (Color){170, 180, 200, 255}; *kind = LK_FAULTY; *strength = 0.35f; return true;
    case SPR_pr_privativa: *radius = 70.0f; *color = (Color){150, 170, 230, 255}; *kind = LK_FAULTY; return true;
    default: return false;
    }
}

void sv_light_init(void)
{
    light_count = 0;
    sv_map_init();

    for (int i = 0; i < sv_map_prop_count(); i++)
    {
        const SvMapProp *prop = sv_map_prop(i);
        float radius, strength;
        Color color;
        int kind;

        // O centro da luz fica na parte de cima do objeto, onde fica a tela ou o painel
        if (prop_light(prop->sprite, &radius, &color, &kind, &strength))
        {
            float lift = sv_sprites[prop->sprite].frame_height > 0 ? sv_sprites[prop->sprite].frame_height * 0.7f : 18.0f;

            add_light((Vector2){prop->x, prop->y - lift}, radius * PROP_RADIUS_SCALE, color, kind, strength * PROP_STRENGTH_SCALE);
        }
    }

    // Lâmpadas do teto: frias, algumas com defeito
    for (int gy = 0; gy * CEILING_SPACING < SV_WORLD_H; gy++)
    {
        for (int gx = 0; gx * CEILING_SPACING < SV_WORLD_W; gx++)
        {
            Vector2 pos = {(gx + 0.5f) * CEILING_SPACING + (hash_f(gx, gy * 5) - 0.5f) * 80.0f,
                           (gy + 0.5f) * CEILING_SPACING + (hash_f(gx * 3, gy) - 0.5f) * 80.0f};

            if (hash_f(gx * 13, gy * 17) * 100.0f >= CEILING_CHANCE) continue;
            if (hash_f(gx * 29, gy * 7) * 100.0f < FAULTY_CHANCE) add_light(pos, 125.0f, (Color){200, 205, 225, 255}, LK_FAULTY, 1.0f);
            else add_light(pos, 115.0f, (Color){170, 178, 205, 255}, LK_STEADY, 1.0f);
        }
    }
}

void sv_light_unload(void)
{
    if (layer.id) UnloadRenderTexture(layer);
    layer = (RenderTexture2D){0};
    layer_w = layer_h = 0;
}

// -----------------------------------------------------------------------------
// Desenho
// -----------------------------------------------------------------------------

// Oscilação de energia do andar inteiro: uma vez a cada 50 a 90 s, por um segundo
static float brownout(float t)
{
    return sv_light_flicker(424242, t * 0.18f) < 0.5f ? 0.45f : 1.0f;
}

static float light_intensity(const Light *l, float t)
{
    switch (l->kind)
    {
    case LK_FAULTY: return sv_light_flicker(l->seed, t);
    case LK_PULSE: return 0.75f + 0.25f * sinf(t * 2.2f + (float)l->seed);
    case LK_TWINKLE: return 0.8f + (hash_f(l->seed, (int)(t * 6.0f)) < 0.3f ? 0.2f : 0.0f);
    default: return 1.0f;
    }
}

// Círculos um dentro do outro somados: o centro fica mais forte e a borda sai em degraus
static void draw_light(Vector2 pos, float radius, Color color, float intensity)
{
    float step = intensity / (float)LIGHT_RINGS;

    if (intensity <= 0.01f || radius < 2.0f) return;
    for (int r = 0; r < LIGHT_RINGS; r++)
    {
        float k = 1.0f - (float)r / (float)LIGHT_RINGS;
        Color c = {(unsigned char)(color.r * step), (unsigned char)(color.g * step), (unsigned char)(color.b * step), 255};

        DrawCircleV(pos, radius * k, c);
    }
}

static bool on_screen(Vector2 pos, float radius, Rectangle view)
{
    return pos.x + radius > view.x && pos.x - radius < view.x + view.width && pos.y + radius > view.y && pos.y - radius < view.y + view.height;
}

void sv_light_draw(Camera2D camera)
{
    int screen_w = GetScreenWidth(), screen_h = GetScreenHeight();
    int w = (int)ceilf(screen_w / camera.zoom), h = (int)ceilf(screen_h / camera.zoom);
    float t = sv_run.time;
    float ambient = sv_light_ambient_for_act(sv_run.act);
    float power = brownout(t);
    Camera2D layer_camera = camera;
    Rectangle view;
    int gem_lights = 0;

    if (light_count == 0) sv_light_init();
    if (!layer.id || w != layer_w || h != layer_h)
    {
        sv_light_unload();
        layer = LoadRenderTexture(w, h);
        SetTextureFilter(layer.texture, TEXTURE_FILTER_POINT);
        layer_w = w;
        layer_h = h;
    }

    // A camada tem 1 texel por unidade de mundo: mesma câmera, sem o zoom da tela
    layer_camera.zoom = 1.0f;
    layer_camera.offset = (Vector2){w / 2.0f, h / 2.0f};
    view = (Rectangle){camera.target.x - w / 2.0f, camera.target.y - h / 2.0f, (float)w, (float)h};

    BeginTextureMode(layer);
    ClearBackground((Color){(unsigned char)(AMBIENT_TINT.r * ambient), (unsigned char)(AMBIENT_TINT.g * ambient), (unsigned char)(AMBIENT_TINT.b * ambient), 255});
    BeginMode2D(layer_camera);
    BeginBlendMode(BLEND_ADDITIVE);

    for (int i = 0; i < light_count; i++)
    {
        const Light *l = &lights[i];

        if (!on_screen(l->pos, l->radius, view)) continue;
        draw_light(l->pos, l->radius, l->color, light_intensity(l, t) * l->strength * power);
    }

    for (int i = 0; i < SV_MAX_GEMS && gem_lights < MAX_GEM_LIGHTS; i++)
    {
        const SvGem *g = &sv_run.gems[i];

        if (!g->alive || !on_screen(g->pos, 20.0f, view)) continue;
        if (g->kind == 0) draw_light(g->pos, 16.0f, (Color){70, 220, 120, 255}, 0.7f);
        else if (g->kind == 3) draw_light(g->pos, 60.0f, (Color){240, 200, 90, 255}, 1.0f);
        else draw_light(g->pos, 26.0f, (Color){240, 200, 90, 255}, 0.8f);
        gem_lights++;
    }

    if (sv_run.boss.active) draw_light(sv_run.boss.pos, 150.0f, (Color){220, 60, 70, 255}, 0.75f + 0.25f * sinf(t * 3.0f));

    // A luz do jogador não oscila com a energia: é a lanterna do celular
    if (!sv_run.duo) draw_light(sv_run.player.pos, sv_light_vision_radius(), (Color){235, 228, 215, 255}, 1.15f);
    else
    {
        // Dupla: cada um tem a sua lanterna. Quem caiu fica com a luz fraca, para o parceiro achar no escuro
        for (int k = 0; k < sv_duo_count(); k++)
        {
            const SvPlayer *p = sv_duo_player(k);

            draw_light(p->pos, p->down ? VISION_BASE * 0.45f : VISION_BASE * p->vision_mult, (Color){235, 228, 215, 255}, p->down ? 0.8f : 1.15f);
        }
    }

    EndBlendMode();
    EndMode2D();
    EndTextureMode();

    // Multiplica a tela pela camada (branco = como está, preto = apagado)
    BeginBlendMode(BLEND_MULTIPLIED);
    DrawTexturePro(layer.texture, (Rectangle){0, 0, (float)w, -(float)h}, (Rectangle){0, 0, w * camera.zoom, h * camera.zoom}, (Vector2){0, 0}, 0.0f, WHITE);
    EndBlendMode();
}
