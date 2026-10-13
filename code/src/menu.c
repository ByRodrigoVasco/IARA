#include "raylib.h"
#include "menu.h"
#include "macros.h"
#include "fonts.h"
#include "helper.h"
#include "animations.h"

int next_screen = NONE;
int mouse_is_hovering = NONE;

bool is_menu_to_close = false;

// Opção atualmente destacada no menu (INICIAR, CREDITOS ou SAIR).
static int selected_menu_option = INICIAR;

static bool menu_fade_in_finished = false;
// Cor compartilhada por todas as texturas do menu. O último valor (alpha = 0)
// é a opacidade: o fade in sobe esse valor e todas as texturas aparecem juntas.
static Color texture_color_to_fade = {255, 255, 255, 0};

static Texture2D bg_office_texture;
static Texture2D second_bg_texture;
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

    Image arrow_icon_image = LoadImage("code/assets/icons/arrow-icon.png");
    float arrow_icon_scale_divisor = 2.5;
    ImageResize(&arrow_icon_image, (float)arrow_icon_image.width / arrow_icon_scale_divisor, (float)arrow_icon_image.height / arrow_icon_scale_divisor);

    Image bg_office_image = LoadImage("code/assets/sprites/tela-inicial/escritorio-tela-inicial.png");
    float bg_office_scale_divisor = 1.15;
    ImageResize(&bg_office_image, (float)screen_width / bg_office_scale_divisor, (float)screen_height / bg_office_scale_divisor);

    Image second_bg_image = LoadImage("code/assets/background/second-background.png");
    float second_bg_scale_divisor = 1;
    ImageResize(&second_bg_image, (float)screen_width / second_bg_scale_divisor, (float)screen_height / second_bg_scale_divisor);

    Image iara_name_image = LoadImage("code/assets/sprites/tela-inicial/iara-nome.png");
    int iara_name_scale_divisor = 3;
    ImageResize(&iara_name_image, (float)iara_name_image.width / iara_name_scale_divisor, (float)iara_name_image.height / iara_name_scale_divisor);

    Texture2D iara_name_texture = LoadTextureFromImage(iara_name_image);

    // TODO: usar GAME_TEXTURE (helper.h) no lugar de Texture2D para os fundos.
    // O GAME_TEXTURE já guarda "middle_width" e "middle_height", o que evita o
    // cálculo de centralização (largura / 2) feito hoje em render_menu().
    arrow_icon_texture = LoadTextureFromImage(arrow_icon_image);
    bg_office_texture = LoadTextureFromImage(bg_office_image);
    second_bg_texture = LoadTextureFromImage(second_bg_image);

    iara_name_gt = CreateGameTextureFromTexture2D(iara_name_texture);

    UnloadImage(bg_office_image);
    UnloadImage(iara_name_image);
    UnloadImage(arrow_icon_image);
}

void initialize_menu_variables(void)
{
    // Zera as variáveis de controle sempre que o menu é carregado.
    // Sem isso, ao tentar voltar pro menu mais tarde no jogo, ele fecharia instantaneamente.
    is_menu_to_close = false;
    next_screen = NONE;
    mouse_is_hovering = NONE;
    selected_menu_option = INICIAR;

    initialize_menu_textures();

    // PARTE ASSOCIADA A BOTÕES
    iniciar_text_size = MeasureTextEx(default_game_font, "INICIAR", DEFAULT_MENU_FONT_SIZE, DEFAULT_LETTER_SPACING);
    creditos_text_size = MeasureTextEx(default_game_font, "CRÉDITOS", DEFAULT_MENU_FONT_SIZE, DEFAULT_LETTER_SPACING);
    sair_text_size = MeasureTextEx(default_game_font, "SAIR", DEFAULT_MENU_FONT_SIZE, DEFAULT_LETTER_SPACING);

    // As opções começam no meio da tela, abaixo do nome IARA.
    float start_y = middle_screen_y;

    iniciar_text_target_position = (Vector2){TEXT_HORIZONTAL_MARGIN, start_y};
    creditos_text_target_position = (Vector2){TEXT_HORIZONTAL_MARGIN, iniciar_text_target_position.y + iniciar_text_size.y + TEXT_VERTICAL_MARGIN};
    sair_text_target_position = (Vector2){TEXT_HORIZONTAL_MARGIN, creditos_text_target_position.y + creditos_text_size.y + TEXT_VERTICAL_MARGIN};

    iniciar_text_rectangle = (Rectangle){iniciar_text_target_position.x, iniciar_text_target_position.y, iniciar_text_size.x, iniciar_text_size.y};
    creditos_text_rectangle = (Rectangle){creditos_text_target_position.x, creditos_text_target_position.y, creditos_text_size.x, creditos_text_size.y};
    sair_text_rectangle = (Rectangle){sair_text_target_position.x, sair_text_target_position.y, sair_text_size.x, sair_text_size.y};
}

// Move a seleção para a opção seguinte (direction = 1) ou anterior (direction = -1),
// voltando ao início/fim da lista ao ultrapassar os limites.
static void move_menu_selection(int direction)
{
    selected_menu_option += direction;

    if (selected_menu_option > SAIR)
    {
        selected_menu_option = INICIAR;
    }
    else if (selected_menu_option < INICIAR)
    {
        selected_menu_option = SAIR;
    }
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

// Confirma a opção selecionada: guarda a escolha e sinaliza o fechamento do menu.
static void confirm_menu_option(void)
{
    SetMouseCursor(MOUSE_CURSOR_ARROW);
    next_screen = selected_menu_option;
    is_menu_to_close = true;
}

// Lê as teclas do menu: seta para cima ou W sobe; seta para baixo ou S desce;
// Enter ou Espaço confirmam a opção.
static void handle_keyboard_navigation(void)
{
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
    {
        move_menu_selection(1);
    }

    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
    {
        move_menu_selection(-1);
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
    {
        confirm_menu_option();
    }
}

// Descobre qual opção do menu está sob o mouse (NONE se nenhuma).
static int get_option_under_mouse(void)
{
    if (check_mouse_hover(iniciar_text_rectangle))
    {
        return INICIAR;
    }

    if (check_mouse_hover(creditos_text_rectangle))
    {
        return CREDITOS;
    }

    if (check_mouse_hover(sair_text_rectangle))
    {
        return SAIR;
    }

    return NONE;
}

// Trata o mouse: mover o mouse destaca a opção sob ele e o clique a confirma.
// O destaque só muda quando o mouse se mexe, para não brigar com o teclado.
void handle_mouse_action()
{
    int hovered_option = get_option_under_mouse();
    Vector2 mouse_movement = GetMouseDelta();
    bool mouse_moved = (mouse_movement.x != 0 || mouse_movement.y != 0);

    mouse_is_hovering = hovered_option;

    if (hovered_option == NONE)
    {
        SetMouseCursor(MOUSE_CURSOR_ARROW);
        return;
    }

    SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);

    if (mouse_moved)
    {
        selected_menu_option = hovered_option;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        selected_menu_option = hovered_option;
        confirm_menu_option();
    }
}

// Desenha a seta ao lado da opção selecionada (por teclado ou mouse).
void handle_render_arrow()
{
    float half_of_arrow_texture = (arrow_icon_texture.height / 2);
    Rectangle selected_rectangle;

    switch (selected_menu_option)
    {
    case INICIAR:
        selected_rectangle = iniciar_text_rectangle;
        break;
    case CREDITOS:
        selected_rectangle = creditos_text_rectangle;
        break;
    case SAIR:
        selected_rectangle = sair_text_rectangle;
        break;
    default:
        return;
    }

    DrawTexture(arrow_icon_texture, selected_rectangle.x - arrow_icon_texture.width, selected_rectangle.y + half_of_arrow_texture, RAYWHITE);
}

// Desenha um quadro do menu: fade in, entradas (teclado e mouse), fundos,
// textos das opções e seta de seleção.
void render_menu(void)
{
    if (!menu_fade_in_finished)
    {
        fade_in_texture(FAST_FADE_SPEED, &texture_color_to_fade, &menu_fade_in_finished);
    }

    handle_keyboard_navigation();
    handle_mouse_action();

    BeginDrawing();
    ClearBackground(BG_COLOR);

    DrawTexture(second_bg_texture, middle_screen_x - (bg_office_texture.width / 2), middle_screen_y - (bg_office_texture.height / 2), texture_color_to_fade);
    DrawTexture(bg_office_texture, middle_screen_x - (bg_office_texture.width / 2.5), middle_screen_y - (bg_office_texture.height / 2), texture_color_to_fade);
    DrawTexture(iara_name_gt.texture, -10, TEXT_VERTICAL_MARGIN, texture_color_to_fade);

    DrawTextEx(default_game_font, "INICIAR", iniciar_text_target_position, DEFAULT_MENU_FONT_SIZE, DEFAULT_LETTER_SPACING, texture_color_to_fade);
    DrawTextEx(default_game_font, "CRÉDITOS", creditos_text_target_position, DEFAULT_MENU_FONT_SIZE, DEFAULT_LETTER_SPACING, texture_color_to_fade);
    DrawTextEx(default_game_font, "SAIR", sair_text_target_position, DEFAULT_MENU_FONT_SIZE, DEFAULT_LETTER_SPACING, texture_color_to_fade);

    handle_render_arrow();

    EndDrawing();
}

void unload_menu()
{
    UnloadTexture(bg_office_texture);
    UnloadTexture(arrow_icon_texture);
    UnloadGameTexture(iara_name_gt);
}