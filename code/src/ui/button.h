#ifndef BUTTON_H
#define BUTTON_H

#include <stdbool.h>
#include "raylib.h"

/**
 * @brief Desenha um botão retangular com texto centralizado e detecta o clique.
 *
 * @param bounds     Área do botão na tela.
 * @param text       Texto exibido dentro do botão.
 * @param is_clicked Recebe true quando o botão é clicado (mouse solto sobre a área).
 */
void draw_button(Rectangle bounds, const char *text, bool *is_clicked);

#endif // BUTTON_H
