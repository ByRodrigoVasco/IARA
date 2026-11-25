#ifndef SV_ACHIEVEMENTS_H
#define SV_ACHIEVEMENTS_H

#include "sv_rules.h"

/*
 * Conquistas do Modo Sobrevivência. Hoje há uma para cada personagem liberado.
 *
 * Nada novo é gravado: a conquista sai do próprio progresso do save. Ao abrir o save, o que já estava
 * liberado é anotado em silêncio; o que passar a valer depois gera uma notificação, uma de cada vez,
 * com som.
 */

// Anota o que o save já tem, sem notificar, e esvazia a fila.
void sv_ach_reset(SvProgress progress);

// Põe na fila o que acabou de ser conquistado. Devolve quantas conquistas novas entraram.
int sv_ach_check(SvProgress progress);

// Personagem da notificação na tela, ou -1 se não há nenhuma.
int sv_ach_current(void);

// Quantas notificações esperam, contando a que está na tela.
int sv_ach_pending(void);

// Avança o tempo e desenha a notificação da vez. Chamar por último em cada quadro.
void sv_ach_draw(float dt);

#endif // SV_ACHIEVEMENTS_H
