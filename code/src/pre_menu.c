#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "game_state.h"
#include "animations.h"
#include "macros.h"
#include "helper.h"

static bool fade_in_animation_finished = false;

bool can_close_pre_menu_safely = false;

static Color bg_color;
/* 
ESTAMOS USANDO APENAS 1 COR DE TEXTURA PARA TODAS AS TEXTURAS DO PRE MENU.
ISSO ESTÁ SENDO FEITO POR QUE O "fade_in_texture" MODIFICARÁ A MESMA COR PARA AS DUAS TEXTURAS, E ASSIM, A ANIMAÇÃO SERÁ FEITA PARA AMBAS TEXTURAS AO MESMO TEMPO.
*/
static Color textures_color = { 255, 255, 255, 0 };

static Texture2D bg_office_texture;
static GAME_TEXTURE iara_name_gt;

void initialize_pre_menu_textures(void)
{
    bg_color = GetColor(BG_COLOR_HEX);

    Image bg_office_image = LoadImage("code/assets/sprites/tela-inicial/escritorio-tela-inicial.png");
    ImageResize(&bg_office_image, WINDOW_WIDTH, WINDOW_HEIGHT);

    Image iara_name_image = LoadImage("code/assets/sprites/tela-inicial/iara-nome.png");
    int fator_de_resize = 4;
    ImageResize(&iara_name_image, iara_name_image.width / fator_de_resize, iara_name_image.height / fator_de_resize);

    Texture2D iara_name_texture = LoadTextureFromImage(iara_name_image);


    bg_office_texture = LoadTextureFromImage(bg_office_image);
    iara_name_gt = CreateGameTextureFromTexture2D(iara_name_texture);
}

bool close_pre_menu(void)
{
    // PODE FECHAR APENAS DEPOIS DA ANIMAÇÃO TERMINAR.
    fade_out_texture(10, &textures_color, &can_close_pre_menu_safely);

    printf("Valor canal alpha: %d\n", textures_color.a);

    BeginDrawing();
    ClearBackground(bg_color);
        
    // A ordem IMPORTA!! Quem ficará em cima de quem depende disso. Ou seja, Background tem que ir primeiro, depois os textos.
    // bg_office_texture não usa middle_height nem middle_width por que já está com o tamanho máximo da tela e fica no meio naturalmente se por 0 em posX e posY.
    DrawTexture(bg_office_texture, 0, -40, textures_color);
    DrawTexture(iara_name_gt.texture, iara_name_gt.middle_width, iara_name_gt.middle_height + 120, textures_color);


    EndDrawing();
}

void render_pre_menu(void)
{
    fade_in_texture(DEFAULT_FADE_SPEED, &textures_color, &fade_in_animation_finished);

    printf("Valor canal alpha: %d\n", textures_color.a);

    BeginDrawing();
    ClearBackground(bg_color);
        
    // A ordem IMPORTA!! Quem ficará em cima de quem depende disso. Ou seja, Background tem que ir primeiro, depois os textos.
    // bg_office_texture não usa middle_height nem middle_width por que já está com o tamanho máximo da tela e fica no meio naturalmente se por 0 em posX e posY.
    DrawTexture(bg_office_texture, 0, -40, textures_color);
    DrawTexture(iara_name_gt.texture, iara_name_gt.middle_width, iara_name_gt.middle_height + 120, textures_color);


    EndDrawing();

    if (fade_in_animation_finished)
    {
        close_pre_menu();
    }
}


void unload_pre_menu_textures(void)
{
    UnloadTexture(bg_office_texture);
    UnloadTexture(iara_name_gt.texture);
}