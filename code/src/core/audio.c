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
static Sound mouse_click_sound = {0};
static Sound iara_voice_sound = {0};
static Sound player_footsteps_sound = {0};
static int active_music_screen = -1;

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
    return true;
}

void update_audio_for_screen(int screen)
{
    int next_music_screen = -1;
    if (screen == PRE_MENU_SCREEN || screen == MENU_SCREEN ||
        screen == CREDITS_SCREEN || screen == SETTINGS_SCREEN ||
        screen == MODE_SELECT_SCREEN || screen == SAVE_SELECT_SCREEN)
    {
        next_music_screen = MENU_SCREEN;
    }
    else if (screen == PLAYING_SCREEN) next_music_screen = PLAYING_SCREEN;

    if (next_music_screen != active_music_screen)
    {
        if (active_music_screen == MENU_SCREEN) StopMusicStream(menu_music);
        else if (active_music_screen == PLAYING_SCREEN)
        {
            StopMusicStream(game_music);
            StopMusicStream(calm_music);
        }

        active_music_screen = next_music_screen;

        if (active_music_screen == MENU_SCREEN) PlayMusicStream(menu_music);
        else if (active_music_screen == PLAYING_SCREEN)
        {
            // Começa no ponto de tensão do dia: nada de trilha de ação logo no início da história
            game_tension_current = game_tension_target;
            PlayMusicStream(game_music);
            PlayMusicStream(calm_music);
        }
    }

    if (active_music_screen == MENU_SCREEN) UpdateMusicStream(menu_music);
    else if (active_music_screen == PLAYING_SCREEN)
    {
        // A tensão sobe e desce devagar (cerca de 3 s de ponta a ponta), sem corte entre as faixas
        float step = GetFrameTime() * 0.35f;

        if (game_tension_current < game_tension_target) game_tension_current = fminf(game_tension_target, game_tension_current + step);
        else if (game_tension_current > game_tension_target) game_tension_current = fmaxf(game_tension_target, game_tension_current - step);

        SetMusicVolume(calm_music, 1.0f - game_tension_current);
        SetMusicVolume(game_music, game_tension_current);
        UpdateMusicStream(calm_music);
        UpdateMusicStream(game_music);
    }
}

void audio_set_game_tension(float tension)
{
    if (tension < 0.0f) tension = 0.0f;
    if (tension > 1.0f) tension = 1.0f;
    game_tension_target = tension;
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
        if (player_footsteps_sound.stream.buffer != NULL) StopSound(player_footsteps_sound);
    }

    active_music_screen = -1;

    if (menu_music.ctxData != NULL) UnloadMusicStream(menu_music);
    if (game_music.ctxData != NULL) UnloadMusicStream(game_music);
    if (calm_music.ctxData != NULL) UnloadMusicStream(calm_music);
    if (mouse_click_sound.stream.buffer != NULL) UnloadSound(mouse_click_sound);
    if (iara_voice_sound.stream.buffer != NULL) UnloadSound(iara_voice_sound);
    if (player_footsteps_sound.stream.buffer != NULL) UnloadSound(player_footsteps_sound);

    menu_music = (Music){0};
    game_music = (Music){0};
    calm_music = (Music){0};
    mouse_click_sound = (Sound){0};
    iara_voice_sound = (Sound){0};
    player_footsteps_sound = (Sound){0};

    if (IsAudioDeviceReady()) CloseAudioDevice();
}
