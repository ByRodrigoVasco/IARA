#include "rooms.h"
#include "rooms_data.h"

const room_def *room_get(room_id id)
{
    if (id < 0 || id >= ROOM_COUNT) return NULL;
    return &room_defs[id];
}

int room_door_touching(room_id id, Rectangle hitbox_reference)
{
    const room_def *room = room_get(id);
    if (!room) return -1;

    for (int i = 0; i < room->door_count; i++)
    {
        if (CheckCollisionRecs(room->doors[i].zone, hitbox_reference)) return i;
    }
    return -1;
}

int room_door_toward_office(room_id id)
{
    const room_def *room = room_get(id);
    if (!room || id == ROOM_ESCRITORIO) return -1;

    // Do corredor a porta é a do escritório; dos outros cômodos, a que volta para o corredor
    room_id target = (id == ROOM_CORREDOR) ? ROOM_ESCRITORIO : ROOM_CORREDOR;
    for (int i = 0; i < room->door_count; i++)
    {
        if (room->doors[i].to_room == target) return i;
    }
    return -1;
}

// ---------------------------------------------------------------------------
// ANIMAÇÕES
// ---------------------------------------------------------------------------

#include <math.h>

#define GLITCH_PERIOD 6.5f        // Segundos entre um defeito e outro do servidor
#define GLITCH_DURATION 0.7f      // Quanto dura o defeito
#define EYES_BLINK_PERIOD 3.7f
#define EYES_BLINK_TIME 0.12f
#define SLIDE_TIME 4.0f           // Segundos de cada slide na TV
#define DARK_FLICKER_PERIOD 11.0f
#define DARK_FLICKER_TIME 0.55f

// Número pseudoaleatório 0..1 a partir de dois inteiros (o mesmo par dá sempre o mesmo número)
static float hash01(int a, int b)
{
    unsigned int h = (unsigned int)a * 374761393u + (unsigned int)b * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return (float)(h ^ (h >> 16)) / 4294967295.0f;
}

static float wrap(float value, float period)
{
    float r = fmodf(value, period);
    return (r < 0.0f) ? r + period : r;
}

// Cor da animação no brilho do cômodo (tint.r) e na opacidade do fade (tint.a)
static Color shade(int r, int g, int b, float light, int alpha, Color tint)
{
    float k = light * tint.r / 255.0f;
    int cr = (int)(r * k), cg = (int)(g * k), cb = (int)(b * k);

    return (Color){(unsigned char)(cr > 255 ? 255 : cr), (unsigned char)(cg > 255 ? 255 : cg),
                   (unsigned char)(cb > 255 ? 255 : cb), (unsigned char)(alpha * tint.a / 255)};
}

static Rectangle to_screen(Rectangle ref, float scale, Vector2 center)
{
    return (Rectangle){center.x + ref.x * scale, center.y + ref.y * scale, ref.width * scale, ref.height * scale};
}

// Um pixel da arte (quadrado do tamanho do pixel da imagem) na posição de referência
static void draw_art_pixel(float x, float y, float size, float scale, Vector2 center, Color color)
{
    DrawRectangleRec(to_screen((Rectangle){x, y, size, size}, scale, center), color);
}

bool room_glitch_active(Rectangle area, float time, float *progress)
{
    // A fase vem da posição: os LEDs, os olhos e o defeito do mesmo servidor ficam sincronizados
    float phase = wrap(area.x * 0.013f + area.y * 0.007f, 1.0f) * GLITCH_PERIOD;
    float u = wrap(time + phase, GLITCH_PERIOD);

    if (progress) *progress = u / GLITCH_DURATION;
    return u < GLITCH_DURATION;
}

static void draw_leds(const room_anim *anim, float time, float scale, Vector2 center, Color tint, bool wave)
{
    bool glitch = room_glitch_active(anim->area, time, NULL);

    for (int i = 0; i < anim->pixel_count; i++)
    {
        const room_pixel *p = &anim->pixels[i];
        bool on;

        if (glitch) on = hash01(i, (int)(time * 25.0f)) > 0.7f;               // No defeito, tudo pisca sem ordem
        else if (wave) on = sinf(time * 5.0f - p->y * 0.06f) > -0.3f;          // Onda descendo pelo rack
        else on = hash01(i, (int)(time * 4.0f + hash01(i, 7) * 10.0f)) > 0.35f; // Cada LED no seu ritmo

        Color c = on ? shade(p->r, p->g, p->b, 1.3f, 255, tint) : shade(p->r, p->g, p->b, 0.28f, 255, tint);
        draw_art_pixel(p->x, p->y, anim->pixel_size, scale, center, c);
    }
}

static void draw_eyes(const room_anim *anim, float time, float scale, Vector2 center, Color tint)
{
    bool glitch = room_glitch_active(anim->area, time, NULL);
    bool closed = wrap(time + anim->phase * EYES_BLINK_PERIOD, EYES_BLINK_PERIOD) < EYES_BLINK_TIME;
    float pulse = 1.0f + 0.25f * sinf(time * 3.0f);

    for (int i = 0; i < anim->pixel_count; i++)
    {
        const room_pixel *p = &anim->pixels[i];
        Color c;

        if (glitch) c = ((int)(time * 20.0f) % 2) ? shade(255, 235, 235, 1.0f, 255, tint) : shade(p->r, p->g, p->b, 1.5f, 255, tint);
        else if (closed) c = shade(14, 6, 8, 1.0f, 255, tint);
        else c = shade(p->r, p->g, p->b, pulse, 255, tint);
        draw_art_pixel(p->x, p->y, anim->pixel_size, scale, center, c);
    }
}

static void draw_glitch(const room_anim *anim, float time, float scale, Vector2 center, Texture2D map, float art_scale, Color tint)
{
    float progress;

    if (!room_glitch_active(anim->area, time, &progress) || map.id == 0) return;

    Rectangle area = anim->area;
    float px = anim->pixel_size;
    int frame = (int)(time * 15.0f);

    // Faixas da imagem deslocadas para os lados, como uma tela com defeito
    for (int k = 0; k < 4; k++)
    {
        float band_h = px * (2.0f + floorf(hash01(k, frame) * 6.0f));
        float band_y = area.y + floorf(hash01(k + 11, frame) * (area.height - band_h) / px) * px;
        float shift = px * (floorf(hash01(k + 23, frame) * 7.0f) - 3.0f);
        Rectangle source = {(area.x + map.width * art_scale / 2.0f) / art_scale, (band_y + map.height * art_scale / 2.0f) / art_scale,
                            area.width / art_scale, band_h / art_scale};
        Rectangle dest = to_screen((Rectangle){area.x + shift, band_y, area.width, band_h}, scale, center);

        DrawTexturePro(map, source, dest, (Vector2){0}, 0.0f, (Color){tint.r, tint.g, tint.b, tint.a});
    }

    // Clarão vermelho piscando, mais forte no começo
    int flash = (frame % 2) ? 90 : 35;
    DrawRectangleRec(to_screen(area, scale, center), shade(255, 40, 40, 1.0f, (int)(flash * (1.0f - progress * 0.6f)), tint));

    // Faíscas nas bordas do servidor
    for (int k = 0; k < 6; k++)
    {
        if (hash01(k + 41, frame) < 0.45f) continue;
        float x = area.x + floorf(hash01(k + 53, frame) * area.width / px) * px;
        float y = area.y + floorf(hash01(k + 67, frame) * area.height / px) * px;
        Color spark = (k % 2) ? shade(255, 240, 150, 1.0f, 255, tint) : shade(255, 255, 255, 1.0f, 255, tint);

        draw_art_pixel(x, y, px, scale, center, spark);
        draw_art_pixel(x + px, y, px, scale, center, shade(255, 160, 60, 1.0f, 200, tint));
    }
}

static void draw_blink(const room_anim *anim, float time, float scale, Vector2 center, Color tint)
{
    if (wrap(time + anim->phase, 1.1f) < 0.6f) return;   // Aceso: a imagem já mostra as letras

    for (int i = 0; i < anim->pixel_count; i++)
    {
        const room_pixel *p = &anim->pixels[i];
        draw_art_pixel(p->x, p->y, anim->pixel_size, scale, center, shade(p->r, p->g, p->b, 0.3f, 255, tint));
    }
}

static void draw_screen(const room_anim *anim, float time, float scale, Vector2 center, Color tint)
{
    Rectangle area = anim->area;
    float px = anim->pixel_size;
    float t = time + anim->phase * 10.0f;
    int noise = (int)(14.0f + 22.0f * hash01((int)(t * 12.0f), 3));

    DrawRectangleRec(to_screen(area, scale, center), shade(255, 255, 255, 1.0f, noise, tint));

    // Linha de varredura descendo devagar
    float y = area.y + floorf(wrap(t * 0.6f, 1.0f) * area.height / px) * px;
    DrawRectangleRec(to_screen((Rectangle){area.x, y, area.width, px}, scale, center), shade(255, 255, 255, 1.0f, 45, tint));

    // De vez em quando a tela dá uma apagada
    if (wrap(t, 9.0f) < 0.18f) DrawRectangleRec(to_screen(area, scale, center), shade(0, 0, 0, 1.0f, 150, tint));
}

static void draw_slides(const room_anim *anim, float time, float scale, Vector2 center, Color tint)
{
    float px = anim->pixel_size;
    Rectangle screen = {anim->area.x + 2 * px, anim->area.y + 2 * px, anim->area.width - 4 * px, anim->area.height - 4 * px};
    int slide = (int)(time / SLIDE_TIME);
    float since = time - slide * SLIDE_TIME;
    float grow = since / 0.6f > 1.0f ? 1.0f : since / 0.6f;
    const unsigned char colors[4][3] = {{72, 196, 196}, {236, 200, 80}, {232, 104, 96}, {150, 120, 220}};

    DrawRectangleRec(to_screen(screen, scale, center), shade(26, 34, 58, 1.0f, 255, tint));

    // Título do slide: duas linhas claras de tamanhos diferentes
    float title_w = floorf((0.35f + 0.4f * hash01(slide, 1)) * screen.width / px) * px;
    DrawRectangleRec(to_screen((Rectangle){screen.x + 2 * px, screen.y + 2 * px, title_w, px}, scale, center), shade(230, 230, 240, 1.0f, 255, tint));
    DrawRectangleRec(to_screen((Rectangle){screen.x + 2 * px, screen.y + 4 * px, title_w * 0.6f, px}, scale, center), shade(140, 150, 180, 1.0f, 255, tint));

    // Gráfico de barras que cresce quando o slide entra
    int bars = 4 + slide % 2;
    float base = screen.y + screen.height - 2 * px;
    float slot = floorf((screen.width - 4 * px) / bars / px) * px;
    for (int k = 0; k < bars; k++)
    {
        float full = floorf((0.2f + 0.6f * hash01(slide, k + 5)) * (screen.height - 9 * px) / px) * px;
        float h = floorf(full * grow / px) * px;
        const unsigned char *c = colors[(k + slide) % 4];

        DrawRectangleRec(to_screen((Rectangle){screen.x + 2 * px + k * slot + px, base - h, slot - 2 * px, h}, scale, center),
                         shade(c[0], c[1], c[2], 1.0f, 255, tint));
    }
    DrawRectangleRec(to_screen((Rectangle){screen.x + 2 * px, base, screen.width - 4 * px, px}, scale, center), shade(200, 200, 210, 1.0f, 255, tint));

    // Clarão rápido na troca de slide
    if (since < 0.12f) DrawRectangleRec(to_screen(screen, scale, center), shade(255, 255, 255, 1.0f, (int)(120 * (1.0f - since / 0.12f)), tint));
}

static void draw_steam(const room_anim *anim, float time, float scale, Vector2 center, Color tint)
{
    float px = anim->pixel_size;
    Rectangle area = anim->area;

    for (int k = 0; k < 3; k++)
    {
        float p = wrap(time * 0.45f + anim->phase + k / 3.0f, 1.0f);
        float x = area.x + area.width / 2.0f + floorf(sinf(time * 2.0f + k * 2.1f) * 1.5f) * px;
        float y = area.y + area.height - floorf(p * area.height / px) * px;
        float size = px * (p < 0.4f ? 2.0f : 3.0f);

        DrawRectangleRec(to_screen((Rectangle){x - size / 2.0f, y, size, size}, scale, center),
                         shade(176, 188, 206, 1.0f, (int)(220 * (1.0f - p)), tint));   // Cinza azulado: aparece na parede branca
    }
}

static void draw_glow(const room_anim *anim, float time, float scale, Vector2 center, Color tint)
{
    int alpha = (int)(30 + 35 * (0.5f + 0.5f * sinf(time * 3.0f + anim->phase * 6.0f)));

    BeginBlendMode(BLEND_ADDITIVE);
    DrawRectangleRec(to_screen(anim->area, scale, center), shade(255, 210, 110, 1.0f, alpha, tint));
    EndBlendMode();
}

static void draw_water(const room_anim *anim, float time, float scale, Vector2 center, Color tint)
{
    float px = anim->pixel_size;
    Vector2 middle = {anim->area.x + anim->area.width / 2.0f, anim->area.y + anim->area.height / 2.0f};

    // Três anéis abrindo do centro, desenhados em pixels da arte
    for (int k = 0; k < 3; k++)
    {
        float p = wrap(time * 0.3f + k / 3.0f, 1.0f);
        float rx = p * anim->area.width / 2.0f, ry = p * anim->area.height / 2.0f;
        int dots = 10 + (int)(p * 26.0f);
        Color c = shade(150, 226, 220, 1.0f, (int)(200 * (1.0f - p)), tint);

        for (int d = 0; d < dots; d++)
        {
            float a = d * 6.2831853f / dots;
            float x = floorf((middle.x + cosf(a) * rx) / px) * px;
            float y = floorf((middle.y + sinf(a) * ry) / px) * px;
            draw_art_pixel(x, y, px, scale, center, c);
        }
    }
}

static void draw_dark_flicker(const room_anim *anim, float time, float scale, Vector2 center, Color tint)
{
    float u = wrap(time + anim->phase * DARK_FLICKER_PERIOD, DARK_FLICKER_PERIOD);

    if (u > DARK_FLICKER_TIME) return;
    int alpha = hash01((int)(time * 18.0f), 9) > 0.5f ? 150 : 40;
    DrawRectangleRec(to_screen(anim->area, scale, center), (Color){0, 0, 0, (unsigned char)(alpha * tint.a / 255)});
}

void room_draw_animations(room_id id, float time, float scale, Vector2 center, Texture2D map, float art_scale,
                          Color tint, bool foreground)
{
    const room_def *room = room_get(id);

    if (!room) return;
    for (int i = 0; i < room->anim_count; i++)
    {
        const room_anim *anim = &room->anims[i];

        if ((anim->type == ROOM_ANIM_DARK_FLICKER) != foreground) continue;
        switch (anim->type)
        {
        case ROOM_ANIM_LEDS: draw_leds(anim, time, scale, center, tint, false); break;
        case ROOM_ANIM_LEDS_WAVE: draw_leds(anim, time, scale, center, tint, true); break;
        case ROOM_ANIM_EYES: draw_eyes(anim, time, scale, center, tint); break;
        case ROOM_ANIM_GLITCH: draw_glitch(anim, time, scale, center, map, art_scale, tint); break;
        case ROOM_ANIM_BLINK: draw_blink(anim, time, scale, center, tint); break;
        case ROOM_ANIM_SCREEN: draw_screen(anim, time, scale, center, tint); break;
        case ROOM_ANIM_SLIDES: draw_slides(anim, time, scale, center, tint); break;
        case ROOM_ANIM_STEAM: draw_steam(anim, time, scale, center, tint); break;
        case ROOM_ANIM_GLOW: draw_glow(anim, time, scale, center, tint); break;
        case ROOM_ANIM_WATER: draw_water(anim, time, scale, center, tint); break;
        case ROOM_ANIM_DARK_FLICKER: draw_dark_flicker(anim, time, scale, center, tint); break;
        default: break;
        }
    }
}
