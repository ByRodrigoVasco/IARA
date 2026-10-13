/**
 * @file pre_menu.h
 * @brief Declarações para a tela de pré-menu do jogo.
 */

// INCLUDE GUARDS
#ifndef PRE_MENU_H
#define PRE_MENU_H

#include "raylib.h"
#include "helper.h"
#include <stdbool.h>

// -----------------------------------------------------------------------------
// Variáveis Globais
// -----------------------------------------------------------------------------

extern bool can_close_pre_menu_safely;  // Variável global para indicar se a animação do pré-menu terminou

// -----------------------------------------------------------------------------
// Declarações de Funções
// -----------------------------------------------------------------------------

/**
 * @brief Inicializa as texturas e cores necessárias para a tela de pré-menu.
 * Deve ser chamada apenas uma vez na inicialização deste estado do jogo.
 */
void initialize_pre_menu_textures(void);

/**
 * @brief Renderiza os elementos do pré-menu na tela.
 * Lembre-se que BeginDrawing() e ClearBackground() podem ser chamados no loop principal 
 * dependendo da arquitetura do seu game_state.
 */
void render_pre_menu(void);

/**
 * @brief Faz a animação de fade_out antes de descarregar as texturas.
 */
void close_pre_menu(void);

/**
 * @brief Descarrega as texturas do pré-menu da memória de vídeo (VRAM).
 * Essencial chamar antes de fechar o jogo ou trocar de cena para evitar Memory Leaks.
 */
void unload_pre_menu_textures(void);

#ifdef __cplusplus
}
#endif

#endif // PRE_MENU_H