#include "raylib.h"

/*
AQUI ESTARÃO OS MACROS GLOBAIS QUE SERÃO USADOS POR DIVERSOS ARQUIVOS.
NEM TODOS OS MACROS ESTARÃO AQUI.
*/
// RELACIONADOS A FONT
#define TITLE_FONT_SIZE 200
#define SUBTITLE_FONT_SIZE (TITLE_FONT_SIZE / 4)
#define DEFAULT_TEXT_FONT_SIZE 30
#define TEXT_HORIZONTAL_MARGIN 60
#define TEXT_VERTICAL_MARGIN 30
#define DEFAULT_LETTER_SPACING 5
#define DEFAULT_MENU_FONT_SIZE 80

// -----------------------------------------------------------------------------
// Configurações de Janela
// PS: essas variáveis são definidas na main.c
// -----------------------------------------------------------------------------
/*
 * @brief width inicial calculada automaticamente na main.c
 */
extern unsigned int screen_width;
/*
 * @brief height inicial calculada automaticamente na main.c
 */
extern unsigned int screen_height;
#define IARA_TITLE "IARA"
/*
 * @brief meio da tela horizontal calculada automaticamente na main.c
 */
extern unsigned int middle_screen_x;
/*
 * @brief meio da tela vertical calculada automaticamente na main.c
 */
extern unsigned int middle_screen_y;

// VARIÁVEIS DE ANIMAÇÕES
#define DEFAULT_FADE_SPEED 1
#define MEDIUM_FADE_SPEED 5
#define FAST_FADE_SPEED 10

// -----------------------------------------------------------------------------
// CORES
// -----------------------------------------------------------------------------

// IMPORTANTE: MELHOR TER CUIDADO COM HEX COLOR! NO FIGMA, USAMOS O "SHORT HEX COLOR" QUE SÓ TEM ---6 DÍGITOS---
// AQUI NO RAYLIB PRECISAMOS DE ---8 DÍGITOS--- NO FORMATO "0xRRGGBBAA"
#define BG_COLOR_HEX 0x140b15FF
#define DEFAULT_TEXTURE_COLOR 0xFFFFFFFF
#define CYAN_HEX_COLOR 0x00FFFFFF

#define BG_COLOR GetColor(BG_COLOR_HEX)
#define COMPUTER_BG_COLOR GetColor(CYAN_HEX_COLOR)

// TIPOS DE TELAS
#define PRE_MENU_SCREEN 0
#define MENU_SCREEN 1
#define PLAYING_SCREEN 2
#define FLASHING_SCREEN 3
#define JUMPING_SCREEN 4
#define CREDITS_SCREEN 5

// TIPOS DE DIAS E TELAS DE GAME_LOOP
#define NOT_YET_DEFINED 0
#define PLAYABLE_DAY 1
#define FLASH_DAY 2
#define JUMP_DAY 3

// PATHS PARA ASSETS
#define SPRITES_PATH "code/assets/sprites/"
#define PERSONAGENS_PATH    SPRITES_PATH "personagens/"
#define IARA_NOME_PNG_PATH "code/assets/sprites/tela-inicial/iara-nome.png"
#define ESCRITORIO_TELA_PNG_PATH "code/assets/sprites/tela-inicial/escritorio-tela-inicial.png"


#define ENVIRONMENT_SPRITES_PATH       SPRITES_PATH "environment/"
#define ACTION_BUTTONS_SPRITES_PATH    ENVIRONMENT_SPRITES_PATH "action.png"
#define MOVE_BUTTONS_SPRITES_PATH      ENVIRONMENT_SPRITES_PATH "move.png"
#define MOVE_OR_BUTTONS_SPRITES_PATH   ENVIRONMENT_SPRITES_PATH "move-or.png"

// Icon de indicação de interação com o computador
#define ARROW_DOWN_PNG_PATH  "code/assets/icons/arrow-down.png"

// PATHS PARA O COMPUTADOR
#define COMPUTER_ASSETS_PATH           "code/assets/sprites/computer/"
#define COMPUTER_WINDOW_PNG_PATH       COMPUTER_ASSETS_PATH "computer-window.png"
#define COMPUTER_CORRECT_ICON          COMPUTER_ASSETS_PATH "correct-icon.png"
#define COMPUTER_WRONG_ICON            COMPUTER_ASSETS_PATH "wrong-icon.png"

// PATHS PARA OS CRÉDITOS
#define TEAM_ASSETS_PATH "docs/assets/team/"
#define LAURA_BRAYNER_CREDITS_PATH TEAM_ASSETS_PATH "laura-brayner.png"
#define RODRIGO_VASCO_CREDITS_PATH TEAM_ASSETS_PATH "rodrigo-vasco.png"
#define ANDERSON_SANTOS_CREDITS_PATH TEAM_ASSETS_PATH "anderson-santos.png"
#define JOAO_VITOR_CREDITS_PATH TEAM_ASSETS_PATH "joao-vitor-medeiros.png"
#define LUAN_ARRUDA_CREDITS_PATH TEAM_ASSETS_PATH "luan-arruda.png"
#define NICOLAS_MAURICIO_CREDITS_PATH TEAM_ASSETS_PATH "nicolas-mauricio.png"
#define JULIO_LINS_CREDITS_PATH TEAM_ASSETS_PATH "julio-cesar-lins.png"

// PATHS PARA PERSONAGENS

// ESTAGIARIO (PLAYER PRINCIPAL)
#define ESTAGIARIO_1_EAST_PATH          PERSONAGENS_PATH "Estagiarios/east.png"
#define ESTAGIARIO_1_NORTHEAST_PATH     PERSONAGENS_PATH "Estagiarios/north-east.png"
#define ESTAGIARIO_1_NORTH_PATH         PERSONAGENS_PATH "Estagiarios/north.png"
#define ESTAGIARIO_1_NORTHWEST_PATH     PERSONAGENS_PATH "Estagiarios/north-west.png"
#define ESTAGIARIO_1_WEST_PATH          PERSONAGENS_PATH "Estagiarios/west.png"
#define ESTAGIARIO_1_SOUTHEAST_PATH     PERSONAGENS_PATH "Estagiarios/south-east.png"
#define ESTAGIARIO_1_SOUTH_PATH         PERSONAGENS_PATH "Estagiarios/south.png"
#define ESTAGIARIO_1_SOUTHWEST_PATH     PERSONAGENS_PATH "Estagiarios/south-west.png"

// RELACIONADOS A MOVIMENTO
#define DEFAULT_PLAYER_MOVEMENT_SPEED 3