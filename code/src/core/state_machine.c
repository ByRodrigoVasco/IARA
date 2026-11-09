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
#include "help.h"
#include "notice.h"
#include "save.h"

bool is_game_to_close = false;

// Salva ao voltar ao menu e diz o que aconteceu: se der certo, o jogador vê; se falhar, sabe o que conferir.
static void save_progress_with_notice(void)
{
    save_result status = save_game_state(SAVE_FILE_PATH);

    if (status == SAVE_OK)
    {
        notice_show("Progresso salvo.", NOTICE_SUCCESS);
    }
    else
    {
        TraceLog(LOG_WARNING, "SAVE: nao foi possivel salvar o progresso: %s", save_result_to_string(status));
        notice_show("Não consegui salvar o progresso. Confira se há espaço livre no disco e se a pasta do jogo permite gravar arquivos.", NOTICE_ERROR);
    }
}

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

    case AJUDA:
        initialize_help_screen();
        return HELP_SCREEN;

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

        if (is_game_loop_quit_requested())
        {
            // SAIR DO JOGO na pausa: a main.c salva o progresso e fecha a janela
            unload_game_loop();
            is_game_to_close = true;
            break;
        }

        if (is_game_loop_finished())
        {
            save_progress_with_notice();
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

    case HELP_SCREEN:
        if (render_help_screen())
        {
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

    // No menu o ESC abre a pergunta de sair, no escritório abre a pausa e na abertura é ignorado:
    // uma tecla só nunca fecha o jogo de uma vez.
    return screen != CREDITS_SCREEN && screen != SETTINGS_SCREEN && screen != HELP_SCREEN && screen != MENU_SCREEN &&
           screen != PLAYING_SCREEN && screen != PRE_MENU_SCREEN && !is_computer_screen_open();
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
