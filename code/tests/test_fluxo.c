/*
 * Testes de sistema do IARA - Bloco 3: fluxo de telas do jogo inteiro.
 *
 * Aqui o jogo e montado por completo (todos os modulos, menos a main.c) e
 * conduzido pela maquina de estados de verdade: pre-menu, menu, escolha da
 * opcao e entrada na partida.
 *
 * Compilar e rodar: tools/tests/run_tests.ps1
 */

#include <stdio.h>
#include <stdbool.h>

#include "raylib.h"
#include "macros.h"
#include "game_state.h"
#include "state_machine.h"
#include "pre_menu.h"
#include "menu.h"
#include "game_loop.h"
#include "fonts.h"

/* A main.c define estas globais; aqui o teste assume esse papel. */
unsigned int screen_width = 0;
unsigned int screen_height = 0;
unsigned int middle_screen_x = 0;
unsigned int middle_screen_y = 0;

/* Definida na state_machine.c, mas sem declaracao no cabecalho. */
int handle_menu_action(int screen);

#define LARGURA_JANELA 1280
#define ALTURA_JANELA 720

#define LIMITE_DE_QUADROS 3000     /* Teto de seguranca para o pre-menu nao prender o teste */
#define SEGUNDOS_DE_PARTIDA 7.0    /* Tempo rodando a partida de verdade na tela */

static int total_casos = 0;
static int casos_ok = 0;

static void checar(const char *id, const char *descricao, bool condicao)
{
    total_casos++;

    if (condicao)
    {
        casos_ok++;
        printf("  [ OK  ]  %-6s  %s\n", id, descricao);
    }
    else
    {
        printf("  [FALHA]  %-6s  %s\n", id, descricao);
    }
}

static void titulo(const char *texto)
{
    printf("\n  --- %s ---\n\n", texto);
}

/* ------------------------------------------------------------------------ */
/* Casos de teste                                                             */
/* ------------------------------------------------------------------------ */

static void testar_estado_inicial(void)
{
    titulo("Estado inicial da partida (CT-25 e CT-26)");

    initialize_game_state_variables();
    initialize_hidden_statistic();

    checar("CT-25", "Partida comeca no dia 1, em um dia jogavel",
           current_game_state_variables.current_day == 1 &&
           current_game_state_variables.day_type == PLAYABLE_DAY &&
           current_game_state_variables.is_resolved == false &&
           current_game_state_variables.user_investigated == false);

    checar("CT-26", "Estatisticas ocultas comecam zeradas no valor base",
           current_hidden_statistic.current_overall_points == 1 &&
           current_hidden_statistic.current_automation_points == 1 &&
           current_hidden_statistic.current_perception_points == 1 &&
           current_hidden_statistic.current_human_bond_points == 1);
}

static void testar_pre_menu(void)
{
    titulo("Abertura do jogo (CT-27)");

    handle_state_initialization();
    current_game_state_variables.current_screen = PRE_MENU_SCREEN;

    int quadros = 0;

    while (current_game_state_variables.current_screen == PRE_MENU_SCREEN &&
           quadros < LIMITE_DE_QUADROS)
    {
        handle_state_machine();
        quadros++;
    }

    printf("           pre-menu concluido em %d quadros\n", quadros);
    checar("CT-27", "Tela de abertura termina sozinha e entrega o menu",
           current_game_state_variables.current_screen == MENU_SCREEN &&
           quadros < LIMITE_DE_QUADROS);
}

static void testar_opcoes_do_menu(void)
{
    titulo("Escolhas do menu (CT-28 a CT-30)");

    checar("CT-28", "Escolher INICIAR leva para a tela de jogo",
           handle_menu_action(INICIAR) == PLAYING_SCREEN);

    checar("CT-29", "Escolher CREDITOS leva para a tela de creditos",
           handle_menu_action(CREDITOS) == CREDITS_SCREEN);

    /* Creditos ainda nao existem: a maquina de estados devolve o jogador ao menu. */
    current_game_state_variables.current_screen = CREDITS_SCREEN;
    handle_state_machine();
    bool creditos_voltam = (current_game_state_variables.current_screen == MENU_SCREEN);

    /* Tela desconhecida nao pode travar o jogo. */
    current_game_state_variables.current_screen = 99;
    handle_state_machine();
    bool desconhecida_volta = (current_game_state_variables.current_screen == MENU_SCREEN);

    checar("CT-30", "Tela sem implementacao devolve o jogador ao menu",
           creditos_voltam && desconhecida_volta);
}

static void testar_partida(void)
{
    titulo("Partida rodando (CT-31)");

    initialize_game_loop_variables();
    current_game_state_variables.current_screen = PLAYING_SCREEN;

    double inicio = GetTime();
    int quadros = 0;

    /* Roda a partida de verdade: cartao do dia, fade e entrada no escritorio. */
    while (GetTime() - inicio < SEGUNDOS_DE_PARTIDA && !WindowShouldClose())
    {
        handle_state_machine();
        quadros++;
    }

    printf("           %d quadros desenhados em %.1f segundos de partida\n",
           quadros, GetTime() - inicio);
    checar("CT-31", "Partida roda do cartao do dia ate o escritorio sem travar",
           quadros > 0 && IsWindowReady());
}

int main(void)
{
    SetTraceLogLevel(LOG_WARNING);

    InitWindow(LARGURA_JANELA, ALTURA_JANELA, "IARA - testes de fluxo");

    screen_width = GetScreenWidth();
    screen_height = GetScreenHeight();
    middle_screen_x = screen_width / 2;
    middle_screen_y = screen_height / 2;

    SetTargetFPS(60);

    printf("\n========================================================================\n");
    printf(" IARA - TESTES DE SISTEMA | Bloco 3: fluxo de telas do jogo completo\n");
    printf("========================================================================\n");

    /* Mesma ordem de preparo da main.c. */
    initialize_fonts();
    initialize_menu_variables();

    testar_estado_inicial();
    testar_pre_menu();
    testar_opcoes_do_menu();
    testar_partida();

    printf("\n------------------------------------------------------------------------\n");
    printf(" Bloco 3: %d de %d casos aprovados", casos_ok, total_casos);

    if (casos_ok != total_casos) printf(" | %d FALHA(S)", total_casos - casos_ok);

    printf("\n========================================================================\n\n");

    unload_fonts();
    CloseWindow();

    return (casos_ok == total_casos) ? 0 : 1;
}
