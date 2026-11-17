#include <stddef.h>
#include <math.h>

#include "raylib.h"

#include "audio.h"
#include "macros.h"

static Music menu_music = {0};
static Music game_music = {0};
static Music calm_music = {0};            // Ambiente calmo dos primeiros dias; cede lugar à trilha de tensão aos poucos
static float game_tension_target = 0.0f; // 0 = só o ambiente calmo, 1 = só a trilha de tensão
static float game_tension_current = 0.0f;
static Music quiz_music = {0};            // Faixa da tela do computador (quiz)
static Sound flash_stinger = {0};         // Som curto na entrada de um Flash do dia
static Sound jump_stinger = {0};          // Som curto na entrada de um salto puro
static AudioContext audio_context = AUDIO_CONTEXT_OFFICE;
static float layer_weight[AUDIO_LAYER_COUNT] = {0}; // Peso atual de cada camada; anda até o alvo a cada quadro
static Sound mouse_click_sound = {0};
static Sound iara_voice_sound = {0};
static Sound player_footsteps_sound = {0};
static int active_music_screen = -1;

// Volumes escolhidos em CONFIGURAÇÕES, de 0 a 1. Ficam guardados mesmo sem dispositivo de áudio,
// para valerem assim que ele estiver pronto e para o arquivo de preferências gravar o valor certo.
static float music_volume = 1.0f;   // Trilha: menu, ambiente calmo e trilha de tensão
static float effects_volume = 1.0f; // Efeitos: clique, passos e voz da IARA

static float clamp_volume(float volume)
{
    if (volume < 0.0f) return 0.0f;
    if (volume > 1.0f) return 1.0f;
    return volume;
}

// Os efeitos não têm mistura própria: o volume deles é o escolhido pelo jogador.
static void apply_effects_volume(void)
{
    if (mouse_click_sound.stream.buffer != NULL) SetSoundVolume(mouse_click_sound, effects_volume);
    if (iara_voice_sound.stream.buffer != NULL) SetSoundVolume(iara_voice_sound, effects_volume);
    if (player_footsteps_sound.stream.buffer != NULL) SetSoundVolume(player_footsteps_sound, effects_volume);
}

bool initialize_audio(void)
{
    InitAudioDevice();
    if (!IsAudioDeviceReady())
    {
        TraceLog(LOG_ERROR, "AUDIO: nao foi possivel inicializar o dispositivo de audio");
        return false;
    }

    menu_music = LoadMusicStream(MENU_MUSIC_PATH);
    game_music = LoadMusicStream(GAME_MUSIC_PATH);
    calm_music = LoadMusicStream(CALM_MUSIC_PATH);
    quiz_music = LoadMusicStream(QUIZ_MUSIC_PATH);
    flash_stinger = LoadSound(FLASH_STINGER_PATH);
    jump_stinger = LoadSound(JUMP_STINGER_PATH);
    mouse_click_sound = LoadSound(MOUSE_CLICK_SOUND_PATH);
    iara_voice_sound = LoadSound(IARA_VOICE_SOUND_PATH);
    player_footsteps_sound = LoadSound(PLAYER_FOOTSTEPS_SOUND_PATH);

    if (menu_music.ctxData == NULL)
    {
        TraceLog(LOG_ERROR, "AUDIO: falha ao carregar %s", MENU_MUSIC_PATH);
        unload_audio();
        return false;
    }

    if (game_music.ctxData == NULL)
    {
        TraceLog(LOG_ERROR, "AUDIO: falha ao carregar %s", GAME_MUSIC_PATH);
        unload_audio();
        return false;
    }

    if (calm_music.ctxData == NULL)
    {
        TraceLog(LOG_ERROR, "AUDIO: falha ao carregar %s", CALM_MUSIC_PATH);
        unload_audio();
        return false;
    }

    if (mouse_click_sound.stream.buffer == NULL)
    {
        TraceLog(LOG_ERROR, "AUDIO: falha ao carregar %s", MOUSE_CLICK_SOUND_PATH);
        unload_audio();
        return false;
    }

    if (iara_voice_sound.stream.buffer == NULL)
    {
        TraceLog(LOG_ERROR, "AUDIO: falha ao carregar %s", IARA_VOICE_SOUND_PATH);
        unload_audio();
        return false;
    }

    if (player_footsteps_sound.stream.buffer == NULL)
    {
        TraceLog(LOG_ERROR, "AUDIO: falha ao carregar %s", PLAYER_FOOTSTEPS_SOUND_PATH);
        unload_audio();
        return false;
    }

    menu_music.looping = true;
    game_music.looping = true;
    calm_music.looping = true;

    // A faixa do quiz e os sons de passagem de dia são extras: se faltarem, a campanha segue só com o resto
    if (quiz_music.ctxData == NULL) TraceLog(LOG_WARNING, "AUDIO: faixa do quiz ausente (%s); o quiz usa a trilha do escritorio", QUIZ_MUSIC_PATH);
    else quiz_music.looping = true;
    if (flash_stinger.stream.buffer == NULL) TraceLog(LOG_WARNING, "AUDIO: som do flash ausente (%s)", FLASH_STINGER_PATH);
    if (jump_stinger.stream.buffer == NULL) TraceLog(LOG_WARNING, "AUDIO: som do salto ausente (%s)", JUMP_STINGER_PATH);

    // As preferências podem ter sido lidas antes: aplica o que já estiver escolhido
    SetMusicVolume(menu_music, music_volume);
    apply_effects_volume();
    return true;
}

// Duração do fade de cada camada, de ponta a ponta: é ele que suaviza a troca de faixa entre telas
#define AUDIO_LAYER_FADE_SECONDS 0.8f
// Quanto da trilha do escritório continua por baixo em cada contexto de passagem
#define AUDIO_QUIZ_BED_RATIO 0.2f
#define AUDIO_FLASH_BED_RATIO 0.5f
#define AUDIO_JUMP_BED_RATIO 0.3f

float audio_step_toward(float current, float target, float delta_seconds, float seconds_for_full_fade)
{
    if (seconds_for_full_fade <= 0.0f || delta_seconds < 0.0f) return target;

    float step = delta_seconds / seconds_for_full_fade;

    if (current < target) return fminf(target, current + step);
    if (current > target) return fmaxf(target, current - step);
    return current;
}

void audio_compute_layer_targets(bool in_menu_group, bool in_game_group, AudioContext context,
                                 float tension, float out_targets[AUDIO_LAYER_COUNT])
{
    for (int layer = 0; layer < AUDIO_LAYER_COUNT; layer++) out_targets[layer] = 0.0f;

    if (in_menu_group)
    {
        out_targets[AUDIO_LAYER_MENU] = 1.0f;
        return;
    }

    if (!in_game_group) return;

    float tense = clamp_volume(tension);
    float calm = 1.0f - tense;
    float bed = 1.0f;

    switch (context)
    {
    case AUDIO_CONTEXT_QUIZ:
        out_targets[AUDIO_LAYER_QUIZ] = 1.0f;
        bed = AUDIO_QUIZ_BED_RATIO;
        break;
    case AUDIO_CONTEXT_FLASH:
        bed = AUDIO_FLASH_BED_RATIO;
        break;
    case AUDIO_CONTEXT_JUMP:
        bed = AUDIO_JUMP_BED_RATIO;
        break;
    default:
        break;
    }

    out_targets[AUDIO_LAYER_CALM] = calm * bed;
    out_targets[AUDIO_LAYER_TENSION] = tense * bed;
}

// Liga a camada quando o peso sobe, desliga quando chega a zero e mantém o volume na mistura
static void update_music_layer(Music music, int layer)
{
    if (music.ctxData == NULL) return;

    float weight = layer_weight[layer];

    if (weight > 0.001f)
    {
        if (!IsMusicStreamPlaying(music)) PlayMusicStream(music);
        SetMusicVolume(music, weight * music_volume);
        UpdateMusicStream(music);
    }
    else if (IsMusicStreamPlaying(music))
    {
        StopMusicStream(music);
    }
}

void update_audio_for_screen(int screen)
{
    bool in_menu_group = screen == PRE_MENU_SCREEN || screen == MENU_SCREEN ||
                         screen == CREDITS_SCREEN || screen == SETTINGS_SCREEN ||
                         screen == MODE_SELECT_SCREEN || screen == SAVE_SELECT_SCREEN;
    bool in_game_group = screen == PLAYING_SCREEN;
    int next_music_screen = in_menu_group ? MENU_SCREEN : (in_game_group ? PLAYING_SCREEN : -1);

    if (next_music_screen != active_music_screen)
    {
        if (next_music_screen == PLAYING_SCREEN)
        {
            // Começa no ponto de tensão do dia: nada de trilha de ação logo no início da história
            game_tension_current = game_tension_target;
            audio_context = AUDIO_CONTEXT_OFFICE;
        }

        active_music_screen = next_music_screen;
    }

    // A tensão sobe e desce devagar (cerca de 3 s de ponta a ponta), sem corte entre as faixas
    if (in_game_group) game_tension_current = audio_step_toward(game_tension_current, game_tension_target, GetFrameTime(), 1.0f / 0.35f);

    // Sem a faixa do quiz, o computador fica com a trilha do escritório
    AudioContext effective_context = audio_context;
    if (effective_context == AUDIO_CONTEXT_QUIZ && quiz_music.ctxData == NULL) effective_context = AUDIO_CONTEXT_OFFICE;

    float targets[AUDIO_LAYER_COUNT];
    audio_compute_layer_targets(in_menu_group, in_game_group, effective_context, game_tension_current, targets);

    // Cada camada anda até o alvo: a faixa que sai some enquanto a que entra sobe (crossfade entre telas e contextos).
    // O volume da trilha só escala o conjunto, então a soma continua igual ao volume escolhido.
    for (int layer = 0; layer < AUDIO_LAYER_COUNT; layer++)
    {
        layer_weight[layer] = audio_step_toward(layer_weight[layer], targets[layer], GetFrameTime(), AUDIO_LAYER_FADE_SECONDS);
    }

    update_music_layer(menu_music, AUDIO_LAYER_MENU);
    update_music_layer(calm_music, AUDIO_LAYER_CALM);
    update_music_layer(game_music, AUDIO_LAYER_TENSION);
    update_music_layer(quiz_music, AUDIO_LAYER_QUIZ);
}

void audio_set_context(AudioContext context)
{
    if (context == audio_context) return;

    audio_context = context;

    if (!IsAudioDeviceReady()) return;

    Sound *stinger = NULL;
    if (context == AUDIO_CONTEXT_FLASH) stinger = &flash_stinger;
    else if (context == AUDIO_CONTEXT_JUMP) stinger = &jump_stinger;

    if (stinger != NULL && stinger->stream.buffer != NULL)
    {
        SetSoundVolume(*stinger, music_volume);
        PlaySound(*stinger);
    }
}

AudioContext audio_get_context(void)
{
    return audio_context;
}

void audio_set_game_tension(float tension)
{
    if (tension < 0.0f) tension = 0.0f;
    if (tension > 1.0f) tension = 1.0f;
    game_tension_target = tension;
}

void audio_set_music_volume(float volume)
{
    music_volume = clamp_volume(volume);
}

float audio_get_music_volume(void)
{
    return music_volume;
}

void audio_set_effects_volume(float volume)
{
    effects_volume = clamp_volume(volume);
    apply_effects_volume();
}

float audio_get_effects_volume(void)
{
    return effects_volume;
}

void play_click_sound(void)
{
    PlaySound(mouse_click_sound);
}

void update_player_footstep_audio(bool is_walking)
{
    if (!IsAudioDeviceReady() || player_footsteps_sound.stream.buffer == NULL) return;

    if (is_walking)
    {
        if (!IsSoundPlaying(player_footsteps_sound)) PlaySound(player_footsteps_sound);
    }
    else if (IsSoundPlaying(player_footsteps_sound))
    {
        StopSound(player_footsteps_sound);
    }
}

void update_iara_voice_typing(bool is_typing)
{
    if (!IsAudioDeviceReady() || iara_voice_sound.stream.buffer == NULL) return;

    if (is_typing)
    {
        if (!IsSoundPlaying(iara_voice_sound)) PlaySound(iara_voice_sound);
    }
    else if (IsSoundPlaying(iara_voice_sound))
    {
        StopSound(iara_voice_sound);
    }
}

void unload_audio(void)
{
    if (IsAudioDeviceReady())
    {
        if (menu_music.ctxData != NULL) StopMusicStream(menu_music);
        if (game_music.ctxData != NULL) StopMusicStream(game_music);
        if (calm_music.ctxData != NULL) StopMusicStream(calm_music);
        if (quiz_music.ctxData != NULL) StopMusicStream(quiz_music);
        if (player_footsteps_sound.stream.buffer != NULL) StopSound(player_footsteps_sound);
    }

    active_music_screen = -1;
    audio_context = AUDIO_CONTEXT_OFFICE;
    for (int layer = 0; layer < AUDIO_LAYER_COUNT; layer++) layer_weight[layer] = 0.0f;

    if (menu_music.ctxData != NULL) UnloadMusicStream(menu_music);
    if (game_music.ctxData != NULL) UnloadMusicStream(game_music);
    if (calm_music.ctxData != NULL) UnloadMusicStream(calm_music);
    if (quiz_music.ctxData != NULL) UnloadMusicStream(quiz_music);
    if (flash_stinger.stream.buffer != NULL) UnloadSound(flash_stinger);
    if (jump_stinger.stream.buffer != NULL) UnloadSound(jump_stinger);
    if (mouse_click_sound.stream.buffer != NULL) UnloadSound(mouse_click_sound);
    if (iara_voice_sound.stream.buffer != NULL) UnloadSound(iara_voice_sound);
    if (player_footsteps_sound.stream.buffer != NULL) UnloadSound(player_footsteps_sound);

    menu_music = (Music){0};
    game_music = (Music){0};
    calm_music = (Music){0};
    quiz_music = (Music){0};
    flash_stinger = (Sound){0};
    jump_stinger = (Sound){0};
    mouse_click_sound = (Sound){0};
    iara_voice_sound = (Sound){0};
    player_footsteps_sound = (Sound){0};

    if (IsAudioDeviceReady()) CloseAudioDevice();
}
