/**
 * @file fonts.h
 * @brief Fontes e funções de manipulação de fontes para o jogo.
 */

#ifndef FONTS_H
#define FONTS_H

#include "raylib.h"

// Fonte padrão usada em todos os textos do jogo.
extern Font default_game_font;

/**
 * @brief Carrega a fonte padrão (ASCII e Latin-1, para suportar acentos).
 * Deve ser chamada depois de InitWindow().
 */
void initialize_fonts(void);

/**
 * @brief Libera a fonte da memória. Chamar antes de CloseWindow().
 */
void unload_fonts(void);

#endif // FONTS_H
