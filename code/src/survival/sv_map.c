#include <math.h>
#include <string.h>
#include "sv_map.h"
#include "sv_assets.h"
#include "sv_game.h"

#include "sv_map_data.h"

#define GRID_SIZE 64
#define GRID_COLS ((SV_WORLD_W / GRID_SIZE) + 1)
#define GRID_ROWS ((SV_WORLD_H / GRID_SIZE) + 1)

static int cell_start[GRID_COLS * GRID_ROWS + 1];
static short cell_items[SV_MAP_PROP_COUNT * 12];
static bool ready = false;

static void cell_range(const SvMapProp *p, int *c0, int *r0, int *c1, int *r1)
{
    *c0 = (int)(p->rx / GRID_SIZE);
    *r0 = (int)(p->ry / GRID_SIZE);
    *c1 = (int)((p->rx + p->rw) / GRID_SIZE);
    *r1 = (int)((p->ry + p->rh) / GRID_SIZE);
    if (*c0 < 0) *c0 = 0;
    if (*r0 < 0) *r0 = 0;
    if (*c1 >= GRID_COLS) *c1 = GRID_COLS - 1;
    if (*r1 >= GRID_ROWS) *r1 = GRID_ROWS - 1;
}

void sv_map_init(void)
{
    static int counts[GRID_COLS * GRID_ROWS];
    int total = 0;

    if (ready) return;
    memset(counts, 0, sizeof(counts));

    for (int i = 0; i < SV_MAP_PROP_COUNT; i++)
    {
        int c0, r0, c1, r1;

        cell_range(&sv_map_props[i], &c0, &r0, &c1, &r1);
        for (int r = r0; r <= r1; r++)
        {
            for (int c = c0; c <= c1; c++) counts[r * GRID_COLS + c]++;
        }
    }

    for (int i = 0; i < GRID_COLS * GRID_ROWS; i++)
    {
        cell_start[i] = total;
        total += counts[i];
        counts[i] = 0;
    }
    cell_start[GRID_COLS * GRID_ROWS] = total;

    for (int i = 0; i < SV_MAP_PROP_COUNT; i++)
    {
        int c0, r0, c1, r1;

        cell_range(&sv_map_props[i], &c0, &r0, &c1, &r1);
        for (int r = r0; r <= r1; r++)
        {
            for (int c = c0; c <= c1; c++)
            {
                int cell = r * GRID_COLS + c;

                cell_items[cell_start[cell] + counts[cell]++] = (short)i;
            }
        }
    }

    ready = true;
}

int sv_map_prop_count(void)
{
    return SV_MAP_PROP_COUNT;
}

const SvMapProp *sv_map_prop(int index)
{
    return &sv_map_props[index];
}

// Empurra o círculo (cx, cy, r) para fora de um retângulo. Devolve a distância empurrada.
static float push_from_rect(float *cx, float *cy, float r, const SvMapProp *p)
{
    float left = p->rx, right = p->rx + p->rw, top = p->ry, bottom = p->ry + p->rh;
    float nx = fminf(fmaxf(*cx, left), right);
    float ny = fminf(fmaxf(*cy, top), bottom);
    float dx = *cx - nx, dy = *cy - ny;
    float d2 = dx * dx + dy * dy;

    if (d2 >= r * r) return 0.0f;

    if (d2 > 0.0001f)
    {
        float d = sqrtf(d2), push = r - d;

        *cx += dx / d * push;
        *cy += dy / d * push;
        return push;
    }

    // O centro está dentro da caixa: sai pelo lado mais próximo
    {
        float to_left = *cx - left, to_right = right - *cx, to_top = *cy - top, to_bottom = bottom - *cy;
        float m = fminf(fminf(to_left, to_right), fminf(to_top, to_bottom));

        if (m == to_left) { *cx = left - r; return to_left + r; }
        if (m == to_right) { *cx = right + r; return to_right + r; }
        if (m == to_top) { *cy = top - r; return to_top + r; }
        *cy = bottom + r;
        return to_bottom + r;
    }
}

float sv_map_collide(Vector2 *pos, float radius, float foot_offset)
{
    float cx = pos->x, cy = pos->y + foot_offset;
    float total = 0.0f;

    if (!ready) sv_map_init();

    for (int pass = 0; pass < 2; pass++)
    {
        int c0 = (int)((cx - radius) / GRID_SIZE), c1 = (int)((cx + radius) / GRID_SIZE);
        int r0 = (int)((cy - radius) / GRID_SIZE), r1 = (int)((cy + radius) / GRID_SIZE);

        if (c0 < 0) c0 = 0;
        if (r0 < 0) r0 = 0;
        if (c1 >= GRID_COLS) c1 = GRID_COLS - 1;
        if (r1 >= GRID_ROWS) r1 = GRID_ROWS - 1;

        for (int r = r0; r <= r1; r++)
        {
            for (int c = c0; c <= c1; c++)
            {
                int cell = r * GRID_COLS + c;

                for (int k = cell_start[cell]; k < cell_start[cell + 1]; k++) total += push_from_rect(&cx, &cy, radius, &sv_map_props[cell_items[k]]);
            }
        }
    }

    pos->x = cx;
    pos->y = cy - foot_offset;
    return total;
}

bool sv_map_point_blocked(Vector2 point, float radius)
{
    int c = (int)(point.x / GRID_SIZE), r = (int)(point.y / GRID_SIZE);

    if (!ready) sv_map_init();
    if (c < 0 || r < 0 || c >= GRID_COLS || r >= GRID_ROWS) return false;

    for (int k = cell_start[r * GRID_COLS + c]; k < cell_start[r * GRID_COLS + c + 1]; k++)
    {
        const SvMapProp *p = &sv_map_props[cell_items[k]];

        if (point.x >= p->rx - radius && point.x <= p->rx + p->rw + radius && point.y >= p->ry - radius && point.y <= p->ry + p->rh + radius) return true;
    }
    return false;
}
