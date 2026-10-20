#ifndef CHARACTERS_H
#define CHARACTERS_H

#include "raylib.h"

// Índices das direções nas texturas do personagem (ver char_textures).
#define EAST      0
#define SOUTH     1
#define NORTH     2
#define WEST      3
#define SOUTHEAST 4
#define NORTHWEST 5
#define SOUTHWEST 6
#define NORTHEAST 7
#define DIRECTION_COUNT 8

// Conjunto de texturas de um personagem: uma por direção.
typedef Texture2D char_textures[DIRECTION_COUNT];

/**
 * @brief Carrega as 8 texturas direcionais do estagiário (jogador principal).
 *
 * @param out Array que recebe as texturas, indexado pelas constantes de direção.
 */
void initialize_estagiario_1_textures(char_textures out);

#define WALK_FRAME_COUNT 4       // Quantidade de frames da animação de andar
#define WALK_FRAME_TIME 0.12f    // Segundos que cada frame do andar fica na tela

/**
 * @brief Informa qual frame do andar mostrar.
 *
 * @param walk_time Segundos desde que o jogador começou a andar.
 * @return Índice do frame, de 0 a WALK_FRAME_COUNT - 1, repetindo em ciclo.
 */
int get_walk_frame(float walk_time);

/**
 * @brief Carrega os frames do andar do estagiário para o leste e para o oeste.
 *
 * @param east Array que recebe os frames do leste.
 * @param west Array que recebe os frames do oeste (espelho do leste).
 */
void initialize_estagiario_1_walk_textures(Texture2D east[WALK_FRAME_COUNT], Texture2D west[WALK_FRAME_COUNT]);

#endif // CHARACTERS_H
