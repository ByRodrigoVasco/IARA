/*
 * Testes de sistema do IARA - Bloco 3: fluxo de telas do jogo inteiro.
 *
 * Aqui o jogo e montado por completo (todos os modulos, menos a main.c) e
 * conduzido pela maquina de estados de verdade: abertura, menu, cartao do dia,
 * caminhada pelo escritorio e entrada no computador.
 *
 * Este bloco roda com a janela do jogo a vista: o que aparece na tela e a
 * partida de verdade, e serve de evidencia visual no screencast.
 *
 * Compilar e rodar: tools/tests/run_tests.ps1
 */

#include <stdio.h>
#include <stdbool.h>

#include "raylib.h"
#include "game_state.h"
#include "state_machine.h"
#include "pre_menu.h"
#include "menu.h"
#include "fonts.h"

/* A main.c define estas globais; aqui o teste assume esse papel. */
unsigned int screen_width = 0;
unsigned int screen_height = 0;
unsigned int middle_screen_x = 0;
unsigned int middle_screen_y = 0;

/* Inclui o .c para alcancar o estado interno do game loop e poder conduzir o
 * personagem pelo escritorio sem depender do teclado. */
#include "game_loop.c"
#include "percurso_do_jogador.h"

/* Definida na state_machine.c, mas sem declaracao no cabecalho. */
int handle_menu_action(int screen);

#define LARGURA_JANELA 1280
#define ALTURA_JANELA 720

#define LIMITE_DE_QUADROS 3000       /* Teto de seguranca para o pre-menu nao prender o teste */
#define SEGUNDOS_DE_MENU 2.5         /* Tempo com o menu a vista */
#define SEGUNDOS_ATE_O_ESCRITORIO 20 /* Teto de espera do cartao do dia (leva ~10s) */
#define SEGUNDOS_DE_PASSEIO 3.0      /* Caminhada livre pelo escritorio */
#define SEGUNDOS_DE_COMPUTADOR 4.0   /* Tempo com a tela do computador a vista */

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
/* Conducao do personagem                                                     */
/* ------------------------------------------------------------------------ */

/* Desenha um quadro do escritorio com o personagem andando.
 *
 * E a mesma sequencia do ramo IN_GAME_SCREEN do jogo, trocando apenas
 * handle_player_input() pelos deslocamentos recebidos: a Raylib nao permite
 * injetar teclas, entao o teste entra no lugar do teclado. Todo o resto
 * (colisao, limites do mapa, animacao, desenho) e o codigo do jogo. */
static void conduzir_quadro(int dx, int dy)
{
    estagiario_computer_collision_rectangle = get_computer_collision_rectangle();
    keep_player_place_on_resize();
    update_obstacle_recs();

    if (dx != 0 || dy != 0)
    {
        if      (dx == 0 && dy < 0)  current_direction = NORTH;
        else if (dx == 0 && dy > 0)  current_direction = SOUTH;
        else if (dx < 0  && dy == 0) current_direction = WEST;
        else if (dx > 0  && dy == 0) current_direction = EAST;
        else if (dx < 0  && dy < 0)  current_direction = NORTHWEST;
        else if (dx > 0  && dy < 0)  current_direction = NORTHEAST;
        else if (dx < 0  && dy > 0)  current_direction = SOUTHWEST;
        else                         current_direction = SOUTHEAST;

        current_estagiario_texture_direction = estagiario_texture[current_direction];
        walk_time += GetFrameTime();

        simular_passo(dx, dy);
    }
    else
    {
        walk_time = 0.0f;
        update_collision_rec();
    }

    handle_map_interactables();

    BeginDrawing();
    ClearBackground(BG_COLOR);

    render_game_loop_escritorio();
    render_player();
    render_control_tips();
    render_interact_arrow();

    EndDrawing();

    bool ignorado = false;
    fade_in_texture(FAST_FADE_SPEED, &color_to_fade, &ignorado);
}

/* Salva um quadro do escritorio em PNG, como evidencia de que a partida
 * realmente desenha. Usa textura de destino porque a Raylib acumula os desenhos
 * em lote: lendo a tela direto, o PNG sairia vazio. */
static void gravar_evidencia_do_jogo(const char *arquivo)
{
    RenderTexture2D alvo = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

    BeginTextureMode(alvo);
    ClearBackground(BG_COLOR);

    render_game_loop_escritorio();
    render_player();
    render_control_tips();
    render_interact_arrow();

    EndTextureMode();

    Image imagem = LoadImageFromTexture(alvo.texture);
    ImageFlipVertical(&imagem); /* textura de destino vem de cabeca para baixo */
    ExportImage(imagem, arquivo);

    UnloadImage(imagem);
    UnloadRenderTexture(alvo);
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

    checar("CT-26", "Estatisticas ocultas comecam no valor base",
           current_hidden_statistic.current_overall_points == 1 &&
           current_hidden_statistic.current_automation_points == 1 &&
           current_hidden_statistic.current_perception_points == 1 &&
           current_hidden_statistic.current_human_bond_points == 1);
}

static void testar_abertura(void)
{
    titulo("Abertura do jogo (CT-27)");

    printf("           rodando a tela de abertura na janela do jogo...\n");

    handle_state_initialization();
    current_game_state_variables.current_screen = PRE_MENU_SCREEN;

    int quadros = 0;

    while (current_game_state_variables.current_screen == PRE_MENU_SCREEN &&
           quadros < LIMITE_DE_QUADROS)
    {
        handle_state_machine();
        quadros++;
    }

    bool entregou_o_menu = (current_game_state_variables.current_screen == MENU_SCREEN);

    /* Menu a vista: o jogo tem que ficar esperando, sem avancar sozinho. */
    printf("           pre-menu concluido em %d quadros; mostrando o menu...\n", quadros);

    double inicio = GetTime();

    while (GetTime() - inicio < SEGUNDOS_DE_MENU && !WindowShouldClose())
    {
        handle_state_machine();
    }

    checar("CT-27", "Abertura entrega o menu, que aguarda a escolha do jogador",
           entregou_o_menu && quadros < LIMITE_DE_QUADROS &&
           current_game_state_variables.current_screen == MENU_SCREEN &&
           is_menu_to_close == false);
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

static void testar_entrada_na_partida(void)
{
    titulo("Cartao do dia e escritorio (CT-31)");

    printf("           o cartao \"Dia 1\" leva cerca de 10s para sair; aguardando...\n");

    current_game_state_variables.current_screen = PLAYING_SCREEN;

    double inicio = GetTime();

    while (current_screen != IN_GAME_SCREEN &&
           GetTime() - inicio < SEGUNDOS_ATE_O_ESCRITORIO &&
           !WindowShouldClose())
    {
        handle_state_machine();
    }

    bool chegou = (current_screen == IN_GAME_SCREEN);

    if (chegou)
    {
        printf("           escritorio na tela depois de %.1fs\n", GetTime() - inicio);
    }

    checar("CT-31", "Cartao do dia termina e o escritorio aparece", chegou);

    /* Deixa o escritorio aparecer por completo antes de andar. */
    for (int quadro = 0; quadro < 30 && !WindowShouldClose(); quadro++)
    {
        handle_state_machine();
    }
}

static void testar_caminhada(void)
{
    titulo("Caminhada pelo escritorio (CT-32)");

    printf("           o personagem anda pelo escritorio e vai ate o computador...\n");

    /* Passeio livre: mostra o andar, a colisao e o deslize nos moveis. */
    const int rota[8][2] = {{1, 0}, {1, -1}, {0, -1}, {-1, -1},
                            {-1, 0}, {-1, 1}, {0, 1}, {1, 1}};
    double inicio = GetTime();
    int trecho = 0;

    while (GetTime() - inicio < SEGUNDOS_DE_PASSEIO && !WindowShouldClose())
    {
        trecho = (int)((GetTime() - inicio) / (SEGUNDOS_DE_PASSEIO / 8.0));

        if (trecho > 7) trecho = 7;

        conduzir_quadro(rota[trecho][0], rota[trecho][1]);
    }

    /* Agora calcula o caminho ate o computador a partir de onde o personagem esta. */
    Vector2 daqui = current_estagiario_position;
    int passos = 0;

    if (!calcular_caminho_ate_o_computador(daqui, &passos))
    {
        checar("CT-32", "Personagem atravessa o escritorio ate o computador", false);
        return;
    }

    printf("           caminho de %d passos ate o computador\n", passos);

    /* O caminho vem do computador para tras, entao percorre do fim para o comeco. */
    for (int k = tamanho_caminho - 2; k >= 0 && !WindowShouldClose(); k--)
    {
        Vector2 atual = posicao_do_caminho(daqui, k + 1);
        Vector2 proxima = posicao_do_caminho(daqui, k);

        int dx = (proxima.x > atual.x) - (proxima.x < atual.x);
        int dy = (proxima.y > atual.y) - (proxima.y < atual.y);

        conduzir_quadro(dx, dy);
    }

    /* Parado em frente ao computador, com a seta piscando por cima. */
    for (int quadro = 0; quadro < 45 && !WindowShouldClose(); quadro++)
    {
        conduzir_quadro(0, 0);
    }

    gravar_evidencia_do_jogo("docs/fds/testes/evidencias/escritorio-em-jogo.png");
    printf("           evidencia em docs/fds/testes/evidencias/escritorio-em-jogo.png\n");

    checar("CT-32", "Personagem atravessa o escritorio ate o computador", is_near_computer);
}

static void testar_computador(void)
{
    titulo("Tela do computador (CT-33)");

    printf("           apertando ESPACO no computador...\n");

    /* Mesmo efeito de apertar ESPACO em cima do computador. */
    enter_computer_screen = true;

    double inicio = GetTime();

    while (GetTime() - inicio < SEGUNDOS_DE_COMPUTADOR && !WindowShouldClose())
    {
        handle_state_machine();
    }

    checar("CT-33", "Interagir com o computador abre a tela do quiz",
           current_screen == COMPUTER_SCREEN);
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
    printf("\n  A janela do jogo mostra a partida de verdade durante este bloco.\n");

    /* Mesma ordem de preparo da main.c. */
    initialize_fonts();
    initialize_menu_variables();

    testar_estado_inicial();
    testar_abertura();
    testar_opcoes_do_menu();
    testar_entrada_na_partida();
    testar_caminhada();
    testar_computador();

    printf("\n------------------------------------------------------------------------\n");
    printf(" Bloco 3: %d de %d casos aprovados", casos_ok, total_casos);

    if (casos_ok != total_casos) printf(" | %d FALHA(S)", total_casos - casos_ok);

    printf("\n========================================================================\n\n");

    unload_fonts();
    CloseWindow();

    return (casos_ok == total_casos) ? 0 : 1;
}
