#include <stdio.h>

#include "raylib.h"
#include "preferences.h"
#include "accessibility.h"
#include "helper.h"
#include "audio.h"

#define PREFERENCES_LINE_SIZE 64

bool load_preferences(const char *path)
{
    FILE *file = fopen(path, "r");

    if (file == NULL) return false;

    char line[PREFERENCES_LINE_SIZE];

    while (fgets(line, sizeof(line), file) != NULL)
    {
        float brightness;
        float volume;
        int enabled;

        if (sscanf(line, "brilho=%f", &brightness) == 1)
        {
            set_game_brightness(brightness);
        }
        else if (sscanf(line, "volume_musica=%f", &volume) == 1)
        {
            if (volume >= 0.0f && volume <= 1.0f) audio_set_music_volume(volume);
        }
        else if (sscanf(line, "volume_efeitos=%f", &volume) == 1)
        {
            if (volume >= 0.0f && volume <= 1.0f) audio_set_effects_volume(volume);
        }
        else if (sscanf(line, "volume=%f", &volume) == 1)
        {
            // Arquivo antigo, de quando havia um volume só: vale para a trilha e para os efeitos
            if (volume >= 0.0f && volume <= 1.0f)
            {
                audio_set_music_volume(volume);
                audio_set_effects_volume(volume);
            }
        }
        else if (sscanf(line, "alto_contraste=%d", &enabled) == 1)
        {
            if ((enabled != 0) != is_high_contrast_enabled()) toggle_high_contrast();
        }
        else if (sscanf(line, "tela_cheia=%d", &enabled) == 1)
        {
            if ((enabled != 0) != is_fullscreen_enabled()) toggle_fullscreen();
        }
    }

    fclose(file);

    return true;
}

bool save_preferences(const char *path)
{
    FILE *file = fopen(path, "w");

    if (file == NULL) return false;

    fprintf(file, "brilho=%.1f\n", get_game_brightness());
    fprintf(file, "volume_musica=%.2f\n", audio_get_music_volume());
    fprintf(file, "volume_efeitos=%.2f\n", audio_get_effects_volume());
    fprintf(file, "alto_contraste=%d\n", is_high_contrast_enabled() ? 1 : 0);
    fprintf(file, "tela_cheia=%d\n", is_fullscreen_enabled() ? 1 : 0);

    return fclose(file) == 0;
}
