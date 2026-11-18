#ifndef ROOMS_H
#define ROOMS_H

#include <stdbool.h>
#include "raylib.h"
#include "characters.h"

/*
 * Cômodos do modo história. O jogador vê um cômodo por vez: ao passar por uma porta, a tela
 * troca para o cômodo do outro lado (como em Zelda). O escritório é o cômodo central e continua
 * com o mapa, as colisões e os NPCs de sempre (game_loop.c); os outros vêm de rooms_data.h,
 * gerado por tools/historia/gerar_comodos.py.
 *
 * Todas as coordenadas estão no espaço de referência do office.png: em pixels do mapa, relativas
 * ao centro dele. O jogo converte para a tela com get_scale() e o centro da janela.
 */

typedef enum
{
    ROOM_ESCRITORIO,
    ROOM_CORREDOR,
    ROOM_COPA,
    ROOM_RH,
    ROOM_REUNIOES,
    ROOM_TI,
    ROOM_PORAO,
    ROOM_COUNT
} room_id;

#define ROOM_MAX_OBSTACLES 48   // Mais obstáculos que isso num cômodo: aumentar (o teste avisa)

typedef struct
{
    Rectangle zone;       // Pisar aqui leva ao outro cômodo
    room_id to_room;      // Cômodo do outro lado
    int to_door;          // Porta do outro cômodo por onde o jogador chega
    Vector2 spawn;        // Centro da hitbox do jogador quando ele chega por esta porta
    int facing;           // Direção em que ele chega olhando (NORTH, SOUTH... de characters.h)
    const char *label;    // Nome do cômodo do outro lado, mostrado perto da porta
} room_door;

typedef struct
{
    const char *name;               // Nome do cômodo (aparece no topo da tela)
    const char *image;              // Imagem do cômodo; NULL no escritório, que usa o office.png
    const Rectangle *obstacles;     // Paredes e móveis; NULL no escritório (colisões ficam no game_loop.c)
    int obstacle_count;
    const room_door *doors;
    int door_count;
} room_def;

/**
 * @brief Dados do cômodo, ou NULL se o id for inválido.
 */
const room_def *room_get(room_id id);

/**
 * @brief Porta cuja zona encosta na hitbox (em coordenadas de referência), ou -1 se nenhuma.
 */
int room_door_touching(room_id id, Rectangle hitbox_reference);

/**
 * @brief Porta do cômodo que leva de volta ao escritório (direto ou pelo corredor), ou -1 no escritório.
 */
int room_door_toward_office(room_id id);

#endif // ROOMS_H
