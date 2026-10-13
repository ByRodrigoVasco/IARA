#ifndef ANIMATIONS_H
#define ANIMATIONS_H

#include <stdbool.h>
#include "raylib.h"

/**
 * @brief Aumenta o canal alpha da cor (fade in) a cada chamada.
 *
 * @param fade_speed            Quanto o alpha aumenta por chamada.
 * @param color                 Cor que será modificada.
 * @param is_animation_finished Recebe true quando o alpha chega ao máximo.
 */
void fade_in_texture(unsigned int fade_speed, Color *color, bool *is_animation_finished);

/**
 * @brief Reduz o canal alpha da cor (fade out) a cada chamada.
 *
 * @param fade_speed            Quanto o alpha diminui por chamada.
 * @param color                 Cor que será modificada.
 * @param is_animation_finished Recebe true quando o alpha chega a zero.
 */
void fade_out_texture(unsigned int fade_speed, Color *color, bool *is_animation_finished);

#endif // ANIMATIONS_H
