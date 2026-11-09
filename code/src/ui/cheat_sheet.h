#ifndef CHEAT_SHEET_H
#define CHEAT_SHEET_H

#include "raylib.h"

// Quantidade de símbolos lógicos da colinha.
int get_cheat_sheet_entry_count(void);

// Explicação de um símbolo da colinha (0 até get_cheat_sheet_entry_count() - 1).
const char *get_cheat_sheet_entry_text(int index);

// Tamanho da fonte com que a colinha cabe na área indicada.
float get_cheat_sheet_font_size(Rectangle bounds);

/**
 * @brief Desenha a colinha, um post-it com os símbolos lógicos, na área indicada.
 */
void draw_cheat_sheet(Rectangle bounds);

/**
 * @brief Desenha a colinha como um post-it colado: levemente inclinado, preso por uma fita na borda de cima.
 *
 * @param bounds Área do post-it antes de inclinar.
 * @param tilt_degrees Inclinação em graus (negativa gira para a esquerda).
 */
void draw_cheat_sheet_stuck(Rectangle bounds, float tilt_degrees);

#endif // CHEAT_SHEET_H
