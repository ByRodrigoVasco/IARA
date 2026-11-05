#ifndef GAME_TEXTURE_H
#define GAME_TEXTURE_H

#include <stdbool.h>
#include "raylib.h"

// typedef define GAME_TEXTURE como um novo TIPO de dados
typedef struct GAME_TEXTURE
{
    Texture2D texture;
    int middle_width;
    int middle_height;
} GAME_TEXTURE;

GAME_TEXTURE CreateGameTextureFromTexture2D(Texture2D texture);
void UnloadGameTexture(GAME_TEXTURE Texture);

/*
 * @brief Copia o tamanho atual da janela para screen_width, screen_height e o meio da tela.
 */
void update_screen_metrics(void);

/*
 * @brief Fator de escala da interface: 1.0 em 1920x1080, menor em janelas menores.
 * Usa o eixo mais apertado para nada sair da janela, qualquer que seja o formato dela.
 */
float get_ui_scale(void);

/*
 * @brief Carrega uma imagem grande com suavização, para reduzir na hora de desenhar sem serrilhar.
 */
Texture2D load_smooth_texture(const char *path);

/*
 * @brief Desenha a textura cobrindo dest sem deformar: o que sobra é cortado igualmente dos lados.
 */
void draw_texture_cover(Texture2D texture, Rectangle dest, Color tint);

/*
 * @brief Alterna entre janela e tela cheia sem bordas. A interface se ajusta sozinha ao novo tamanho.
 */
void toggle_fullscreen(void);

/*
 * @brief Informa se o jogo está em tela cheia.
 */
bool is_fullscreen_enabled(void);

#endif // GAME_TEXTURE_H
