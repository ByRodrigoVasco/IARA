#include "raylib.h"

/*
AQUI ESTARÃO OS MACROS GLOBAIS QUE SERÃO USADOS POR DIVERSOS ARQUIVOS.
NEM TODOS OS MACROS ESTARÃO AQUI.
*/

// CONFIGURAÇÃO DE JANELA
#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 576
#define IARA_TITLE "IARA"
#define MIDDLE_SCREEN_X (WINDOW_WIDTH / 2)
#define MIDDLE_SCREEN_Y (WINDOW_HEIGHT / 2)

// VARIÁVEIS DE ANIMAÇÕES
#define DEFAULT_FADE_SPEED 1
#define MEDIUM_FADE_SPEED 5
#define FAST_FADE_SPEED 10

// -----------------------------------------------------------------------------
// CORES
// -----------------------------------------------------------------------------

// IMPORTANTE: MELHOR TER CUIDADO COM HEX COLOR! NO FIGMA, USAMOS O "SHORT HEX COLOR" QUE SÓ TEM ---6 DÍGITOS---
// AQUI NO RAYLIB PRECISAMOS DE ---8 DÍGITOS--- NO FORMATO "0xRRGGBBAA"
#define BG_COLOR_HEX 0x140B15FF
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
#define IARA_NOME_PNG_PATH "code/assets/sprites/tela-inicial/iara-nome.png"
#define ESCRITORIO_TELA_PNG_PATH "code/assets/sprites/tela-inicial/escritorio-tela-inicial.png"