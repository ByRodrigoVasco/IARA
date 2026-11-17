#include <stdio.h>

#include "state_machine.h"
#include "game_state.h"
#include "audio.h"
#include "pre_menu.h"
#include "menu.h"
#include "fonts.h"
#include "macros.h"
#include "save.h"
#include "save_slots.h"
#include "helper.h"
#include "accessibility.h"
#include "preferences.h"

unsigned int screen_width = 0;
unsigned int screen_height = 0;
unsigned int middle_screen_x = 0;
unsigned int middle_screen_y = 0;

// Quadros que passam de 60 ms ficam registrados em iara-desempenho.log (cada execução recomeça o arquivo).
// Assim, quando o jogo trava na máquina de alguém, o arquivo diz em que tela e quando.
#define FRAME_SPIKE_LOG_PATH "iara-desempenho.log"
#define FRAME_SPIKE_MS 60.0
#define FRAME_SPIKE_MAX_LINES 200

static void log_frame_spike(double frame_ms, int screen)
{
    static int lines = 0;
    FILE *file;

    if (frame_ms < FRAME_SPIKE_MS || lines >= FRAME_SPIKE_MAX_LINES) return;

    TraceLog(LOG_WARNING, "DESEMPENHO: quadro de %.0f ms na tela %d", frame_ms, screen);
    file = fopen(FRAME_SPIKE_LOG_PATH, lines == 0 ? "w" : "a");
    if (file == NULL) return;
    fprintf(file, "%.1f s  tela %d  quadro de %.0f ms\n", GetTime(), screen, frame_ms);
    fclose(file);
    lines++;
}

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

    /* Saves em pastas: saves/normal/<nome> e saves/sobrevivencia/<nome>. O jogador escolhe
     * o save depois de INICIAR (modo e save), então aqui só se garante que as pastas existem.
     * Os saves de antes (iara.save e iara.survival, soltos ao lado do executável) viram o
     * save "Principal" de cada modo, para ninguém perder o progresso. */
    save_slots_ensure_folders();
    int migrated_files = save_slots_migrate_legacy();
    if (migrated_files > 0)
    {
        TraceLog(LOG_INFO, "SAVE: %d arquivo(s) de save antigo levado(s) para o save \"Principal\"", migrated_files);
    }

    handle_state_initialization();

    current_game_state_variables.current_screen = PRE_MENU_SCREEN;

    /* WindowShouldClose() fica true quando o jogador fecha a janela pelo X,
     * ou por ALT+F4 (o sistema manda o mesmo pedido de fechar). A opção SAIR do
     * menu e a tecla ESC, fora dos créditos e das configurações, também encerram. */
    double last_frame_time = GetTime();

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

        /* Registra os quadros que travaram (mais de 60 ms), para dar para descobrir onde e quando. */
        double now = GetTime();
        log_frame_spike((now - last_frame_time) * 1000.0, current_game_state_variables.current_screen);
        last_frame_time = now;
    }

    /* Save automático (US01): grava o progresso ANTES de liberar qualquer
     * recurso, para nenhum progresso recente se perder ao fechar. Só a campanha
     * precisa disto aqui: o Modo Sobrevivência grava o dele a cada compra e no fim
     * de cada partida, e quem fecha o jogo no meio de uma leva o que juntou. */
    save_active_campaign_progress();

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
