#include "state_machine.h"
#include "game_state.h"
#include "pre_menu.h"
#include "menu.h"
#include "fonts.h"
#include "macros.h"
#include "save.h"

unsigned int screen_width = 0;
unsigned int screen_height = 0;
unsigned int middle_screen_x = 0;
unsigned int middle_screen_y = 0;

int main(void)
{
    /* InitWindow(0, 0) faz a Raylib abrir em tela cheia, então iniciamos com um
     *  tamanho fixo (janela) e depois ajustamos ao tamanho do monitor.
     */
    InitWindow(800, 450, IARA_TITLE);

    int monitor_id = GetCurrentMonitor();

    int monitor_width = GetMonitorWidth(monitor_id);
    int monitor_height = GetMonitorHeight(monitor_id);

    /* Modo janela: usa um tamanho menor que o monitor (para o gravador de tela capturar). */
    screen_width = monitor_width * 3 / 4;
    screen_height = monitor_height * 3 / 4;

    SetWindowSize(screen_width, screen_height);

    middle_screen_x = screen_width / 2;
    middle_screen_y = screen_height / 2;


    SetWindowSize(screen_width, screen_height);
    SetWindowPosition((monitor_width - (int)screen_width) / 2, (monitor_height - (int)screen_height) / 2);

    SetTargetFPS(60);

    // A ORDEM IMPORTA. NÃO MUDE.
    initialize_fonts();
    
    initialize_menu_variables();
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
     * por ALT+F4 (o sistema manda o mesmo pedido de fechar) ou pela tecla ESC.
     * A opção SAIR do menu também encerra este loop. */
    while (!WindowShouldClose())
    {
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

    unload_fonts();
    CloseWindow();
    return 0;
}
