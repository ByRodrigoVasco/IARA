/*
 * Testes de sistema do IARA - Bloco 3: fluxo de telas do jogo inteiro.
 *
 * Monta o jogo completo (todos os modulos, menos a main.c) e o conduz do inicio
 * ao fim, na ordem em que um jogador percorreria:
 *
 *   abertura -> menu -> CREDITOS (e a volta ao menu) -> INICIAR -> cartao do dia
 *   -> escritorio -> computador -> as perguntas do quiz
 *
 * A janela do jogo fica a vista durante todo o bloco: o que aparece na tela e a
 * partida de verdade, e serve de evidencia visual no screencast.
 *
 * Compilar e rodar: tools/tests/run_tests.ps1
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "raylib.h"
#include "janela_de_teste.h"
#include "game_state.h"
#include "state_machine.h"
#include "pre_menu.h"
#include "fonts.h"
#include "settings.h"

/* A main.c define estas globais; aqui o teste assume esse papel. */
unsigned int screen_width = 0;
unsigned int screen_height = 0;
unsigned int middle_screen_x = 0;
unsigned int middle_screen_y = 0;

/* Os .c entram inteiros (e nao so os .h) porque o teste precisa alcancar o
 * estado interno de cada tela: conduzir o personagem, mover a selecao do menu e
 * avancar o roteiro do quiz sem depender de teclado e mouse.
 */
#include "computer_loop.c"

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
    render_interact_arrow(estagiario_computer_collision_rectangle);

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
    render_day_counter();
    render_interact_arrow(get_next_objective_rectangle());

    EndTextureMode();

    Image imagem = LoadImageFromTexture(alvo.texture);
    ImageFlipVertical(&imagem); /* textura de destino vem de cabeca para baixo */
    ExportImage(imagem, arquivo);

    UnloadImage(imagem);
    UnloadRenderTexture(alvo);
}

/* Salva um quadro do cartao do dia que estiver na tela. */
static void gravar_evidencia_do_cartao(const char *arquivo)
{
    RenderTexture2D alvo = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

    BeginTextureMode(alvo);
    ClearBackground(BG_COLOR);

    render_calendar_day_card();

    EndTextureMode();

    Image imagem = LoadImageFromTexture(alvo.texture);
    ImageFlipVertical(&imagem);
    ExportImage(imagem, arquivo);

    UnloadImage(imagem);
    UnloadRenderTexture(alvo);
}

/* Salva um quadro do Flash do dia ou do salto que estiver na tela. */
static void gravar_evidencia_da_passagem(const char *arquivo)
{
    RenderTexture2D alvo = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

    BeginTextureMode(alvo);
    ClearBackground(BG_COLOR);

    handle_day_transition();

    EndTextureMode();

    Image imagem = LoadImageFromTexture(alvo.texture);
    ImageFlipVertical(&imagem);
    ExportImage(imagem, arquivo);

    UnloadImage(imagem);
    UnloadRenderTexture(alvo);
}

/* Salva um quadro da tela do computador, com a pergunta e a resposta marcada. */
static void gravar_evidencia_do_quiz(const char *arquivo)
{
    RenderTexture2D alvo = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

    BeginTextureMode(alvo);

    render_computer_backdrop();
    render_computer_loop();
    render_current_step();
    draw_stuck_cheat_sheet();

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

/* Como apertar INICIAR depois do final da campanha: a partida recomeca do Dia 1. */
static void iniciar_partida_nova(void)
{
    unload_game_loop();
    current_game_state_variables.resolved_puzzles[TOTAL_GAME_DAYS] = true;
    current_game_state_variables.current_screen = handle_menu_action(INICIAR);

    for (int quadro = 0; quadro < 5 && !WindowShouldClose(); quadro++)
    {
        handle_state_machine();
    }
}

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

    /* O teste nao pode prender o mouse de quem o executa. */
    computer_mouse_capture_enabled = false;

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

/* Coloca cada chamado da mesa de triagem no destino que a norma exige. */
static void resolver_a_mesa(void)
{
    DialogStep *mesa = &dialog_sequence[current_step_index];

    for (int c = 0; c < mesa->card_count; c++)
    {
        triage_choices[c] = get_triage_correct_choice(mesa, c);
    }
}

/* Mesmo efeito de resolver o chamado atual certo e clicar em Continuar. */
static void resolver_o_chamado(void)
{
    if (dialog_sequence[current_step_index].type == STEP_TRIAGE)
    {
        resolver_a_mesa();
        submit_triage();
    }
    else
    {
        answer_current_question(dialog_sequence[current_step_index].correct_option);
    }

    continue_after_answer();
}

static void testar_quiz(void)
{
    titulo("Chamados do dia inicial (CT-34)");
    etapa("[7/7] resolvendo os chamados ate o relatorio do dia");

    int respondidas = 0;
    int perguntas = 0;

    for (int p = 0; p < TOTAL_STEPS; p++) if (step_is_ticket(&dialog_sequence[p])) perguntas++;

    while (current_step_index < TOTAL_STEPS && !WindowShouldClose())
    {
        DialogStep *passo = &dialog_sequence[current_step_index];

        /* Tela na frente do jogador: texto, chamado ou relatorio. */
        rodar_por(SEGUNDOS_DE_TEXTO);

        if (passo->type == STEP_QUIZ)
        {
            if (respondidas == 0)
            {
                /* Um erro antes do acerto, para a evidencia da dica. */
                answer_current_question((passo->correct_option + 1) % passo->option_count);
                rodar_por(SEGUNDOS_DE_RESPOSTA);
                gravar_evidencia_do_quiz("docs/fds/testes/evidencias/erro-com-dica.png");
                continue_after_answer();
            }
            else if (respondidas == 1)
            {
                /* O caminho responsavel: a sugestao, a conferencia e a decisao do jogador. */
                ask_iara();
                rodar_por(SEGUNDOS_DE_TEXTO);
                gravar_evidencia_do_quiz("docs/fds/testes/evidencias/sugestao-da-iara.png");
                start_checking();
                mark_claim(0, CLAIM_CONFIRMED);
                mark_claim(1, CLAIM_CONFIRMED);
                rodar_por(SEGUNDOS_DE_TEXTO);
                gravar_evidencia_do_quiz("docs/fds/testes/evidencias/conferindo-a-iara.png");
                finish_checking();
                rodar_por(0.1);
            }

            /* Mesmo efeito de clicar na alternativa certa. */
            answer_current_question(passo->correct_option);
            respondidas++;

            etapa(TextFormat("      chamado %d resolvido na alternativa %c",
                             respondidas, 'A' + passo->correct_option));

            /* Tempo com o icone de acerto ao lado da resposta. */
            rodar_por(SEGUNDOS_DE_RESPOSTA);

            if (respondidas == 1)
            {
                gravar_evidencia_do_quiz("docs/fds/testes/evidencias/quiz-respondido.png");
            }

            /* Mesmo efeito de clicar em Continuar. */
            continue_after_answer();
        }
        else if (passo->type == STEP_TRIAGE)
        {
            /* Primeiro um destino errado, para a evidencia de que o jogo so diz quantos sao. */
            resolver_a_mesa();
            triage_choices[3] = TRIAGE_TO_IT;
            gravar_evidencia_do_quiz("docs/fds/testes/evidencias/mesa-de-triagem.png");
            submit_triage();
            rodar_por(SEGUNDOS_DE_RESPOSTA);
            gravar_evidencia_do_quiz("docs/fds/testes/evidencias/triagem-com-erro.png");
            continue_after_answer();

            resolver_a_mesa();
            submit_triage();
            respondidas++;
            etapa("      mesa de triagem resolvida");
            rodar_por(SEGUNDOS_DE_RESPOSTA);
            continue_after_answer();
        }
        else
        {
            /* Mesmo efeito de clicar no botao do texto. */
            advance_step();
        }
    }

    /* Passado o ultimo passo, a tela do computador se da por encerrada. */
    rodar_por(1.0);

    checar("CT-34", "Jogador resolve todos os chamados e conclui o dia pelo relatorio",
           respondidas == perguntas && computer_tasks_finished);
}

/* Refaz a conta de largura de render_current_step() e confere se o texto cabe
 * dentro da caixa desenhada. */
static int conferir_layout_do_quiz(float win_w, float font_size)
{
    float letter_spacing = font_size * QUIZ_LETTER_SPACING_RATIO;
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
                                          font_size, letter_spacing).x;

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
                                          font_size, letter_spacing).x;

            if (largura > max_box_width)
            {
                printf("           passo %d, opcao %d: texto tem %.0f px e o botao so tem %.0f px\n",
                       p, o, largura, max_box_width);
                vazamentos++;
            }
        }

        /* Cada afirmacao da IARA divide a faixa com os botoes Sim e Nao. */
        float faixa_da_afirmacao = max_box_width - (2.0f * win_w * (CLAIM_BUTTON_WIDTH_RATIO + OPTION_SPACING_RATIO));

        for (int a = 0; passo->type == STEP_QUIZ && a < MAX_AUTHORED_CLAIMS; a++)
        {
            float largura = MeasureTextEx(default_game_font, passo->claims[a],
                                          font_size, letter_spacing).x;

            if (largura > faixa_da_afirmacao)
            {
                printf("           passo %d, afirmacao %d: texto tem %.0f px e a faixa so tem %.0f px\n",
                       p, a, largura, faixa_da_afirmacao);
                vazamentos++;
            }
        }

        /* Botao de enviar sem conferir: maior no ultimo chamado, depois de crescer a cada uso. */
        float maior_botao = win_w * UNCHECKED_BUTTON_WIDTH_RATIO * (1.0f + (UNCHECKED_BUTTON_GROWTH * (TICKET_COUNT - 1)));

        if ((win_w * CHECK_BUTTON_WIDTH_RATIO) + (win_w * SUGGESTION_BUTTON_GAP_RATIO) + maior_botao > win_w)
        {
            printf("           os botoes da sugestao passam da janela da IARA\n");
            vazamentos++;
        }

        for (int d = 0; passo->type == STEP_QUIZ && d < MAX_HINTS; d++)
        {
            float largura = MeasureTextEx(default_game_font, passo->hints[d],
                                          font_size, letter_spacing).x;

            if (largura > max_box_width)
            {
                printf("           passo %d, dica %d: texto tem %.0f px e a faixa so tem %.0f px\n",
                       p, d, largura, max_box_width);
                vazamentos++;
            }
        }
    }

    return vazamentos;
}

/* Refaz a conta de update_computer_layout() para um tamanho de janela, sem precisar
 * redimensionar a janela do teste. Alem do texto, confere se o maior passo do roteiro
 * cabe na altura da janela da IARA e se a tela do monitor fica dentro da janela do jogo. */
static int conferir_layout_na_janela(int largura, int altura)
{
    float screen_h = altura * MONITOR_SCREEN_HEIGHT_RATIO;
    float screen_w = screen_h * MONITOR_SCREEN_ASPECT;

    if (screen_w > largura * MONITOR_SCREEN_MAX_WIDTH_RATIO)
    {
        screen_w = largura * MONITOR_SCREEN_MAX_WIDTH_RATIO;
        screen_h = screen_w / MONITOR_SCREEN_ASPECT;
    }

    float taskbar_h = screen_h * TASKBAR_HEIGHT_RATIO;
    float janela_h = screen_h - taskbar_h - (screen_h * DESKTOP_WINDOW_MARGIN_RATIO * 2.0f);
    float win_w = screen_w * DESKTOP_WINDOW_WIDTH_RATIO;
    float win_h = janela_h - (janela_h * WINDOW_ART_TITLE_RATIO);

    float por_altura = win_h * QUIZ_FONT_SIZE_RATIO;
    float por_largura = (win_w * QUIZ_MAX_WIDTH_RATIO * QUIZ_TEXT_FIT_RATIO) / get_longest_text_width_per_font_size();
    float font_size = (por_altura < por_largura) ? por_altura : por_largura;

    int problemas = conferir_layout_do_quiz(win_w, font_size);

    /* A colinha fica entre a borda do jogo, a janela da IARA e a barra de tarefas. */
    float screen_x = (largura - screen_w) / 2.0f;
    float screen_y = (altura * MONITOR_TOP_RATIO) + (altura * MONITOR_BEZEL_RATIO);
    Rectangle barra = {screen_x, screen_y + screen_h - taskbar_h, screen_w, taskbar_h};
    Rectangle janela = {screen_x + (screen_w * DESKTOP_WINDOW_LEFT_RATIO), screen_y + (screen_h * DESKTOP_WINDOW_MARGIN_RATIO), win_w, janela_h};
    Rectangle colinha = get_cheat_sheet_rect(janela, barra, screen_w, screen_h, (float)largura);
    float fonte_da_colinha = get_cheat_sheet_font_size(colinha);

    if (colinha.x < 0.0f || CheckCollisionRecs(colinha, janela) || CheckCollisionRecs(colinha, barra) || fonte_da_colinha < 12.0f)
    {
        printf("           janela %dx%d: colinha em %.0f,%.0f de %.0fx%.0f com fonte %.1f\n", largura, altura,
               colinha.x, colinha.y, colinha.width, colinha.height, fonte_da_colinha);
        problemas++;
    }

    /* Maior caso do roteiro: quatro linhas, tres opcoes, a dica de erro e Tentar de novo. */
    float altura_do_quiz = (win_h * QUIZ_BOX_TOP_MARGIN_RATIO) + (4.0f * font_size) + (3.0f * win_h * QUIZ_LINE_SPACING_RATIO) +
                           (win_h * QUIZ_PADDING_Y_RATIO * 2.0f) + (win_h * QUIZ_BUTTONS_TOP_GAP_RATIO) +
                           (4.0f * win_h * (OPTION_BUTTON_HEIGHT_RATIO + OPTION_SPACING_RATIO)) +
                           (win_h * CONTINUE_BUTTON_GAP_RATIO) + (win_h * ACTION_BUTTON_HEIGHT_RATIO);

    /* Maior relatorio: titulo, os chamados, a linha da tarja e Concluir. */
    float altura_do_relatorio = (win_h * QUIZ_BOX_TOP_MARGIN_RATIO) + font_size + (win_h * QUIZ_PADDING_Y_RATIO * 2.0f) +
                                (win_h * QUIZ_BUTTONS_TOP_GAP_RATIO) +
                                ((TICKET_COUNT + 1) * win_h * (OPTION_BUTTON_HEIGHT_RATIO + OPTION_SPACING_RATIO)) +
                                (win_h * CONTINUE_BUTTON_GAP_RATIO) + (win_h * ACTION_BUTTON_HEIGHT_RATIO);

    /* Maior sugestao: o chamado de tres linhas, a caixa da IARA de quatro e as duas fileiras de botoes. */
    float altura_da_sugestao = (win_h * QUIZ_BOX_TOP_MARGIN_RATIO) + (7.0f * font_size) + (5.0f * win_h * QUIZ_LINE_SPACING_RATIO) +
                               (win_h * QUIZ_PADDING_Y_RATIO * 4.0f) + (2.0f * win_h * QUIZ_BUTTONS_TOP_GAP_RATIO) +
                               (2.0f * win_h * ACTION_BUTTON_HEIGHT_RATIO) + (win_h * OPTION_SPACING_RATIO);

    /* Conferencia sem memoria: trecho do manual, as paginas, tres afirmacoes e Decidir. */
    float altura_da_conferencia = (win_h * QUIZ_BOX_TOP_MARGIN_RATIO) + (3.0f * font_size) + (2.0f * win_h * QUIZ_LINE_SPACING_RATIO) +
                                  (win_h * QUIZ_PADDING_Y_RATIO * 2.0f) + (win_h * OPTION_SPACING_RATIO) +
                                  (4.0f * win_h * (OPTION_BUTTON_HEIGHT_RATIO + OPTION_SPACING_RATIO)) +
                                  (win_h * CONTINUE_BUTTON_GAP_RATIO) + (win_h * ACTION_BUTTON_HEIGHT_RATIO);

    /* Mesa de triagem: a norma, os quatro chamados, o resultado do envio e o botao. */
    float altura_da_mesa = (win_h * QUIZ_BOX_TOP_MARGIN_RATIO) + (3.0f * font_size) + (2.0f * win_h * QUIZ_LINE_SPACING_RATIO) +
                           (win_h * QUIZ_PADDING_Y_RATIO * 2.0f) + (win_h * OPTION_SPACING_RATIO) +
                           (5.0f * win_h * (OPTION_BUTTON_HEIGHT_RATIO + OPTION_SPACING_RATIO)) +
                           (win_h * ACTION_BUTTON_HEIGHT_RATIO);

    if (altura_da_conferencia > win_h || altura_da_mesa > win_h)
    {
        printf("           janela %dx%d: conferencia pede %.0f px, mesa %.0f px e so ha %.0f px\n",
               largura, altura, altura_da_conferencia, altura_da_mesa, win_h);
        problemas++;
    }

    if (altura_do_relatorio > win_h || altura_da_sugestao > win_h)
    {
        printf("           janela %dx%d: relatorio pede %.0f px, sugestao %.0f px e so ha %.0f px\n",
               largura, altura, altura_do_relatorio, altura_da_sugestao, win_h);
        problemas++;
    }

    if (altura_do_quiz > win_h)
    {
        printf("           janela %dx%d: o quiz pede %.0f px de altura e so ha %.0f px\n", largura, altura, altura_do_quiz, win_h);
        problemas++;
    }

    if (screen_w > largura || (altura * MONITOR_TOP_RATIO) + (altura * MONITOR_BEZEL_RATIO) + screen_h > altura)
    {
        printf("           janela %dx%d: a tela do monitor sai da janela\n", largura, altura);
        problemas++;
    }

    return problemas;
}

/* Depois do quiz o jogador volta ao mapa, sai pela porta e o dia termina. */
static void testar_fim_do_dia(void)
{
    titulo("Saida do quiz e fim do dia (CT-44 a CT-46)");

    double inicio = GetTime();

    while (current_screen != IN_GAME_SCREEN && GetTime() - inicio < 5.0 && !WindowShouldClose())
    {
        handle_state_machine();
    }

    Vector2 antes = current_estagiario_position;

    for (int quadro = 0; quadro < 20 && !WindowShouldClose(); quadro++)
    {
        conduzir_quadro(-1, 0);
    }

    bool voltou_a_andar = current_estagiario_position.x != antes.x;

    /* O computador nao abre de novo, nem com o jogador em cima dele. */
    current_estagiario_position = (Vector2){
        estagiario_computer_collision_rectangle.x - HITBOX_OFFSET_X_AT_REFERENCE * get_scale(),
        estagiario_computer_collision_rectangle.y - HITBOX_OFFSET_Y_AT_REFERENCE * get_scale()};
    update_collision_rec();
    handle_map_interactables();

    checar("CT-44", "Concluir o quiz devolve o jogador ao mapa, andando e com a tarefa resolvida",
           current_screen == IN_GAME_SCREEN && voltou_a_andar && !computer_loop_loaded &&
           current_game_state_variables.is_resolved && !enter_computer_screen);

    Rectangle zona = get_exit_zone();
    int passos = 0;
    bool alcancavel = calcular_caminho_ate(zona, (Vector2){
        estagiario_computer_collision_rectangle.x - 40.0f * get_scale(),
        estagiario_computer_collision_rectangle.y + 60.0f * get_scale()}, &passos);

    /* Com os pes logo acima da porta, sem encostar no vidro. */
    current_estagiario_position = (Vector2){
        zona.x + (zona.width / 2.0f) - ((HITBOX_OFFSET_X_AT_REFERENCE + (HITBOX_WIDTH_AT_REFERENCE / 2.0f)) * get_scale()),
        zona.y - (HITBOX_OFFSET_Y_AT_REFERENCE * get_scale())};
    update_collision_rec();
    handle_map_interactables();
    bool perto_da_porta = is_near_exit;

    rodar_por(0.5);
    gravar_evidencia_do_jogo("docs/fds/testes/evidencias/saida-pela-porta.png");

    /* Mesmo efeito de apertar ESPACO em frente a porta. */
    leave_office = true;
    rodar_por(2.0);
    gravar_evidencia_do_cartao("docs/fds/testes/evidencias/fim-do-dia.png");

    inicio = GetTime();

    while (current_screen != DAY_TRANSITION_SCREEN && GetTime() - inicio < 20.0 && !WindowShouldClose())
    {
        handle_state_machine();
    }

    printf("           caminho do computador ate a porta: %d passos\n", passos);
    checar("CT-45", "Jogador alcanca a porta, encerra o dia e segue para o Flash do Dia 2, com o Dia 1 salvo",
           alcancavel && perto_da_porta && current_screen == DAY_TRANSITION_SCREEN &&
           current_game_state_variables.current_day == 2 && current_game_state_variables.day_type == FLASH_DAY &&
           current_game_state_variables.resolved_puzzles[1]);

    /* O flash espera o tempo minimo e so avanca com o toque do jogador. */
    rodar_por(0.3);
    gravar_evidencia_da_passagem("docs/fds/testes/evidencias/flash-do-dia-2.png");
    bool flash_espera = current_game_state_variables.current_day == 2;

    transition_time = FLASH_MIN_TIME + 1.0f;
    rodar_por(0.2);
    bool flash_nao_avanca_sozinho = current_game_state_variables.current_day == 2;

    advance_to_next_day();
    rodar_por(0.1);

    checar("CT-46", "Flash do Dia 2 so avanca ao toque e abre o Dia 3, com a tarefa nova e o jogador na porta",
           flash_espera && flash_nao_avanca_sozinho &&
           current_game_state_variables.current_day == 3 && current_game_state_variables.day_type == PLAYABLE_DAY &&
           current_screen == CALENDAR_DAY_CARD_SCREEN && !current_game_state_variables.is_resolved &&
           active_script->day == 3 && current_step_index == 0);

    iniciar_partida_nova();

    computer_mouse_capture_enabled = false;
}

/* Espera o jogo chegar a uma tela interna, rodando pela maquina de estados. */
static void rodar_ate_a_tela(int tela, double limite)
{
    double inicio = GetTime();

    while (current_screen != tela && GetTime() - inicio < limite && !WindowShouldClose())
    {
        handle_state_machine();
    }
}

/* Passa pelas telas de abertura ate o primeiro chamado. */
static void ir_para_o_primeiro_chamado(void)
{
    while (dialog_sequence[current_step_index].type != STEP_QUIZ)
    {
        advance_step();
    }
}

/* Pula o cartao do dia e abre o computador no primeiro chamado. */
static void abrir_o_primeiro_chamado(void)
{
    current_screen = IN_GAME_SCREEN;
    enter_computer_screen = true;
    rodar_ate_a_tela(COMPUTER_SCREEN, 5.0);
    ir_para_o_primeiro_chamado();
}

/* Os outros caminhos do dia, na partida de verdade: mural, colinha, sem conferir e failsafe. */
static void testar_caminhos_do_dia(void)
{
    titulo("Caminhos do Dia 1 na partida (CT-53 a CT-56)");

    /* Partida recomecada pelo CT-46, ainda no cartao do dia. */
    current_screen = IN_GAME_SCREEN;

    bool contador = strcmp(get_day_counter_text(), "Dia 1 de 100") == 0;

    toggle_mural_note();
    bool investigou = current_game_state_variables.user_investigated;
    toggle_mural_note();

    checar("CT-53", "Contador mostra Dia 1 de 100 e ler o mural da a colinha e registra a investigacao, sem somar pontos ainda",
           contador && investigou && current_game_state_variables.has_cheat_sheet && !is_hint_note_open() &&
           current_hidden_statistic.current_overall_points == 1 &&
           current_hidden_statistic.current_perception_points == 1);

    enter_computer_screen = true;
    rodar_ate_a_tela(COMPUTER_SCREEN, 5.0);
    ir_para_o_primeiro_chamado();

    /* A colinha ja esta colada quando o primeiro chamado aparece. */
    rodar_por(1.5);
    gravar_evidencia_do_quiz("docs/fds/testes/evidencias/colinha-no-monitor.png");

    Rectangle colinha = get_cheat_sheet_rect(iara_window_rect, taskbar_rect, screen_rect.width, screen_rect.height, (float)GetScreenWidth());

    checar("CT-54", "Colinha fica colada no canto inferior esquerdo do monitor, sem cobrir a janela da IARA",
           current_game_state_variables.has_cheat_sheet &&
           colinha.x + colinha.width <= iara_window_rect.x && colinha.y + colinha.height <= taskbar_rect.y &&
           colinha.y > screen_rect.y + (screen_rect.height / 2.0f));

    /* Enviar tudo sem conferir: o relatorio cobra, o jogador volta ao mapa e sai pela porta. */
    while (step_is_ticket(&dialog_sequence[current_step_index]))
    {
        ask_iara();

        if (current_ticket_view == TICKET_PRIVACY)
        {
            rodar_por(SEGUNDOS_DE_TEXTO);
            gravar_evidencia_do_quiz("docs/fds/testes/evidencias/tarja-de-cpf.png");
            decide_personal_data(false);
        }

        /* Terceiro chamado sem conferir: a IARA ja esta sem memoria e o jogador procura a pagina. */
        if (current_step_index == FIRST_TICKET_STEP + 2)
        {
            rodar_por(SEGUNDOS_DE_TEXTO);
            gravar_evidencia_do_quiz("docs/fds/testes/evidencias/iara-sem-memoria.png");
            current_ticket_view = TICKET_CHECKING;
            rodar_por(0.1);
            gravar_evidencia_do_quiz("docs/fds/testes/evidencias/procurando-no-manual.png");
            current_ticket_view = TICKET_SUGGESTION;
        }

        rodar_por(0.1);
        send_without_checking();
    }

    rodar_por(SEGUNDOS_DE_TEXTO);
    gravar_evidencia_do_quiz("docs/fds/testes/evidencias/relatorio-do-dia.png");
    advance_step();
    rodar_ate_a_tela(IN_GAME_SCREEN, 5.0);

    bool voltou_ao_mapa = current_screen == IN_GAME_SCREEN && !computer_failsafe_triggered;

    leave_office = true;
    rodar_por(0.1);

    checar("CT-55", "Enviar sem conferir devolve o jogador ao mapa e o fim do dia conta que a IARA resolveu",
           voltou_ao_mapa && current_screen == DAY_END_CARD_SCREEN &&
           strcmp(current_card_subtitle_line_1, DAY_END_CARD_DELEGATED_LINE_1) == 0 &&
           current_hidden_statistic.current_overall_points == 1 + POINTS_WITHOUT_CHECKING &&
           current_hidden_statistic.current_automation_points == 2 &&
           current_hidden_statistic.current_perception_points == 2);

    /* Recomecar nao acumula os pontos da partida anterior. */
    iniciar_partida_nova();
    bool pontos_zerados = current_hidden_statistic.current_automation_points == 1 &&
                          current_hidden_statistic.current_perception_points == 1 &&
                          !current_game_state_variables.user_investigated &&
                          !current_game_state_variables.has_cheat_sheet;

    /* Failsafe: tres erros seguidos, o relatorio e o expediente acaba, sem voltar ao mapa. */
    abrir_o_primeiro_chamado();

    for (int erro = 0; erro < FAILSAFE_WRONG_ANSWERS; erro++)
    {
        DialogStep *passo = &dialog_sequence[current_step_index];

        /* Um clique real na janela do teste pode avancar o roteiro: vira falha do CT-56, nao queda. */
        if (passo->type != STEP_QUIZ) break;

        answer_current_question((passo->correct_option + 1) % passo->option_count);
        rodar_por(0.4);

        if (erro == FAILSAFE_WRONG_ANSWERS - 1)
        {
            gravar_evidencia_do_quiz("docs/fds/testes/evidencias/failsafe-terceiro-erro.png");
        }

        continue_after_answer();
    }

    bool foi_ao_relatorio = current_step_index == REPORT_STEP;

    rodar_por(0.1);
    advance_step();
    rodar_ate_a_tela(DAY_END_CARD_SCREEN, 5.0);
    bool cartao_do_failsafe = current_screen == DAY_END_CARD_SCREEN &&
                              strcmp(current_card_subtitle_line_1, DAY_END_CARD_FAILSAFE_LINE_1) == 0;

    rodar_por(2.0);
    gravar_evidencia_do_cartao("docs/fds/testes/evidencias/fim-do-dia-failsafe.png");

    checar("CT-56", "Failsafe passa pelo relatorio e encerra o dia direto no cartao, e recomecar nao acumula pontos",
           pontos_zerados && foi_ao_relatorio && cartao_do_failsafe && !computer_loop_loaded &&
           current_game_state_variables.resolved_puzzles[1] &&
           current_hidden_statistic.current_overall_points == 1);

    iniciar_partida_nova();
}

/* Sair do computador no meio do dia e voltar para o mesmo ponto (CT-59). */
static void testar_saida_no_meio_do_dia(void)
{
    titulo("Sair do computador sem concluir (CT-59)");

    abrir_o_primeiro_chamado();

    /* Primeiro chamado resolvido e o segundo parado na sugestao da IARA. */
    DialogStep *primeiro = &dialog_sequence[current_step_index];

    answer_current_question(primeiro->correct_option);
    continue_after_answer();
    ask_iara();

    int passo_antes = current_step_index;
    int pontos_antes = current_hidden_statistic.current_overall_points;
    int memoria_antes = iara_memory;

    /* A cobranca da chefia precisa caber antes do botao de sair. */
    ticket_time = BOSS_SECOND_DELAY;
    rodar_por(SEGUNDOS_DE_TEXTO);
    gravar_evidencia_do_quiz("docs/fds/testes/evidencias/sair-do-computador.png");

    float tarefa = taskbar_rect.height * 0.62f;
    float inicio_da_cobranca = taskbar_rect.x + (taskbar_rect.height * 0.14f * 5.0f) + (taskbar_rect.width * 0.34f);
    float fim_da_cobranca = inicio_da_cobranca + MeasureTextEx(default_game_font, get_boss_message(), tarefa, tarefa * QUIZ_LETTER_SPACING_RATIO).x;
    bool barra_cabe = fim_da_cobranca <= get_taskbar_exit_rect().x;

    bool esc_nao_fecha = !escape_closes_current_screen();

    leave_computer();
    rodar_ate_a_tela(IN_GAME_SCREEN, 5.0);

    bool voltou_ao_mapa = current_screen == IN_GAME_SCREEN && !computer_loop_loaded &&
                          !current_game_state_variables.is_resolved && !computer_failsafe_triggered &&
                          current_hidden_statistic.current_overall_points == pontos_antes;

    rodar_por(0.2);
    enter_computer_screen = true;
    rodar_ate_a_tela(COMPUTER_SCREEN, 5.0);
    rodar_por(0.5);

    bool retomou = current_screen == COMPUTER_SCREEN && current_step_index == passo_antes &&
                   current_ticket_view == TICKET_SUGGESTION && ticket_asked_iara &&
                   ticket_results[0].outcome == TASK_SOLVED_ALONE && !computer_exit_finished &&
                   iara_memory == memoria_antes;

    /* Termina o dia e confirma que o que foi feito antes de sair conta uma vez so. */
    send_without_checking();

    while (step_is_ticket(&dialog_sequence[current_step_index]))
    {
        resolver_o_chamado();
    }

    bool pontos_certos = current_hidden_statistic.current_overall_points ==
                         pontos_antes + ((TICKET_COUNT - 1) * POINTS_SOLVED_ALONE);

    rodar_por(0.1);
    advance_step();
    rodar_ate_a_tela(IN_GAME_SCREEN, 5.0);
    leave_office = true;
    rodar_por(0.1);
    iniciar_partida_nova();

    checar("CT-59", "Sair do computador volta ao mapa sem concluir, e voltar retoma o mesmo ponto com os pontos certos",
           barra_cabe && esc_nao_fecha && voltou_ao_mapa && retomou && pontos_certos);

    /* O INICIAR acima e uma partida nova: os chamados recomecam, sem herdar o ponto anterior. */
    checar("CT-60", "Partida nova recomeca os chamados do inicio",
           current_step_index == 0 && ticket_results[0].outcome == TASK_PENDING && unchecked_sends == 0);
}

/* Acha a marcacao que resolve a mesa aberta, testando todas, como prova de que ela existe. */
static void preencher_a_mesa_certa(DialogStep *mesa)
{
    int choices[MAX_TRIAGE_CARDS] = {0};

    for (int combinacao = 0; combinacao < (1 << mesa->card_count); combinacao++)
    {
        for (int c = 0; c < mesa->card_count; c++) choices[c] = (combinacao >> c) & 1;

        if (count_board_wrong(mesa, choices) == 0)
        {
            for (int c = 0; c < mesa->card_count; c++) triage_choices[c] = choices[c];
            return;
        }
    }
}

/* Joga um dia dentro do computador: resolve cada chamado, responde as conversas e assina. */
static void jogar_o_computador(int opcao_das_conversas)
{
    char arquivo[96];
    int dia = current_game_state_variables.current_day;

    while (current_step_index < REPORT_STEP && !WindowShouldClose())
    {
        DialogStep *passo = &dialog_sequence[current_step_index];

        rodar_por(0.3);

        if (passo->type == STEP_STATEMENT)
        {
            advance_step();
        }
        else if (passo->type == STEP_CHOICE)
        {
            snprintf(arquivo, sizeof(arquivo), "docs/fds/testes/evidencias/dia-%d-conversa.png", dia);
            gravar_evidencia_do_quiz(arquivo);
            choose_day_option(passo, opcao_das_conversas);
        }
        else if (passo->type == STEP_QUIZ)
        {
            resolver_o_chamado();
        }
        else if (dia == 1)
        {
            resolver_o_chamado();
        }
        else
        {
            snprintf(arquivo, sizeof(arquivo), "docs/fds/testes/evidencias/dia-%d-mesa.png", dia);
            preencher_a_mesa_certa(passo);
            rodar_por(0.1);
            gravar_evidencia_do_quiz(arquivo);

            /* Mostra o que o jogador ve depois de enviar, inclusive o resultado da mini-IA. */
            submit_triage();
            rodar_por(0.1);
            snprintf(arquivo, sizeof(arquivo), "docs/fds/testes/evidencias/dia-%d-enviada.png", dia);
            gravar_evidencia_do_quiz(arquivo);
            continue_after_answer();
        }
    }

    rodar_por(0.1);
    advance_step();
}

/* A campanha inteira pela maquina de estados: do Dia 1 ao final (CT-71 e CT-72). */
static void testar_campanha(void)
{
    titulo("Campanha do Dia 1 ao Dia 100 (CT-71 e CT-72)");

    double inicio = GetTime();
    int dias_jogados = 0;
    int flashes_vistos = 0;
    int dias_de_salto = 0;
    int dia_anterior = 0;
    bool dias_em_ordem = true;

    iniciar_partida_nova();

    while (current_screen != ENDING_CARD_SCREEN && GetTime() - inicio < 240.0 && !WindowShouldClose())
    {
        int dia = current_game_state_variables.current_day;

        if (dia != dia_anterior)
        {
            if (dia != dia_anterior + 1) dias_em_ordem = false;
            dia_anterior = dia;
        }

        if (current_screen == DAY_TRANSITION_SCREEN)
        {
            if (current_game_state_variables.day_type == FLASH_DAY)
            {
                flashes_vistos++;
                rodar_por(0.05);
                advance_to_next_day();
            }
            else
            {
                dias_de_salto++;
                while (current_game_state_variables.current_day == dia && !WindowShouldClose()) handle_state_machine();
            }
            continue;
        }

        if (current_screen == SCREEN_NONE) handle_state_machine();

        /* Pula o cartao de chegada e joga o dia. */
        current_screen = IN_GAME_SCREEN;
        dias_jogados++;

        /* Investiga todo dia que tem o que investigar: e o que abre as evidencias. */
        if (calendar_has_investigation(dia))
        {
            toggle_mural_note();
            if (dia == 58)
            {
                rodar_por(0.05);
                gravar_evidencia_do_jogo("docs/fds/testes/evidencias/mural-do-dia-58.png");
            }
            toggle_mural_note();
        }

        enter_computer_screen = true;
        rodar_ate_a_tela(COMPUTER_SCREEN, 5.0);
        jogar_o_computador((dia == 12) ? 1 : 0);
        rodar_ate_a_tela(IN_GAME_SCREEN, 5.0);

        if (dia == 89)
        {
            rodar_por(0.3);
            gravar_evidencia_do_jogo("docs/fds/testes/evidencias/escritorio-no-dia-89.png");
        }

        leave_office = true;
        rodar_ate_a_tela(DAY_END_CARD_SCREEN, 5.0);
        render_calendar_day_card_finished = true;
        handle_state_machine();
    }

    GameEnding final_da_partida = get_game_ending();

    rodar_por(0.1);
    rodar_por(2.0);
    gravar_evidencia_do_cartao("docs/fds/testes/evidencias/final-ruptura.png");

    printf("           %d dias jogados, %d flashes, %d dias de salto, em %.0fs\n", dias_jogados, flashes_vistos, dias_de_salto, GetTime() - inicio);
    printf("           Automacao %u, Percepcao %u, Vinculo %u, pontos %d\n",
           current_hidden_statistic.current_automation_points - HIDDEN_STATISTIC_START,
           current_hidden_statistic.current_perception_points - HIDDEN_STATISTIC_START,
           current_hidden_statistic.current_human_bond_points - HIDDEN_STATISTIC_START,
           current_hidden_statistic.current_overall_points);

    checar("CT-71", "Campanha passa pelos 100 dias em ordem: 9 jogaveis, 14 flashes e o resto em salto",
           dias_jogados == 9 && flashes_vistos == 14 && dias_de_salto == 100 - 9 - 14 && dias_em_ordem &&
           current_game_state_variables.resolved_puzzles[TOTAL_GAME_DAYS]);
    checar("CT-72", "Investigar, conversar e fechar a prova leva ao Final D, com as 3 evidencias e a Mariana",
           current_screen == ENDING_CARD_SCREEN && final_da_partida == ENDING_RUPTURE &&
           current_game_state_variables.found_mariana && current_game_state_variables.proof_succeeded &&
           current_game_state_variables.evidence_mask == (EVIDENCE_NO_RESIGNATION_LETTER | EVIDENCE_BADGE_AFTER_LEAVING | EVIDENCE_NO_HUMAN_ORDER));

    /* Do final de volta ao menu; INICIAR comeca outra partida do Dia 1. */
    render_calendar_day_card_finished = true;
    handle_state_machine();

    bool voltou_ao_menu = current_game_state_variables.current_screen == MENU_SCREEN;

    unload_menu();
    current_game_state_variables.current_screen = handle_menu_action(INICIAR);
    handle_state_machine();

    checar("CT-73", "Depois do final, o jogo volta ao menu e INICIAR recomeca do Dia 1 sem herdar nada",
           voltou_ao_menu && current_game_state_variables.current_day == 1 &&
           current_game_state_variables.evidence_mask == 0 && !current_game_state_variables.found_mariana &&
           current_hidden_statistic.current_perception_points == HIDDEN_STATISTIC_START);

    /* Retomar um save: o jogo volta no dia em que parou, seja qual for o tipo dele (CA4). */
    unload_game_loop();
    start_game_day(5);
    current_game_state_variables.current_screen = handle_menu_action(INICIAR);
    bool retomou_no_salto = current_screen == DAY_TRANSITION_SCREEN && current_game_state_variables.current_day == 5 &&
                            current_game_state_variables.day_type == JUMP_DAY;

    unload_game_loop();
    start_game_day(27);
    current_game_state_variables.current_screen = handle_menu_action(INICIAR);
    bool retomou_no_dia = current_screen == SCREEN_NONE && current_game_state_variables.current_day == 27 && active_script->day == 27;

    /* Fechado no cartao de fim do dia: o dia ja estava encerrado, entao segue para o proximo. */
    unload_game_loop();
    start_game_day(12);
    current_game_state_variables.resolved_puzzles[12] = true;
    current_game_state_variables.current_screen = handle_menu_action(INICIAR);
    bool seguiu_para_o_proximo = current_game_state_variables.current_day == 13 && current_screen == DAY_TRANSITION_SCREEN;

    checar("CT-75", "Save retoma no salto, no dia jogavel com o roteiro certo e depois de um dia ja encerrado",
           retomou_no_salto && retomou_no_dia && seguiu_para_o_proximo);

    iniciar_partida_nova();
}

/* Confere, em cada dia, se os textos dos botoes da mesa cabem neles e se a mesa cabe na janela da IARA. */
static int conferir_mesas_na_janela(int largura, int altura)
{
    float screen_h = altura * MONITOR_SCREEN_HEIGHT_RATIO;
    float screen_w = screen_h * MONITOR_SCREEN_ASPECT;

    if (screen_w > largura * MONITOR_SCREEN_MAX_WIDTH_RATIO)
    {
        screen_w = largura * MONITOR_SCREEN_MAX_WIDTH_RATIO;
        screen_h = screen_w / MONITOR_SCREEN_ASPECT;
    }

    float janela_h = screen_h - (screen_h * TASKBAR_HEIGHT_RATIO) - (screen_h * DESKTOP_WINDOW_MARGIN_RATIO * 2.0f);
    float win_w = screen_w * DESKTOP_WINDOW_WIDTH_RATIO;
    float win_h = janela_h - (janela_h * WINDOW_ART_TITLE_RATIO);
    float vao = win_h * OPTION_SPACING_RATIO;
    float largura_da_linha = win_w * QUIZ_MAX_WIDTH_RATIO;
    float largura_da_marca = win_w * BOARD_BUTTON_WIDTH_RATIO;
    float largura_do_envio = (largura_da_linha - (win_w * SUGGESTION_BUTTON_GAP_RATIO)) / 2.0f;
    int problemas = 0;

    quiz_area_rect = (Rectangle){0, 0, win_w, win_h};

    for (int d = 0; d < DAY_SCRIPT_COUNT; d++)
    {
        select_day_script(day_scripts[d].day);

        float por_altura = win_h * QUIZ_FONT_SIZE_RATIO;
        float por_largura = (largura_da_linha * QUIZ_TEXT_FIT_RATIO) / get_longest_text_width_per_font_size();

        quiz_font_size = (por_altura < por_largura) ? por_altura : por_largura;

        for (int p = 0; p < TOTAL_STEPS; p++)
        {
            const DialogStep *passo = &dialog_sequence[p];

            if (passo->type != STEP_TRIAGE) continue;

            float primeira_linha = (win_h * QUIZ_BOX_TOP_MARGIN_RATIO) + (passo->line_count * quiz_font_size) +
                                   ((passo->line_count - 1) * win_h * QUIZ_LINE_SPACING_RATIO) +
                                   (win_h * QUIZ_PADDING_Y_RATIO * 2.0f) + vao;
            bool tem_mini_ia = passo->board_mode == BOARD_TRAINING;
            float altura_da_linha = get_board_row_height(passo, primeira_linha, tem_mini_ia);
            int linhas_depois = passo->card_count + (tem_mini_ia ? 1 : 0);
            float fim_da_mesa = primeira_linha + (linhas_depois * (altura_da_linha + vao)) + (2.0f * altura_da_linha) + vao;
            const char *textos[] = {TextFormat("[%s]", passo->choice_labels[0]), TextFormat("[%s]", passo->choice_labels[1]),
                                    "Perguntar à IARA", passo->submit_label};
            float larguras[] = {largura_da_marca, largura_da_marca, largura_do_envio, largura_do_envio};

            if (fim_da_mesa > win_h)
            {
                printf("           Dia %d em %dx%d: a mesa pede %.0f px e so ha %.0f px\n", day_scripts[d].day, largura, altura, fim_da_mesa, win_h);
                problemas++;
            }

            for (int t = 0; t < 4; t++)
            {
                Rectangle botao = {0, 0, larguras[t], altura_da_linha};
                float fonte = get_board_button_font_size(textos[t], botao);
                float texto = MeasureTextEx(default_game_font, textos[t], fonte, fonte * BUTTON_LETTER_SPACING_RATIO).x;

                if (texto > botao.width || fonte > botao.height || fonte < 9.0f)
                {
                    printf("           Dia %d em %dx%d: \"%s\" tem %.0f px, fonte %.1f, em botao de %.0fx%.0f\n",
                           day_scripts[d].day, largura, altura, textos[t], texto, fonte, botao.width, botao.height);
                    problemas++;
                }
            }
        }
    }

    select_day_script(current_game_state_variables.current_day);

    return problemas;
}

static void testar_layout_do_quiz(void)
{
    titulo("Layout da tela do computador (CT-35)");

    /* Minima, padrao em monitor 1080p, 1080p, 1440p, 4:3 e 21:9. */
    int janelas[][2] = {{960, 540}, {1280, 720}, {1632, 918}, {1920, 1080}, {2560, 1440}, {1024, 768}, {2560, 1080}};
    int problemas = 0;
    int problemas_nas_mesas = 0;

    for (int i = 0; i < (int)(sizeof(janelas) / sizeof(janelas[0])); i++)
    {
        problemas += conferir_layout_na_janela(janelas[i][0], janelas[i][1]);
        problemas_nas_mesas += conferir_mesas_na_janela(janelas[i][0], janelas[i][1]);
    }

    checar("CT-35", "Texto do computador cabe na caixa, nos botoes e na janela em 7 tamanhos",
           problemas == 0);
    checar("CT-76", "Botoes das mesas de todos os dias cabem, com o texto legivel, em 7 tamanhos",
           problemas_nas_mesas == 0);
}

int main(void)
{
    SetTraceLogLevel(LOG_WARNING);

    abrir_janela_de_teste(LARGURA_JANELA, ALTURA_JANELA, "IARA - testes de fluxo");

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
    testar_fim_do_dia();
    testar_caminhos_do_dia();
    testar_saida_no_meio_do_dia();
    testar_campanha();

    /* CONFIGURACOES abre a propria tela e VOLTAR devolve o jogador ao menu. */
    current_game_state_variables.current_screen = handle_menu_action(CONFIGURACOES);
    bool abriu_configuracoes = (current_game_state_variables.current_screen == SETTINGS_SCREEN);

    for (int quadro = 0; quadro < 5; quadro++)
    {
        handle_state_machine();
    }

    bool continua_nas_configuracoes = (current_game_state_variables.current_screen == SETTINGS_SCREEN);

    checar("CT-42", "Escolher CONFIGURACOES abre a tela e ela permanece aberta",
           abriu_configuracoes && continua_nas_configuracoes && activate_settings_option(SETTINGS_BACK));

    unload_settings_screen();

    /* SAIR nao fecha a janela na hora: a main.c ainda precisa salvar e descarregar tudo. */
    int tela_apos_sair = handle_menu_action(SAIR);
    checar("CT-36", "Escolher SAIR pede o fechamento e mantem a janela viva para o save",
           is_game_to_close && tela_apos_sair == NONE && IsWindowReady());

    printf("\n------------------------------------------------------------------------\n");
    printf(" Bloco 3: %d de %d casos aprovados", casos_ok, total_casos);

    if (casos_ok != total_casos) printf(" | %d FALHA(S)", total_casos - casos_ok);

    printf("\n========================================================================\n\n");

    /* Encerrar com a partida recomecada libera tudo, e uma segunda chamada nao faz nada. */
    unload_game_loop();
    unload_game_loop();

    unload_fonts();
    CloseWindow();

    return (casos_ok == total_casos) ? 0 : 1;
}
