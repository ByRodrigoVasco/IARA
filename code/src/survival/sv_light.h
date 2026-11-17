#ifndef SV_LIGHT_H
#define SV_LIGHT_H

#include "raylib.h"

/*
 * Iluminação do Modo Sobrevivência: o escritório está apagado e só se vê o que alguma luz alcança.
 *
 * Por cima do mundo vai uma camada de escuridão. Nela são somadas as luzes: a do jogador (que cresce
 * com o passivo Lanterna), as dos objetos do mapa (racks, monitores, máquinas de café...), as lâmpadas
 * do teto e o brilho dos chefes e dos chips de XP. Algumas lâmpadas têm defeito e piscam de vez em
 * quando, e às vezes a energia do andar inteiro oscila por um instante.
 *
 * A camada é desenhada em resolução de mundo (1 texel = 1 pixel da arte), então a borda da luz fica
 * em degraus, no mesmo estilo da pixel art.
 */

// Monta a lista de luzes fixas a partir dos objetos do mapa. Pode ser chamada mais de uma vez.
void sv_light_init(void);

void sv_light_unload(void);

// Desenha a escuridão e as luzes. Chamar depois de EndMode2D() e antes da interface.
void sv_light_draw(Camera2D camera);

// Raio da luz do jogador, em unidades de mundo.
float sv_light_vision_radius(void);

// Quanto o ambiente está claro no ato indicado (0 = breu, 1 = sem escuridão).
float sv_light_ambient_for_act(int act);

// Intensidade (0 a 1) de uma lâmpada com defeito no instante t. Pura, para os testes.
float sv_light_flicker(int seed, float t);

#endif // SV_LIGHT_H
