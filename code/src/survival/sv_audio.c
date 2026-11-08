#include <math.h>
#include "raylib.h"
#include "sv_audio.h"

#define MAX_VOICES 3
#define SMALL_SOUND_GAP 0.1       // No máximo 10 sons pequenos por segundo
#define DUCK_ATTACK_SECONDS 0.015f
#define DUCK_RELEASE_SECONDS 0.4f
#define MUSIC_FADE_SECONDS 1.2f

typedef struct
{
    Sound base;
    Sound voices[MAX_VOICES];
    int voice_count;
    int next_voice;
    double last_played;
    SvTier tier;
    float gap;
    bool loaded;
} SfxEntry;

static SfxEntry sfx[SND_COUNT];
static const char *sfx_names[SND_COUNT] = {
#define X(name, tier, gap, voices) #name,
    SV_SFX_LIST(X)
#undef X
};
static const SvTier sfx_tiers[SND_COUNT] = {
#define X(name, tier, gap, voices) tier,
    SV_SFX_LIST(X)
#undef X
};
static const float sfx_gaps[SND_COUNT] = {
#define X(name, tier, gap, voices) gap,
    SV_SFX_LIST(X)
#undef X
};
static const int sfx_voice_counts[SND_COUNT] = {
#define X(name, tier, gap, voices) voices,
    SV_SFX_LIST(X)
#undef X
};

static const char *music_names[MUS_COUNT] = {
    "musMenu", "musLoja", "musGameOver", "musVitoria",
    "musAto1", "musAto2", "musAto3", "musAto4", "musAto5",
    "musImpressora", "musModelo", "musDiego", "musRene", "musIara", "musHoraExtra"};

static const char *ambient_names[SV_AMBIENT_COUNT] = {
    "ambEscritorio", "ambCopa", "ambReuniao", "ambDiretoria", "ambDataCenter", "ambTreino", "ambArquivo",
    "ambSuporte", "ambRH", "ambCatracas", "ambRecepcao", "ambAlagada", "ambFundo"};

static bool audio_ready = false;
static bool audio_disabled = false;
static Music music_stream;
static bool music_loaded = false;
static SvMusic music_id = MUS_NONE;
static float music_fade = 1.0f;

static Music ambient_stream;
static bool ambient_loaded = false;
static int ambient_id = -1;
static float ambient_fade = 1.0f;

// Barramentos: ganho atual (linear) de cada um.
static float bus_music = 1.0f, bus_ambient = 1.0f, bus_small = 1.0f;
static float duck_hold = 0.0f;
static float duck_music_db = 0.0f, duck_ambient_db = 0.0f, duck_small_db = 0.0f;

static double last_small_sound = -1.0;
static double small_times[16];
static int small_count = 0;

static float db_to_gain(float db)
{
    return powf(10.0f, db / 20.0f);
}

static bool is_small_tier(SvTier tier)
{
    return tier == TIER_INIMIGO || tier == TIER_FREQUENTE || tier == TIER_MICRO || tier == TIER_PASSO;
}

bool sv_audio_ready(void)
{
    return audio_ready;
}

void sv_audio_disable(bool disabled)
{
    audio_disabled = disabled;
}

static bool audio_started = false;   // O dispositivo de áudio já foi aberto
static int audio_next = 0;           // Quantos sons já foram carregados

static void load_one_sound(int i)
{
    const char *path = TextFormat("%s%s.wav", SV_AUDIO_PATH, sfx_names[i]);

    sfx[i].tier = sfx_tiers[i];
    sfx[i].gap = sfx_gaps[i];
    sfx[i].last_played = -10.0;
    sfx[i].base = LoadSound(path);
    sfx[i].loaded = sfx[i].base.frameCount > 0;
    sfx[i].voice_count = sfx_voice_counts[i] > MAX_VOICES ? MAX_VOICES : sfx_voice_counts[i];
    sfx[i].next_voice = 0;

    if (!sfx[i].loaded) return;

    // A primeira voz é o próprio som; as outras dividem os mesmos dados
    sfx[i].voices[0] = sfx[i].base;
    for (int v = 1; v < sfx[i].voice_count; v++) sfx[i].voices[v] = LoadSoundAlias(sfx[i].base);
}

float sv_audio_init_step(void)
{
    if (audio_ready || audio_disabled) return 1.0f;

    if (!audio_started)
    {
        InitAudioDevice();
        if (!IsAudioDeviceReady()) return 1.0f;   // Sem placa de som: o jogo segue sem áudio
        audio_started = true;
        audio_next = 0;
        return 0.03f;
    }

    if (audio_next < SND_COUNT)
    {
        load_one_sound(audio_next);
        audio_next++;
    }

    if (audio_next >= SND_COUNT)
    {
        audio_ready = true;
        return 1.0f;
    }
    return 0.03f + 0.97f * (float)audio_next / (float)SND_COUNT;
}

void sv_audio_init(void)
{
    while (sv_audio_init_step() < 1.0f) { }
}

static void unload_streams(void)
{
    if (music_loaded) { StopMusicStream(music_stream); UnloadMusicStream(music_stream); music_loaded = false; }
    if (ambient_loaded) { StopMusicStream(ambient_stream); UnloadMusicStream(ambient_stream); ambient_loaded = false; }
    music_id = MUS_NONE;
    ambient_id = -1;
}

void sv_audio_close(void)
{
    if (!audio_ready && !audio_started) return;

    unload_streams();
    for (int i = 0; i < SND_COUNT; i++)
    {
        if (!sfx[i].loaded) continue;
        for (int v = 1; v < sfx[i].voice_count; v++) UnloadSoundAlias(sfx[i].voices[v]);
        UnloadSound(sfx[i].base);
        sfx[i].loaded = false;
    }
    CloseAudioDevice();
    audio_ready = false;
    audio_started = false;
    audio_next = 0;
}

// Quanto cada faixa abaixa os outros canais, em dB: trilha, ambiente e sons pequenos.
static void get_duck(SvTier tier, float *music_db, float *ambient_db, float *small_db, float *hold)
{
    switch (tier)
    {
    case TIER_ALERTA: *music_db = -6.0f; *ambient_db = -8.0f; *small_db = -7.0f; *hold = 0.9f; break;
    case TIER_ATAQUE: *music_db = -5.0f; *ambient_db = -7.0f; *small_db = -5.0f; *hold = 0.7f; break;
    case TIER_EVENTO: *music_db = -3.0f; *ambient_db = -4.0f; *small_db = -3.0f; *hold = 0.5f; break;
    default: *music_db = *ambient_db = *small_db = 0.0f; *hold = 0.0f; break;
    }
}

int sv_audio_small_per_second(void)
{
    double now = GetTime();
    int count = 0;

    for (int i = 0; i < small_count; i++)
    {
        if (now - small_times[i] <= 1.0) count++;
    }
    return count;
}

void sv_sfx(SvSound sound)
{
    SfxEntry *entry;
    double now;

    if (!audio_ready || sound < 0 || sound >= SND_COUNT) return;
    entry = &sfx[sound];
    if (!entry->loaded) return;

    now = GetTime();

    // Intervalo mínimo do próprio som
    if (now - entry->last_played < entry->gap) return;

    // Sons pequenos dividem um limite de 10 por segundo
    if (is_small_tier(entry->tier))
    {
        if (now - last_small_sound < SMALL_SOUND_GAP) return;
        last_small_sound = now;
        if (small_count < 16) small_times[small_count++] = now;
        else
        {
            for (int i = 1; i < 16; i++) small_times[i - 1] = small_times[i];
            small_times[15] = now;
        }
    }

    entry->last_played = now;

    // Alerta, ataque de chefe e evento abaixam o resto por um instante
    {
        float music_db, ambient_db, small_db, hold;

        get_duck(entry->tier, &music_db, &ambient_db, &small_db, &hold);
        if (hold > 0.0f)
        {
            if (music_db < duck_music_db || duck_hold <= 0.0f) duck_music_db = music_db;
            if (ambient_db < duck_ambient_db || duck_hold <= 0.0f) duck_ambient_db = ambient_db;
            if (small_db < duck_small_db || duck_hold <= 0.0f) duck_small_db = small_db;
            if (hold > duck_hold) duck_hold = hold;
        }
    }

    {
        Sound voice = entry->voices[entry->next_voice];

        entry->next_voice = (entry->next_voice + 1) % entry->voice_count;
        SetSoundVolume(voice, is_small_tier(entry->tier) ? bus_small : 1.0f);
        PlaySound(voice);
    }
}

static void start_stream(Music *stream, bool *loaded, const char *name, float volume)
{
    if (*loaded) { StopMusicStream(*stream); UnloadMusicStream(*stream); *loaded = false; }
    *stream = LoadMusicStream(TextFormat("%s%s.wav", SV_AUDIO_PATH, name));
    if (stream->frameCount == 0) return;

    stream->looping = true;
    SetMusicVolume(*stream, volume);
    PlayMusicStream(*stream);
    *loaded = true;
}

void sv_music_play(SvMusic music)
{
    if (!audio_ready || music == music_id) return;

    music_id = music;
    music_fade = 0.0f;
    if (music == MUS_NONE)
    {
        if (music_loaded) { StopMusicStream(music_stream); UnloadMusicStream(music_stream); music_loaded = false; }
        return;
    }
    start_stream(&music_stream, &music_loaded, music_names[music], 0.0f);
}

SvMusic sv_music_current(void)
{
    return music_id;
}

void sv_ambient_play(int ambient)
{
    if (!audio_ready || ambient == ambient_id) return;

    ambient_id = ambient;
    ambient_fade = 0.0f;
    if (ambient < 0 || ambient >= SV_AMBIENT_COUNT)
    {
        ambient_id = -1;
        if (ambient_loaded) { StopMusicStream(ambient_stream); UnloadMusicStream(ambient_stream); ambient_loaded = false; }
        return;
    }
    start_stream(&ambient_stream, &ambient_loaded, ambient_names[ambient], 0.0f);
}

// Aproxima o ganho do alvo: desce rápido e volta devagar, para o susto passar sem pulsar.
static float approach_gain(float current, float target, float dt)
{
    float time_constant = target < current ? DUCK_ATTACK_SECONDS : DUCK_RELEASE_SECONDS;

    return current + (target - current) * (1.0f - expf(-dt / time_constant));
}

void sv_audio_update(float dt)
{
    float target_music = 1.0f, target_ambient = 1.0f, target_small = 1.0f;

    if (!audio_ready) return;

    if (duck_hold > 0.0f)
    {
        duck_hold -= dt;
        target_music = db_to_gain(duck_music_db);
        target_ambient = db_to_gain(duck_ambient_db);
        target_small = db_to_gain(duck_small_db);
    }

    bus_music = approach_gain(bus_music, target_music, dt);
    bus_ambient = approach_gain(bus_ambient, target_ambient, dt);
    bus_small = approach_gain(bus_small, target_small, dt);

    if (music_fade < 1.0f) music_fade += dt / MUSIC_FADE_SECONDS;
    if (ambient_fade < 1.0f) ambient_fade += dt / MUSIC_FADE_SECONDS;
    if (music_fade > 1.0f) music_fade = 1.0f;
    if (ambient_fade > 1.0f) ambient_fade = 1.0f;

    if (music_loaded)
    {
        SetMusicVolume(music_stream, bus_music * music_fade);
        UpdateMusicStream(music_stream);
    }

    if (ambient_loaded)
    {
        SetMusicVolume(ambient_stream, bus_ambient * ambient_fade);
        UpdateMusicStream(ambient_stream);
    }
}
