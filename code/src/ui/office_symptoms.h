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

#endif // OFFICE_SYMPTOMS_H
