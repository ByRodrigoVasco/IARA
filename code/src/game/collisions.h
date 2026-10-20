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

/**
 * @brief Informa se a hitbox bate em algum retângulo de uma lista de obstáculos.
 *
 * @param hitbox Área a ser testada.
 * @param obstacles Lista de retângulos obstáculo.
 * @param count Quantidade de obstáculos na lista.
 * @return true se a hitbox bate em pelo menos um obstáculo.
 */
bool collides_with_any(Rectangle hitbox, const Rectangle *obstacles, int count);

/**
 * @brief Converte um retângulo medido no mapa de referência para coordenadas da tela.
 *
 * @param ref Retângulo na escala de referência.
 * @param scale Escala atual da tela.
 * @param origin Ponto da tela que corresponde à origem do mapa de referência.
 * @return Retângulo com posição e tamanho escalados, somado à origem.
 */
Rectangle scale_reference_rect(Rectangle ref, float scale, Vector2 origin);

#endif // COLLISIONS_H
