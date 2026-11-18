// Gerado por tools/historia/gerar_comodos.py: não editar à mão (mude o script e gere de novo).
// Coordenadas no espaço do office.png, relativas ao centro (as mesmas das colisões do game_loop.c).
// Porta: zona que leva ao outro cômodo, cômodo e porta de destino, centro da hitbox do jogador ao chegar,
// direção em que ele chega olhando e o nome que aparece perto da porta.
#ifndef ROOMS_DATA_H
#define ROOMS_DATA_H

#include <stddef.h>
#include "rooms.h"

static const room_door room_doors_escritorio[] = {
    {{244.0f, -344.0f, 56.0f, 24.0f}, ROOM_CORREDOR, 0, {272.0f, -296.0f}, SOUTH, "CORREDOR"},
};

static const Rectangle room_obstacles_corredor[] = {
    {-531.0f, -438.0f, 1062.0f, 320.0f},
    {-531.0f, -118.0f, 80.0f, 556.0f},
    {451.0f, -118.0f, 80.0f, 556.0f},
    {-451.0f, 162.0f, 72.0f, 276.0f},
    {-379.0f, 190.0f, 104.0f, 248.0f},
    {-275.0f, 162.0f, 424.0f, 276.0f},
    {149.0f, 190.0f, 104.0f, 248.0f},
    {253.0f, 162.0f, 198.0f, 276.0f},
    {355.0f, -118.0f, 8.0f, 196.0f},
    {-425.1f, -69.2f, 36.1f, 19.2f},
    {-270.9f, -67.6f, 99.8f, 21.6f},
    {-56.8f, -74.2f, 27.5f, 16.2f},
    {15.2f, -74.2f, 27.5f, 16.2f},
    {271.5f, -69.2f, 43.0f, 19.2f},
};
static const room_door room_doors_corredor[] = {
    {{-379.0f, 166.0f, 104.0f, 20.0f}, ROOM_ESCRITORIO, 0, {-327.0f, 118.0f}, NORTH, "ESCRITÓRIO"},
    {{-359.0f, -118.0f, 64.0f, 16.0f}, ROOM_COPA, 0, {-327.0f, -66.0f}, SOUTH, "COPA"},
    {{-147.0f, -118.0f, 64.0f, 16.0f}, ROOM_RH, 0, {-115.0f, -66.0f}, SOUTH, "RH"},
    {{73.0f, -118.0f, 128.0f, 16.0f}, ROOM_REUNIOES, 0, {137.0f, -66.0f}, SOUTH, "SALA DE REUNIÕES"},
    {{149.0f, 166.0f, 104.0f, 20.0f}, ROOM_TI, 0, {201.0f, 118.0f}, NORTH, "TI"},
    {{375.0f, -78.0f, 68.0f, 188.0f}, ROOM_PORAO, 0, {311.0f, 102.0f}, WEST, "PORÃO"},
};

static const Rectangle room_obstacles_copa[] = {
    {-531.0f, -438.0f, 1062.0f, 260.0f},
    {-531.0f, -178.0f, 192.0f, 616.0f},
    {341.0f, -178.0f, 190.0f, 616.0f},
    {-339.0f, 262.0f, 288.0f, 176.0f},
    {-51.0f, 290.0f, 104.0f, 148.0f},
    {53.0f, 262.0f, 288.0f, 176.0f},
    {-328.0f, -82.0f, 74.0f, 36.0f},
    {-235.4f, -114.0f, 80.8f, 24.0f},
    {-141.4f, -113.0f, 80.8f, 27.0f},
    {-47.4f, -114.0f, 80.8f, 24.0f},
    {46.6f, -114.0f, 80.8f, 24.0f},
    {138.3f, -114.0f, 53.3f, 24.0f},
    {200.3f, -118.0f, 89.4f, 36.0f},
    {299.2f, -114.2f, 27.5f, 16.2f},
    {-205.7f, 108.8f, 53.3f, 25.2f},
    {154.3f, 108.8f, 53.3f, 25.2f},
    {-192.8f, 166.8f, 27.5f, 19.2f},
    {167.2f, 166.8f, 27.5f, 19.2f},
    {-321.7f, 221.8f, 53.3f, 28.2f},
    {-334.6f, 86.0f, 79.1f, 36.0f},
    {295.2f, 219.0f, 27.5f, 27.0f},
    {299.2f, 45.8f, 27.5f, 16.2f},
};
static const room_door room_doors_copa[] = {
    {{-51.0f, 266.0f, 104.0f, 20.0f}, ROOM_CORREDOR, 1, {1.0f, 218.0f}, NORTH, "CORREDOR"},
};

static const Rectangle room_obstacles_rh[] = {
    {-531.0f, -438.0f, 1062.0f, 260.0f},
    {-531.0f, -178.0f, 192.0f, 616.0f},
    {341.0f, -178.0f, 190.0f, 616.0f},
    {-339.0f, 262.0f, 288.0f, 176.0f},
    {-51.0f, 290.0f, 104.0f, 148.0f},
    {53.0f, 262.0f, 288.0f, 176.0f},
    {-328.8f, -100.0f, 51.6f, 30.0f},
    {-264.8f, -100.0f, 51.6f, 30.0f},
    {-193.4f, -91.0f, 44.7f, 33.0f},
    {215.2f, -100.0f, 51.6f, 30.0f},
    {279.2f, -100.0f, 51.6f, 30.0f},
    {-43.7f, 34.6f, 89.4f, 23.4f},
    {-54.5f, 102.0f, 31.0f, 16.0f},
    {25.5f, 102.0f, 31.0f, 16.0f},
    {122.1f, 14.0f, 37.8f, 16.0f},
    {-119.3f, -66.0f, 32.7f, 16.0f},
    {-279.7f, 211.8f, 89.4f, 22.2f},
    {-328.8f, 119.8f, 51.6f, 22.2f},
    {-261.7f, 116.8f, 53.3f, 25.2f},
    {235.2f, 191.8f, 51.6f, 22.2f},
    {295.8f, 218.8f, 34.4f, 31.2f},
    {181.2f, 223.4f, 39.6f, 18.6f},
};
static const room_door room_doors_rh[] = {
    {{-51.0f, 266.0f, 104.0f, 20.0f}, ROOM_CORREDOR, 2, {1.0f, 218.0f}, NORTH, "CORREDOR"},
};

static const Rectangle room_obstacles_reunioes[] = {
    {-531.0f, -438.0f, 1062.0f, 248.0f},
    {-531.0f, -190.0f, 132.0f, 628.0f},
    {401.0f, -190.0f, 130.0f, 628.0f},
    {-399.0f, 282.0f, 348.0f, 156.0f},
    {-51.0f, 310.0f, 104.0f, 128.0f},
    {53.0f, 282.0f, 348.0f, 156.0f},
    {-179.0f, 50.0f, 360.0f, 72.0f},
    {-47.2f, -97.2f, 96.3f, 31.2f},
    {-371.1f, -104.2f, 72.2f, 34.2f},
    {-280.8f, -106.0f, 27.5f, 36.0f},
    {219.7f, -119.6f, 106.6f, 21.6f},
    {355.2f, -113.0f, 27.5f, 27.0f},
    {-148.8f, 130.8f, 27.5f, 19.2f},
    {-60.8f, 130.8f, 27.5f, 19.2f},
    {31.2f, 130.8f, 27.5f, 19.2f},
    {123.2f, 130.8f, 27.5f, 19.2f},
    {-211.9f, 79.2f, 25.8f, 22.8f},
    {188.1f, 79.2f, 25.8f, 22.8f},
    {-354.3f, 241.4f, 118.7f, 24.6f},
    {-384.8f, 246.0f, 27.5f, 16.0f},
    {268.3f, 237.4f, 65.4f, 24.6f},
    {361.0f, 233.8f, 24.1f, 28.2f},
};
static const room_door room_doors_reunioes[] = {
    {{-51.0f, 286.0f, 104.0f, 20.0f}, ROOM_CORREDOR, 3, {1.0f, 238.0f}, NORTH, "CORREDOR"},
};

static const Rectangle room_obstacles_ti[] = {
    {-531.0f, -438.0f, 1062.0f, 280.0f},
    {-531.0f, -158.0f, 192.0f, 596.0f},
    {341.0f, -158.0f, 190.0f, 596.0f},
    {-339.0f, 282.0f, 680.0f, 156.0f},
    {-334.0f, -54.0f, 110.1f, 36.0f},
    {-226.0f, -54.0f, 110.1f, 36.0f},
    {146.0f, -54.0f, 110.1f, 36.0f},
    {242.0f, -54.0f, 110.1f, 36.0f},
    {-284.6f, 126.0f, 275.2f, 36.0f},
    {155.7f, 118.0f, 82.6f, 24.0f},
    {181.5f, 163.2f, 31.0f, 22.8f},
    {32.8f, 243.6f, 120.4f, 26.4f},
    {-314.5f, 245.2f, 31.0f, 16.8f},
    {-269.9f, 254.0f, 37.8f, 16.0f},
    {283.2f, 237.8f, 27.5f, 16.2f},
};
static const room_door room_doors_ti[] = {
    {{-31.0f, -158.0f, 64.0f, 16.0f}, ROOM_CORREDOR, 4, {1.0f, -106.0f}, SOUTH, "CORREDOR"},
};

static const Rectangle room_obstacles_porao[] = {
    {-531.0f, -438.0f, 1062.0f, 248.0f},
    {-531.0f, -190.0f, 132.0f, 628.0f},
    {401.0f, -190.0f, 130.0f, 628.0f},
    {-399.0f, 310.0f, 800.0f, 128.0f},
    {305.0f, -190.0f, 8.0f, 88.0f},
    {-378.6f, -86.0f, 79.1f, 36.0f},
    {-272.8f, -86.0f, 75.7f, 36.0f},
    {-176.8f, -86.0f, 75.7f, 36.0f},
    {-370.5f, 74.0f, 31.0f, 16.0f},
    {-339.6f, 134.0f, 41.3f, 16.0f},
    {-367.6f, 194.0f, 41.3f, 16.0f},
    {189.0f, 214.0f, 24.1f, 16.0f},
    {226.7f, 234.0f, 20.6f, 16.0f},
    {36.9f, -74.8f, 48.2f, 28.8f},
    {326.1f, 266.0f, 37.8f, 16.0f},
    {289.5f, 257.2f, 31.0f, 16.8f},
    {-67.0f, 109.4f, 136.0f, 97.1f},
};
static const room_door room_doors_porao[] = {
    {{325.0f, -166.0f, 68.0f, 80.0f}, ROOM_CORREDOR, 5, {357.0f, 10.0f}, SOUTH, "CORREDOR"},
};

static const room_def room_defs[ROOM_COUNT] = {
    [ROOM_ESCRITORIO] = {"ESCRITÓRIO", NULL, NULL, 0, room_doors_escritorio, (int)(sizeof(room_doors_escritorio) / sizeof(room_doors_escritorio[0]))},
    [ROOM_CORREDOR] = {"CORREDOR", "code/assets/sprites/environment/comodos/corredor.png", room_obstacles_corredor, (int)(sizeof(room_obstacles_corredor) / sizeof(room_obstacles_corredor[0])), room_doors_corredor, (int)(sizeof(room_doors_corredor) / sizeof(room_doors_corredor[0]))},
    [ROOM_COPA] = {"COPA", "code/assets/sprites/environment/comodos/copa.png", room_obstacles_copa, (int)(sizeof(room_obstacles_copa) / sizeof(room_obstacles_copa[0])), room_doors_copa, (int)(sizeof(room_doors_copa) / sizeof(room_doors_copa[0]))},
    [ROOM_RH] = {"RH", "code/assets/sprites/environment/comodos/rh.png", room_obstacles_rh, (int)(sizeof(room_obstacles_rh) / sizeof(room_obstacles_rh[0])), room_doors_rh, (int)(sizeof(room_doors_rh) / sizeof(room_doors_rh[0]))},
    [ROOM_REUNIOES] = {"SALA DE REUNIÕES", "code/assets/sprites/environment/comodos/reunioes.png", room_obstacles_reunioes, (int)(sizeof(room_obstacles_reunioes) / sizeof(room_obstacles_reunioes[0])), room_doors_reunioes, (int)(sizeof(room_doors_reunioes) / sizeof(room_doors_reunioes[0]))},
    [ROOM_TI] = {"TI", "code/assets/sprites/environment/comodos/ti.png", room_obstacles_ti, (int)(sizeof(room_obstacles_ti) / sizeof(room_obstacles_ti[0])), room_doors_ti, (int)(sizeof(room_doors_ti) / sizeof(room_doors_ti[0]))},
    [ROOM_PORAO] = {"PORÃO", "code/assets/sprites/environment/comodos/porao.png", room_obstacles_porao, (int)(sizeof(room_obstacles_porao) / sizeof(room_obstacles_porao[0])), room_doors_porao, (int)(sizeof(room_doors_porao) / sizeof(room_doors_porao[0]))},
};

#endif // ROOMS_DATA_H
