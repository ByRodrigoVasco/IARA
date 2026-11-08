#ifndef SV_DEBUG_H
#define SV_DEBUG_H

#include <stdbool.h>

/*
 * Modo debug do Sobrevivência, para testar o jogo à mão.
 *
 *   F1  liga e desliga o modo debug. Ele mostra os obstáculos do mapa, os inimigos, os chefes, os
 *       tiros com a trajetória, as zonas e os perigos, e um painel com os números da partida e o
 *       tempo de cada quadro. Também libera todos os personagens.
 *   F2  (só com o debug ligado, durante a partida) abre o menu de cheats e para a partida: dar
 *       poderes e passivos, chamar chefes e inimigos, ficar invulnerável, pular o tempo e mais.
 *
 * Uma partida em que algum cheat foi usado não conta para o progresso do save.
 */

/** @brief Liga ou desliga o modo debug (F1). Ao desligar, os cheats ligados também são desligados. */
void sv_debug_toggle(void);

/** @brief Trata F2 e o menu de cheats. Devolve true se o menu está aberto (a partida deve ficar parada). */
bool sv_debug_update(void);

bool sv_debug_menu_open(void);

/** @brief Desenhos do mundo (obstáculos, hitboxes, trajetórias). Chamar dentro da câmera. */
void sv_debug_draw_world(void);

/** @brief Painel com os números da partida e o gráfico dos quadros. Chamar em coordenadas de tela. */
void sv_debug_draw_hud(void);

/** @brief Menu de cheats, por cima da partida. */
void sv_debug_draw_menu(void);

#endif // SV_DEBUG_H
