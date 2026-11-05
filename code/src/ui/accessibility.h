#ifndef ACCESSIBILITY_H
#define ACCESSIBILITY_H

#include <stdbool.h>

// Brilho do jogo: 1.0 mantém as cores originais.
#define BRIGHTNESS_MIN 0.5f
#define BRIGHTNESS_MAX 1.5f
#define BRIGHTNESS_STEP 0.1f

/**
 * @brief Liga ou desliga o modo de alto contraste, sem reiniciar o jogo (US19).
 */
void toggle_high_contrast(void);

/**
 * @brief Informa se o modo de alto contraste está ligado.
 *
 * @return true com o modo ligado.
 */
bool is_high_contrast_enabled(void);

/**
 * @brief Informa se o filtro de alto contraste foi compilado pela placa de vídeo.
 *
 * @return true se o filtro já foi carregado e é válido.
 */
bool is_high_contrast_filter_ready(void);

/**
 * @brief Começa a aplicar o filtro no que for desenhado, se o modo estiver ligado.
 */
void begin_high_contrast_filter(void);

/**
 * @brief Encerra o filtro iniciado por begin_high_contrast_filter().
 */
void end_high_contrast_filter(void);

/**
 * @brief Define o brilho do jogo, arredondado para o passo e limitado à faixa permitida.
 *
 * @param brightness Brilho desejado.
 */
void set_game_brightness(float brightness);

/**
 * @brief Brilho atual do jogo.
 *
 * @return Valor entre BRIGHTNESS_MIN e BRIGHTNESS_MAX.
 */
float get_game_brightness(void);

/**
 * @brief Aplica o brilho atual sobre tudo o que já foi desenhado na janela ou na textura ativa.
 */
void draw_brightness_overlay(void);

/**
 * @brief Começa um quadro do jogo: BeginDrawing() e, se ligado, o filtro de alto contraste.
 */
void begin_game_drawing(void);

/**
 * @brief Termina um quadro do jogo iniciado por begin_game_drawing().
 */
void end_game_drawing(void);

/**
 * @brief Libera o filtro de alto contraste, se tiver sido carregado.
 */
void unload_accessibility(void);

#endif // ACCESSIBILITY_H
