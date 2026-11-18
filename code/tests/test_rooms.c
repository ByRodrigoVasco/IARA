// Testes dos cômodos do modo história (rooms.c e rooms_data.h, gerado por tools/historia/gerar_comodos.py):
// portas de ida e volta, todos os cômodos alcançáveis, chegada fora dos móveis e todas as portas
// alcançáveis andando dentro de cada cômodo.
// Compilar e rodar: tools/tests/run_tests.ps1
//   ou: gcc code/tests/test_rooms.c code/src/game/rooms.c -Icode/src/game -o test_rooms.exe -lraylib -lopengl32 -lgdi32 -lwinmm && ./test_rooms.exe
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "raylib.h"
#include "rooms.h"

// Hitbox do jogador no espaço de referência (game_loop.c: HITBOX_WIDTH/HEIGHT_AT_REFERENCE)
#define HITBOX_W 36.0f
#define HITBOX_H 24.0f
// Metade do mapa: os cômodos têm o tamanho do office.png
#define HALF_W 531.0f
#define HALF_H 438.0f
#define STEP 4.0f
#define GRID_W 257   // (2 * 531 - 36) / 4 + 1 posições da hitbox na horizontal
#define GRID_H 214   // (2 * 438 - 24) / 4 + 1 na vertical

static int total = 0;
static int passed = 0;

static void check(const char *id, const char *description, bool condition)
{
    total++;
    if (condition) passed++;
    printf("  [%s] %s: %s\n", condition ? "OK" : "FALHA", id, description);
}

static Rectangle hitbox_at_spawn(Vector2 spawn)
{
    return (Rectangle){spawn.x - HITBOX_W / 2.0f, spawn.y - HITBOX_H / 2.0f, HITBOX_W, HITBOX_H};
}

static bool hits_obstacle(const room_def *room, Rectangle box)
{
    for (int i = 0; i < room->obstacle_count; i++)
    {
        if (CheckCollisionRecs(room->obstacles[i], box)) return true;
    }
    return false;
}

static bool touches_any_door(const room_def *room, Rectangle box)
{
    for (int i = 0; i < room->door_count; i++)
    {
        if (CheckCollisionRecs(room->doors[i].zone, box)) return true;
    }
    return false;
}

static bool reached[GRID_H][GRID_W];
static int queue_x[GRID_W * GRID_H], queue_y[GRID_W * GRID_H];

// Anda pela grade a partir da chegada e diz se cada porta foi alcançada
static void walk_room(const room_def *room, Vector2 spawn, bool door_reached[])
{
    Rectangle start = hitbox_at_spawn(spawn);
    int sx = (int)((start.x + HALF_W) / STEP), sy = (int)((start.y + HALF_H) / STEP);
    int head = 0, tail = 0;

    memset(reached, 0, sizeof(reached));
    for (int d = 0; d < room->door_count; d++) door_reached[d] = false;
    reached[sy][sx] = true;
    queue_x[tail] = sx;
    queue_y[tail++] = sy;

    while (head < tail)
    {
        int x = queue_x[head], y = queue_y[head++];
        Rectangle box = {-HALF_W + x * STEP, -HALF_H + y * STEP, HITBOX_W, HITBOX_H};

        for (int d = 0; d < room->door_count; d++)
        {
            if (CheckCollisionRecs(room->doors[d].zone, box)) door_reached[d] = true;
        }

        const int dx[4] = {1, -1, 0, 0}, dy[4] = {0, 0, 1, -1};
        for (int k = 0; k < 4; k++)
        {
            int nx = x + dx[k], ny = y + dy[k];
            if (nx < 0 || ny < 0 || nx >= GRID_W || ny >= GRID_H || reached[ny][nx]) continue;

            Rectangle next = {-HALF_W + nx * STEP, -HALF_H + ny * STEP, HITBOX_W, HITBOX_H};
            if (hits_obstacle(room, next)) continue;
            reached[ny][nx] = true;
            queue_x[tail] = nx;
            queue_y[tail++] = ny;
        }
    }
}

int main(void)
{
    char text[160];

    printf("\nCOMODOS DO MODO HISTORIA\n");

    // RM-01: toda porta tem a porta de volta no outro cômodo
    bool reciprocal = true;
    for (int r = 0; r < ROOM_COUNT; r++)
    {
        const room_def *room = room_get((room_id)r);
        for (int d = 0; d < room->door_count; d++)
        {
            const room_door *door = &room->doors[d];
            const room_def *other = room_get(door->to_room);
            bool ok = other && door->to_door >= 0 && door->to_door < other->door_count &&
                      other->doors[door->to_door].to_room == (room_id)r && other->doors[door->to_door].to_door == d;
            if (!ok)
            {
                printf("    porta %d de %s nao volta\n", d, room->name);
                reciprocal = false;
            }
        }
    }
    check("RM-01", "Cada porta leva a uma porta que volta para ela", reciprocal);

    // RM-02: todos os cômodos alcançáveis a partir do escritório
    bool seen[ROOM_COUNT] = {false};
    room_id stack[ROOM_COUNT * 8];
    int top = 0;
    seen[ROOM_ESCRITORIO] = true;
    stack[top++] = ROOM_ESCRITORIO;
    while (top > 0)
    {
        const room_def *room = room_get(stack[--top]);
        for (int d = 0; d < room->door_count; d++)
        {
            room_id next = room->doors[d].to_room;
            if (!seen[next])
            {
                seen[next] = true;
                stack[top++] = next;
            }
        }
    }
    bool all_seen = true;
    for (int r = 0; r < ROOM_COUNT; r++) all_seen = all_seen && seen[r];
    check("RM-02", "Todos os comodos sao alcancaveis a partir do escritorio", all_seen);

    // RM-03 e RM-04: limites e imagens
    bool within_limit = true, images_ok = true;
    for (int r = 0; r < ROOM_COUNT; r++)
    {
        const room_def *room = room_get((room_id)r);
        within_limit = within_limit && room->obstacle_count <= ROOM_MAX_OBSTACLES;
        if (r != ROOM_ESCRITORIO) images_ok = images_ok && room->image && FileExists(room->image);
    }
    check("RM-03", "Nenhum comodo passa de ROOM_MAX_OBSTACLES obstaculos", within_limit);
    check("RM-04", "Cada comodo (fora o escritorio) tem a imagem no disco", images_ok);

    // RM-05: chegar por uma porta nunca prende o jogador num móvel nem o devolve pela porta na hora
    bool spawns_ok = true;
    for (int r = 0; r < ROOM_COUNT; r++)
    {
        const room_def *room = room_get((room_id)r);
        for (int d = 0; d < room->door_count; d++)
        {
            Rectangle box = hitbox_at_spawn(room->doors[d].spawn);
            if (hits_obstacle(room, box) || touches_any_door(room, box))
            {
                printf("    chegada pela porta %d de %s presa ou em cima de uma porta\n", d, room->name);
                spawns_ok = false;
            }
        }
    }
    check("RM-05", "A chegada por cada porta fica livre de moveis e fora das zonas de porta", spawns_ok);

    // RM-06: andando, o jogador alcança todas as portas do cômodo, chegue por onde chegar
    for (int r = 1; r < ROOM_COUNT; r++)
    {
        const room_def *room = room_get((room_id)r);
        bool all_doors = true;
        for (int d = 0; d < room->door_count; d++)
        {
            bool door_reached[16];
            walk_room(room, room->doors[d].spawn, door_reached);
            for (int k = 0; k < room->door_count; k++)
            {
                if (!door_reached[k])
                {
                    printf("    em %s, vindo pela porta %d, nao se chega na porta %d (%s)\n", room->name, d, k, room->doors[k].label);
                    all_doors = false;
                }
            }
        }
        snprintf(text, sizeof(text), "Em %s todas as portas sao alcancaveis andando", room->name);
        check("RM-06", text, all_doors);
    }

    // RM-07: fora do escritório sempre há o caminho de volta para a seta apontar
    bool way_back = room_door_toward_office(ROOM_ESCRITORIO) == -1;
    for (int r = 1; r < ROOM_COUNT; r++) way_back = way_back && room_door_toward_office((room_id)r) >= 0;
    check("RM-07", "Todo comodo tem a porta de volta para o escritorio", way_back);

    printf("\n%d de %d testes passaram\n", passed, total);
    return (passed == total) ? 0 : 1;
}
