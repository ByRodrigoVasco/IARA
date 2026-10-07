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

#define BG_COLOR GetColor(BG_COLOR_HEX)

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