#ifndef SV_RANKING_CLOUD_H
#define SV_RANKING_CLOUD_H

#include <stdbool.h>
#include "sv_ranking.h"

/*
 * Ranking de todos os notebooks (cloud.h). O ranking local (sv_ranking.h) continua sendo gravado e
 * é a reserva: vale quando a nuvem está desligada ou sem conexão.
 */

// O servidor guarda um número só (0 a 255) para o personagem. Na dupla ele leva os dois:
// J1 + 16 x (J2 + 1). Abaixo de 16 há um personagem só (o ranking solo). Cabem até 15 personagens.
// A página do ranking ao vivo decodifica do mesmo jeito.
#define SV_CLOUD_CHARACTER_BASE 16
#define SV_CLOUD_DUO_CHARACTERS(j1, j2) ((j1) + SV_CLOUD_CHARACTER_BASE * ((j2) + 1))

typedef enum
{
    SV_SEND_NONE = 0,   // Nenhuma partida enviada nesta tela
    SV_SEND_WAITING,    // Enviando
    SV_SEND_DONE,       // O servidor respondeu: sv_ranking_cloud_position()
    SV_SEND_REFUSED,    // O servidor não aceitou a partida
    SV_SEND_OFFLINE     // Sem resposta: está na fila e sobe depois
} SvSendState;

/** @brief Começa de novo para o ranking solo ou o de duplas, e busca na nuvem. */
void sv_ranking_cloud_open(bool duo);

/** @brief Um quadro: recebe as respostas e atualiza a tabela de tempos em tempos. */
void sv_ranking_cloud_update(void);

/** @brief Manda uma partida sem trapaça para o ranking da nuvem. */
void sv_ranking_cloud_submit(const char *name, int seconds, int kills, int level, int bosses, int character, bool won, int extra_kills);

/** @brief Esquece o envio da partida anterior. */
void sv_ranking_cloud_new_run(void);

/** @brief true quando a tabela da nuvem está disponível e atualizada. */
bool sv_ranking_cloud_online(void);

int sv_ranking_cloud_count(void);
const SvRankEntry *sv_ranking_cloud_get(int index);   // 0 = primeiro lugar

SvSendState sv_ranking_cloud_send_state(void);
int sv_ranking_cloud_position(void);                  // 1 = primeiro lugar

#endif // SV_RANKING_CLOUD_H
