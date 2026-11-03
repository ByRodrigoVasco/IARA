#ifndef BUTTON_H
#define BUTTON_H

#include <stdbool.h>
#include "raylib.h"

/**
 * @brief Desenha um botão retangular com texto centralizado e detecta o clique.
 *
 * @param bounds     Área do botão na tela.
 * @param text       Texto exibido dentro do botão.
 * @param font_size  Tamanho do texto, calculado pela tela conforme o tamanho da janela.
 * @param fill       Cor do botão em repouso.
 * @param enabled    false desenha o botão sem reagir ao mouse (resposta já marcada).
 * @param is_clicked Recebe true quando o botão é clicado (mouse solto sobre a área).
 */
void draw_button(Rectangle bounds, const char *text, float font_size, Color fill, bool enabled, bool *is_clicked);

#endif // BUTTON_H
