#ifndef SCREEN_STYLE_H
#define SCREEN_STYLE_H

#include <stdbool.h>
#include "raylib.h"

/*
 * Visual das telas de escolha (modo e save) e do carregamento: o mesmo fundo escuro
 * e a mesma fonte do menu, sem carregar nenhuma imagem pesada.
 */

#define STYLE_TEXT_COLOR ((Color){240, 234, 244, 255})
#define STYLE_DIM_COLOR ((Color){168, 156, 176, 255})
#define STYLE_BLOOD_COLOR ((Color){200, 12, 20, 255})
#define STYLE_WARNING_COLOR ((Color){255, 110, 96, 255})
#define STYLE_PANEL_COLOR ((Color){22, 14, 26, 245})

/** @brief Limpa a tela e desenha o fundo escuro com a vinheta roxa do menu. */
void style_backdrop(void);

/** @brief Título grande no alto da tela e, se houver, uma linha menor embaixo. */
void style_title(const char *title, const char *subtitle);

/** @brief Título centralizado no alto da tela e, se houver, uma linha menor embaixo. */
void style_title_centered(const char *title, const char *subtitle);

/** @brief Primeiro y do conteúdo das telas centralizadas, abaixo do título. */
float style_content_top(void);

/** @brief Moldura de cartão (fundo, sombra e borda). Escolhido: borda vermelha e fundo mais claro. */
void style_card_frame(Rectangle bounds, bool selected, bool accent);

/** @brief Texto com quebra de linha, cada linha centralizada na largura. Devolve a altura usada. */
float style_wrapped_text_centered(const char *text, float x, float y, float width, float size, Color color);

/** @brief Altura que uma linha de lista ocupa (com o espaço entre elas), no tamanho atual da janela. */
float style_row_height(void);

/** @brief Primeiro y da lista, abaixo do título. */
float style_list_top(void);

/** @brief Linha de lista: nome, uma linha de detalhe e a seta quando está escolhida. Devolve a área clicável. */
Rectangle style_row(float y, const char *label, const char *detail, bool selected);

/** @brief Dica de teclas no rodapé da tela. */
void style_hint(const char *text);

/** @brief Painel escuro com borda, para janelas por cima da tela. */
void style_panel(Rectangle bounds);

/** @brief Botão de uma janela. Devolve a área clicável. */
Rectangle style_button(Rectangle bounds, const char *label, bool selected, bool danger);

/** @brief Texto com quebra de linha dentro de uma largura. Devolve a altura usada. */
float style_wrapped_text(const char *text, float x, float y, float width, float size, Color color);

/** @brief Tamanho de fonte relativo ao do menu, já na escala da janela. */
float style_font_size(float ratio);

/** @brief Se o mouse está sobre a área (só com a janela em foco). */
bool style_mouse_over(Rectangle area);

/** @brief Barra de progresso com a porcentagem, centralizada. Usada pela tela de carregamento. */
void style_progress(float percent, const char *label);

/** @brief Barra pequena com a porcentagem, na parte de baixo da tela, para ficar por cima de outra tela (a abertura). */
void style_progress_small(float percent);

/** @brief Libera o que as telas de escolha carregaram. */
void style_unload(void);

#endif // SCREEN_STYLE_H
