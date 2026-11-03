#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "game_state.h"
#include "animations.h"
#include "macros.h"
#include "helper.h"
#include "accessibility.h"

// Largura do nome IARA em 1080p, como fração da imagem original.
#define IARA_NAME_SCALE_AT_REFERENCE 0.5f

static bool fade_in_animation_finished = false;

bool can_close_pre_menu_safely = false;

static Color bg_color;
/*
ESTAMOS USANDO APENAS 1 COR DE TEXTURA PARA TODAS AS TEXTURAS DO PRE MENU.
ISSO ESTÁ SENDO FEITO POR QUE O "fade_in_texture" MODIFICARÁ A MESMA COR PARA AS DUAS TEXTURAS, E ASSIM, A ANIMAÇÃO SERÁ FEITA PARA AMBAS TEXTURAS AO MESMO TEMPO.
*/
static Color textures_color = {255, 255, 255, 0};

static Texture2D bg_office_texture;
static Texture2D iara_name_texture;

void initialize_pre_menu_textures(void)
{
    bg_color = GetColor(BG_COLOR_HEX);

    bg_office_texture = load_smooth_texture("code/assets/sprites/tela-inicial/escritorio-tela-inicial.png");
    iara_name_texture = load_smooth_texture("code/assets/sprites/tela-inicial/iara-nome.png");
}

// Desenha o fundo cobrindo a janela e o nome IARA centralizado, conforme o tamanho atual da janela.
static void draw_pre_menu_textures(void)
{
    // A ordem IMPORTA!! Quem ficará em cima de quem depende disso. Ou seja, Background tem que ir primeiro, depois os textos.
    draw_texture_cover(bg_office_texture, (Rectangle){0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()}, textures_color);

    float name_scale = IARA_NAME_SCALE_AT_REFERENCE * get_ui_scale();
    float name_width = iara_name_texture.width * name_scale;
    float name_height = iara_name_texture.height * name_scale;
    Rectangle name_source = {0, 0, (float)iara_name_texture.width, (float)iara_name_texture.height};
    Rectangle name_dest = {(GetScreenWidth() - name_width) / 2.0f, (GetScreenHeight() - name_height) / 2.0f, name_width, name_height};

    DrawTexturePro(iara_name_texture, name_source, name_dest, (Vector2){0, 0}, 0.0f, textures_color);
}

void close_pre_menu(void)
{
    // PODE FECHAR APENAS DEPOIS DA ANIMAÇÃO TERMINAR.
    fade_out_texture(10, &textures_color, &can_close_pre_menu_safely);
}

void render_pre_menu(void)
{
    if (fade_in_animation_finished)
    {
        close_pre_menu();
    }
    else
    {
        fade_in_texture(DEFAULT_FADE_SPEED, &textures_color, &fade_in_animation_finished);
    }

    begin_game_drawing();
    ClearBackground(bg_color);

    draw_pre_menu_textures();

    end_game_drawing();
}

void unload_pre_menu_textures(void)
{
    UnloadTexture(bg_office_texture);
    UnloadTexture(iara_name_texture);
}
