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
 * @brief Momento do jogo que decide a mistura da trilha dentro da campanha.
 * OFFICE: escritório (ambiente calmo e tensão). QUIZ: tela do computador, com a faixa própria do quiz.
 * FLASH e JUMP: cartões de passagem de dia, com a trilha abafada e um som curto de entrada.
 */
typedef enum
{
    AUDIO_CONTEXT_OFFICE = 0,
    AUDIO_CONTEXT_QUIZ,
    AUDIO_CONTEXT_FLASH,
    AUDIO_CONTEXT_JUMP
} AudioContext;

/** Camadas de música que se misturam. */
typedef enum
{
    AUDIO_LAYER_MENU = 0,
    AUDIO_LAYER_CALM,
    AUDIO_LAYER_TENSION,
    AUDIO_LAYER_QUIZ,
    AUDIO_LAYER_COUNT
} AudioLayer;

/**
 * @brief Muda o contexto da trilha da campanha. A troca é suave (as camadas fazem fade) e,
 * ao entrar em FLASH ou JUMP, toca o som curto de entrada. Repetir o contexto atual não faz nada.
 */
void audio_set_context(AudioContext context);
AudioContext audio_get_context(void);

/**
 * @brief Peso (0 a 1) que cada camada deve ter, sem tocar nada. Menu e jogo nunca soam juntos
 * como alvo final; o fade entre eles acontece porque o peso atual de cada camada anda até o alvo.
 *
 * @param in_menu_group true nas telas de menu, configurações, créditos e escolhas de modo e save.
 * @param in_game_group true na campanha (PLAYING_SCREEN).
 * @param context Contexto da campanha.
 * @param tension Tensão de 0 a 1.
 * @param jump_progress Progresso do bloco de salto puro, de 0 a 1 (só vale no contexto JUMP).
 * @param out_targets Recebe AUDIO_LAYER_COUNT valores.
 */
void audio_compute_layer_targets(bool in_menu_group, bool in_game_group, AudioContext context,
                                 float tension, float jump_progress, float out_targets[AUDIO_LAYER_COUNT]);

/**
 * @brief Quanto do bloco de salto puro já passou, de 0 (primeiro dia) a quase 1 (último). A trilha
 * perde uma fatia a cada dia do bloco, reforçando a sensação de tempo acelerando.
 */
void audio_set_jump_progress(float progress);

/**
 * @brief Anda de current até target no máximo seconds_to_full * (de 0 a 1) por segundo.
 * Função pura: usada pelo fade de cada camada.
 */
float audio_step_toward(float current, float target, float delta_seconds, float seconds_for_full_fade);

/**
 * @brief Volume da trilha (menu, ambiente calmo e trilha de tensão), de 0 a 1.
 * Multiplica a mistura da tensão, então o crossfade entre as faixas continua igual.
 *
 * @param volume Valor de 0 a 1; fora disso é limitado.
 */
void audio_set_music_volume(float volume);
float audio_get_music_volume(void);

/**
 * @brief Volume dos efeitos sonoros (clique, passos e voz da IARA), de 0 a 1.
 *
 * @param volume Valor de 0 a 1; fora disso é limitado.
 */
void audio_set_effects_volume(float volume);
float audio_get_effects_volume(void);

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
