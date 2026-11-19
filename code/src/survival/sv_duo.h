#ifndef SV_DUO_H
#define SV_DUO_H

#include <stdbool.h>
#include "raylib.h"
#include "sv_game.h"

/*
 * Modo Sobrevivência em dupla: dois jogadores no mesmo computador, na mesma tela.
 *
 * Como funciona por dentro: quase todo o código das armas, dos inimigos e dos itens fala com
 * sv_run.player (o "jogador ativo"). Na dupla, o jogador que não está ativo fica guardado em
 * sv_run.seats, e sv_duo_use() troca quem está em cena antes de cada parte da simulação que é de um
 * jogador só (as armas dele, os tiros dele, o inimigo que persegue ele). Assim o código do solo não
 * muda e, com sv_run.duo desligado, nada aqui é chamado.
 *
 * Regras da dupla:
 *  - Cada jogador tem os seus poderes, a sua vida e o seu personagem. A XP é dividida: um chip pego
 *    por qualquer um sobe a barra dos dois, e cada um escolhe a sua carta (primeiro o 1, depois o 2).
 *  - Quem fica sem vida cai. O parceiro levanta quem caiu ficando parado ao lado por SV_DUO_REVIVE_TIME.
 *    A partida só acaba quando os dois caem.
 *  - Os dois ficam sempre na mesma tela: um não consegue se afastar demais do outro.
 *  - Mais inimigos e chefes mais fortes, para a dupla não atravessar o jogo fácil demais.
 */

// O ranking das duplas é um arquivo próprio, separado do solo: SV_RANKING_DUO_PATH, em sv_ranking.h

// Dificuldade da dupla
#define SV_DUO_SPAWN_MULT 1.6f       // Inimigos nascendo
#define SV_DUO_ENEMY_HP_MULT 1.25f   // Vida dos inimigos
#define SV_DUO_BOSS_HP_MULT 1.6f     // Vida dos chefes

// Levantar o parceiro
#define SV_DUO_REVIVE_TIME 3.0f      // Segundos ao lado de quem caiu
#define SV_DUO_REVIVE_RANGE 40.0f    // Distância para contar como "ao lado"
#define SV_DUO_REVIVE_HP 0.5f        // Vida com que volta (fração da máxima)

// Distância máxima entre os dois (a câmera mostra 1280 x 720 de mundo numa tela 16:9)
#define SV_DUO_LEASH_X 1000.0f
#define SV_DUO_LEASH_Y 540.0f

// Distância entre os dois no começo da partida
#define SV_DUO_START_GAP 40.0f

/** @brief Põe o jogador index (0 ou 1) em cena: sv_run.player passa a ser ele. No solo não faz nada. */
void sv_duo_use(int index);

/** @brief Jogador index, esteja ele em cena ou guardado. No solo, index 0 é sv_run.player. */
SvPlayer *sv_duo_player(int index);

/** @brief Quantos jogadores a partida tem: 1 no solo, 2 na dupla. */
int sv_duo_count(void);

/** @brief Está de pé (não caiu). */
bool sv_duo_standing(int index);

/** @brief Jogador de pé mais perto do ponto, ou -1 se os dois caíram. */
int sv_duo_nearest(Vector2 pos);

/** @brief Centro da dupla: o meio entre os dois. É para onde a câmera olha e em volta de onde os inimigos nascem. No solo, a posição do jogador. */
Vector2 sv_duo_focus(void);

/** @brief Levantar quem caiu e manter os dois perto um do outro. Chamado a cada quadro da partida. */
void sv_duo_update(float dt);

/** @brief O jogador index cai (ficou sem vida e não tinha Ctrl+Z). */
void sv_duo_fall(int index);

/** @brief Os dois caíram: fim da partida. */
bool sv_duo_all_down(void);

/** @brief Quanto já foi feito para levantar o jogador index, de 0 a 1 (0 se ninguém está ao lado). */
float sv_duo_revive_progress(int index);

#endif // SV_DUO_H
