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

/* A main.c define estas globais; aqui o teste assume esse papel. */
unsigned int screen_width = 0;
unsigned int screen_height = 0;
unsigned int middle_screen_x = 0;
unsigned int middle_screen_y = 0;

/* Inclui os .c para alcancar o roteiro e o estado interno, que sao estaticos. */
#include "computer_loop.c"
#include "menu.c"

#define LARGURA_JANELA 1280
#define ALTURA_JANELA 720

#define CICLOS_DO_MENU 20          /* Vezes que o menu e aberto e fechado no teste de vazamento */
#define FOLGA_DE_MEMORIA_MB 16     /* Crescimento aceitavel de memoria nesses ciclos */

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

    return contadores.WorkingSetSize / (1024 * 1024);
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

    checar("CT-18", "Roteiro abre e fecha com uma tela de texto",
           TOTAL_STEPS >= 2 &&
           dialog_sequence[0].type == STEP_STATEMENT &&
           dialog_sequence[TOTAL_STEPS - 1].type == STEP_STATEMENT);

    /* Percorre o roteiro inteiro como um jogador: responde cada pergunta e avanca. */
    current_step_index = 0;
    current_quiz_answered = false;
    current_quiz_selected = -1;
    computer_tasks_finished = false;

    int avancos = 0;
    int respondidas = 0;

    while (current_step_index < TOTAL_STEPS && avancos <= TOTAL_STEPS)
    {
        DialogStep *passo = &dialog_sequence[current_step_index];

        if (passo->type == STEP_QUIZ)
        {
            /* Equivale a clicar numa alternativa e depois em Continuar. */
            current_quiz_answered = true;
            current_quiz_selected = passo->correct_option;
            respondidas++;
        }

        current_step_index++;
        current_quiz_answered = false;
        current_quiz_selected = -1;
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

/* ------------------------------------------------------------------------ */
/* Menu principal                                                             */
/* ------------------------------------------------------------------------ */

static void testar_menu(void)
{
    titulo("Menu principal (CT-21 a CT-23)");

    initialize_menu_variables();

    checar("CT-21", "Menu abre em INICIAR, destravado",
           selected_menu_option == INICIAR && !is_menu_to_close && next_screen == NONE);

    /* Descer tres vezes deve voltar para INICIAR (lista circular). */
    move_menu_selection(1);
    bool desceu_para_creditos = (selected_menu_option == CREDITOS);

    move_menu_selection(1);
    bool desceu_para_sair = (selected_menu_option == SAIR);

    move_menu_selection(1);
    bool voltou_ao_topo = (selected_menu_option == INICIAR);

    /* Subir a partir do topo deve ir para o fim da lista. */
    move_menu_selection(-1);
    bool subiu_para_o_fim = (selected_menu_option == SAIR);

    checar("CT-22", "Selecao do menu circula nos dois sentidos",
           desceu_para_creditos && desceu_para_sair && voltou_ao_topo && subiu_para_o_fim);

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
static void testar_ciclo_do_menu(void)
{
    titulo("Reabertura do menu (CT-24)");

    /* Primeira volta fora da medicao: alocacoes legitimas que so acontecem uma vez. */
    initialize_menu_variables();
    unload_menu();

    size_t antes = memoria_em_uso_mb();

    for (int ciclo = 0; ciclo < CICLOS_DO_MENU; ciclo++)
    {
        initialize_menu_variables();
        unload_menu();
    }

    size_t depois = memoria_em_uso_mb();
    size_t crescimento = (depois > antes) ? depois - antes : 0;

    printf("           memoria do processo: %zu MB antes, %zu MB depois de %d ciclos\n",
           antes, depois, CICLOS_DO_MENU);
    printf("           crescimento de %zu MB (limite aceitavel: %d MB)\n",
           crescimento, FOLGA_DE_MEMORIA_MB);

    checar("CT-24", "Abrir e fechar o menu varias vezes nao acumula memoria",
           crescimento <= FOLGA_DE_MEMORIA_MB);
}

int main(void)
{
    SetTraceLogLevel(LOG_WARNING);

    InitWindow(LARGURA_JANELA, ALTURA_JANELA, "IARA - testes de conteudo");

    screen_width = GetScreenWidth();
    screen_height = GetScreenHeight();
    middle_screen_x = screen_width / 2;
    middle_screen_y = screen_height / 2;

    printf("\n========================================================================\n");
    printf(" IARA - TESTES DE SISTEMA | Bloco 2: quiz do computador e menu\n");
    printf("========================================================================\n");

    initialize_fonts();

    testar_roteiro_do_quiz();
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
