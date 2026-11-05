#ifndef SV_RENDER_H
#define SV_RENDER_H

#include "raylib.h"
#include "sv_game.h"

/*
 * Desenho do Modo Sobrevivência: mundo (com câmera), interface da partida e escolha de nível.
 * Quem chama abre e fecha o quadro (begin_game_drawing / end_game_drawing).
 */

// Mundo e interface da partida.
void sv_render_game(void);

// Escolha de poder ao subir de nível. selected é a carta destacada (0 a 2).
void sv_render_levelup(int selected);
Rectangle sv_levelup_card(int index);

// Texto no estilo do jogo. size é em pixels de tela.
void sv_text(const char *text, float x, float y, float size, Color color);
void sv_text_centered(const char *text, float center_x, float y, float size, Color color);
float sv_text_width(const char *text, float size);

// Fator de escala da interface do modo (1.0 em 1080p).
float sv_ui(void);

#endif // SV_RENDER_H
