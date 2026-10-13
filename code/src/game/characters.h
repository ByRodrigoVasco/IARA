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

#endif // CHARACTERS_H
