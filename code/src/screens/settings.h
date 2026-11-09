#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdbool.h>

// Linhas da tela de configurações, na ordem em que aparecem.
#define SETTINGS_CONTRAST 0
#define SETTINGS_BRIGHTNESS 1
#define SETTINGS_VOLUME 2
#define SETTINGS_FULLSCREEN 3
#define SETTINGS_BACK 4
#define SETTINGS_OPTION_COUNT 5

/**
 * @brief Carrega o fundo da tela de configurações e seleciona a primeira linha.
 */
void initialize_settings_screen(void);

/**
 * @brief Executa um quadro da tela de configurações.
 *
 * @return true quando o jogador escolhe voltar ao menu.
 */
bool render_settings_screen(void);

/**
 * @brief Aciona uma caixa de seleção ou pede a volta ao menu. Os sliders são ajustados pela navegação.
 *
 * @param option Linha acionada.
 * @return true se a linha acionada foi a de voltar ao menu.
 */
bool activate_settings_option(int option);

/**
 * @brief Libera o fundo carregado por initialize_settings_screen().
 */
void unload_settings_screen(void);

#endif // SETTINGS_H
