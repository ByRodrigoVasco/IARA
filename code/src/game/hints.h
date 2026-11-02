#ifndef HINTS_H
#define HINTS_H

#include <stdbool.h>
#include "raylib.h"

/**
 * @brief Zona em que o jogador consegue examinar o bilhete do mural.
 *
 * @param scale Escala atual do mapa.
 * @param center Centro do mapa na tela.
 * @return Zona de interação em coordenadas da tela.
 */
Rectangle get_hint_zone(float scale, Vector2 center);

/**
 * @brief Área do mural, onde a seta e o contorno do item são desenhados.
 *
 * @param scale Escala atual do mapa.
 * @param center Centro do mapa na tela.
 * @return Área do mural em coordenadas da tela.
 */
Rectangle get_hint_item_rectangle(float scale, Vector2 center);

/**
 * @brief Informa se a hitbox do jogador alcança o bilhete do mural.
 *
 * @param hitbox Hitbox atual do jogador.
 * @param scale Escala atual do mapa.
 * @param center Centro do mapa na tela.
 * @return true se a hitbox encosta na zona de interação.
 */
bool is_hitbox_on_hint(Rectangle hitbox, float scale, Vector2 center);

/**
 * @brief Abre o bilhete, se estiver fechado, ou o fecha. Não altera a pontuação (US04).
 */
void toggle_hint_note(void);

/**
 * @brief Informa se o bilhete está aberto na tela.
 *
 * @return true enquanto o jogador lê o bilhete.
 */
bool is_hint_note_open(void);

/**
 * @brief Quantidade de linhas do bilhete.
 *
 * @return Número de linhas devolvidas por get_hint_note_line().
 */
int get_hint_note_line_count(void);

/**
 * @brief Texto de uma linha do bilhete.
 *
 * @param index Linha desejada, de 0 a get_hint_note_line_count() - 1.
 * @return Texto da linha.
 */
const char *get_hint_note_line(int index);

/**
 * @brief Desenha o bilhete sobre o jogo, ajustado ao tamanho da janela, se estiver aberto.
 */
void draw_hint_note(void);

#endif // HINTS_H
