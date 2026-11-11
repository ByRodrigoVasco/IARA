/*
 * Testes de sistema do IARA - Bloco 2: conteudo do quiz e navegacao do menu.
 *
 * Percorre o roteiro do computador do primeiro ao ultimo passo, como um jogador
 * faria, e navega o menu principal pelo teclado e pelo mouse.
 *
 * Compilar e rodar: tools/tests/run_tests.ps1
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "raylib.h"
#include "janela_de_teste.h"

/* A main.c define estas globais; aqui o teste assume esse papel. */
unsigned int screen_width = 0;
unsigned int screen_height = 0;
unsigned int middle_screen_x = 0;
unsigned int middle_screen_y = 0;

/* Inclui os .c para alcancar o roteiro e o estado interno, que sao estaticos. */
#include "computer_loop.c"
#include "calendar.h"
#include "menu.c"
#include "settings.h"
#include "preferences.h"
#include "accessibility.h"

#define LARGURA_JANELA 1280
#define ALTURA_JANELA 720

#define CICLOS_POR_TRECHO 20       /* Vezes que o menu e aberto e fechado em cada trecho medido */
#define TRECHOS_DO_MENU 4          /* Trechos medidos no teste de vazamento */
#define CICLOS_DE_AQUECIMENTO 25   /* Voltas antes de comecar a medir */
#define FOLGA_DE_MEMORIA_MB 16     /* Crescimento aceitavel em um trecho */

/* Declaracoes minimas do Windows para medir a memoria do processo. Nao incluimos
 * windows.h porque ele define Rectangle, CloseWindow e LoadImage, que colidem
 * com os nomes da Raylib. K32GetProcessMemoryInfo vive na kernel32. */
typedef struct
{
    unsigned long cb;
    unsigned long PageFaultCount;
    size_t PeakWorkingSetSize;
    size_t WorkingSetSize;
    size_t QuotaPeakPagedPoolUsage;
    size_t QuotaPagedPoolUsage;
    size_t QuotaPeakNonPagedPoolUsage;
    size_t QuotaNonPagedPoolUsage;
    size_t PagefileUsage;
    size_t PeakPagefileUsage;
} CONTADORES_DE_MEMORIA;

extern void *GetCurrentProcess(void);
extern int K32GetProcessMemoryInfo(void *processo, CONTADORES_DE_MEMORIA *contadores,
                                   unsigned long tamanho);

static size_t memoria_em_uso_mb(void)
{
    CONTADORES_DE_MEMORIA contadores;
    contadores.cb = sizeof(contadores);

    if (!K32GetProcessMemoryInfo(GetCurrentProcess(), &contadores, sizeof(contadores))) return 0;

    /* Memoria privada: o working set varia com o que o Windows decide manter na RAM. */
    return contadores.PagefileUsage / (1024 * 1024);
}

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
/* Roteiro do computador                                                      */
/* ------------------------------------------------------------------------ */

/* Volta a tarefa do dia para o comeco, como numa partida nova. */
static void reiniciar_tarefa(void)
{
    initialize_game_state_variables();
    initialize_hidden_statistic();
    current_step_index = 0;
    reset_step_state();
    step_arrival = 1.0f;
    current_wrong_answers = 0;
    unchecked_sends = 0;
    iara_memory = IARA_MEMORY_MAX;
    computer_tasks_finished = false;
    computer_failsafe_triggered = false;

    for (int i = 0; i < TICKET_COUNT; i++)
    {
        ticket_results[i] = (TaskResult){0};
    }
}

/* Chamado aberto no momento. */
static DialogStep *chamado_atual(void)
{
    return &dialog_sequence[current_step_index];
}

/* Coloca cada chamado da mesa no destino que a norma exige. */
static void preencher_a_mesa(bool certo)
{
    DialogStep *mesa = chamado_atual();

    for (int c = 0; c < mesa->card_count; c++)
    {
        int destino = get_triage_correct_choice(mesa, c);

        triage_choices[c] = certo ? destino : (destino == TRIAGE_TO_IT ? TRIAGE_TO_QUEUE : TRIAGE_TO_IT);
    }
}

/* Mesmo efeito de resolver o chamado certo e clicar em Continuar. */
static void resolver_sozinho(void)
{
    if (chamado_atual()->type == STEP_TRIAGE)
    {
        preencher_a_mesa(true);
        submit_triage();
    }
    else
    {
        answer_current_question(chamado_atual()->correct_option);
    }

    continue_after_answer();
}

/* Abre a IARA, confere marcando a verdade de cada afirmacao e decide pela resposta certa. */
static void resolver_com_a_iara(bool tarjar)
{
    ask_iara();

    if (current_ticket_view == TICKET_PRIVACY) decide_personal_data(tarjar);

    start_checking();

    if (chamado_atual()->type == STEP_QUIZ)
    {
        IaraSuggestion iara = get_iara_suggestion(current_step_index, ticket_memory_level);

        for (int i = 0; i < iara.claim_count; i++)
        {
            mark_claim(i, iara.claim_is_true[i] ? CLAIM_CONFIRMED : CLAIM_REJECTED);
        }

        finish_checking();
    }

    resolver_sozinho();
}

/* Abre a IARA e envia a sugestao como veio. */
static void enviar_sem_conferir(bool tarjar)
{
    ask_iara();

    if (current_ticket_view == TICKET_PRIVACY) decide_personal_data(tarjar);

    send_without_checking();
}

/* Passa pelas telas de abertura ate o primeiro chamado. */
static void ir_para_o_primeiro_chamado(void)
{
    while (chamado_atual()->type != STEP_QUIZ)
    {
        advance_step();
    }
}

/* Alternativa errada do chamado atual. */
static int opcao_errada(void)
{
    return (chamado_atual()->correct_option + 1) % chamado_atual()->option_count;
}

static void testar_modos_e_pontos(void)
{
    titulo("Formas de resolver, pontuacao, erro e colinha (CT-47 a CT-52, CT-57 e CT-58)");

    /* Sozinho: a IARA nunca precisa ser aberta. */
    reiniciar_tarefa();
    ir_para_o_primeiro_chamado();

    while (step_is_ticket(chamado_atual())) resolver_sozinho();

    checar("CT-47", "Resolver sozinho nao exige abrir a IARA e cada chamado vale 10 pontos",
           current_step_index == REPORT_STEP && current_game_state_variables.is_resolved &&
           current_game_state_variables.mode_type == MODE_MANUAL &&
           current_hidden_statistic.current_overall_points == 1 + (TICKET_COUNT * POINTS_SOLVED_ALONE) &&
           current_hidden_statistic.current_automation_points == 1 &&
           current_hidden_statistic.current_perception_points == 1);

    /* Com a IARA e com cuidado: vale o mesmo que sozinho, mais o bonus dos erros pegos. */
    reiniciar_tarefa();
    ir_para_o_primeiro_chamado();

    /* A memoria cai a cada conferencia: 6, 5, 4, 3 e 2. Sao 3 erros para pegar: a afirmacao
     * extra do chamado 3, o chamado 4 e a isca da mesa. */
    int erros_da_iara = 3;
    int erros_pegos = 0;

    while (step_is_ticket(chamado_atual())) resolver_com_a_iara(true);

    for (int i = 0; i < TICKET_COUNT; i++)
    {
        if (ticket_results[i].caught_iara_error) erros_pegos++;
    }

    checar("CT-48", "Conferir a IARA vale o mesmo que resolver sozinho, e pegar erro dela soma 5 e Percepcao",
           erros_pegos == erros_da_iara &&
           current_game_state_variables.mode_type == MODE_MANUAL &&
           ticket_results[0].outcome == TASK_SOLVED_WITH_IARA &&
           current_hidden_statistic.current_overall_points == 1 + (TICKET_COUNT * POINTS_SOLVED_WITH_IARA) + (erros_da_iara * POINTS_CAUGHT_IARA_ERROR) &&
           current_hidden_statistic.current_automation_points == 1 &&
           current_hidden_statistic.current_perception_points == 2);

    /* Sem conferir: conclui na hora, sem aviso, e o erro so aparece no relatorio. A memoria
     * cai 2 por vez: os chamados 1 e 2 saem certos, e os 3 seguintes ja saem errados. */
    reiniciar_tarefa();
    ir_para_o_primeiro_chamado();

    int passos_por_chamado = 0;
    bool sem_nova_tentativa = true;

    while (step_is_ticket(chamado_atual()))
    {
        int antes = current_step_index;

        enviar_sem_conferir(false);
        passos_por_chamado++;
        if (current_step_index != antes + 1) sem_nova_tentativa = false;
    }

    int relatos_de_erro = 0;

    for (int i = 0; i < TICKET_COUNT; i++)
    {
        if (ticket_went_wrong(i) && strcmp(get_ticket_report_line(i), dialog_sequence[FIRST_TICKET_STEP + i].wrong_result) == 0) relatos_de_erro++;
    }

    bool lista_de_privacidade = false;
    checar("CT-49", "Enviar sem conferir nao trava, vale 0, soma Automacao e o erro aparece no relatorio",
           passos_por_chamado == TICKET_COUNT && sem_nova_tentativa && relatos_de_erro == erros_da_iara &&
           current_game_state_variables.mode_type == MODE_AUTOMATIC &&
           current_hidden_statistic.current_overall_points == 1 &&
           current_hidden_statistic.current_automation_points == 2 &&
           current_hidden_statistic.current_perception_points == 1 &&
           get_privacy_report_line(&lista_de_privacidade) != NULL && lista_de_privacidade);

    /* Errar duas vezes mostra as duas dicas; acertar zera a contagem. */
    reiniciar_tarefa();
    ir_para_o_primeiro_chamado();

    int pergunta = current_step_index;

    answer_current_question(opcao_errada());
    bool primeira_dica = strcmp(get_wrong_answer_hint(), dialog_sequence[pergunta].hints[0]) == 0;
    continue_after_answer();
    bool nova_tentativa = current_step_index == pergunta && !current_quiz_answered;

    answer_current_question(opcao_errada());
    bool segunda_dica = strcmp(get_wrong_answer_hint(), dialog_sequence[pergunta].hints[1]) == 0;
    continue_after_answer();

    answer_current_question(dialog_sequence[pergunta].correct_option);
    bool zerou = current_wrong_answers == 0;
    continue_after_answer();

    checar("CT-50", "Erro mostra dica, abre nova tentativa e o acerto zera a contagem",
           primeira_dica && nova_tentativa && segunda_dica && zerou &&
           current_step_index == pergunta + 1 && !current_game_state_variables.is_resolved);

    /* Tres erros seguidos: a IARA conclui o que falta, sem pontos e sem contar como escolha. */
    answer_current_question(opcao_errada());
    continue_after_answer();
    answer_current_question(opcao_errada());
    continue_after_answer();
    answer_current_question(opcao_errada());
    bool texto_do_failsafe = strcmp(get_wrong_answer_hint(), FAILSAFE_HINT_TEXT) == 0;
    continue_after_answer();

    checar("CT-51", "Terceiro erro seguido aciona o failsafe: a IARA conclui o que falta e o jogador nao trava",
           texto_do_failsafe && computer_failsafe_triggered && current_step_index == REPORT_STEP &&
           current_game_state_variables.is_resolved &&
           current_game_state_variables.mode_type == MODE_AUTOMATIC &&
           ticket_results[0].outcome == TASK_SOLVED_ALONE &&
           ticket_results[TICKET_COUNT - 1].outcome == TASK_TAKEN_OVER_BY_IARA &&
           current_hidden_statistic.current_overall_points == 1 + POINTS_SOLVED_ALONE &&
           current_hidden_statistic.current_automation_points == 1);

    /* A colinha traz os 5 simbolos, e te-la a vista nao custa pontos. */
    int entradas_ok = 0;
    struct hidden_statistic antes = current_hidden_statistic;

    for (int i = 0; i < get_cheat_sheet_entry_count(); i++)
    {
        if (strlen(get_cheat_sheet_entry_text(i)) > 0) entradas_ok++;
    }

    reiniciar_tarefa();
    bool comeca_sem_colinha = !current_game_state_variables.has_cheat_sheet;

    antes = current_hidden_statistic;
    current_game_state_variables.has_cheat_sheet = true;

    BeginDrawing();
    draw_cheat_sheet((Rectangle){10, 10, 240, 180});
    EndDrawing();

    checar("CT-52", "Colinha traz os 5 simbolos, comeca fora do monitor e nao altera a pontuacao",
           entradas_ok == 5 && comeca_sem_colinha &&
           current_hidden_statistic.current_overall_points == antes.current_overall_points &&
           current_hidden_statistic.current_perception_points == antes.current_perception_points);

    /* Tarjar ou nao tarjar muda o relatorio, nunca os pontos. */
    reiniciar_tarefa();
    ir_para_o_primeiro_chamado();

    while (!chamado_atual()->has_personal_data) resolver_sozinho();

    ask_iara();
    bool pediu_tarja = current_ticket_view == TICKET_PRIVACY;
    decide_personal_data(true);
    bool foi_para_sugestao = current_ticket_view == TICKET_SUGGESTION;
    start_checking();
    mark_claim(0, CLAIM_CONFIRMED);
    mark_claim(1, CLAIM_CONFIRMED);
    finish_checking();
    bool sem_bonus_se_nao_apontou = !ticket_caught_error;
    resolver_sozinho();

    while (step_is_ticket(chamado_atual())) resolver_sozinho();

    bool privacidade_ruim = true;
    bool relatou_tarja = get_privacy_report_line(&privacidade_ruim) != NULL && !privacidade_ruim;

    bool pontos_sem_mudar = current_hidden_statistic.current_overall_points == 1 + (TICKET_COUNT * POINTS_SOLVED_ALONE);

    /* O failsafe nao apaga do relatorio o que o jogador ja fez com os CPFs. */
    reiniciar_tarefa();
    ir_para_o_primeiro_chamado();

    while (!chamado_atual()->has_personal_data) resolver_sozinho();

    ask_iara();
    decide_personal_data(false);
    start_checking();
    finish_checking();

    for (int erro = 0; erro < FAILSAFE_WRONG_ANSWERS; erro++)
    {
        answer_current_question(opcao_errada());
        continue_after_answer();
    }

    bool failsafe_mantem_tarja = computer_failsafe_triggered &&
                                 get_privacy_report_line(&privacidade_ruim) != NULL && privacidade_ruim;

    /* A chefia so cobra com um chamado aberto, e o cronometro nao tira o chamado do jogador. */
    reiniciar_tarefa();
    bool sem_cobranca_na_abertura = true;

    ticket_time = BOSS_SECOND_DELAY;
    sem_cobranca_na_abertura = get_boss_message() == NULL;
    ir_para_o_primeiro_chamado();

    int chamado_aberto = current_step_index;
    bool sem_cobranca_no_inicio = get_boss_message() == NULL;

    ticket_time = BOSS_FIRST_DELAY;
    bool primeira_cobranca = get_boss_message() != NULL;
    ticket_time = BOSS_SECOND_DELAY;
    bool segunda_cobranca = get_boss_message() != NULL;

    checar("CT-57", "Chamado com CPF pede a tarja, que vai ao relatorio sem mudar os pontos, e a chefia so pressiona",
           pediu_tarja && foi_para_sugestao && sem_bonus_se_nao_apontou && relatou_tarja && pontos_sem_mudar &&
           failsafe_mantem_tarja && sem_cobranca_na_abertura && sem_cobranca_no_inicio &&
           primeira_cobranca && segunda_cobranca && current_step_index == chamado_aberto);

    /* Investigar soma para quem resolveu algum chamado, e so uma vez por dia. */
    reiniciar_tarefa();
    current_game_state_variables.user_investigated = true;
    ir_para_o_primeiro_chamado();

    while (step_is_ticket(chamado_atual())) resolver_sozinho();

    resolve_day_tasks(ticket_results, TICKET_COUNT);
    bool investigou_e_resolveu = current_hidden_statistic.current_overall_points == 1 + (TICKET_COUNT * POINTS_SOLVED_ALONE) + POINTS_INVESTIGATED &&
                                 current_hidden_statistic.current_perception_points == 2;

    reiniciar_tarefa();
    current_game_state_variables.user_investigated = true;
    ir_para_o_primeiro_chamado();

    while (step_is_ticket(chamado_atual())) enviar_sem_conferir(true);

    checar("CT-58", "Investigar soma 5 para quem resolveu, nao conta duas vezes e soma Percepcao em todo caso",
           investigou_e_resolveu &&
           current_hidden_statistic.current_overall_points == 1 &&
           current_hidden_statistic.current_perception_points == 2);
}

/* Tabela-verdade dos 5 conectivos, a mesa de triagem e a memoria da IARA. */
static void testar_logica_e_memoria(void)
{
    titulo("Formulas, mesa de triagem e memoria da IARA (CT-61 a CT-63)");

    /* Linhas P, Q: VV, VF, FV, FF. */
    LogicExpression p = {.operator = LOGIC_VARIABLE, .variable = 0};
    LogicExpression q = {.operator = LOGIC_VARIABLE, .variable = 1};
    LogicExpression negacao = {.operator = LOGIC_NOT, .left = &p};
    LogicExpression conjuncao = {.operator = LOGIC_AND, .left = &p, .right = &q};
    LogicExpression disjuncao = {.operator = LOGIC_OR, .left = &p, .right = &q};
    LogicExpression condicional = {.operator = LOGIC_IMPLIES, .left = &p, .right = &q};
    LogicExpression bicondicional = {.operator = LOGIC_IFF, .left = &p, .right = &q};
    bool linhas[4][LOGIC_MAX_VARIABLES] = {{true, true}, {true, false}, {false, true}, {false, false}};
    struct { const LogicExpression *formula; bool esperado[4]; } tabela[] = {
        {&negacao, {false, false, true, true}},
        {&conjuncao, {true, false, false, false}},
        {&disjuncao, {true, true, true, false}},
        {&condicional, {true, false, true, true}},
        {&bicondicional, {true, false, false, true}},
    };
    int linhas_erradas = 0;

    for (int f = 0; f < 5; f++)
    {
        for (int l = 0; l < 4; l++)
        {
            if (logic_evaluate(tabela[f].formula, linhas[l]) != tabela[f].esperado[l]) linhas_erradas++;
        }
    }

    bool p_verdadeiro[LOGIC_MAX_VARIABLES] = {true, false};
    bool p_falso[LOGIC_MAX_VARIABLES] = {false, false};

    checar("CT-61", "Avaliador acerta a tabela-verdade dos 5 conectivos e aplica modus ponens",
           linhas_erradas == 0 && logic_forces_variable(&condicional, p_verdadeiro, 1) && !logic_forces_variable(&condicional, p_falso, 1));

    /* Mesa de triagem: so os urgentes vao ao TI, e nenhuma outra combinacao passa. */
    reiniciar_tarefa();

    while (chamado_atual()->type != STEP_TRIAGE) advance_step();

    DialogStep *mesa = chamado_atual();
    bool destinos_certos = get_triage_correct_choice(mesa, 0) == TRIAGE_TO_IT && get_triage_correct_choice(mesa, 1) == TRIAGE_TO_QUEUE &&
                           get_triage_correct_choice(mesa, 2) == TRIAGE_TO_IT && get_triage_correct_choice(mesa, 3) == TRIAGE_TO_QUEUE;
    int combinacoes_certas = 0;

    for (int combinacao = 0; combinacao < (1 << mesa->card_count); combinacao++)
    {
        for (int c = 0; c < mesa->card_count; c++)
        {
            triage_choices[c] = (combinacao >> c) & 1;
        }

        current_wrong_answers = 0;
        submit_triage();

        if (triage_wrong_cards == 0) combinacoes_certas++;
    }

    current_wrong_answers = 0;
    current_quiz_answered = false;

    /* Um destino errado: conta o erro, nao avanca e mantem o que o jogador marcou. */
    preencher_a_mesa(true);
    triage_choices[3] = TRIAGE_TO_IT;
    int passo_da_mesa = current_step_index;

    submit_triage();
    bool contou_um = triage_wrong_cards == 1 && current_wrong_answers == 1;
    continue_after_answer();
    bool manteve = current_step_index == passo_da_mesa && !current_quiz_answered && triage_choices[3] == TRIAGE_TO_IT;

    /* A IARA cai na isca, e corrigir o que ela marcou conta como erro pego. */
    bool iara_erra = !iara_answer_is_correct(passo_da_mesa, IARA_MEMORY_FULL);

    ask_iara();
    bool foi_a_sugestao = current_ticket_view == TICKET_SUGGESTION;
    start_checking();
    bool mesa_preenchida = triage_choices[3] == TRIAGE_TO_IT && current_ticket_view == TICKET_ANSWERING;
    preencher_a_mesa(true);
    submit_triage();
    bool pegou = ticket_caught_error && triage_wrong_cards == 0;

    checar("CT-62", "Mesa de triagem aceita uma unica combinacao, diz so quantos erros ha e a IARA erra a isca",
           destinos_certos && combinacoes_certas == 1 && contou_um && manteve && iara_erra && foi_a_sugestao && mesa_preenchida && pegou);

    /* Memoria: conferir gasta 1, enviar gasta 2, sozinho devolve 1. */
    reiniciar_tarefa();
    ir_para_o_primeiro_chamado();

    int passo = current_step_index;
    IaraSuggestion cheia = get_iara_suggestion(passo + 1, IARA_MEMORY_FULL);
    IaraSuggestion media = get_iara_suggestion(passo + 1, IARA_MEMORY_MEDIUM);
    IaraSuggestion baixa = get_iara_suggestion(passo + 1, IARA_MEMORY_LOW);

    bool niveis = cheia.claim_count == 2 && media.claim_count == 3 && baixa.claim_count == 3 &&
                  cheia.confidence < media.confidence && media.confidence < baixa.confidence &&
                  media.option == cheia.option && strstr(media.aside, dialog_sequence[passo].sender) != NULL &&
                  baixa.option != dialog_sequence[passo + 1].correct_option && !baixa.claim_is_true[0] && !baixa.claim_is_true[2];

    /* Todo chamado sai errado com pouca memoria, e toda sugestao errada tem o que apontar. */
    bool coerente = true;

    for (int t = FIRST_TICKET_STEP; t < FIRST_TICKET_STEP + TICKET_COUNT; t++)
    {
        if (iara_answer_is_correct(t, IARA_MEMORY_LOW)) coerente = false;
    }

    enviar_sem_conferir(true);
    bool gastou_dois = iara_memory == IARA_MEMORY_MAX - IARA_COST_UNCHECKED;
    resolver_sozinho();
    bool sozinho_devolve = iara_memory == IARA_MEMORY_MAX - IARA_COST_UNCHECKED + IARA_RECOVERY_ALONE;
    resolver_com_a_iara(true);
    bool conferir_gasta_um = iara_memory == IARA_MEMORY_MAX - IARA_COST_UNCHECKED + IARA_RECOVERY_ALONE - IARA_COST_CHECKED;

    /* Nova conversa enche a memoria, mas volta a pedir o chamado e a tarja, e o que vazou nao volta. */
    iara_memory = 2;
    ask_iara();
    bool nivel_baixo = ticket_memory_level == IARA_MEMORY_LOW;
    decide_personal_data(false);
    start_new_conversation();
    bool encheu = iara_memory == IARA_MEMORY_MAX && current_ticket_view == TICKET_PASTE;
    ask_iara();
    bool pediu_tarja_de_novo = current_ticket_view == TICKET_PRIVACY && ticket_memory_level == IARA_MEMORY_FULL;
    decide_personal_data(true);
    bool vazou_fica = ticket_exposed_data && !ticket_redacted_data;

    checar("CT-63", "Memoria da IARA cai com o uso, a confianca sobe, e nova conversa recupera sem apagar o que vazou",
           niveis && coerente && gastou_dois && sozinho_devolve && conferir_gasta_um && nivel_baixo && encheu && pediu_tarja_de_novo && vazou_fica);
}

static void testar_roteiro_do_quiz(void)
{
    titulo("Roteiro do computador (CT-17 a CT-20)");

    int problemas = 0;
    int perguntas = 0;
    int declaracoes = 0;

    for (int p = 0; p < TOTAL_STEPS; p++)
    {
        DialogStep *passo = &dialog_sequence[p];

        if (passo->line_count < 1 || passo->line_count > MAX_LINES)
        {
            printf("           passo %d: line_count invalido (%d)\n", p, passo->line_count);
            problemas++;
        }

        for (int l = 0; l < passo->line_count; l++)
        {
            if (strlen(passo->lines[l]) == 0)
            {
                printf("           passo %d, linha %d: texto vazio\n", p, l);
                problemas++;
            }
        }

        if (passo->type == STEP_QUIZ)
        {
            perguntas++;

            if (passo->option_count < 2 || passo->option_count > MAX_OPTIONS)
            {
                printf("           passo %d: option_count invalido (%d)\n", p, passo->option_count);
                problemas++;
            }

            if (passo->correct_option < 0 || passo->correct_option >= passo->option_count)
            {
                printf("           passo %d: correct_option fora da faixa (%d)\n",
                       p, passo->correct_option);
                problemas++;
            }

            /* As afirmacoes da IARA precisam bater com a sugestao: sugestao errada tem afirmacao falsa. */
            if (passo->low_option == passo->correct_option || strlen(passo->low_reason) == 0 || strlen(passo->low_claim) == 0 || passo->manual_page == 0)
            {
                printf("           passo %d: fala da IARA sem memoria incompleta\n", p);
                problemas++;
            }

            bool todas_verdadeiras = passo->claim_is_true[0] && passo->claim_is_true[1];

            if (passo->iara_option < 0 || passo->iara_option >= passo->option_count ||
                (passo->iara_option == passo->correct_option) != todas_verdadeiras ||
                passo->source_count < 1 || strlen(passo->wrong_result) == 0 || strlen(passo->sender) == 0)
            {
                printf("           passo %d: sugestao da IARA incoerente com o gabarito\n", p);
                problemas++;
            }

            for (int d = 0; d < MAX_HINTS; d++)
            {
                if (strlen(passo->hints[d]) == 0)
                {
                    printf("           passo %d, dica %d: texto vazio\n", p, d);
                    problemas++;
                }
            }

            for (int o = 0; o < passo->option_count; o++)
            {
                if (strlen(passo->options[o]) == 0)
                {
                    printf("           passo %d, opcao %d: texto vazio\n", p, o);
                    problemas++;
                }

                /* Duas alternativas iguais deixariam a pergunta sem resposta unica. */
                for (int outra = o + 1; outra < passo->option_count; outra++)
                {
                    if (strcmp(passo->options[o], passo->options[outra]) == 0)
                    {
                        printf("           passo %d: opcoes %d e %d sao iguais\n", p, o, outra);
                        problemas++;
                    }
                }
            }
        }
        else if (passo->type == STEP_TRIAGE)
        {
            perguntas++;

            if (passo->card_count < 2 || passo->card_count > MAX_TRIAGE_CARDS || passo->norm == NULL ||
                strlen(passo->hints[0]) == 0 || strlen(passo->hints[1]) == 0 || strlen(passo->wrong_result) == 0)
            {
                printf("           passo %d: mesa de triagem incompleta\n", p);
                problemas++;
            }
        }
        else
        {
            declaracoes++;

            if (passo->button_name == NULL || strlen(passo->button_name) == 0)
            {
                printf("           passo %d: declaracao sem nome de botao\n", p);
                problemas++;
            }
        }
    }

    printf("           %d passos no roteiro: %d declaracoes e %d perguntas\n",
           TOTAL_STEPS, declaracoes, perguntas);
    checar("CT-17", "Roteiro do computador integro (textos, opcoes e gabarito)", problemas == 0);

    checar("CT-18", "Roteiro abre com uma tela de texto e fecha com o relatorio do dia",
           TOTAL_STEPS >= 2 &&
           dialog_sequence[0].type == STEP_STATEMENT &&
           dialog_sequence[TOTAL_STEPS - 1].type == STEP_REPORT);

    /* Percorre o roteiro inteiro como um jogador: responde cada pergunta e avanca. */
    reiniciar_tarefa();

    int avancos = 0;
    int respondidas = 0;

    while (current_step_index < TOTAL_STEPS && avancos <= TOTAL_STEPS)
    {
        DialogStep *passo = &dialog_sequence[current_step_index];

        if (passo->type == STEP_QUIZ)
        {
            /* Equivale a clicar numa alternativa e depois em Continuar. */
            answer_current_question(passo->correct_option);
            continue_after_answer();
            respondidas++;
        }
        else if (passo->type == STEP_TRIAGE)
        {
            resolver_sozinho();
            respondidas++;
        }
        else
        {
            advance_step();
        }

        avancos++;
    }

    printf("           %d avancos ate o fim; %d perguntas respondidas\n", avancos, respondidas);
    checar("CT-19", "Jogador percorre o roteiro do inicio ao fim",
           avancos == TOTAL_STEPS && respondidas == perguntas);

    /* Passado o ultimo passo, a tela do computador se da por encerrada. */
    BeginDrawing();
    render_current_step();
    EndDrawing();

    checar("CT-20", "Fim do roteiro encerra a tela do computador", computer_tasks_finished);
}

/* Desenha um fundo cinza com o brilho atual e devolve a cor resultante. */
/* Conta quantas marcacoes de uma mesa o jogo aceita, testando todas. */
static int contar_mesas_aceitas(const DialogStep *mesa)
{
    int choices[MAX_TRIAGE_CARDS] = {0};
    int aceitas = 0;

    for (int combinacao = 0; combinacao < (1 << mesa->card_count); combinacao++)
    {
        for (int c = 0; c < mesa->card_count; c++) choices[c] = (combinacao >> c) & 1;

        if (count_board_wrong(mesa, choices) == 0) aceitas++;
    }

    return aceitas;
}

/* ------------------------------------------------------------------------ */
/* US03: manual ou automatico em cada tarefa jogavel (CT-77 e CT-78)          */
/* ------------------------------------------------------------------------ */

#define SO_SOZINHO 0      /* Resolve todo chamado sem abrir a IARA */
#define SO_SEM_CONFERIR 1 /* Abre a IARA e envia toda sugestao como veio */

/* Resolve a mesa sozinho com a primeira marcacao aceita. O treino e a prova nao
 * tem destino por linha, entao a resposta sai da propria regra da mesa. */
static void resolver_a_mesa_sozinho(void)
{
    DialogStep *mesa = chamado_atual();

    for (int combinacao = 0; combinacao < (1 << mesa->card_count); combinacao++)
    {
        for (int c = 0; c < mesa->card_count; c++) triage_choices[c] = (combinacao >> c) & 1;

        if (count_board_wrong(mesa, triage_choices) == 0) break;
    }

    submit_triage();
    continue_after_answer();
}

/* Joga um dia inteiro usando sempre a mesma forma de resolver. Devolve false se algum
 * chamado nao ofereceu as duas formas, se a IARA nao respondeu na hora ou se um
 * chamado precisou de mais de uma acao para andar. */
static bool jogar_o_dia_com_uma_forma(int dia, int forma, bool sem_memoria, bool tarjar)
{
    reiniciar_tarefa();
    start_game_day(dia);
    reset_computer_progress();
    current_game_state_variables.evidence_mask = EVIDENCE_NO_RESIGNATION_LETTER | EVIDENCE_BADGE_AFTER_LEAVING | EVIDENCE_NO_HUMAN_ORDER;

    if (sem_memoria) iara_memory = 0;

    bool tudo_certo = true;
    int passos = 0;

    while (current_step_index < REPORT_STEP && passos++ <= TOTAL_STEPS)
    {
        DialogStep *passo = chamado_atual();
        int antes = current_step_index;

        if (!step_is_ticket(passo))
        {
            if (passo->type == STEP_CHOICE) choose_day_option(passo, 0);
            else advance_step();
            continue;
        }

        /* As duas formas a vista: as alternativas ou a mesa, e o botao "Perguntar a IARA". */
        if (current_ticket_view != TICKET_ANSWERING || ticket_asked_iara || current_quiz_answered) tudo_certo = false;

        if (forma == SO_SOZINHO)
        {
            if (passo->type == STEP_TRIAGE) resolver_a_mesa_sozinho();
            else resolver_sozinho();
        }
        else
        {
            ask_iara();

            /* Instantaneo: a sugestao aparece no mesmo clique, sem tela de espera. */
            if (current_ticket_view != TICKET_SUGGESTION && current_ticket_view != TICKET_PRIVACY) tudo_certo = false;
            if (current_ticket_view == TICKET_PRIVACY) decide_personal_data(tarjar);
            if (current_ticket_view != TICKET_SUGGESTION) tudo_certo = false;

            send_without_checking();
        }

        if (current_step_index != antes + 1 || current_wrong_answers != 0) tudo_certo = false;
    }

    return tudo_certo && current_step_index == REPORT_STEP && current_game_state_variables.is_resolved &&
           !computer_failsafe_triggered;
}

static void testar_manual_ou_automatico(void)
{
    titulo("US03: manual ou automatico em todos os dias jogaveis (CT-77 e CT-78)");

    bool manual_em_todos = true;
    bool automatico_em_todos = true;
    int dias = 0;
    int partidas_automaticas = 0;

    for (int s = 0; s < DAY_SCRIPT_COUNT; s++)
    {
        int dia = day_scripts[s].day;

        /* Manual: o resultado depende do acerto, vale pontos e nao conta como Automacao. */
        bool manual_ok = jogar_o_dia_com_uma_forma(dia, SO_SOZINHO, false, true);
        int automacao_da_escolha = (day_choice_effects & DAY_EFFECT_AUTOMATION) ? 1 : 0;

        manual_ok = manual_ok && current_game_state_variables.mode_type == MODE_MANUAL &&
                    current_hidden_statistic.current_overall_points == HIDDEN_STATISTIC_START + (TICKET_COUNT * POINTS_SOLVED_ALONE) &&
                    current_hidden_statistic.current_automation_points == HIDDEN_STATISTIC_START + automacao_da_escolha;

        for (int i = 0; i < TICKET_COUNT; i++)
        {
            if (ticket_results[i].outcome != TASK_SOLVED_ALONE) manual_ok = false;
        }

        /* Automatico: com e sem memoria, tarjando ou nao. Nunca trava e nunca da recompensa. */
        bool automatico_ok = true;

        for (int variacao = 0; variacao < 4; variacao++)
        {
            bool sem_memoria = (variacao & 1) != 0;
            bool tarjar = (variacao & 2) != 0;
            bool rodou = jogar_o_dia_com_uma_forma(dia, SO_SEM_CONFERIR, sem_memoria, tarjar);

            rodou = rodou && current_game_state_variables.mode_type == MODE_AUTOMATIC &&
                    current_hidden_statistic.current_overall_points == HIDDEN_STATISTIC_START &&
                    current_hidden_statistic.current_automation_points >= HIDDEN_STATISTIC_START + 1 &&
                    unchecked_sends == TICKET_COUNT;

            for (int i = 0; i < TICKET_COUNT; i++)
            {
                if (ticket_results[i].outcome != TASK_SENT_UNCHECKED) rodou = false;
            }

            if (!rodou) automatico_ok = false;
            partidas_automaticas++;
        }

        printf("           Dia %3d: %d chamado(s), manual %s, automatico %s\n", dia, TICKET_COUNT,
               manual_ok ? "ok" : "FALHOU", automatico_ok ? "ok nas 4 variacoes" : "FALHOU");

        if (!manual_ok) manual_em_todos = false;
        if (!automatico_ok) automatico_em_todos = false;
        dias++;
    }

    checar("CT-77", "Todo chamado dos 9 dias jogaveis oferece resolver sozinho ou pedir a IARA, e o manual vale pontos",
           manual_em_todos && dias == 9);
    checar("CT-78", "Modo automatico resolve na hora, nunca trava nem aciona o failsafe e vale 0 em todos os dias",
           automatico_em_todos && partidas_automaticas == 9 * 4);

    select_day_script(1);
    reiniciar_tarefa();
}

/* Calendario da campanha, gabaritos calculados e finais (CT-64 a CT-70). */
static void testar_dias_seguintes(void)
{
    titulo("Dias seguintes da campanha (CT-64 a CT-70)");

    static const int jogaveis[] = {1, 3, 12, 27, 41, 58, 74, 89, 100};
    int dias_jogaveis = 0;
    int dias_de_flash = 0;
    bool jogaveis_certos = true;

    for (int dia = 1; dia <= TOTAL_GAME_DAYS; dia++)
    {
        int tipo = calendar_get_day_type(dia);

        if (tipo == PLAYABLE_DAY)
        {
            if (dias_jogaveis >= 9 || jogaveis[dias_jogaveis] != dia || calendar_get_arrival_line(dia, 0)[0] == '\0') jogaveis_certos = false;
            dias_jogaveis++;
        }

        if (tipo == FLASH_DAY) dias_de_flash++;
    }

    printf("           %d dias jogaveis, %d flashes, %d saltos\n", dias_jogaveis, dias_de_flash, TOTAL_GAME_DAYS - dias_jogaveis - dias_de_flash);

    checar("CT-64", "Calendario tem os 9 dias jogaveis do GDD, o Dia 2 e um flash e o Dia 4 abre um bloco de salto",
           jogaveis_certos && dias_jogaveis == 9 && calendar_get_day_type(2) == FLASH_DAY &&
           calendar_get_day_type(4) == JUMP_DAY && calendar_get_jump_block_length(4) == 3 &&
           calendar_get_water_level(1) == 0.0f && calendar_get_water_level(TOTAL_GAME_DAYS) == 1.0f);

    /* De Morgan, dupla negacao, tautologia, satisfazibilidade e silogismo. */
    const LogicExpression *p = L_VAR(0);
    const LogicExpression *q = L_VAR(1);
    const LogicExpression *contraditorias[] = {p, L_NOT(p)};
    const LogicExpression *consistentes[] = {p, L_IMPLIES(p, q)};
    CategoricalStatement premissas[] = {{CATEGORICAL_ALL, 0, 1}, {CATEGORICAL_ALL, 1, 2}};

    checar("CT-65", "Avaliador reconhece equivalencia, tautologia, contradicao e silogismo valido",
           logic_are_equivalent(L_NOT(L_AND(p, q)), L_OR(L_NOT(p), L_NOT(q)), 2) &&
           !logic_are_equivalent(L_NOT(L_OR(p, q)), L_OR(L_NOT(p), L_NOT(q)), 2) &&
           logic_are_equivalent(L_NOT(L_NOT(p)), p, 1) &&
           logic_is_tautology(L_OR(p, L_NOT(p)), 1) && !logic_is_tautology(p, 1) &&
           !logic_is_satisfiable(contraditorias, 2, 1) && logic_is_satisfiable(consistentes, 2, 2) &&
           logic_conclusion_follows(premissas, 2, (CategoricalStatement){CATEGORICAL_ALL, 0, 2}) &&
           !logic_conclusion_follows(premissas, 2, (CategoricalStatement){CATEGORICAL_ALL, 2, 0}) &&
           !logic_conclusion_follows(premissas, 2, (CategoricalStatement){CATEGORICAL_SOME, 0, 2}));

    /* Cada puzzle tem uma so marcacao aceita, sem multipla escolha, e a IARA erra quando deve. */
    current_game_state_variables.evidence_mask = EVIDENCE_NO_RESIGNATION_LETTER | EVIDENCE_BADGE_AFTER_LEAVING | EVIDENCE_NO_HUMAN_ORDER;

    bool gabaritos = true;
    bool iara_certa = true;
    bool iara_sem_memoria_erra = true;

    for (int s = 1; s < DAY_SCRIPT_COUNT; s++)
    {
        select_day_script(day_scripts[s].day);

        DialogStep *mesa = &dialog_sequence[FIRST_TICKET_STEP];
        int aceitas = contar_mesas_aceitas(mesa);
        bool iara_cheia = iara_answer_is_correct(FIRST_TICKET_STEP, IARA_MEMORY_FULL);
        bool deve_acertar = day_scripts[s].day == 3 || day_scripts[s].day == 27;

        printf("           Dia %d: %d marcacao(oes) aceita(s) de %d, IARA cheia %s\n", day_scripts[s].day, aceitas, 1 << mesa->card_count, iara_cheia ? "acerta" : "erra");

        /* A prova do Dia 100 aceita um par de cada, ou D, D -> C e nao C: 3 refutacoes minimas. */
        if (aceitas != ((mesa->board_mode == BOARD_REFUTATION) ? 3 : 1) || mesa->type != STEP_TRIAGE) gabaritos = false;
        if (iara_cheia != deve_acertar) iara_certa = false;
        if (iara_answer_is_correct(FIRST_TICKET_STEP, IARA_MEMORY_LOW)) iara_sem_memoria_erra = false;
    }

    checar("CT-66", "Cada puzzle dos dias 3 a 100 tem gabarito calculado e unico, sem multipla escolha",
           gabaritos);
    checar("CT-67", "IARA acerta os dias 3 e 27, erra os outros com memoria cheia e erra todos sem memoria",
           iara_certa && iara_sem_memoria_erra);

    /* Dia 41: conjuntos de treino diferentes mudam a mini-IA (CA3). */
    select_day_script(41);

    DialogStep *treino = &dialog_sequence[FIRST_TICKET_STEP];
    int vies[MAX_TRIAGE_CARDS] = {1, 1, 1, 0, 0, 0};
    int justo[MAX_TRIAGE_CARDS] = {1, 0, 1, 1, 0, 1};

    checar("CT-68", "Treinos diferentes mudam o resultado da mini-IA, e o vies deixa a Periferia sem aprovacao",
           get_mini_ia_approval(treino, vies, true) == 0 && get_mini_ia_approval(treino, vies, false) == 66 &&
           get_mini_ia_approval(treino, justo, true) == 50 && get_mini_ia_approval(treino, justo, false) == 50 &&
           count_board_wrong(treino, justo) == 0 && count_board_wrong(treino, vies) > 0);

    /* Dia 12: a decisao muda Automacao ou Percepcao (CA2), so uma vez e so ao concluir o dia. */
    reiniciar_tarefa();
    start_game_day(12);
    reset_computer_progress();
    current_step_index = FIRST_TICKET_STEP;
    reset_step_state();
    step_arrival = 1.0f;

    DialogStep *conciliacao = chamado_atual();

    for (int c = 0; c < conciliacao->card_count; c++) triage_choices[c] = get_triage_correct_choice(conciliacao, c);

    submit_triage();
    continue_after_answer();

    int automacao_antes = current_hidden_statistic.current_automation_points;
    bool antes_de_concluir_nada_muda = true;

    choose_day_option(chamado_atual(), 1);
    if (current_hidden_statistic.current_perception_points != HIDDEN_STATISTIC_START + 1 ||
        !(current_game_state_variables.evidence_mask & EVIDENCE_NO_RESIGNATION_LETTER)) antes_de_concluir_nada_muda = false;

    int pediu_revisao_automacao = current_hidden_statistic.current_automation_points;

    reiniciar_tarefa();
    start_game_day(12);
    reset_computer_progress();
    current_step_index = FIRST_TICKET_STEP + 1;
    reset_step_state();
    choose_day_option(chamado_atual(), 0);

    /* Quem delega muito ja comeca o dia com uma IARA pior. */
    current_hidden_statistic.current_automation_points = HIDDEN_STATISTIC_START + 2;
    start_game_day(27);
    reset_computer_progress();

    int memoria_depois_de_delegar = iara_memory;

    checar("CT-69", "Decisao do Dia 12 muda Automacao ou Percepcao, e dias delegados reduzem a memoria inicial da IARA",
           antes_de_concluir_nada_muda && pediu_revisao_automacao == automacao_antes &&
           memoria_depois_de_delegar == IARA_MEMORY_MAX - 2);

    /* Os 4 finais, com as condicoes da secao 6.5 do GDD. */
    int finais_certos = 0;
    int casos_de_final = 0;

    for (int automacao = 0; automacao <= 1; automacao++)
    for (int percepcao = 0; percepcao <= 1; percepcao++)
    for (int vinculo = 0; vinculo <= 1; vinculo++)
    for (int mariana = 0; mariana <= 1; mariana++)
    for (int prova = 0; prova <= 1; prova++)
    {
        GameEnding esperado = ENDING_HYPNOTIZED;

        if (!percepcao) esperado = automacao ? ENDING_PAMONHA : ENDING_UNHARMED;
        else if (!automacao && vinculo && mariana && prova) esperado = ENDING_RUPTURE;

        current_hidden_statistic.current_automation_points = HIDDEN_STATISTIC_START + (automacao ? AUTOMATION_HIGH_TOTAL : AUTOMATION_HIGH_TOTAL - 1);
        current_hidden_statistic.current_perception_points = HIDDEN_STATISTIC_START + (percepcao ? PERCEPTION_HIGH_TOTAL : PERCEPTION_HIGH_TOTAL - 1);
        current_hidden_statistic.current_human_bond_points = HIDDEN_STATISTIC_START + (vinculo ? HUMAN_BOND_HIGH_TOTAL : HUMAN_BOND_HIGH_TOTAL - 1);
        current_game_state_variables.found_mariana = mariana;
        current_game_state_variables.proof_succeeded = prova;

        casos_de_final++;

        if (get_game_ending() == esperado && get_game_ending() == esperado) finais_certos++;
    }

    checar("CT-70", "Os 4 finais saem das condicoes do GDD, sem sorte, em todas as combinacoes",
           finais_certos == casos_de_final && casos_de_final == 32);

    /* Failsafe em cada puzzle: 3 erros levam ao relatorio, sem prender o jogador (RF05). */
    bool failsafe_em_todos = true;

    for (int s = 1; s < DAY_SCRIPT_COUNT; s++)
    {
        reiniciar_tarefa();
        start_game_day(day_scripts[s].day);
        reset_computer_progress();
        current_step_index = FIRST_TICKET_STEP;
        reset_step_state();

        for (int erro = 0; erro < FAILSAFE_WRONG_ANSWERS; erro++)
        {
            for (int c = 0; c < MAX_TRIAGE_CARDS; c++) triage_choices[c] = 0;

            submit_triage();
            if (triage_wrong_cards == 0) failsafe_em_todos = false;
            continue_after_answer();
        }

        if (current_step_index != REPORT_STEP || !computer_failsafe_triggered ||
            !current_game_state_variables.is_resolved || current_game_state_variables.mode_type != MODE_AUTOMATIC ||
            ticket_results[0].outcome != TASK_TAKEN_OVER_BY_IARA || current_game_state_variables.proof_succeeded ||
            current_hidden_statistic.current_automation_points != HIDDEN_STATISTIC_START)
        {
            failsafe_em_todos = false;
        }
    }

    checar("CT-74", "Tres erros em qualquer puzzle acionam o failsafe, vao ao relatorio e nao contam como prova",
           failsafe_em_todos);

    select_day_script(1);
    reiniciar_tarefa();
}

static Color cor_com_brilho_atual(void)
{
    RenderTexture2D alvo = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

    BeginTextureMode(alvo);
    ClearBackground((Color){128, 128, 128, 255});
    draw_brightness_overlay();
    EndTextureMode();

    Image quadro = LoadImageFromTexture(alvo.texture);
    Color cor = GetImageColor(quadro, 10, 10);

    UnloadImage(quadro);
    UnloadRenderTexture(alvo);

    return cor;
}

/* ------------------------------------------------------------------------ */
/* Menu principal                                                             */
/* ------------------------------------------------------------------------ */

static void testar_menu(void)
{
    titulo("Menu principal (CT-21 a CT-23)");

    initialize_menu_variables();

    checar("CT-21", "Menu abre em INICIAR, destravado",
           selected_menu_option == INICIAR && !is_menu_to_close && next_screen == NONE);

    /* Descer cinco vezes deve voltar para INICIAR (lista circular). */
    move_menu_selection(1);
    bool desceu_para_creditos = (selected_menu_option == CREDITOS);

    move_menu_selection(1);
    bool desceu_para_configuracoes = (selected_menu_option == CONFIGURACOES);

    move_menu_selection(1);
    bool desceu_para_ajuda = (selected_menu_option == AJUDA);

    move_menu_selection(1);
    bool desceu_para_sair = (selected_menu_option == SAIR);

    move_menu_selection(1);
    bool voltou_ao_topo = (selected_menu_option == INICIAR);

    /* Subir a partir do topo deve ir para o fim da lista. */
    move_menu_selection(-1);
    bool subiu_para_o_fim = (selected_menu_option == SAIR);

    checar("CT-22", "Selecao do menu circula nos dois sentidos",
           desceu_para_creditos && desceu_para_configuracoes && desceu_para_ajuda && desceu_para_sair && voltou_ao_topo && subiu_para_o_fim);

    /* SAIR pergunta antes de fechar: escolher a opcao abre a confirmacao e ainda nao pede o fechamento. */
    selected_menu_option = SAIR;
    confirm_menu_option();
    checar("UX-13", "Escolher SAIR no menu abre a pergunta e nao fecha o menu na hora",
           confirm_is_open() && !is_menu_to_close && next_screen == NONE);
    confirm_close();

    /* AJUDA vai para a tela de ajuda como as outras opcoes. */
    selected_menu_option = AJUDA;
    confirm_menu_option();
    checar("UX-14", "Escolher AJUDA no menu pede a tela de ajuda", !confirm_is_open() && is_menu_to_close && next_screen == AJUDA);
    initialize_menu_variables();

    /* As caixas da tela de configuracoes mudam na hora, e so VOLTAR sai dela. */
    bool saiu_ao_ligar = activate_settings_option(SETTINGS_CONTRAST);
    bool ligou = is_high_contrast_enabled() && is_high_contrast_filter_ready() && !saiu_ao_ligar;

    bool saiu_ao_desligar = activate_settings_option(SETTINGS_CONTRAST);
    bool desligou = !is_high_contrast_enabled() && !saiu_ao_desligar;

    checar("CT-37", "Alto contraste liga e desliga na tela de configuracoes", ligou && desligou && activate_settings_option(SETTINGS_BACK));

    /* O brilho respeita os limites e muda de verdade o que aparece na tela. */
    set_game_brightness(9.0f);
    bool teto = get_game_brightness() == BRIGHTNESS_MAX;
    Color claro = cor_com_brilho_atual();

    set_game_brightness(0.0f);
    bool piso = get_game_brightness() == BRIGHTNESS_MIN;
    Color escuro = cor_com_brilho_atual();

    set_game_brightness(1.0f);
    Color normal = cor_com_brilho_atual();

    checar("CT-41", "Brilho respeita os limites e altera a imagem",
           teto && piso && escuro.r < normal.r && claro.r > normal.r);

    /* As preferencias voltam iguais do arquivo, e um arquivo ruim nao derruba o jogo. */
    const char *arquivo = "build/tests/preferencias-teste.ini";

    set_game_brightness(0.7f);
    toggle_high_contrast();
    bool gravou = save_preferences(arquivo);

    set_game_brightness(1.0f);
    toggle_high_contrast();
    bool leu = load_preferences(arquivo);
    bool voltou_igual = leu && is_high_contrast_enabled() && (int)((get_game_brightness() * 10.0f) + 0.5f) == 7;

    toggle_high_contrast();
    set_game_brightness(1.0f);

    SaveFileText(arquivo, "lixo\nbrilho=abc\nbrilho=nan\nalto_contraste=\n");
    bool leu_invalido = load_preferences(arquivo);
    bool valores_validos = get_game_brightness() >= BRIGHTNESS_MIN && get_game_brightness() <= BRIGHTNESS_MAX && !is_high_contrast_enabled();

    remove(arquivo);
    bool sem_arquivo = !load_preferences(arquivo);

    set_game_brightness(1.0f);

    checar("CT-43", "Preferencias voltam do arquivo e arquivo invalido nao quebra o jogo",
           gravou && voltou_igual && leu_invalido && valores_validos && sem_arquivo);

    /* Confirmar guarda a escolha e sinaliza o fechamento do menu. */
    selected_menu_option = CREDITOS;
    confirm_menu_option();

    checar("CT-23", "Confirmar guarda a opcao escolhida e fecha o menu",
           is_menu_to_close && next_screen == CREDITOS);

    unload_menu();
}

/* ------------------------------------------------------------------------ */
/* Vazamento de texturas ao reabrir o menu                                    */
/* ------------------------------------------------------------------------ */

/* O jogo volta ao menu depois dos creditos, chamando initialize_menu_variables()
 * de novo. Se o que foi carregado nao for todo devolvido, cada volta ao menu
 * consome memoria que nunca mais volta. Aqui medimos isso de verdade. */
static void reabrir_menu(void)
{
    /* O driver so devolve uma textura apagada depois de um quadro, como no jogo real. */
    initialize_menu_variables();
    BeginDrawing();
    EndDrawing();
    unload_menu();
    BeginDrawing();
    EndDrawing();
}

static void testar_ciclo_do_menu(void)
{
    titulo("Reabertura do menu (CT-24)");

    /* Voltas fora da medicao: o driver de video monta os proprios caches nas primeiras. */
    for (int ciclo = 0; ciclo < CICLOS_DE_AQUECIMENTO; ciclo++)
    {
        reabrir_menu();
    }

    /* Vazamento cresce em todo trecho. O cache do driver salta uma vez e para, entao um     * salto isolado nao reprova, e uma unica comparacao de antes e depois nao serviria. */
    size_t anterior = memoria_em_uso_mb();
    int trechos_crescendo = 0;

    for (int trecho = 0; trecho < TRECHOS_DO_MENU; trecho++)
    {
        for (int ciclo = 0; ciclo < CICLOS_POR_TRECHO; ciclo++)
        {
            reabrir_menu();
        }

        size_t atual = memoria_em_uso_mb();

        printf("           trecho %d: %zu MB\n", trecho + 1, atual);

        if (atual > anterior + FOLGA_DE_MEMORIA_MB) trechos_crescendo++;

        anterior = atual;
    }

    printf("           trechos crescendo: %d de %d (limite por trecho: %d MB)\n",
           trechos_crescendo, TRECHOS_DO_MENU, FOLGA_DE_MEMORIA_MB);

    checar("CT-24", "Abrir e fechar o menu varias vezes nao acumula memoria",
           trechos_crescendo <= 1);
}

int main(void)
{
    SetTraceLogLevel(LOG_WARNING);

    abrir_janela_de_teste(LARGURA_JANELA, ALTURA_JANELA, "IARA - testes de conteudo");

    screen_width = GetScreenWidth();
    screen_height = GetScreenHeight();
    middle_screen_x = screen_width / 2;
    middle_screen_y = screen_height / 2;

    printf("\n========================================================================\n");
    printf(" IARA - TESTES DE SISTEMA | Bloco 2: quiz do computador e menu\n");
    printf("========================================================================\n");

    initialize_fonts();

    testar_roteiro_do_quiz();
    testar_logica_e_memoria();
    testar_dias_seguintes();
    testar_modos_e_pontos();
    testar_manual_ou_automatico();
    testar_menu();
    testar_ciclo_do_menu();

    printf("\n------------------------------------------------------------------------\n");
    printf(" Bloco 2: %d de %d casos aprovados", casos_ok, total_casos);

    if (casos_ok != total_casos) printf(" | %d FALHA(S)", total_casos - casos_ok);

    printf("\n========================================================================\n\n");

    unload_fonts();
    CloseWindow();

    return (casos_ok == total_casos) ? 0 : 1;
}
