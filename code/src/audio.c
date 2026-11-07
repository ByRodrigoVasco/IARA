#include <stdbool.h>
#include <stddef.h> // Define a constante NULL

#include "raylib.h"
#include "audio.h"

// ============================================================================
// CAMINHOS DOS ARQUIVOS DE ÁUDIO
// ATENÇÃO: os nomes precisam ser IDÊNTICOS aos arquivos da pasta (inclusive os espaços!).
// Ex.: "DEMO -  MENU.mp3" tem DOIS espaços antes de MENU.
// ============================================================================
#define SOUNDS_PATH "code/assets/sounds/"

#define MUSIC_MENU_PATH   SOUNDS_PATH "DEMO -  MENU.mp3"
#define MUSIC_GAME_PATH   SOUNDS_PATH "DEMO - GAME.mp3"

#define CLICK_PATH        SOUNDS_PATH "CLICK MOUSE.mp3"
#define VOICE_BLIP_PATH   SOUNDS_PATH "ARI VOICE BLIP.mp3"
#define DOOR_PATH         SOUNDS_PATH "PORTA ABRINDO.mp3"
#define GLITCH_1_PATH     SOUNDS_PATH "GLITCH 1.mp3"
#define GLITCH_2_PATH     SOUNDS_PATH "GLITCH 2.mp3"
#define SCREAM_PATH       SOUNDS_PATH "GRITO.mp3"

// ============================================================================
// VOLUMES (de 0.0 = mudo até 1.0 = máximo). Mexa aqui se algo estiver alto/baixo demais.
// ============================================================================
#define MUSIC_VOLUME    0.5f
#define CLICK_VOLUME    0.8f
#define VOICE_VOLUME    0.7f
#define DOOR_VOLUME     1.0f
#define GLITCH_VOLUME   1.0f
#define SCREAM_VOLUME   1.0f

static bool audio_ready = false;

// Efeitos curtos (carregados inteiros na memória)
static Sound click_sound;
static Sound voice_sound;
static Sound door_sound;
static Sound glitch_1_sound;
static Sound glitch_2_sound;
static Sound scream_sound;

// Músicas longas (tocadas aos poucos a partir do disco - "streaming")
static Music menu_music;
static Music game_music;
static Music *current_music = NULL; // Qual música está tocando agora (NULL = nenhuma)

// ----------------------------------------------------------------------------
// FUNÇÕES AUXILIARES (privadas deste arquivo)
// ----------------------------------------------------------------------------

static Sound load_effect(const char *path, float volume)
{
    Sound sound = LoadSound(path);

    // frameCount == 0 significa que o arquivo não foi encontrado/carregado
    if (sound.frameCount > 0)
    {
        SetSoundVolume(sound, volume);
    }

    return sound;
}

static void play_effect(Sound sound)
{
    if (audio_ready && sound.frameCount > 0)
    {
        PlaySound(sound);
    }
}

static void unload_effect(Sound sound)
{
    if (sound.frameCount > 0)
    {
        UnloadSound(sound);
    }
}

static void play_music(Music *music, const char *path)
{
    if (!audio_ready)
    {
        return;
    }

    // Já é essa música e ela está tocando: não reinicia.
    if (current_music == music && IsMusicStreamPlaying(*music))
    {
        return;
    }

    audio_stop_music();

    // A música só é carregada na primeira vez que for pedida (deixa o jogo abrir mais rápido).
    if (music->frameCount == 0)
    {
        *music = LoadMusicStream(path);

        if (music->frameCount == 0)
        {
            return; // Arquivo não encontrado
        }

        music->looping = true;
        SetMusicVolume(*music, MUSIC_VOLUME);
    }

    PlayMusicStream(*music);
    current_music = music;
}

// ----------------------------------------------------------------------------
// FUNÇÕES PÚBLICAS (declaradas em audio.h)
// ----------------------------------------------------------------------------

void audio_init(void)
{
    InitAudioDevice();
    audio_ready = IsAudioDeviceReady();

    if (!audio_ready)
    {
        return; // Sem placa de som: o jogo segue sem áudio.
    }

    click_sound    = load_effect(CLICK_PATH, CLICK_VOLUME);
    voice_sound    = load_effect(VOICE_BLIP_PATH, VOICE_VOLUME);
    door_sound     = load_effect(DOOR_PATH, DOOR_VOLUME);
    glitch_1_sound = load_effect(GLITCH_1_PATH, GLITCH_VOLUME);
    glitch_2_sound = load_effect(GLITCH_2_PATH, GLITCH_VOLUME);
    scream_sound   = load_effect(SCREAM_PATH, SCREAM_VOLUME);
}

void audio_update(void)
{
    if (!audio_ready)
    {
        return;
    }

    // A raylib exige que a música seja "alimentada" a cada quadro, senão ela trava.
    if (current_music != NULL)
    {
        UpdateMusicStream(*current_music);
    }

    // Som de clique em qualquer clique esquerdo do mouse (menu, botões, quiz...).
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        audio_play_click();
    }
}

void audio_close(void)
{
    if (!audio_ready)
    {
        return;
    }

    audio_stop_music();

    if (menu_music.frameCount > 0) UnloadMusicStream(menu_music);
    if (game_music.frameCount > 0) UnloadMusicStream(game_music);

    unload_effect(click_sound);
    unload_effect(voice_sound);
    unload_effect(door_sound);
    unload_effect(glitch_1_sound);
    unload_effect(glitch_2_sound);
    unload_effect(scream_sound);

    CloseAudioDevice();
    audio_ready = false;
}

void audio_play_music_menu(void)
{
    play_music(&menu_music, MUSIC_MENU_PATH);
}

void audio_play_music_game(void)
{
    play_music(&game_music, MUSIC_GAME_PATH);
}

void audio_stop_music(void)
{
    if (current_music != NULL)
    {
        StopMusicStream(*current_music);
        current_music = NULL;
    }
}

void audio_play_click(void)
{
    play_effect(click_sound);
}

void audio_voice_start(void)
{
    if (!audio_ready || voice_sound.frameCount == 0)
    {
        return;
    }

    // Variação pequena no tom para a voz não soar sempre idêntica a cada fala.
    SetSoundPitch(voice_sound, 0.95f + (GetRandomValue(0, 10) / 100.0f));

    StopSound(voice_sound);
    PlaySound(voice_sound);
}

void audio_voice_stop(void)
{
    if (audio_ready && voice_sound.frameCount > 0)
    {
        StopSound(voice_sound);
    }
}

void audio_play_door(void)
{
    play_effect(door_sound);
}

void audio_play_glitch(void)
{
    // Sorteia entre GLITCH 1 e GLITCH 2 para não ficar repetitivo.
    if (GetRandomValue(0, 1) == 0)
    {
        play_effect(glitch_1_sound);
    }
    else
    {
        play_effect(glitch_2_sound);
    }
}

void audio_play_scream(void)
{
    play_effect(scream_sound);
}
