#ifndef AUDIO_H
#define AUDIO_H

#include <stdbool.h>

/**
 * @brief Inicializa o dispositivo de áudio e carrega músicas e efeitos sonoros.
 *
 * @return true se todos os recursos foram carregados; false em caso de falha.
 */
bool initialize_audio(void);

/**
 * @brief Mantém a música correspondente à tela ativa.
 *
 * @param screen Estado atual da tela do jogo.
 */
void update_audio_for_screen(int screen);

/**
 * @brief Ajusta a tensão da trilha do jogo: 0 toca só o ambiente calmo e 1 só a trilha de suspense.
 * A troca é gradual. Os primeiros dias ficam calmos, e a tensão sobe com o avanço da história.
 *
 * @param tension Valor de 0 a 1.
 */
void audio_set_game_tension(float tension);

/**
 * @brief Reproduz o efeito sonoro de clique do mouse.
 */
void play_click_sound(void);

/**
 * @brief Reproduz em sequência o som de passos enquanto o jogador realmente se move.
 *
 * @param is_walking true se o personagem mudou de posição neste quadro.
 */
void update_player_footstep_audio(bool is_walking);

/**
 * @brief Mantém o blip da IARA tocando enquanto o texto é digitado e o interrompe ao terminar.
 *
 * @param is_typing true enquanto ainda há texto sendo revelado.
 */
void update_iara_voice_typing(bool is_typing);

/**
 * @brief Para as músicas, descarrega os recursos de áudio e fecha o dispositivo.
 */
void unload_audio(void);

#endif // AUDIO_H
