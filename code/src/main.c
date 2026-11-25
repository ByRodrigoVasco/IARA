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
#include "assets_fs.h"
#include "data_dir.h"
#include "cloud_sync.h"

unsigned int screen_width = 0;
unsigned int screen_height = 0;
unsigned int middle_screen_x = 0;
unsigned int middle_screen_y = 0;

// Quadros que passam de 60 ms ficam registrados em iara-desempenho.log (cada execução recomeça o arquivo).
// Assim, quando o jogo trava na máquina de alguém, o arquivo diz em que tela e quando.
#define FRAME_SPIKE_LOG_PATH "iara-desempenho.log"
#define FRAME_SPIKE_MS 60.0
#define FRAME_SPIKE_MAX_LINES 200

// Além do total, cada linha diz quanto ficou dentro de handle_state_machine() (lógica, desenho e
// EndDrawing) e quanto ficou fora dele, e se a janela estava sem foco ou foi mexida no quadro. Isso
// separa uma carga do jogo de um evento do Windows (BUG-17, ainda não reproduzido).
static void log_frame_spike(double frame_ms, double state_machine_ms, int screen, bool focused, bool window_changed)
{
    static int lines = 0;
    FILE *file;

    if (frame_ms < FRAME_SPIKE_MS || lines >= FRAME_SPIKE_MAX_LINES) return;

    TraceLog(LOG_WARNING, "DESEMPENHO: quadro de %.0f ms na tela %d (%.0f ms na maquina de estados)", frame_ms, screen, state_machine_ms);
    file = fopen(FRAME_SPIKE_LOG_PATH, lines == 0 ? "w" : "a");
    if (file == NULL) return;
    fprintf(file, "%.1f s  tela %d  quadro de %.0f ms  (maquina de estados %.0f ms, fora dela %.0f ms)  foco %s  janela mexida %s\n",
            GetTime(), screen, frame_ms, state_machine_ms, frame_ms - state_machine_ms,
            focused ? "sim" : "nao", window_changed ? "sim" : "nao");
    fclose(file);
    lines++;
}

int main(void)
{
    /* Assets e save são relativos à pasta do executável (US02): assim o jogo
     * abre certo por atalho ou por outro terminal, em qualquer pasta. */
    ChangeDirectory(GetApplicationDirectory());

    /* No build de release os assets vêm de dentro do .exe (assets_fs.h); nos outros, do disco. */
    assets_fs_init();

    /* No executável de release os dados do jogador (saves, iara.ini, rankings e o log de desempenho)
     * ficam em Documentos\IARA, e não ao lado do .exe: assim ele pode estar em qualquer pasta (Área
     * de Trabalho, Downloads, pendrive) sem espalhar arquivos. Todos esses caminhos são relativos,
     * então basta trabalhar dentro dessa pasta. Na primeira vez, o que já existia ao lado do .exe é
     * copiado para lá. Se Documentos não estiver disponível, fica tudo ao lado do .exe, como antes.
     * O build de desenvolvimento continua na pasta do repositório, onde estão os assets. */
    if (assets_fs_embedded())
    {
        char data_dir[512];

        if (data_dir_prepare(data_dir, sizeof(data_dir)))
        {
            int copied = data_dir_migrate_from_exe_folder(data_dir);

            if (ChangeDirectory(data_dir))
            {
                if (copied > 0) TraceLog(LOG_INFO, "DADOS: %d arquivo(s) de antes copiados para %s", copied, data_dir);
            }
            else TraceLog(LOG_WARNING, "DADOS: nao deu para usar %s; os dados ficam ao lado do executavel", data_dir);
        }
    }

    /* Saves e rankings compartilhados entre notebooks (cloud.h). Só o build com a URL e a chave liga
     * a nuvem; a fila offline fica na pasta de dados, por isso vem depois da troca de pasta. */
    cloud_sync_start();

    /* InitWindow(0, 0) faz a Raylib abrir em tela cheia, então iniciamos com um
     *  tamanho fixo (janela) e depois ajustamos ao tamanho do monitor.
     */
    /* VSync: o quadro espera o monitor em vez de o processador ficar girando até dar 1/60 s.
     * Gasta bem menos CPU e deixa o movimento mais liso. O SetTargetFPS(60) abaixo continua
     * valendo como teto em monitores acima de 60 Hz e onde o driver ignora o VSync. */
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(800, 450, IARA_TITLE);
    SetWindowMinSize(WINDOW_MIN_WIDTH, WINDOW_MIN_HEIGHT);
    {
        /* Ícone da janela e da barra de tarefas. O do iara.exe no Explorer vem do tools/release/iara.rc. */
        Image icon = LoadImage(IARA_ICON_PNG_PATH);

        if (icon.data != NULL)
        {
            SetWindowIcon(icon);
            UnloadImage(icon);
        }
    }

    if (!initialize_audio())
    {
        cloud_sync_stop();
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

    /* A janela abre no tamanho fixo (sem bordas para arrastar). Maximizar ou tela cheia
     * ficam nas configurações e no F11. */
    mark_window_fixed_at_startup();

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

        /* Registra os quadros que travaram (mais de 60 ms), para dar para descobrir onde e quando. */
        int screen_in_frame = current_game_state_variables.current_screen;
        bool window_changed = IsWindowResized() || IsWindowMinimized();
        double state_machine_start = GetTime();

        handle_state_machine();
        cloud_sync_update();

        double now = GetTime();
        log_frame_spike((now - last_frame_time) * 1000.0, (now - state_machine_start) * 1000.0, screen_in_frame,
                        IsWindowFocused(), window_changed || IsWindowResized());
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

    /* Depois de fechar a janela: envia o que falta e libera o save, sem a tela parada esperando a rede. */
    cloud_sync_stop();
    return 0;
}
