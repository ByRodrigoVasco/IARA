#ifndef AUDIO_H
#define AUDIO_H

/**
 * @brief Abre o dispositivo de áudio e carrega os efeitos sonoros.
 *
 * Chamar UMA vez na main.c, depois do InitWindow. Se a máquina não tiver
 * placa de som (ou algum arquivo faltar), o jogo continua funcionando, só sem aquele som.
 */
void audio_init(void);

/**
 * @brief Deve ser chamada UMA vez por quadro (dentro do while da main.c).
 *
 * Mantém a música tocando e toca o som de clique a cada clique do mouse.
 */
void audio_update(void);

/**
 * @brief Descarrega todos os sons e fecha o dispositivo de áudio. Chamar no fim da main.c.
 */
void audio_close(void);

// ------------------------------- MÚSICAS (em loop) -------------------------------

/** Toca a música do menu (DEMO - MENU.mp3). Se já estiver tocando, não faz nada. */
void audio_play_music_menu(void);

/** Toca a música do jogo (DEMO - GAME.mp3). Troca a música do menu automaticamente. */
void audio_play_music_game(void);

/** Para a música que estiver tocando. */
void audio_stop_music(void);

// ------------------------------- EFEITOS -------------------------------

/** Clique do mouse. Já é tocado sozinho por audio_update() em todo clique esquerdo. */
void audio_play_click(void);

/** Começa a "voz" do NPC (ARI VOICE BLIP.mp3). Chamar quando uma fala nova aparece. */
void audio_voice_start(void);

/** Corta a "voz" do NPC (ex.: quando a fala termina ou some da tela). */
void audio_voice_stop(void);

/** Porta abrindo (PORTA ABRINDO.mp3). */
void audio_play_door(void);

/** Glitch. Alterna aleatoriamente entre GLITCH 1.mp3 e GLITCH 2.mp3. */
void audio_play_glitch(void);

/** Grito (GRITO.mp3). */
void audio_play_scream(void);

#endif // AUDIO_H
