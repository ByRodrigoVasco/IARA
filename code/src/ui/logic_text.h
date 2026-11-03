#ifndef LOGIC_TEXT_H
#define LOGIC_TEXT_H

#include "raylib.h"

/**
 * @brief Mede um texto que pode ter símbolos lógicos (¬ ∧ ∨ → ↔ ∀ ∃).
 */
Vector2 measure_logic_text(const char *text, float font_size, float letter_spacing);

/**
 * @brief Desenha um texto com a fonte do jogo. Ela não tem os símbolos lógicos, então eles saem em traços.
 */
void draw_logic_text(const char *text, Vector2 position, float font_size, float letter_spacing, Color color);

/**
 * @brief Desenha um símbolo lógico dentro de uma célula quadrada.
 *
 * @param codepoint Código Unicode do símbolo.
 */
void draw_logic_symbol(int codepoint, Vector2 origin, float size, float thickness, Color color);

#endif // LOGIC_TEXT_H
