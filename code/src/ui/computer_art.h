#ifndef COMPUTER_ART_H
#define COMPUTER_ART_H

#include <stdbool.h>
#include "raylib.h"

/*
 * Arte da cena do computador, toda em pixel art desenhada em código (sem arquivos de imagem):
 * ícones do desktop, janela da IARA, teclado e mouse da mesa. O teclado e o mouse reagem às
 * teclas e aos cliques de verdade, e cada clique na tela solta uma onda que mostra onde foi.
 */

typedef enum {
    DESKTOP_ICON_TRASH,    // Lixeira
    DESKTOP_ICON_TICKETS,  // Chamados (um envelope)
    DESKTOP_ICON_CLOCK,    // Ponto (um relógio)
    DESKTOP_ICON_MANUAL    // Manual (um livro)
} DesktopIcon;

/** @brief Ícone do desktop de 16x16 pixels, esticado para caber em bounds (quadrado). */
void computer_art_draw_icon(DesktopIcon icon, Rectangle bounds);

/**
 * @brief Moldura da janela da IARA: borda e barra de título nas cores do desktop, três botões
 * quadrados e o corpo claro onde o quiz é jogado. O texto do título é desenhado por quem chama.
 *
 * @param window Retângulo total da janela.
 * @param title_height Altura da barra de título.
 */
void computer_art_draw_window(Rectangle window, float title_height);

/** @brief Largura, em pixels, de cada um dos três botões da barra de título (para alinhar o título). */
float computer_art_window_buttons_width(float title_height);

/** @brief Teclado da mesa: cada tecla que o jogador aperta afunda e acende enquanto está pressionada. */
void computer_art_draw_keyboard(Rectangle bounds);

/** @brief Mouse da mesa, com o botão esquerdo e o direito que afundam e acendem ao clicar. */
void computer_art_draw_mouse(Rectangle bounds);

/** @brief Solta uma onda na posição do ponteiro a cada clique dentro de `area`. Chamar uma vez por quadro. */
void computer_art_update_click_ripples(Rectangle area);

/** @brief Desenha as ondas de clique ainda vivas. `scale` é a altura da janela dividida por 900. */
void computer_art_draw_click_ripples(float scale);

/** @brief Apaga as ondas de clique (ao abrir ou fechar o computador). */
void computer_art_reset(void);

#endif // COMPUTER_ART_H
