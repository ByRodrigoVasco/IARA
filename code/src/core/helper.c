#include <stdio.h>
#include <string.h>

#include "helper.h"
#include "macros.h"

GAME_TEXTURE CreateGameTextureFromTexture2D(Texture2D texture)
{
    GAME_TEXTURE game_txtr;

    game_txtr.texture = texture;

    // Calcula a posição centralizada considerando o tamanho da textura
    game_txtr.middle_height = middle_screen_y - (texture.height / 2);
    game_txtr.middle_width = middle_screen_x - (texture.width / 2);

    return game_txtr;
}

void UnloadGameTexture(GAME_TEXTURE Texture)
{
    UnloadTexture(Texture.texture);
}

void update_screen_metrics(void)
{
    screen_width = GetScreenWidth();
    screen_height = GetScreenHeight();
    middle_screen_x = screen_width / 2;
    middle_screen_y = screen_height / 2;
}

float get_ui_scale(void)
{
    float scale_x = GetScreenWidth() / UI_REFERENCE_WIDTH;
    float scale_y = GetScreenHeight() / UI_REFERENCE_HEIGHT;

    return (scale_x < scale_y) ? scale_x : scale_y;
}

Texture2D load_smooth_texture(const char *path)
{
    Texture2D texture = LoadTexture(path);

    GenTextureMipmaps(&texture);
    SetTextureFilter(texture, TEXTURE_FILTER_TRILINEAR);

    return texture;
}

// Imagens grandes que várias telas usam (o escritório da abertura, do menu e das configurações, o nome IARA, a seta):
// ficam numa tabela com a contagem de quem as usa e só saem da memória quando o último devolve.
#define SHARED_TEXTURE_SLOTS 8
#define SHARED_TEXTURE_PATH_MAX 128

typedef struct
{
    char path[SHARED_TEXTURE_PATH_MAX];
    Texture2D texture;
    int users;
} shared_texture_slot;

static shared_texture_slot shared_textures[SHARED_TEXTURE_SLOTS];

Texture2D acquire_smooth_texture(const char *path)
{
    shared_texture_slot *free_slot = NULL;
    Texture2D texture;

    for (int i = 0; i < SHARED_TEXTURE_SLOTS; i++)
    {
        shared_texture_slot *slot = &shared_textures[i];

        if (slot->users > 0 && strcmp(slot->path, path) == 0)
        {
            slot->users++;
            return slot->texture;
        }
        if (slot->users == 0 && free_slot == NULL) free_slot = slot;
    }

    texture = load_smooth_texture(path);

    // Sem lugar na tabela (ou sem imagem): a textura fica só com quem pediu, e release_smooth_texture a descarrega
    if (texture.id == 0 || free_slot == NULL || strlen(path) >= SHARED_TEXTURE_PATH_MAX) return texture;

    snprintf(free_slot->path, sizeof(free_slot->path), "%s", path);
    free_slot->texture = texture;
    free_slot->users = 1;

    return texture;
}

void release_smooth_texture(Texture2D texture)
{
    if (texture.id == 0) return;

    for (int i = 0; i < SHARED_TEXTURE_SLOTS; i++)
    {
        shared_texture_slot *slot = &shared_textures[i];

        if (slot->users > 0 && slot->texture.id == texture.id)
        {
            if (--slot->users == 0)
            {
                UnloadTexture(slot->texture);
                slot->texture = (Texture2D){0};
                slot->path[0] = '\0';
            }
            return;
        }
    }

    UnloadTexture(texture);
}

int shared_texture_users(const char *path)
{
    for (int i = 0; i < SHARED_TEXTURE_SLOTS; i++)
    {
        if (shared_textures[i].users > 0 && strcmp(shared_textures[i].path, path) == 0) return shared_textures[i].users;
    }
    return 0;
}

void draw_texture_cover(Texture2D texture, Rectangle dest, Color tint)
{
    Rectangle source = {0, 0, (float)texture.width, (float)texture.height};
    float dest_aspect = dest.width / dest.height;
    float texture_aspect = source.width / source.height;

    if (texture_aspect > dest_aspect)
    {
        source.width = source.height * dest_aspect;
        source.x = (texture.width - source.width) / 2.0f;
    }
    else
    {
        source.height = source.width / dest_aspect;
        source.y = (texture.height - source.height) / 2.0f;
    }

    DrawTexturePro(texture, source, dest, (Vector2){0, 0}, 0.0f, tint);
}

static WindowMode current_window_mode = WINDOW_MODE_FIXED;
static WindowMode last_window_mode = WINDOW_MODE_FIXED; // Janela a que a tela cheia volta

// Posição e tamanho da janela fixa: o que ela tinha ao abrir o jogo (tamanho padrão, centralizada no monitor).
static Rectangle fixed_window_rect = {0};
static bool has_fixed_window_rect = false;

// Janela de tamanho fixo: volta ao tamanho e ao lugar de quando o jogo abriu, sem bordas para arrastar.
static void apply_fixed_window(void)
{
    if (IsWindowMaximized()) RestoreWindow();
    ClearWindowState(FLAG_WINDOW_RESIZABLE);

    if (has_fixed_window_rect)
    {
        SetWindowSize((int)fixed_window_rect.width, (int)fixed_window_rect.height);
        SetWindowPosition((int)fixed_window_rect.x, (int)fixed_window_rect.y);
    }
}

void set_window_mode(WindowMode mode)
{
    if (mode == current_window_mode) return;

    if (current_window_mode == WINDOW_MODE_FULLSCREEN)
    {
        if (IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE)) ToggleBorderlessWindowed();
    }
    else
    {
        last_window_mode = current_window_mode;
    }

    switch (mode)
    {
    case WINDOW_MODE_FULLSCREEN:
        if (!IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE)) ToggleBorderlessWindowed();
        break;

    case WINDOW_MODE_MAXIMIZED:
        SetWindowState(FLAG_WINDOW_RESIZABLE); // Sem isso o sistema nao maximiza
        if (!IsWindowMaximized()) MaximizeWindow();
        break;

    case WINDOW_MODE_FIXED:
    default:
        apply_fixed_window();
        break;
    }

    current_window_mode = mode;
}

WindowMode get_window_mode(void)
{
    return current_window_mode;
}

WindowMode get_windowed_mode(void)
{
    return (current_window_mode == WINDOW_MODE_FULLSCREEN) ? last_window_mode : current_window_mode;
}

void toggle_fullscreen(void)
{
    set_window_mode((current_window_mode == WINDOW_MODE_FULLSCREEN) ? last_window_mode : WINDOW_MODE_FULLSCREEN);
}

void toggle_window_maximized(void)
{
    set_window_mode((current_window_mode == WINDOW_MODE_MAXIMIZED) ? WINDOW_MODE_FIXED : WINDOW_MODE_MAXIMIZED);
}

bool is_fullscreen_enabled(void)
{
    return current_window_mode == WINDOW_MODE_FULLSCREEN;
}

bool is_window_maximized_enabled(void)
{
    return current_window_mode == WINDOW_MODE_MAXIMIZED;
}

void mark_window_fixed_at_startup(void)
{
    Vector2 position = GetWindowPosition();

    ClearWindowState(FLAG_WINDOW_RESIZABLE);
    fixed_window_rect = (Rectangle){position.x, position.y, (float)GetScreenWidth(), (float)GetScreenHeight()};
    has_fixed_window_rect = true;
    current_window_mode = WINDOW_MODE_FIXED;
    last_window_mode = WINDOW_MODE_FIXED;
}
