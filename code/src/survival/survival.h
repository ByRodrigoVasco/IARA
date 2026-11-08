#ifndef SURVIVAL_H
#define SURVIVAL_H

#include <stdbool.h>
#include "loading.h"

/*
 * Modo Sobrevivência: telas do modo (início, Plano de Carreira, partida, pausa e resultado).
 * A máquina de estados do jogo chama estas três funções, como faz com os créditos.
 */

// Os trabalhos de carregamento do modo, para a tela de carregamento rodar um pedaço por quadro. Devolve quantos são.
int survival_loading_jobs(loading_job *jobs);

// Carrega tudo de uma vez (sem tela de carregamento).
void initialize_survival(void);

// Desenha um quadro. Devolve true quando o jogador volta ao menu principal.
bool render_survival(void);

void unload_survival(void);

#endif // SURVIVAL_H
