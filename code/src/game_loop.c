#include <time.h>
#include <stdio.h>

#include "raylib.h"
#include "game_loop.h"
#include "macros.h"
#include "fonts.h"
#include "animations.h"
#include "characters.h"

#define DID_NOT_START -30

// GAME LOOP SCREENS
#define SCREEN_NONE 0
#define CALENDAR_DAY_CARD_SCREEN 1
#define IN_GAME_SCREEN 2
#define COMPUTER_SCREEN 3

// CALENDAR DAY STATES
#define NONE 0
#define FADING_IN 1
#define AWAIT 2
#define FADING_OUT 3

#define NO_KEY_PRESSED 0

#define REFERENCE_HEIGHT 1080.0f
#define PLAYER_SIZE_AT_REFERENCE 75.0f
#define COMPUTER_OFFSET_X_AT_REFERENCE 170.0f
#define COMPUTER_OFFSET_Y_AT_REFERENCE 93.0f
#define COMPUTER_SIZE_AT_REFERENCE 100.0f

static bool animation_finished = false;

static int current_key_pressed = NO_KEY_PRESSED;

static int current_calendar_day_state = NONE;

static bool render_calendar_day_card_finished = false;
static int current_screen = SCREEN_NONE;

static bool enter_computer_screen = false;

static Texture2D second_background_texture;
static Color color_to_fade = {255, 255, 255, 0};

static char *current_card_title = "Dia 1";
static Vector2 calendar_card_title_text_size;
static char *current_card_subtitle_line_1 = "Sua mesa fica na salinha ao lado do corredor, um pouco afastada do resto.";
static Vector2 calendar_card_subtitle_text_size_line_1;

static char *current_card_subtitle_line_2 = "Ninguém te recebe. O computador já está ligado, esperando.";
static Vector2 calendar_card_subtitle_text_size_line_2;

// IN-GAME VARIABLES
static Texture2D in_game_office;

static char_textures estagiario_texture;
static Vector2 current_estagiario_position;
static Texture2D current_estagiarion_texture_direction;

static Rectangle estagiario_computer_collision_rectangle;
static Rectangle estagiario_collision_rec;

// ---------------------------------------------------------------------------
// SCALING
// ---------------------------------------------------------------------------

// 1.0 at 1080p, 0.667 at 720p, 2.0 at 2160p...
static float get_scale(void)
{
    return GetScreenHeight() / REFERENCE_HEIGHT;
}

// 90 px at 1080p, scales with the window
static float get_player_size(void)
{
    return PLAYER_SIZE_AT_REFERENCE * get_scale();
}

static Rectangle get_computer_collision_rectangle(void)
{
    float scale = get_scale();
    return (Rectangle){
        GetScreenWidth() / 2.0f + COMPUTER_OFFSET_X_AT_REFERENCE * scale,
        GetScreenHeight() / 2.0f + COMPUTER_OFFSET_Y_AT_REFERENCE * scale,
        COMPUTER_SIZE_AT_REFERENCE * scale,
        COMPUTER_SIZE_AT_REFERENCE * scale};
}

// ---------------------------------------------------------------------------

// VAI FAZER O UPDATE DA POSIÇÃO DO RETÂNGULO DE COLISÃO
void update_collision_rec(void)
{
    float size = get_player_size();
    estagiario_collision_rec = (Rectangle){current_estagiario_position.x, current_estagiario_position.y, size, size};
}

void initialize_game_loop_variables(void)
{
    second_background_texture = LoadTexture("code/assets/background/second-background.png");
    calendar_card_title_text_size = MeasureTextEx(default_game_font, current_card_title, TITLE_FONT_SIZE, DEFAULT_LETTER_SPACING);
    calendar_card_subtitle_text_size_line_1 = MeasureTextEx(default_game_font, current_card_subtitle_line_1, SUBTITLE_FONT_SIZE, DEFAULT_LETTER_SPACING);
    calendar_card_subtitle_text_size_line_2 = MeasureTextEx(default_game_font, current_card_subtitle_line_2, SUBTITLE_FONT_SIZE, DEFAULT_LETTER_SPACING);

    in_game_office = LoadTexture("code/assets/sprites/environment/office.png");

    initialize_estagiario_1_textures(estagiario_texture);

    // ONDE ELE IRÁ SPAWNAR
    current_estagiario_position = (Vector2){middle_screen_x, middle_screen_y};
    current_estagiarion_texture_direction = estagiario_texture[SOUTH];

    estagiario_computer_collision_rectangle = get_computer_collision_rectangle();
    update_collision_rec();
}

void render_calendar_day_card(void)
{
    DrawTextEx(default_game_font, current_card_title,
               (Vector2){middle_screen_x - (calendar_card_title_text_size.x / 2), middle_screen_y - (calendar_card_subtitle_text_size_line_1.y * 5)},
               TITLE_FONT_SIZE, DEFAULT_LETTER_SPACING, color_to_fade);

    DrawTextEx(default_game_font, current_card_subtitle_line_1, (Vector2){middle_screen_x - (calendar_card_subtitle_text_size_line_1.x / 2), middle_screen_y - (calendar_card_subtitle_text_size_line_1.y / 2)},
               SUBTITLE_FONT_SIZE, DEFAULT_LETTER_SPACING, color_to_fade);

    DrawTextEx(default_game_font, current_card_subtitle_line_2, (Vector2){middle_screen_x - (calendar_card_subtitle_text_size_line_2.x / 2), middle_screen_y + (calendar_card_subtitle_text_size_line_1.y / 2)},
               SUBTITLE_FONT_SIZE, DEFAULT_LETTER_SPACING, color_to_fade);
}

void handle_calendar_day_card(void)
{
    bool fading_in_finished = false;

    static double timer = DID_NOT_START;

    double amount_of_seconds_to_wait = 0;

    switch (current_calendar_day_state)
    {

    case NONE:
        current_calendar_day_state = FADING_IN;
        break;

    case FADING_IN:
        fade_in_texture(FAST_FADE_SPEED, &color_to_fade, &fading_in_finished);
        render_calendar_day_card();

        if (fading_in_finished)
        {
            current_calendar_day_state = AWAIT;
        }

        break;

    case AWAIT:
        render_calendar_day_card();

        if (timer == DID_NOT_START)
        {
            timer = GetTime();
        }

        // tempo decorrido, não o timestamp
        if (GetTime() - timer > amount_of_seconds_to_wait)
        {
            timer = DID_NOT_START;
            current_calendar_day_state = FADING_OUT;
        }

        break;

    case FADING_OUT:
        fade_out_texture(FAST_FADE_SPEED, &color_to_fade, &render_calendar_day_card_finished);
        render_calendar_day_card();
        break;

    default:
        printf("----------CURRENT CALENDAR DAY STATE INCORRECT----------\n");
        break;
    }
}

void unload_calendar_day_textures(void)
{
    UnloadTexture(second_background_texture);
}

void handle_map_interactables(void)
{
    if (CheckCollisionRecs(estagiario_collision_rec, estagiario_computer_collision_rectangle))
    {
        if (IsKeyPressed(KEY_SPACE))
        {
            enter_computer_screen = true;
            printf("Computer screen...\n");
        }
    }
}

// Código muito estranho, mas vamos lá!
// E necessita ser melhorado. O player acaba ficando preso por uns milésimos quando se aperta várias teclas simultaneamente.
void handle_player_input(void)
{
    float speed = DEFAULT_PLAYER_MOVEMENT_SPEED * get_scale();

    // Esses daqui são os modificadores de posição
    int dx = 0;
    int dy = 0;
    
    // Se algum desses controles aqui forem pressionados, mexem nos modificadores.
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    dy -= 1;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  dy += 1;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  dx -= 1;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) dx += 1;

    // Para caso nada seja pressionado
    if (dx == 0 && dy == 0)
    {
        update_collision_rec();
        return;
    }

    // Aqui que tem a estranheza.
    // Cada if statement está checando se dx e dy está sendo modificado.
    // Ou seja, se dx (variável de modificação da posição horizontal) e dy (variável de modificação da posição vertical) forem modificados, agirá de acordo.
    if      (dx == 0 && dy < 0) current_estagiarion_texture_direction = estagiario_texture[NORTH];
    else if (dx == 0 && dy > 0) current_estagiarion_texture_direction = estagiario_texture[SOUTH];
    else if (dx < 0  && dy == 0) current_estagiarion_texture_direction = estagiario_texture[WEST];
    else if (dx > 0  && dy == 0) current_estagiarion_texture_direction = estagiario_texture[EAST];
    else if (dx < 0  && dy < 0) current_estagiarion_texture_direction = estagiario_texture[NORTHWEST];
    else if (dx > 0  && dy < 0) current_estagiarion_texture_direction = estagiario_texture[NORTHEAST];
    else if (dx < 0  && dy > 0) current_estagiarion_texture_direction = estagiario_texture[SOUTHWEST];
    else                        current_estagiarion_texture_direction = estagiario_texture[SOUTHEAST];

    // Isso é mais esquisito ainda, mas é só um scaler para ajeitar a velocidade diagonal (que é mais alta que a velocidade horizontal/vertical).
    if (dx != 0 && dy != 0)
    {
        speed *= 0.7071f;
    }

    current_estagiario_position.x += dx * speed;
    current_estagiario_position.y += dy * speed;

    update_collision_rec();
}

void render_player(void)
{
    float size = get_player_size();

    Rectangle source = {0, 0,
                        (float)current_estagiarion_texture_direction.width,
                        (float)current_estagiarion_texture_direction.height};
    Rectangle dest = {current_estagiario_position.x,
                      current_estagiario_position.y,
                      size, size};

    DrawTexturePro(current_estagiarion_texture_direction, source, dest,
                   (Vector2){0, 0}, 0.0f, color_to_fade);
}

void render_game_loop_escritorio(void)
{
    float scale = get_scale();
    float office_width = in_game_office.width * scale;
    float office_height = in_game_office.height * scale;

    Rectangle source = {0, 0, (float)in_game_office.width, (float)in_game_office.height};
    Rectangle dest = {(GetScreenWidth() - office_width) / 2.0f,
                      (GetScreenHeight() - office_height) / 2.0f,
                      office_width, office_height};

    DrawTexturePro(in_game_office, source, dest, (Vector2){0, 0}, 0.0f, color_to_fade);

    bool trash;
    fade_in_texture(FAST_FADE_SPEED, &color_to_fade, &trash);
}

void handle_game_loop_screens(void)
{
    switch (current_screen)
    {
    case SCREEN_NONE:
        current_screen = CALENDAR_DAY_CARD_SCREEN;
        break;

    case CALENDAR_DAY_CARD_SCREEN:
        handle_calendar_day_card();

        if (render_calendar_day_card_finished)
        {
            unload_calendar_day_textures();
            current_screen = IN_GAME_SCREEN;
        }
        break;

    case IN_GAME_SCREEN:
        // se a janela for redimensionável, mantém o retângulo do computador atualizado
        estagiario_computer_collision_rectangle = get_computer_collision_rectangle();

        handle_player_input();
        handle_map_interactables();

        // ORDEM IMPORTA: primeiro o fundo, depois o jogador
        render_game_loop_escritorio();
        render_player();
        break;

    default:
        break;
    }
}

void render_game_loop(void)
{
    BeginDrawing();
    ClearBackground(BG_COLOR);

    handle_game_loop_screens();

    EndDrawing();
}

void unload_game_loop(void)
{
    // second_background_texture já é descarregada em unload_calendar_day_textures()
    UnloadTexture(in_game_office);

    for (int i = 0; i < DIRECTION_COUNT; i++)
    {
        UnloadTexture(estagiario_texture[i]);
    }
}