/*
 * Testes de sistema do IARA - Bloco 3: fluxo de telas do jogo inteiro.
 *
 * Monta o jogo completo (todos os modulos, menos a main.c) e o conduz do inicio
 * ao fim, na ordem em que um jogador percorreria:
 *
 *   abertura -> menu -> CREDITOS (e a volta ao menu) -> INICIAR -> cartao do dia
 *   -> escritorio -> computador -> as 4 perguntas do quiz
 *
 * A janela do jogo fica a vista durante todo o bloco: o que aparece na tela e a
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
#include "fonts.h"

/* A main.c define estas globais; aqui o teste assume esse papel. */
unsigned int screen_width = 0;
unsigned int screen_height = 0;
unsigned int middle_screen_x = 0;
unsigned int middle_screen_y = 0;

/* Os .c entram inteiros (e nao so os .h) porque o teste precisa alcancar o
 * estado interno de cada tela: conduzir o personagem, mover a selecao do menu e
 * avancar o roteiro do quiz sem depender de teclado e mouse.
 *
 * computer_loop.c e game_loop.c tem, cada um, um estatico chamado
 * color_to_fade. Juntos no mesmo arquivo dariam erro de redefinicao, entao o do
 * computador e renomeado na hora de incluir. */
#define color_to_fade color_to_fade_do_computador
#include "computer_loop.c"
#undef color_to_fade

#include "menu.c"
#include "game_loop.c"
#include "percurso_do_jogador.h"

/* A raylib nao permite injetar teclas. A tela de creditos so volta ao menu
 * quando o jogador aperta uma tecla, entao, so enquanto credits.c e incluido,
 * a consulta de teclas passa por este gancho: o teste liga
 * creditos_tecla_de_voltar para simular o jogador apertando ESC. O resto da
 * tela (desenho, botao, retorno da funcao) e o codigo real. */
static bool creditos_tecla_de_voltar = false;

static bool tecla_dos_creditos(int tecla)
{
    return creditos_tecla_de_voltar && tecla == KEY_ESCAPE;
}

#define IsKeyPressed(tecla) tecla_dos_creditos(tecla)
#include "credits.c"
#undef IsKeyPressed

#define LARGURA_JANELA 1280
#define ALTURA_JANELA 720

#define LIMITE_DE_QUADROS 3000        /* Teto de seguranca para o pre-menu nao prender o teste */
#define SEGUNDOS_DE_MENU 2.0          /* Tempo com o menu a vista */
#define SEGUNDOS_POR_OPCAO 0.45       /* Tempo em cada opcao ao navegar o menu */
#define SEGUNDOS_ATE_O_ESCRITORIO 20  /* Teto de espera do cartao do dia (leva ~10s) */
#define SEGUNDOS_DE_PASSEIO 2.5       /* Caminhada livre pelo escritorio */
#define SEGUNDOS_DE_TEXTO 0.9         /* Tempo lendo cada tela do computador */
#define SEGUNDOS_DE_RESPOSTA 1.0      /* Tempo com a resposta marcada na tela */

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

static void etapa(const char *texto)
{
    printf("           %s\n", texto);
}

/* ------------------------------------------------------------------------ */
/* Conducao do jogo                                                           */
/* ------------------------------------------------------------------------ */

/* Deixa o jogo rodando pelo tempo pedido, pela maquina de estados de verdade. */
static void rodar_por(double segundos)
{
    double inicio = GetTime();

    while (GetTime() - inicio < segundos && !WindowShouldClose())
    {
        handle_state_machine();
    }
}

/* Move a seta do menu, uma opcao por vez, ate chegar na escolhida. */
static void navegar_menu_ate(int opcao)
{
    int voltas = 0;

    while (selected_menu_option != opcao && voltas < 6 && !WindowShouldClose())
    {
        move_menu_selection(1);
        rodar_por(SEGUNDOS_POR_OPCAO);
        voltas++;
    }
}

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

/* Salva um quadro da tela do computador, com a pergunta e a resposta marcada. */
static void gravar_evidencia_do_quiz(const char *arquivo)
{
    RenderTexture2D alvo = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

    BeginTextureMode(alvo);

    render_computer_loop();
    render_current_step();

    EndTextureMode();

    Image imagem = LoadImageFromTexture(alvo.texture);
    ImageFlipVertical(&imagem);
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
    etapa("[1/7] tela de abertura e menu principal");

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

    etapa(TextFormat("      abertura concluida em %d quadros", quadros));

    /* Menu a vista: o jogo tem que ficar esperando, sem avancar sozinho. */
    rodar_por(SEGUNDOS_DE_MENU);

    checar("CT-27", "Abertura entrega o menu, que aguarda a escolha do jogador",
           entregou_o_menu && quadros < LIMITE_DE_QUADROS &&
           current_game_state_variables.current_screen == MENU_SCREEN &&
           is_menu_to_close == false);
}

static void testar_creditos(void)
{
    titulo("Opcao CREDITOS (CT-28 e CT-29)");
    etapa("[2/7] escolhendo CREDITOS: a tela de creditos abre e o jogador volta ao menu");

    /* Caminho real: a seta desce ate CREDITOS e o jogador confirma. */
    navegar_menu_ate(CREDITOS);
    confirm_menu_option();

    /* Tempo com a tela de creditos a vista; sem tecla, ela nao pode sair sozinha. */
    rodar_por(1.5);

    checar("CT-28", "Escolher CREDITOS abre a tela de creditos e ela permanece aberta",
           current_game_state_variables.current_screen == CREDITS_SCREEN);

    /* O jogador aperta a tecla de voltar: o quadro seguinte da maquina de
     * estados descarrega a tela de creditos e devolve o menu. */
    creditos_tecla_de_voltar = true;
    handle_state_machine();
    creditos_tecla_de_voltar = false;

    bool voltou_ao_menu = (current_game_state_variables.current_screen == MENU_SCREEN);

    rodar_por(0.8);

    bool menu_aguarda = (current_game_state_variables.current_screen == MENU_SCREEN);

    /* Tela desconhecida tambem nao pode travar o jogo. */
    current_game_state_variables.current_screen = 99;
    handle_state_machine();
    bool desconhecida_volta = (current_game_state_variables.current_screen == MENU_SCREEN);

    rodar_por(0.8);

    checar("CT-29", "Voltar dos creditos e tela desconhecida devolvem o jogador ao menu",
           voltou_ao_menu && menu_aguarda && desconhecida_volta);
}

static void testar_iniciar(void)
{
    titulo("Opcao INICIAR (CT-30)");
    etapa("[3/7] escolhendo INICIAR");

    navegar_menu_ate(INICIAR);
    confirm_menu_option();

    /* Um quadro basta: a maquina de estados processa a escolha. */
    handle_state_machine();

    checar("CT-30", "Escolher INICIAR no menu comeca a partida",
           current_game_state_variables.current_screen == PLAYING_SCREEN);
}

static void testar_cartao_do_dia(void)
{
    titulo("Cartao do dia e escritorio (CT-31)");
    etapa("[4/7] cartao \"Dia 1\": leva cerca de 10s para sair");

    double inicio = GetTime();

    while (current_screen != IN_GAME_SCREEN &&
           GetTime() - inicio < SEGUNDOS_ATE_O_ESCRITORIO &&
           !WindowShouldClose())
    {
        handle_state_machine();
    }

    bool chegou = (current_screen == IN_GAME_SCREEN);

    if (chegou) etapa(TextFormat("      escritorio na tela depois de %.1fs", GetTime() - inicio));

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
    etapa("[5/7] o personagem anda, desvia dos moveis e vai ate o computador");

    /* Passeio livre: mostra o andar, a colisao e o deslize nos moveis. */
    const int rota[8][2] = {{1, 0}, {1, -1}, {0, -1}, {-1, -1},
                            {-1, 0}, {-1, 1}, {0, 1}, {1, 1}};
    double inicio = GetTime();

    while (GetTime() - inicio < SEGUNDOS_DE_PASSEIO && !WindowShouldClose())
    {
        int trecho = (int)((GetTime() - inicio) / (SEGUNDOS_DE_PASSEIO / 8.0));

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

    etapa(TextFormat("      caminho de %d passos ate o computador", passos));

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

    checar("CT-32", "Personagem atravessa o escritorio ate o computador", is_near_computer);
}

static void testar_computador(void)
{
    titulo("Tela do computador (CT-33)");
    etapa("[6/7] apertando ESPACO no computador");

    /* Mesmo efeito de apertar ESPACO em cima do computador. */
    enter_computer_screen = true;

    double inicio = GetTime();

    while (current_screen != COMPUTER_SCREEN &&
           GetTime() - inicio < 5.0 &&
           !WindowShouldClose())
    {
        handle_state_machine();
    }

    checar("CT-33", "Interagir com o computador abre a tela do quiz",
           current_screen == COMPUTER_SCREEN);
}

static void testar_quiz(void)
{
    titulo("Quiz do dia inicial (CT-34)");
    etapa("[7/7] respondendo as perguntas ate o fim do roteiro");

    int respondidas = 0;
    int perguntas = 0;

    for (int p = 0; p < TOTAL_STEPS; p++) if (dialog_sequence[p].type == STEP_QUIZ) perguntas++;

    while (current_step_index < TOTAL_STEPS && !WindowShouldClose())
    {
        DialogStep *passo = &dialog_sequence[current_step_index];

        /* Tela na frente do jogador: enunciado ou pergunta com as alternativas. */
        rodar_por(SEGUNDOS_DE_TEXTO);

        if (passo->type == STEP_QUIZ)
        {
            /* Mesmo efeito de clicar na alternativa certa. */
            current_quiz_answered = true;
            current_quiz_selected = passo->correct_option;
            respondidas++;

            etapa(TextFormat("      pergunta %d respondida na alternativa %c",
                             respondidas, 'A' + passo->correct_option));

            /* Tempo com o icone de acerto ao lado da resposta. */
            rodar_por(SEGUNDOS_DE_RESPOSTA);

            if (respondidas == 1)
            {
                gravar_evidencia_do_quiz("docs/fds/testes/evidencias/quiz-respondido.png");
            }
        }

        /* Mesmo efeito de clicar em Continuar. */
        current_step_index++;
        current_quiz_answered = false;
        current_quiz_selected = -1;
    }

    /* Passado o ultimo passo, a tela do computador se da por encerrada. */
    rodar_por(1.0);

    checar("CT-34", "Jogador responde todas as perguntas e conclui o quiz",
           respondidas == perguntas && computer_tasks_finished);
}

/* Refaz a conta de largura de render_current_step() e confere se o texto cabe
 * dentro da caixa desenhada. A caixa tem um teto de 85% da janela; passando
 * disso, o texto continua sendo desenhado centralizado e vaza pelas laterais. */
static void testar_layout_do_quiz(void)
{
    titulo("Layout da tela do computador (CT-35)");

    float win_w = (float)window_texture.width;
    float win_h = (float)window_texture.height;

    float font_size = win_h * QUIZ_FONT_SIZE_RATIO;
    float padding_x = win_w * QUIZ_PADDING_X_RATIO;
    float max_box_width = win_w * QUIZ_MAX_WIDTH_RATIO;
    float min_box_width = win_w * QUIZ_MIN_WIDTH_RATIO;

    int vazamentos = 0;

    for (int p = 0; p < TOTAL_STEPS; p++)
    {
        DialogStep *passo = &dialog_sequence[p];

        float maior_linha = 0.0f;

        for (int l = 0; l < passo->line_count; l++)
        {
            float largura = MeasureTextEx(default_game_font, passo->lines[l],
                                          font_size, DEFAULT_LETTER_SPACING).x;

            if (largura > maior_linha) maior_linha = largura;
        }

        float largura_da_caixa = maior_linha + (padding_x * 2.0f);

        if (largura_da_caixa > max_box_width) largura_da_caixa = max_box_width;
        if (largura_da_caixa < min_box_width) largura_da_caixa = min_box_width;

        if (maior_linha > largura_da_caixa)
        {
            printf("           passo %d: texto tem %.0f px e a caixa so tem %.0f px\n",
                   p, maior_linha, largura_da_caixa);
            vazamentos++;
        }

        for (int o = 0; o < passo->option_count; o++)
        {
            float largura = MeasureTextEx(default_game_font, passo->options[o],
                                          font_size, DEFAULT_LETTER_SPACING).x;

            if (largura > max_box_width)
            {
                printf("           passo %d, opcao %d: texto tem %.0f px e o botao so tem %.0f px\n",
                       p, o, largura, max_box_width);
                vazamentos++;
            }
        }
    }

    checar("CT-35", "Texto do computador cabe dentro da caixa e dos botoes",
           vazamentos == 0);
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
    printf(" IARA - TESTES DE SISTEMA | Bloco 3: o jogo do inicio ao fim\n");
    printf("========================================================================\n");
    printf("\n  A janela do jogo mostra a partida de verdade durante este bloco.\n");

    /* Mesma ordem de preparo da main.c. */
    initialize_fonts();
    initialize_menu_variables();

    testar_estado_inicial();
    testar_abertura();
    testar_creditos();
    testar_iniciar();
    testar_cartao_do_dia();
    testar_caminhada();
    testar_computador();
    testar_quiz();
    testar_layout_do_quiz();

    printf("\n------------------------------------------------------------------------\n");
    printf(" Bloco 3: %d de %d casos aprovados", casos_ok, total_casos);

    if (casos_ok != total_casos) printf(" | %d FALHA(S)", total_casos - casos_ok);

    printf("\n========================================================================\n\n");

    unload_fonts();
    CloseWindow();

    return (casos_ok == total_casos) ? 0 : 1;
}
