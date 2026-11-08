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
#include "survival.h"
#include "preferences.h"
#include "mode_select.h"
#include "save_select.h"
#include "save_slots.h"
#include "save.h"
#include "loading.h"
#include "screen_style.h"
#include "state_machine.h"

bool is_game_to_close = false;

// Verdadeiro entre o momento em que o estado da campanha foi preparado a partir do save e o momento em que o jogador sai dela.
// Sem isso, fechar o jogo no meio do carregamento gravaria um estado velho dentro do save.
static bool campaign_state_ready = false;

void handle_state_initialization(void)
{
    initialize_pre_menu_textures();
}

// -----------------------------------------------------------------------------
// Carregamento do save escolhido
// -----------------------------------------------------------------------------

static float job_campaign_state(void)
{
    char path[512];

    // Começa de um estado novo; se o save já tem progresso, ele o substitui
    initialize_game_state_variables();
    initialize_hidden_statistic();

    if (active_save_path(path, sizeof(path)))
    {
        save_result status = load_game_state(path);

        if (status == SAVE_OK) TraceLog(LOG_INFO, "SAVE: \"%s\" retomado no dia %d", active_save_name(), current_game_state_variables.current_day);
        else if (status == SAVE_NOT_FOUND) TraceLog(LOG_INFO, "SAVE: \"%s\" ainda não tem progresso, começando do Dia 1", active_save_name());
        else TraceLog(LOG_WARNING, "SAVE: \"%s\" ignorado (%s), começando do Dia 1", active_save_name(), save_result_to_string(status));
    }

    // A campanha já chegou ao final: a partida recomeça do Dia 1
    if (current_game_state_variables.resolved_puzzles[TOTAL_GAME_DAYS])
    {
        initialize_game_state_variables();
        initialize_hidden_statistic();
    }

    campaign_state_ready = true;
    return 1.0f;
}

static float job_campaign_office(void)
{
    initialize_game_loop_variables();
    return 1.0f;
}

static int campaign_loading_jobs(loading_job *jobs)
{
    jobs[0] = (loading_job){"Lendo o save", 0.3f, job_campaign_state};
    jobs[1] = (loading_job){"Montando o escritório", 1.0f, job_campaign_office};
    return 2;
}

static int active_save_loading_jobs(loading_job *jobs)
{
    return active_save_mode() == SAVE_MODE_SURVIVAL ? survival_loading_jobs(jobs) : campaign_loading_jobs(jobs);
}

static int active_save_screen(void)
{
    return active_save_mode() == SAVE_MODE_SURVIVAL ? SURVIVAL_SCREEN : PLAYING_SCREEN;
}

int begin_loading_active_save(void)
{
    loading_job jobs[LOADING_MAX_JOBS];
    int count = active_save_loading_jobs(jobs);

    loading_begin("CARREGANDO", jobs, count, active_save_screen());
    return LOADING_SCREEN;
}

int start_active_save_now(void)
{
    loading_job jobs[LOADING_MAX_JOBS];
    int count = active_save_loading_jobs(jobs);

    loading_run_all(jobs, count);
    return active_save_screen();
}

bool save_active_campaign_progress(void)
{
    char path[512];
    save_result status;

    if (!campaign_state_ready || !active_save_is_set() || active_save_mode() != SAVE_MODE_NORMAL || !active_save_path(path, sizeof(path))) return false;

    status = save_game_state(path);
    if (status == SAVE_OK) TraceLog(LOG_INFO, "SAVE: progresso de \"%s\" gravado (dia %d)", active_save_name(), current_game_state_variables.current_day);
    else TraceLog(LOG_WARNING, "SAVE: não foi possível gravar \"%s\": %s", active_save_name(), save_result_to_string(status));
    return status == SAVE_OK;
}

// -----------------------------------------------------------------------------
// Menu e telas
// -----------------------------------------------------------------------------

int handle_menu_action(int screen)
{
    switch (screen)
    {
    case INICIAR:
        // O jogador escolhe o modo e depois o save; o jogo só começa quando o save carregar
        initialize_mode_select();
        return MODE_SELECT_SCREEN;

    case CREDITOS:
        initialize_credits_screen();
        return CREDITS_SCREEN;

    case CONFIGURACOES:
        initialize_settings_screen();
        return SETTINGS_SCREEN;

    case SAIR:
        // Só avisa a main.c: fechar a janela aqui derrubaria o jogo no unload_menu() logo depois
        is_game_to_close = true;
        return NONE;
    default:
        return NONE;
    }
}

void handle_state_machine(void)
{
    switch (current_game_state_variables.current_screen)
    {

    case PRE_MENU_SCREEN:
        // As imagens do menu carregam por trás da abertura, uma por quadro, com a porcentagem na tela
        if (!menu_preload_done()) pre_menu_set_loading_percent(menu_preload_step() * 100.0f);
        else pre_menu_set_loading_percent(100.0f);

        render_pre_menu();

        if (can_close_pre_menu_safely && menu_preload_done())
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

    case MODE_SELECT_SCREEN:
    {
        int status = render_mode_select();

        if (status == MODE_SELECT_BACK)
        {
            initialize_menu_variables();
            current_game_state_variables.current_screen = MENU_SCREEN;
        }
        else if (status == MODE_SELECT_CHOSEN)
        {
            initialize_save_select(mode_select_chosen_mode());
            current_game_state_variables.current_screen = SAVE_SELECT_SCREEN;
        }
        break;
    }

    case SAVE_SELECT_SCREEN:
    {
        int status = render_save_select();

        if (status == SAVE_SELECT_BACK)
        {
            initialize_mode_select();
            current_game_state_variables.current_screen = MODE_SELECT_SCREEN;
        }
        else if (status == SAVE_SELECT_CHOSEN)
        {
            current_game_state_variables.current_screen = begin_loading_active_save();
        }
        break;
    }

    case LOADING_SCREEN:
        render_loading();

        if (is_loading_finished())
        {
            current_game_state_variables.current_screen = loading_next_screen();
        }
        break;

    case PLAYING_SCREEN:
        render_game_loop();

        if (is_game_loop_finished())
        {
            // Quem sai da campanha leva o progresso para o save dela
            save_active_campaign_progress();
            campaign_state_ready = false;
            active_save_clear();

            unload_game_loop();
            initialize_menu_variables();
            current_game_state_variables.current_screen = MENU_SCREEN;
        }
        break;

    case SURVIVAL_SCREEN:
        if (render_survival())
        {
            unload_survival();
            active_save_clear();
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
}

bool escape_closes_current_screen(void)
{
    int screen = current_game_state_variables.current_screen;

    // Nestas telas o ESC volta um passo, pausa a partida ou é ignorado (no carregamento)
    return screen != CREDITS_SCREEN && screen != SETTINGS_SCREEN && screen != SURVIVAL_SCREEN && screen != MODE_SELECT_SCREEN && screen != SAVE_SELECT_SCREEN && screen != LOADING_SCREEN &&
           !is_computer_screen_open();
}

void handle_state_shutdown(void)
{
    // A opção SAIR já liberou a tela do menu na própria transição
    if (!is_game_to_close)
    {
        switch (current_game_state_variables.current_screen)
        {
        case PRE_MENU_SCREEN:
            unload_pre_menu_textures();
            break;

        case PLAYING_SCREEN:
            unload_game_loop();
            break;

        case SURVIVAL_SCREEN:
            unload_survival();
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

    // As imagens do menu e das telas de escolha ficam carregadas durante o jogo todo
    unload_menu_textures();
    style_unload();
}
