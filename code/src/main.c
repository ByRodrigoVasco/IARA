#include "state_machine.h"
#include "game_state.h"
#include "audio.h"
#include "pre_menu.h"
#include "menu.h"
#include "fonts.h"
#include "macros.h"
#include "save.h"
#include "helper.h"
#include "accessibility.h"
#include "preferences.h"

unsigned int screen_width = 0;
unsigned int screen_height = 0;
unsigned int middle_screen_x = 0;
unsigned int middle_screen_y = 0;

int main(void)
{
    /* Assets e save são relativos à pasta do executável (US02): assim o jogo
     * abre certo por atalho ou por outro terminal, em qualquer pasta. */
    ChangeDirectory(GetApplicationDirectory());

    /* InitWindow(0, 0) faz a Raylib abrir em tela cheia, então iniciamos com um
     *  tamanho fixo (janela) e depois ajustamos ao tamanho do monitor.
     */
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 450, IARA_TITLE);
    SetWindowMinSize(WINDOW_MIN_WIDTH, WINDOW_MIN_HEIGHT);

    if (!initialize_audio())
    {
        CloseWindow();
        return 1;
    }

    int monitor_id = GetCurrentMonitor();

    int monitor_width = GetMonitorWidth(monitor_id);
    int monitor_height = GetMonitorHeight(monitor_id);
    Vector2 monitor_position = GetMonitorPosition(monitor_id);

    /* Modo janela: um pouco menor que o monitor. F11 alterna com a tela cheia. */
    int window_width = monitor_width * WINDOW_MONITOR_RATIO;
    int window_height = monitor_height * WINDOW_MONITOR_RATIO;

    SetWindowSize(window_width, window_height);
    SetWindowPosition((int)monitor_position.x + ((monitor_width - window_width) / 2),
                      (int)monitor_position.y + ((monitor_height - window_height) / 2));

    update_screen_metrics();

    SetTargetFPS(60);

    // ESC é tratado no loop principal: nos créditos e nas configurações ele volta ao menu
    SetExitKey(KEY_NULL);

    // A ORDEM IMPORTA. NÃO MUDE.
    initialize_fonts();
    load_preferences(PREFERENCES_FILE_PATH);

    initialize_game_state_variables();
    initialize_hidden_statistic();

    /* Retomada da partida (US01): se houver um save válido, ele substitui o
     * estado inicial do Dia 1. Se não houver save, ou se ele estiver
     * corrompido, o estado acima continua valendo e o jogo começa do zero. */
    save_result load_status = load_game_state(SAVE_FILE_PATH);
    if (load_status == SAVE_OK)
    {
        TraceLog(LOG_INFO, "SAVE: partida retomada no dia %d", current_game_state_variables.current_day);
    }
    else if (load_status == SAVE_NOT_FOUND)
    {
        TraceLog(LOG_INFO, "SAVE: nenhum save encontrado, iniciando nova partida");
    }
    else
    {
        TraceLog(LOG_WARNING, "SAVE: save ignorado (%s), iniciando nova partida", save_result_to_string(load_status));
    }

    handle_state_initialization();

    current_game_state_variables.current_screen = PRE_MENU_SCREEN;

    /* WindowShouldClose() fica true quando o jogador fecha a janela pelo X,
     * ou por ALT+F4 (o sistema manda o mesmo pedido de fechar). A opção SAIR do
     * menu e a tecla ESC, fora dos créditos e das configurações, também encerram. */
    while (!WindowShouldClose() && !is_game_to_close)
    {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) play_click_sound();

        if (IsKeyPressed(KEY_ESCAPE) && escape_closes_current_screen())
        {
            break;
        }

        /* A janela pode mudar de tamanho a qualquer momento (borda, maximizar ou F11). */
        if (IsKeyPressed(KEY_F11))
        {
            toggle_fullscreen();
        }

        update_screen_metrics();

        handle_state_machine();
    }

    /* Save automático (US01): grava o progresso ANTES de liberar qualquer
     * recurso, para nenhum progresso recente se perder ao fechar. */
    save_result save_status = save_game_state(SAVE_FILE_PATH);
    if (save_status == SAVE_OK)
    {
        TraceLog(LOG_INFO, "SAVE: progresso salvo em %s (dia %d)", SAVE_FILE_PATH, current_game_state_variables.current_day);
    }
    else
    {
        TraceLog(LOG_WARNING, "SAVE: nao foi possivel salvar o progresso: %s", save_result_to_string(save_status));
    }

    if (!save_preferences(PREFERENCES_FILE_PATH))
    {
        TraceLog(LOG_WARNING, "PREFERENCIAS: nao foi possivel salvar em %s", PREFERENCES_FILE_PATH);
    }

    handle_state_shutdown();
    unload_accessibility();
    unload_fonts();
    unload_audio();
    CloseWindow();
    return 0;
}
