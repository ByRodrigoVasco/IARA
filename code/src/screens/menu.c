#include "raylib.h"
#include "menu.h"
#include "macros.h"
#include "fonts.h"
#include "helper.h"
#include "animations.h"
#include "accessibility.h"

// Tamanhos em 1080p, como divisores da imagem original ou da janela.
#define ARROW_ICON_SCALE_DIVISOR 2.5f
#define IARA_NAME_SCALE_DIVISOR 3.0f
#define MENU_OFFICE_SCALE_DIVISOR 1.15f

#define FULLSCREEN_HINT "F11: tela cheia"

int next_screen = NONE;
int mouse_is_hovering = NONE;

bool is_menu_to_close = false;

// Opção atualmente destacada no menu (INICIAR, CREDITOS, CONFIGURACOES ou SAIR).
static int selected_menu_option = INICIAR;

static bool menu_fade_in_finished = false;
// Cor compartilhada por todas as texturas do menu. O último valor (alpha = 0)
// é a opacidade: o fade in sobe esse valor e todas as texturas aparecem juntas.
static Color texture_color_to_fade = {255, 255, 255, 0};

static Texture2D bg_office_texture;
static Texture2D second_bg_texture;
static Texture2D arrow_icon_texture;
static Texture2D iara_name_texture;

static float menu_scale;
static float menu_font_size;
static float menu_letter_spacing;

static Vector2 iniciar_text_size;
static Vector2 creditos_text_size;
static Vector2 configuracoes_text_size;
static Vector2 sair_text_size;

static Vector2 iniciar_text_target_position;
static Vector2 creditos_text_target_position;
static Vector2 configuracoes_text_target_position;
static Vector2 sair_text_target_position;

static Rectangle iniciar_text_rectangle;
static Rectangle creditos_text_rectangle;
static Rectangle configuracoes_text_rectangle;
static Rectangle sair_text_rectangle;

static void initialize_menu_textures(void)
{
    // Carregadas no tamanho original: o tamanho na tela é calculado a cada quadro
    arrow_icon_texture = load_smooth_texture("code/assets/icons/arrow-icon.png");
    bg_office_texture = load_smooth_texture("code/assets/sprites/tela-inicial/escritorio-tela-inicial.png");
    second_bg_texture = load_smooth_texture("code/assets/background/second-background.png");
    iara_name_texture = load_smooth_texture("code/assets/sprites/tela-inicial/iara-nome.png");
}

// Recalcula as opções do menu para o tamanho atual da janela.
static void update_menu_layout(void)
{
    menu_scale = get_ui_scale();
    menu_font_size = DEFAULT_MENU_FONT_SIZE * menu_scale;
    menu_letter_spacing = DEFAULT_LETTER_SPACING * menu_scale;

    float margin_x = TEXT_HORIZONTAL_MARGIN * menu_scale;
    float margin_y = TEXT_VERTICAL_MARGIN * menu_scale;

    iniciar_text_size = MeasureTextEx(default_game_font, "INICIAR", menu_font_size, menu_letter_spacing);
    creditos_text_size = MeasureTextEx(default_game_font, "CRÉDITOS", menu_font_size, menu_letter_spacing);
    configuracoes_text_size = MeasureTextEx(default_game_font, "CONFIGURAÇÕES", menu_font_size, menu_letter_spacing);
    sair_text_size = MeasureTextEx(default_game_font, "SAIR", menu_font_size, menu_letter_spacing);

    // As opções começam no meio da tela, abaixo do nome IARA.
    float start_y = GetScreenHeight() / 2.0f;

    iniciar_text_target_position = (Vector2){margin_x, start_y};
    creditos_text_target_position = (Vector2){margin_x, iniciar_text_target_position.y + iniciar_text_size.y + margin_y};
    configuracoes_text_target_position = (Vector2){margin_x, creditos_text_target_position.y + creditos_text_size.y + margin_y};
    sair_text_target_position = (Vector2){margin_x, configuracoes_text_target_position.y + configuracoes_text_size.y + margin_y};

    iniciar_text_rectangle = (Rectangle){iniciar_text_target_position.x, iniciar_text_target_position.y, iniciar_text_size.x, iniciar_text_size.y};
    creditos_text_rectangle = (Rectangle){creditos_text_target_position.x, creditos_text_target_position.y, creditos_text_size.x, creditos_text_size.y};
    configuracoes_text_rectangle = (Rectangle){configuracoes_text_target_position.x, configuracoes_text_target_position.y, configuracoes_text_size.x, configuracoes_text_size.y};
    sair_text_rectangle = (Rectangle){sair_text_target_position.x, sair_text_target_position.y, sair_text_size.x, sair_text_size.y};
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
    update_menu_layout();
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

    if (check_mouse_hover(configuracoes_text_rectangle))
    {
        return CONFIGURACOES;
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
    float arrow_width = (arrow_icon_texture.width / ARROW_ICON_SCALE_DIVISOR) * menu_scale;
    float arrow_height = (arrow_icon_texture.height / ARROW_ICON_SCALE_DIVISOR) * menu_scale;
    Rectangle selected_rectangle;

    switch (selected_menu_option)
    {
    case INICIAR:
        selected_rectangle = iniciar_text_rectangle;
        break;
    case CREDITOS:
        selected_rectangle = creditos_text_rectangle;
        break;
    case CONFIGURACOES:
        selected_rectangle = configuracoes_text_rectangle;
        break;
    case SAIR:
        selected_rectangle = sair_text_rectangle;
        break;
    default:
        return;
    }

    Rectangle arrow_source = {0, 0, (float)arrow_icon_texture.width, (float)arrow_icon_texture.height};
    Rectangle arrow_dest = {selected_rectangle.x - arrow_width, selected_rectangle.y + (selected_rectangle.height - arrow_height) / 2.0f, arrow_width, arrow_height};

    DrawTexturePro(arrow_icon_texture, arrow_source, arrow_dest, (Vector2){0, 0}, 0.0f, RAYWHITE);
}

// Desenha um quadro do menu: fade in, entradas (teclado e mouse), fundos,
// textos das opções e seta de seleção.
void render_menu(void)
{
    if (!menu_fade_in_finished)
    {
        fade_in_texture(FAST_FADE_SPEED, &texture_color_to_fade, &menu_fade_in_finished);
    }

    update_menu_layout();

    handle_keyboard_navigation();
    handle_mouse_action();

    begin_game_drawing();
    ClearBackground(BG_COLOR);

    float window_w = (float)GetScreenWidth();
    float window_h = (float)GetScreenHeight();
    float office_w = window_w / MENU_OFFICE_SCALE_DIVISOR;
    float office_h = window_h / MENU_OFFICE_SCALE_DIVISOR;

    draw_texture_cover(second_bg_texture, (Rectangle){0, 0, window_w, window_h}, texture_color_to_fade);
    draw_texture_cover(bg_office_texture, (Rectangle){(window_w / 2.0f) - (office_w / 2.5f), (window_h - office_h) / 2.0f, office_w, office_h}, texture_color_to_fade);

    float name_scale = menu_scale / IARA_NAME_SCALE_DIVISOR;
    Rectangle name_source = {0, 0, (float)iara_name_texture.width, (float)iara_name_texture.height};
    Rectangle name_dest = {-10.0f * menu_scale, TEXT_VERTICAL_MARGIN * menu_scale, iara_name_texture.width * name_scale, iara_name_texture.height * name_scale};

    DrawTexturePro(iara_name_texture, name_source, name_dest, (Vector2){0, 0}, 0.0f, texture_color_to_fade);

    DrawTextEx(default_game_font, "INICIAR", iniciar_text_target_position, menu_font_size, menu_letter_spacing, texture_color_to_fade);
    DrawTextEx(default_game_font, "CRÉDITOS", creditos_text_target_position, menu_font_size, menu_letter_spacing, texture_color_to_fade);
    DrawTextEx(default_game_font, "CONFIGURAÇÕES", configuracoes_text_target_position, menu_font_size, menu_letter_spacing, texture_color_to_fade);
    DrawTextEx(default_game_font, "SAIR", sair_text_target_position, menu_font_size, menu_letter_spacing, texture_color_to_fade);

    handle_render_arrow();

    // Mostra o atalho da tela cheia, que de outro modo o jogador não teria como descobrir
    float hint_size = DEFAULT_TEXT_FONT_SIZE * menu_scale;
    Vector2 hint_measure = MeasureTextEx(default_game_font, FULLSCREEN_HINT, hint_size, menu_letter_spacing / 2.0f);
    Vector2 hint_position = {window_w - hint_measure.x - (TEXT_VERTICAL_MARGIN * menu_scale), window_h - hint_measure.y - (TEXT_VERTICAL_MARGIN * menu_scale)};

    DrawTextEx(default_game_font, FULLSCREEN_HINT, hint_position, hint_size, menu_letter_spacing / 2.0f, texture_color_to_fade);

    end_game_drawing();
}

void unload_menu()
{
    UnloadTexture(bg_office_texture);
    UnloadTexture(second_bg_texture);
    UnloadTexture(arrow_icon_texture);
    UnloadTexture(iara_name_texture);
}