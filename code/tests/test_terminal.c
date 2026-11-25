// Teste do jogo de terminal (PIF, Unidade 2).
//
// Confere duas coisas que um jogador não teria como conferir sozinho:
//
//   1. Toda mesa de triagem tem solução. Para cada mesa, o teste experimenta todas as
//      combinações de escolhas possíveis e conta quantas o corretor aceita. Se alguma
//      mesa der zero, o dia é impossível de fechar e o jogo estaria quebrado.
//   2. O corretor do jogo de terminal concorda com a lógica das fórmulas: a solução
//      encontrada à força bruta é a mesma que sai de mesa_escolha_certa_da_linha() nas
//      mesas em que cada linha tem a sua resposta.
//
// Compilar e rodar:
//   gcc -O2 -std=c11 code/tests/test_terminal.c code/terminal/mesa.c
//       code/terminal/texto.c code/src/game/logic.c -Icode/terminal -Icode/src/game
//       -Icode/src/core -Icode/src/screens -o test_terminal.exe && ./test_terminal.exe

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "logic.h"
#include "mesa.h"
#include "roteiro.h"
#include "texto.h"

#include "evidencias.h"
#include "computer_days.h"

#define TODAS_AS_EVIDENCIAS (EVIDENCE_NO_RESIGNATION_LETTER | EVIDENCE_BADGE_AFTER_LEAVING | \
                             EVIDENCE_NO_HUMAN_ORDER)

typedef struct {
    int numero;
    DialogStep *passos;
    int quantidade_de_passos;
} DiaDoTeste;

#define DIA(numero, roteiro) {(numero), (roteiro), (int)(sizeof(roteiro) / sizeof((roteiro)[0]))}

static const DiaDoTeste dias[] = {
    DIA(3, day_3_script),
    DIA(12, day_12_script),
    DIA(27, day_27_script),
    DIA(41, day_41_script),
    DIA(58, day_58_script),
    DIA(74, day_74_script),
    DIA(89, day_89_script),
    DIA(100, day_100_script),
};

#define QUANTIDADE_DE_DIAS ((int)(sizeof(dias) / sizeof(dias[0])))

static int falhas = 0;

static void conferir(bool condicao, const char *o_que)
{
    printf("  [%s] %s\n", condicao ? "ok" : "FALHOU", o_que);
    if (!condicao) falhas++;
}

// Experimenta todas as combinações de escolhas da mesa e devolve quantas o jogo aceita.
// Guarda a primeira que fecha em solucao, para o teste poder mostrá-la.
static long contar_solucoes(const DialogStep *passo, int *solucao)
{
    int quantidade = mesa_quantidade_de_linhas(passo);
    long combinacoes = 1L << quantidade;
    long aceitas = 0;

    for (long tentativa = 0; tentativa < combinacoes; tentativa++)
    {
        int escolhas[MAX_TRIAGE_CARDS];
        bool possivel = true;

        for (int linha = 0; linha < quantidade; linha++)
        {
            escolhas[linha] = (int)((tentativa >> linha) & 1);

            // Linha sem a evidência exigida fica fora: o jogo nem pergunta por ela.
            if (!mesa_linha_disponivel(passo, linha, TODAS_AS_EVIDENCIAS) && escolhas[linha] == 1)
            {
                possivel = false;
            }
        }

        if (!possivel) continue;

        if (mesa_quantos_erros(passo, escolhas) == 0)
        {
            if (aceitas == 0 && solucao != NULL)
            {
                memcpy(solucao, escolhas, sizeof(int) * (size_t)quantidade);
            }

            aceitas++;
        }
    }

    return aceitas;
}

// Nas mesas com gabarito por linha, a resposta certa tem de ser única e igual à das fórmulas.
static void conferir_gabarito_por_linha(const DialogStep *passo, const int *solucao)
{
    char descricao[256];
    bool igual = true;

    for (int linha = 0; linha < mesa_quantidade_de_linhas(passo); linha++)
    {
        if (solucao[linha] != mesa_escolha_certa_da_linha(passo, linha)) igual = false;
    }

    snprintf(descricao, sizeof(descricao),
             "a solução encontrada é a que as fórmulas mandam, linha por linha");
    conferir(igual, descricao);
}

int main(void)
{
    printf("Teste do jogo de terminal: toda mesa tem solução?\n");

    int mesas = 0;

    for (int i = 0; i < QUANTIDADE_DE_DIAS; i++)
    {
        const DiaDoTeste *dia = &dias[i];

        printf("\nDia %d\n", dia->numero);

        for (int indice = 0; indice < dia->quantidade_de_passos; indice++)
        {
            const DialogStep *passo = &dia->passos[indice];

            if (passo->type != STEP_TRIAGE) continue;

            mesas++;

            int quantidade = mesa_quantidade_de_linhas(passo);
            int solucao[MAX_TRIAGE_CARDS] = {0};
            long aceitas = contar_solucoes(passo, solucao);
            char descricao[256];

            snprintf(descricao, sizeof(descricao), "a mesa \"%s\" (%d linhas) tem solução: %ld de %ld",
                     passo->sender, quantidade, aceitas, 1L << quantidade);
            conferir(aceitas > 0, descricao);

            if (aceitas == 0) continue;

            // Mostra a solução com os rótulos do próprio roteiro, para dar para conferir à mão.
            printf("       ");

            for (int linha = 0; linha < quantidade; linha++)
            {
                printf("%s%s", (linha > 0) ? " | " : "", passo->choice_labels[solucao[linha]]);
            }

            printf("\n");

            if (mesa_tem_gabarito_por_linha(passo))
            {
                conferir(aceitas == 1, "a resposta certa é única");
                conferir_gabarito_por_linha(passo, solucao);
            }
        }
    }

    printf("\n%d mesas conferidas, %d falhas\n", mesas, falhas);
    return (falhas == 0) ? 0 : 1;
}
