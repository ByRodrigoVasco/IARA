#include <stdio.h>

#include "state_machine.h"
#include "game_state.h"
#include "pre_menu.h"
#include "menu.h"
#include "fonts.h"
#include "macros.h"

unsigned int screen_width = 0;
unsigned int screen_height = 0;
unsigned int middle_screen_x = 0;
unsigned int middle_screen_y = 0;

int monitor_id = 0;

const char *title = "IARA";

int main(void)
{
    /* Infelizmente Raylib não tem um método para pegar a "screen width e height"
     *  antes de inicializar a janela. A solução é colocar o InitWindow com 0 para
     *  width e height por que o próprio raylib (por baixo dos panos) pegará o tamanho
     *  nativo da tela. Aí logo depois nós extraimos o tamanho horizontal e vertical da tela.
     */
    InitWindow(0, 0, IARA_TITLE);

    monitor_id = GetCurrentMonitor();

    screen_width = GetMonitorWidth(monitor_id);
    screen_height = GetMonitorHeight(monitor_id);

    middle_screen_x = screen_width / 2;
    middle_screen_y = screen_height / 2;

    printf("screen_width = %d\n", screen_width);
    printf("screen_height = %d\n", screen_height);
    printf("middle_screen_x = %d\n", middle_screen_x);
    printf("middle_screen_Y = %d\n", middle_screen_y);

    ToggleBorderlessWindowed();
    
    SetTargetFPS(60);

    // A ORDEM IMPORTA. NÃO MUDE.
    initialize_fonts();
    
    initialize_menu_variables();
    initialize_game_state_variables();
    initialize_hidden_statistic();


    handle_state_initialization();

    current_game_state_variables.current_screen = PRE_MENU_SCREEN;

    while (!WindowShouldClose())
    {
        handle_state_machine();
    }

    unload_fonts();
    CloseWindow();
    return 0;
}