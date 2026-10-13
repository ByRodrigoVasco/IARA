#ifndef COLLISIONS_H
#define COLLISIONS_H

#include <stdbool.h>
#include "raylib.h"

/**
 * @brief Informa se o mouse está sobre um retângulo.
 *
 * @param collision_rectangle Área a ser testada.
 * @return true se a janela está em foco e o mouse está sobre a área.
 */
bool is_mouse_hovering(Rectangle collision_rectangle);

#endif // COLLISIONS_H
