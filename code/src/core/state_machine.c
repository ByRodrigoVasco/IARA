#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "game_state.h"
#include "macros.h"
#include "pre_menu.h"
#include "menu.h"
#include "game_loop.h"
#include "credits.h"
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
        initialize_credits_screen();
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

            unload_menu();
        }
        break;

    case PLAYING_SCREEN:
        render_game_loop();
        break;

    case FLASHING_SCREEN:
        // TODO: tela de dia de flash ainda não implementada
        break;

    case JUMPING_SCREEN:
        // TODO: tela de dia de salto ainda não implementada
        break;

    case CREDITS_SCREEN:
        if (render_credits_screen())
        {
            unload_credits_screen();
            initialize_menu_variables();
            current_game_state_variables.current_screen = MENU_SCREEN;
        }
        break;

    default:
        // Tela desconhecida: volta para o menu por segurança.
        initialize_menu_variables();
        current_game_state_variables.current_screen = MENU_SCREEN;
        break;
    }
}