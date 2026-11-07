#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "game_state.h"
#include "macros.h"
#include "pre_menu.h"
#include "menu.h"
#include "game_loop.h"
#include "credits.h"
#include "settings.h"
#include "preferences.h"
#include "state_machine.h"
#include "audio.h"

bool is_game_to_close = false;

void handle_state_initialization(void)
{
    initialize_pre_menu_textures();
}

int handle_menu_action(int screen)
{
    switch (screen)
    {
    case INICIAR:
        // A campanha já chegou ao final: a partida recomeça do Dia 1
        if (current_game_state_variables.resolved_puzzles[TOTAL_GAME_DAYS])
        {
            initialize_game_state_variables();
            initialize_hidden_statistic();
        }

        initialize_game_loop_variables();
        return PLAYING_SCREEN;
        break;

    case CREDITOS:
        initialize_credits_screen();
        return CREDITS_SCREEN;
        break;

    case CONFIGURACOES:
        initialize_settings_screen();
        return SETTINGS_SCREEN;
        break;

    case SAIR:
        // Só avisa a main.c: fechar a janela aqui derrubaria o jogo no unload_menu() logo depois
        is_game_to_close = true;
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

        if (is_game_loop_finished())
        {
            unload_game_loop();
            initialize_menu_variables();
            current_game_state_variables.current_screen = MENU_SCREEN;
        }
        break;

    case CREDITS_SCREEN:
        if (render_credits_screen())
        {
            unload_credits_screen();
            initialize_menu_variables();
            current_game_state_variables.current_screen = MENU_SCREEN;
        }
        break;

    case SETTINGS_SCREEN:
        if (render_settings_screen())
        {
            save_preferences(PREFERENCES_FILE_PATH);
            unload_settings_screen();
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

    update_audio_for_screen(current_game_state_variables.current_screen);
}

bool escape_closes_current_screen(void)
{
    int screen = current_game_state_variables.current_screen;

    return screen != CREDITS_SCREEN && screen != SETTINGS_SCREEN && !is_computer_screen_open();
}

void handle_state_shutdown(void)
{
    // A opção SAIR já liberou o menu na própria transição
    if (is_game_to_close) return;

    switch (current_game_state_variables.current_screen)
    {
    case PRE_MENU_SCREEN:
        unload_pre_menu_textures();
        break;

    case MENU_SCREEN:
        unload_menu();
        break;

    case PLAYING_SCREEN:
        unload_game_loop();
        break;

    case CREDITS_SCREEN:
        unload_credits_screen();
        break;

    case SETTINGS_SCREEN:
        unload_settings_screen();
        break;

    default:
        break;
    }
}
