#include <stdio.h>

#include "raylib.h"
#include "macros.h"
#include "fonts.h"
#include "helper.h"
#include "animations.h"

#define NONE 0
#define INICIAR 1
#define CREDITOS 2
#define SAIR 3

#define TEXT_HORIZONTAL_MARGIN 40
#define TEXT_VERTICAL_MARGIN 30
#define DEFAULT_MENU_LETTER_SPACING 5
#define DEFAULT_MENU_FONT_SIZE 35

#define INICIAR_MENU_FONT_SIZE 50

int next_screen = NONE;
int mouse_is_hovering = NONE;

bool is_menu_to_close = false;

static bool menu_fade_in_finished = false;
// ESSE "0" REPRESENTA O "ALPHA CHANNEL" QUE SERVE COMO OPACIDADE.
// PARA FAZER A ANIMAÇÃO DE FADE IN, TODOS OS OBJETOS RECEBERIAM ESSA COR PARA FAZER O FADE IN JUNTOS.
static Color texture_color_to_fade = {255, 255, 255, 0};

static int text_mouse_is_hovering = NONE;
static Color bg_color;

static Texture2D bg_office_texture;
static Texture2D arrow_icon_texture;
static GAME_TEXTURE iara_name_gt;

static Vector2 iniciar_text_size;
static Vector2 creditos_text_size;
static Vector2 sair_text_size;

static Vector2 iniciar_text_target_position;
static Vector2 creditos_text_target_position;
static Vector2 sair_text_target_position;

static Rectangle iniciar_text_rectangle;
static Rectangle creditos_text_rectangle;
static Rectangle sair_text_rectangle;

static void initialize_menu_textures(void)
{
    // -----------------------------------------------------------------------------------------------
    // PARTE ASSOCIADA AO BACKGROUND DO MENU
    // -----------------------------------------------------------------------------------------------
    bg_color = GetColor(BG_COLOR_HEX);

    float resize_factor_arrow_icon = 3;
    Image arrow_icon_image = LoadImage("code/assets/icons/arrow-icon.png");
    ImageResize(&arrow_icon_image, arrow_icon_image.width / resize_factor_arrow_icon, arrow_icon_image.height / resize_factor_arrow_icon);

    float resize_factor_bg_office = 1.2;
    Image bg_office_image = LoadImage("code/assets/sprites/tela-inicial/escritorio-tela-inicial.png");
    ImageResize(&bg_office_image, (float)WINDOW_WIDTH / resize_factor_bg_office, (float)WINDOW_HEIGHT / resize_factor_bg_office);

    Image iara_name_image = LoadImage("code/assets/sprites/tela-inicial/iara-nome.png");
    int resize_factor_iara_name = 6;
    ImageResize(&iara_name_image, iara_name_image.width / resize_factor_iara_name, iara_name_image.height / resize_factor_iara_name);

    Texture2D iara_name_texture = LoadTextureFromImage(iara_name_image);
    arrow_icon_texture = LoadTextureFromImage(arrow_icon_image);

    bg_office_texture = LoadTextureFromImage(bg_office_image);
    iara_name_gt = CreateGameTextureFromTexture2D(iara_name_texture);

    UnloadImage(bg_office_image);
    UnloadImage(iara_name_image);
    UnloadImage(arrow_icon_image);
}

void initialize_menu_variables(void)
{
    // [CORREÇÃO]: Zera as variáveis de controle sempre que o menu é carregado.
    // Sem isso, ao tentar voltar pro menu mais tarde no jogo, ele fecharia instantaneamente.
    is_menu_to_close = false;
    next_screen = NONE;
    mouse_is_hovering = NONE;

    initialize_menu_textures();

    // PARTE ASSOCIADA A BOTÕES
    iniciar_text_size = MeasureTextEx(default_font_menu, "INICIAR", INICIAR_MENU_FONT_SIZE, DEFAULT_MENU_LETTER_SPACING);
    creditos_text_size = MeasureTextEx(default_font_menu, "CRÉDITOS", DEFAULT_MENU_FONT_SIZE, DEFAULT_MENU_LETTER_SPACING);
    sair_text_size = MeasureTextEx(default_font_menu, "SAIR", DEFAULT_MENU_FONT_SIZE, DEFAULT_MENU_LETTER_SPACING);

    float total_menu_height = iniciar_text_size.y + creditos_text_size.y + sair_text_size.y + (2 * TEXT_VERTICAL_MARGIN);

    // COLOQUEI + 40 NO MEIO PARA FICAR MAIS ABAIXO DO NOME IARA
    float start_y = (MIDDLE_SCREEN_Y + 40) - (total_menu_height / 2.0f);

    iniciar_text_target_position = (Vector2){TEXT_HORIZONTAL_MARGIN, start_y};
    creditos_text_target_position = (Vector2){TEXT_HORIZONTAL_MARGIN, iniciar_text_target_position.y + iniciar_text_size.y + TEXT_VERTICAL_MARGIN};
    sair_text_target_position = (Vector2){TEXT_HORIZONTAL_MARGIN, creditos_text_target_position.y + creditos_text_size.y + TEXT_VERTICAL_MARGIN};

    iniciar_text_rectangle = (Rectangle){iniciar_text_target_position.x, iniciar_text_target_position.y, iniciar_text_size.x, iniciar_text_size.y};
    creditos_text_rectangle = (Rectangle){creditos_text_target_position.x, creditos_text_target_position.y, creditos_text_size.x, creditos_text_size.y};
    sair_text_rectangle = (Rectangle){sair_text_target_position.x, sair_text_target_position.y, sair_text_size.x, sair_text_size.y};
}

bool check_mouse_hover(Rectangle collision_rectangle)
{
    if (IsWindowFocused())
    {
        Vector2 mouse_position = GetMousePosition();
        Rectangle mouse_pointer_rectangle = {mouse_position.x, mouse_position.y, 1, 1};

        if (CheckCollisionRecs(mouse_pointer_rectangle, collision_rectangle))
        {
            return true;
        }

        return false;
    }

    return false;
}

void handle_mouse_action()
{
    if (check_mouse_hover(iniciar_text_rectangle))
    {
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        mouse_is_hovering = INICIAR;

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            printf("1\n");
            next_screen = INICIAR;
            is_menu_to_close = true;
        }
    }
    else if (check_mouse_hover(creditos_text_rectangle))
    {
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        mouse_is_hovering = CREDITOS;

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            printf("2\n");
            next_screen = CREDITOS;
            is_menu_to_close = true;
        }
    }
    else if (check_mouse_hover(sair_text_rectangle))
    {
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        mouse_is_hovering = SAIR;

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            printf("3\n");
            next_screen = SAIR;
            is_menu_to_close = true;
        }
    }
    else
    {
        mouse_is_hovering = NONE;
        SetMouseCursor(MOUSE_CURSOR_ARROW);
    }
}

void handle_render_arrow()
{
    switch (mouse_is_hovering)
    {
    case NONE:
        break;
    case INICIAR:
        DrawTexture(arrow_icon_texture, iniciar_text_target_position.x - arrow_icon_texture.width, iniciar_text_target_position.y + 3, RAYWHITE);
        break;
    case CREDITOS:
        DrawTexture(arrow_icon_texture, creditos_text_rectangle.x - arrow_icon_texture.width, creditos_text_rectangle.y - 3, RAYWHITE);
        break;
    case SAIR:
        DrawTexture(arrow_icon_texture, sair_text_rectangle.x - arrow_icon_texture.width, sair_text_rectangle.y - 3, RAYWHITE);
        break;
    default:
        break;
    }
}

void render_menu(void)
{
    if (!menu_fade_in_finished)
    {
        fade_in_texture(FAST_FADE_SPEED, &texture_color_to_fade, &menu_fade_in_finished);
    }

    handle_mouse_action();

    BeginDrawing();
    ClearBackground(BG_COLOR);

    DrawTexture(bg_office_texture, 200, 50, texture_color_to_fade);
    DrawTexture(iara_name_gt.texture, -10, TEXT_VERTICAL_MARGIN, texture_color_to_fade);

    DrawTextEx(default_font_menu, "INICIAR", iniciar_text_target_position, INICIAR_MENU_FONT_SIZE, DEFAULT_MENU_LETTER_SPACING, texture_color_to_fade);
    DrawTextEx(default_font_menu, "CRÉDITOS", creditos_text_target_position, DEFAULT_MENU_FONT_SIZE, DEFAULT_MENU_LETTER_SPACING, texture_color_to_fade);
    DrawTextEx(default_font_menu, "SAIR", sair_text_target_position, DEFAULT_MENU_FONT_SIZE, DEFAULT_MENU_LETTER_SPACING, texture_color_to_fade);

    handle_render_arrow();

    EndDrawing();
}

void unload_menu()
{
    UnloadTexture(bg_office_texture);
    UnloadTexture(arrow_icon_texture);
    UnloadGameTexture(iara_name_gt);
}