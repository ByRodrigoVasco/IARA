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

// Medidas da carta de escolha, em pixels de referência (1080p): multiplicar por sv_ui() para a tela.
#define SV_CARD_W 360.0f
#define SV_CARD_H 560.0f
#define SV_CARD_ICON_Y 80.0f
#define SV_CARD_TITLE_Y 140.0f
#define SV_CARD_TITLE_SIZE 31.0f
#define SV_CARD_TITLE_MIN 20.0f
#define SV_CARD_TAG_Y 188.0f
#define SV_CARD_TAG_SIZE 25.0f
#define SV_CARD_DESC_Y 232.0f
#define SV_CARD_DESC_SIZE 21.0f
#define SV_CARD_DESC_MIN 15.0f
#define SV_CARD_DESC_MARGIN 16.0f
#define SV_CARD_TEXT_MARGIN 12.0f
#define SV_CARD_CHANGE_SIZE 23.0f
#define SV_CARD_CHANGE_MIN 14.0f
#define SV_CARD_CHANGE_STEP 28.0f
#define SV_CARD_GAP 10.0f
#define SV_CARD_BOTTOM_MARGIN 16.0f
#define SV_CARD_MAX_DESC_LINES 8
#define SV_CARD_LINE_CHARS 260

// Onde cada parte do texto da carta fica (referência 1080p), calculado a partir do que a escolha diz.
typedef struct
{
    float title_size;   // Título, reduzido até caber na largura
    float desc_size;    // Descrição, reduzida até ela e as linhas de mudança caberem
    int desc_lines;
    float divider_y;    // Linha que separa a descrição do que muda
    float changes_y;
    float change_size;
    int change_count;
    float bottom;       // Onde termina a última linha de mudança
} SvCardLayout;

void sv_levelup_layout(const SvChoice *choice, SvCardLayout *layout);

// Texto no estilo do jogo. size é em pixels de tela.
void sv_text(const char *text, float x, float y, float size, Color color);
void sv_text_centered(const char *text, float center_x, float y, float size, Color color);
float sv_text_width(const char *text, float size);

// Fator de escala da interface do modo (1.0 em 1080p).
float sv_ui(void);

/** @brief Cor de cada jogador na dupla (0 vermelho, 1 azul), a mesma do HUD, das setas e dos nomes. */
Color sv_duo_color(int index);

/** @brief Retângulo do botão de velocidade do tempo no HUD (x1, x2, x3). */
Rectangle sv_speed_button_rect(void);

#endif // SV_RENDER_H
