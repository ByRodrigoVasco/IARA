#ifndef OFFICE_SYMPTOMS_H
#define OFFICE_SYMPTOMS_H

#include "raylib.h"
#include "symptoms.h"

/**
 * @brief Cor da luz do escritório: o escurecimento do dia, mais fria com a automação.
 *
 * No nível forte de automação a luz falha de vez em quando: quedas curtas e suaves,
 * no máximo duas a cada ciclo de alguns segundos, longe do limite de 3 flashes por segundo.
 *
 * @param symptoms Sintomas atuais (symptoms_get_scene()).
 * @param brightness Brilho do dia, de 0 a 255, já com o escurecimento da campanha.
 * @param alpha Opacidade da cena (fade de entrada e saída).
 * @param time Tempo em segundos (GetTime()), para a falha da luz.
 */
Color office_symptoms_light_tint(const SceneSymptoms *symptoms, unsigned char brightness, unsigned char alpha, double time);

/**
 * @brief Desenha sobre o mapa os elementos que denunciam as estatísticas ocultas.
 *
 * Chamar logo depois de desenhar o office.png, antes dos NPCs e do jogador.
 *
 * @param symptoms Sintomas atuais (symptoms_get_scene()).
 * @param office Retângulo do office.png na tela.
 * @param alpha Opacidade da cena (fade de entrada e saída).
 * @param time Tempo em segundos (GetTime()), para as pulsações.
 */
void draw_office_symptoms(const SceneSymptoms *symptoms, Rectangle office, unsigned char alpha, double time);

/**
 * @brief Poças de luz das lâmpadas do teto no escritório escuro, com uma lâmpada quebrada que pisca.
 *
 * Só aparece depois que o escurecimento da campanha começa. Chamar antes de draw_office_symptoms(),
 * para os sinais continuarem legíveis por cima.
 *
 * @param office Retângulo do office.png na tela.
 * @param campaign_progress De 0 a 1, o quanto da campanha já passou (calendar_get_water_level()).
 * @param alpha Opacidade da cena (fade de entrada e saída).
 * @param time Tempo em segundos (GetTime()).
 */
void draw_office_broken_lamps(Rectangle office, float campaign_progress, unsigned char alpha, double time);

/**
 * @brief Cor com que um personagem deve ser desenhado num ponto do escritório escurecido.
 *
 * Parte da luz ambiente do cenário (a mesma que escurece o office.png) e acrescenta a luz das
 * lâmpadas, a quebrada inclusive: quem passa na poça de luz clareia, quem está no escuro fica escuro.
 *
 * @param ambient Tint aplicado ao office.png.
 * @param office Retângulo do office.png na tela.
 * @param point Ponto da tela onde o personagem pisa.
 * @param campaign_progress De 0 a 1 (calendar_get_water_level()).
 * @param time Tempo em segundos (GetTime()).
 */
Color office_symptoms_light_at(Color ambient, Rectangle office, Vector2 point, float campaign_progress, double time);

#endif // OFFICE_SYMPTOMS_H
