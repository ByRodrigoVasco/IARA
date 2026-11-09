#ifndef UI_THEME_H
#define UI_THEME_H

#include <stdbool.h>
#include "raylib.h"

/*
 * Cores e peças comuns das telas de apoio (aviso, confirmação, ajuda e pausa), para o jogo
 * inteiro falar a mesma língua visual: o mesmo roxo escuro, o mesmo vermelho de destaque e
 * as mesmas dicas de tecla no rodapé.
 */

#define UI_TEXT_COLOR ((Color){240, 234, 244, 255})
#define UI_DIM_COLOR ((Color){168, 156, 176, 255})
#define UI_BLOOD_COLOR ((Color){200, 12, 20, 255})
#define UI_WARNING_COLOR ((Color){255, 110, 96, 255})
#define UI_SUCCESS_COLOR ((Color){150, 230, 160, 255})
#define UI_PANEL_COLOR ((Color){22, 14, 26, 250})
#define UI_BORDER_COLOR ((Color){120, 90, 140, 255})

/** @brief Tamanho de fonte relativo ao texto padrão, já na escala da janela. */
float ui_font_size(float ratio);

/** @brief Se o mouse está sobre a área (só com a janela em foco). */
bool ui_mouse_over(Rectangle area);

/** @brief Fundo escuro com a vinheta roxa, para telas inteiras. */
void ui_draw_backdrop(void);

/** @brief Escurece o que está atrás de uma janela. */
void ui_draw_dim(void);

/** @brief Painel com sombra e borda. */
void ui_draw_panel(Rectangle bounds);

/** @brief Botão com rótulo centralizado. Escolhido: fundo claro; perigo: vermelho. */
void ui_draw_button(Rectangle bounds, const char *label, bool selected, bool danger);

/** @brief Texto com quebra de linha. Devolve a altura usada. */
float ui_draw_wrapped(const char *text, float x, float y, float width, float size, Color color, bool centered);

/** @brief Altura que o texto ocupa com a quebra de linha, sem desenhar. */
float ui_measure_wrapped(const char *text, float width, float size);

/** @brief Texto centralizado na horizontal. */
void ui_draw_centered(const char *text, float center_x, float y, float size, Color color);

/** @brief Dicas de tecla no rodapé, centralizadas. */
void ui_draw_hint(const char *text);

#endif // UI_THEME_H
