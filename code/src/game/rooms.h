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

// Animações desenhadas por cima da imagem do cômodo (os pixels vêm das próprias peças: ver o gerador)
typedef enum
{
    ROOM_ANIM_LEDS,          // LEDs piscando como atividade de rede, cada um no seu ritmo
    ROOM_ANIM_LEDS_WAVE,     // LEDs do servidor vermelho acendendo em onda, de cima para baixo
    ROOM_ANIM_EYES,          // Olhos do servidor vermelho: brilham, piscam e acendem no defeito
    ROOM_ANIM_GLITCH,        // Servidor dando defeito de vez em quando: imagem tremendo, clarão e faíscas
    ROOM_ANIM_BLINK,         // Pixels que acendem e apagam juntos (o "ERRO" do console)
    ROOM_ANIM_SCREEN,        // Tela ligada: tremulação e uma linha de varredura descendo
    ROOM_ANIM_SLIDES,        // TV da sala de reuniões passando slides com gráficos
    ROOM_ANIM_STEAM,         // Vapor subindo (cafeteiras)
    ROOM_ANIM_GLOW,          // Luz pulsando (micro-ondas)
    ROOM_ANIM_WATER,         // Ondinhas na água do poço
    ROOM_ANIM_DARK_FLICKER,  // A luz do cômodo falha de vez em quando (desenhada por cima do jogador)
    ROOM_ANIM_TYPE_COUNT
} room_anim_type;

typedef struct
{
    float x, y;                // Canto de cima à esquerda do pixel, no espaço de referência
    unsigned char r, g, b;     // Cor original do pixel na imagem
} room_pixel;

typedef struct
{
    room_anim_type type;
    Rectangle area;            // Onde a animação acontece (espaço de referência)
    const room_pixel *pixels;  // Pixels da peça que mexem (LEDs, olhos, letras); NULL se não usa
    int pixel_count;
    float pixel_size;          // Tamanho de um pixel da arte no espaço de referência (2 nos cômodos, 1 no escritório)
    float phase;               // 0..1: desencontra animações iguais (dois servidores não falham juntos)
} room_anim;

typedef struct
{
    const char *name;               // Nome do cômodo (aparece no topo da tela)
    const char *image;              // Imagem do cômodo; NULL no escritório, que usa o office.png
    const Rectangle *obstacles;     // Paredes e móveis; NULL no escritório (colisões ficam no game_loop.c)
    int obstacle_count;
    const room_door *doors;
    int door_count;
    const room_anim *anims;         // Animações do cômodo; NULL se não tem
    int anim_count;
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

/**
 * @brief Desenha as animações do cômodo por cima da imagem dele.
 *
 * @param time Segundos de jogo (GetTime()); as animações dependem só dele, sem estado guardado.
 * @param scale Escala do mapa na tela (get_scale()).
 * @param center Centro do mapa na tela.
 * @param map Imagem do cômodo (o defeito do servidor recorta faixas dela para tremer).
 * @param art_scale Quantas unidades de referência vale um pixel da imagem (2 nos cômodos, 1 no escritório).
 * @param tint Brilho do cômodo (r = g = b) e opacidade do fade, os mesmos usados para desenhar o mapa.
 * @param foreground false: animações do cenário (antes do jogador); true: as que cobrem tudo (luz falhando).
 */
void room_draw_animations(room_id id, float time, float scale, Vector2 center, Texture2D map, float art_scale,
                          Color tint, bool foreground);

/**
 * @brief Diz se o servidor da área está dando defeito agora e há quanto tempo (0..1 da duração).
 */
bool room_glitch_active(Rectangle area, float time, float *progress);

#endif // ROOMS_H
