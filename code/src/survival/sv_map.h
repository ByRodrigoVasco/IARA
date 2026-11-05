#ifndef SV_MAP_H
#define SV_MAP_H

#include <stdbool.h>
#include "raylib.h"

/*
 * Objetos soltos do mapa e a colisão deles. O chão é contínuo, sem paredes: só os objetos bloqueiam.
 * A caixa de colisão cobre a base do objeto, então dá para passar "atrás" de uma mesa e o desenho
 * (ordenado pela base) cobre quem está atrás.
 */

typedef struct
{
    int sprite;          // SvSpriteId
    float x, y;          // Centro da base do objeto
    float rx, ry, rw, rh; // Caixa de colisão (canto, largura e altura)
} SvMapProp;

void sv_map_init(void);

int sv_map_prop_count(void);
const SvMapProp *sv_map_prop(int index);

/**
 * @brief Empurra para fora dos objetos um círculo cujo centro fica foot_offset abaixo de pos.
 * @return Quantos pixels foi preciso empurrar (0 se estava livre).
 */
float sv_map_collide(Vector2 *pos, float radius, float foot_offset);

// true se o ponto (com a folga radius) está dentro de algum objeto. Serve para tiros.
bool sv_map_point_blocked(Vector2 point, float radius);

#endif // SV_MAP_H
