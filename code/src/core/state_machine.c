#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "game_state.h"
#include "macros.h"
#include "pre_menu.h"
#include "menu.h"
#include "game_loop.h"
#include "state_machine.h"

void handle_state_initialization(void)
{
    initialize_pre_menu_textures();
}

int handle_menu_action(int screen)
{
    switch (screen)
    {
    case INICIAR:
        initialize_game_loop_variables();
        return PLAYING_SCREEN;
        break;

    case CREDITOS:
        return CREDITS_SCREEN;
        break;

    case SAIR:
        CloseWindow();
        return NONE;
    default:
        return NONE;
        break;
    }
}

void handle_state_machine(void)
{
    switch (current_game_state_variables.current_screen)
    {

    case PRE_MENU_SCREEN:
        render_pre_menu();

        if (can_close_pre_menu_safely)
        {
            unload_pre_menu_textures();
            initialize_menu_variables();

            current_game_state_variables.current_screen = MENU_SCREEN;
        }
        break;

    case MENU_SCREEN:
        render_menu();

        if (is_menu_to_close)
        {

            current_game_state_variables.current_screen = handle_menu_action(next_screen);
            printf("---------------------------OI---------------------------\n");

            initialize_game_loop_variables();

            unload_menu();
        }
        break;

    case PLAYING_SCREEN:

        // SÓ REINICIALIZA O MENU. QUANDO APERTAMOS UM BOTÃO, ELE FAZ O UNLOAD DO MENU.
        // DAÍ É SO INICIALIZAR DENOVO E COLOCAR A "current_screen" (tela atual) PARA O PRÓPRIO MENU.
        // initialize_menu_variables();
        // printf("------------------------------------------------------\n");
        // printf("-------- GAME_LOOP AINDA NAO ESTA DISPONIVEL --------\n");
        // printf("------------------------------------------------------\n");
        // current_game_state_variables.current_screen = MENU_SCREEN;

        render_game_loop();
        break;

    case FLASHING_SCREEN:
        printf("Transição para a tela de dia de flash\n");
        break;

    case JUMPING_SCREEN:
        printf("Transição para a tela de dia de salto\n");
        break;

    case CREDITS_SCREEN:

        initialize_menu_variables();
        printf("------------------------------------------------------\n");
        printf("-------- CREDITOS AINDA NAO ESTA DISPONIVEL --------\n");
        printf("------------------------------------------------------\n");
        current_game_state_variables.current_screen = MENU_SCREEN;
        break;

    default:

        initialize_menu_variables();
        printf("------------------------------------------------------\n");
        printf("---- SCREEN NAO RECONHECIDA, VOLTANDO PARA O MENU ----\n");
        printf("------------------------------------------------------\n");
        current_game_state_variables.current_screen = MENU_SCREEN;
        break;
    }
}