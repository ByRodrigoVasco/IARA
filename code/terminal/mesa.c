#include <stdio.h>

#include "logic.h"
#include "mesa.h"
#include "texto.h"

// Protótipos do que só esta tradução usa.
static void valores_da_linha(const TriageCard *linha, bool *valores);
static int clausulas_violadas(const DialogStep *passo, const int *escolhas);
static int registros_no_treino(const DialogStep *passo, const int *escolhas,
                               unsigned int querido, unsigned int indesejado);
static int regras_de_treino_violadas(const DialogStep *passo, const int *escolhas);
static bool prova_fecha(const DialogStep *passo, const int *escolhas);

int mesa_quantidade_de_linhas(const DialogStep *passo)
{
    if (passo == NULL || passo->card_count < 0) return 0;
    if (passo->card_count > MAX_TRIAGE_CARDS) return MAX_TRIAGE_CARDS;
    return passo->card_count;
}

bool mesa_tem_gabarito_por_linha(const DialogStep *passo)
{
    if (passo == NULL) return false;

    switch (passo->board_mode)
    {
    case BOARD_ASSIGNMENT:
    case BOARD_TRAINING:
    case BOARD_REFUTATION:
        return false;

    default:
        return true;
    }
}

// Valor de cada variável nesta linha, desempacotado dos bits de values.
static void valores_da_linha(const TriageCard *linha, bool *valores)
{
    for (int i = 0; i < LOGIC_MAX_VARIABLES; i++)
    {
        valores[i] = (linha->values >> i) & 1;
    }
}

int mesa_escolha_certa_da_linha(const DialogStep *passo, int linha)
{
    const TriageCard *item = &passo->cards[linha];
    bool valores[LOGIC_MAX_VARIABLES] = {false};

    switch (passo->board_mode)
    {
    case BOARD_TRUTH_TABLE:
        valores_da_linha(item, valores);
        return logic_evaluate(passo->norm, valores) ? 1 : 0;

    case BOARD_EQUIVALENCE:
        if (item->right == NULL) return logic_is_tautology(item->left, passo->variable_count) ? 1 : 0;
        return logic_are_equivalent(item->left, item->right, passo->variable_count) ? 1 : 0;

    case BOARD_SYLLOGISM:
        return logic_conclusion_follows(passo->premises, passo->premise_count, item->conclusion) ? 1 : 0;

    default:
        valores[TRIAGE_VARIABLE_URGENT] = item->is_urgent;
        return logic_forces_variable(passo->norm, valores, TRIAGE_VARIABLE_GOES_TO_IT) ? TRIAGE_TO_IT : TRIAGE_TO_QUEUE;
    }
}

// Cláusulas que as escolhas violam, quando as escolhas são os valores das variáveis.
static int clausulas_violadas(const DialogStep *passo, const int *escolhas)
{
    bool valores[LOGIC_MAX_VARIABLES] = {false};
    int violadas = 0;

    for (int linha = 0; linha < mesa_quantidade_de_linhas(passo); linha++)
    {
        valores[linha] = (escolhas[linha] == 1);
    }

    for (int clausula = 0; clausula < passo->clause_count; clausula++)
    {
        if (!logic_evaluate(passo->clauses[clausula], valores)) violadas++;
    }

    return violadas;
}

// Quantos registros usados no treino têm todos os bits de querido e nenhum de indesejado.
static int registros_no_treino(const DialogStep *passo, const int *escolhas,
                               unsigned int querido, unsigned int indesejado)
{
    int quantos = 0;

    for (int linha = 0; linha < mesa_quantidade_de_linhas(passo); linha++)
    {
        unsigned int valores = passo->cards[linha].values;

        if (escolhas[linha] == 1 && (valores & querido) == querido && (valores & indesejado) == 0) quantos++;
    }

    return quantos;
}

// Regras da mesa de treino: nenhum registro com CPF, e os dois resultados nos dois bairros.
static int regras_de_treino_violadas(const DialogStep *passo, const int *escolhas)
{
    int violadas = 0;

    if (registros_no_treino(passo, escolhas, TRAINING_HAS_CPF, 0) > 0) violadas++;

    if (registros_no_treino(passo, escolhas, TRAINING_APPROVED, TRAINING_OUTSKIRTS) == 0 ||
        registros_no_treino(passo, escolhas, 0, TRAINING_OUTSKIRTS | TRAINING_APPROVED) == 0) violadas++;

    if (registros_no_treino(passo, escolhas, TRAINING_OUTSKIRTS | TRAINING_APPROVED, 0) == 0 ||
        registros_no_treino(passo, escolhas, TRAINING_OUTSKIRTS, TRAINING_APPROVED) == 0) violadas++;

    return violadas;
}

int mesa_aprovacao_da_mini_ia(const DialogStep *passo, const int *escolhas, bool periferia)
{
    unsigned int grupo = periferia ? TRAINING_OUTSKIRTS : 0;
    unsigned int outro_grupo = periferia ? 0 : TRAINING_OUTSKIRTS;
    int vistos = registros_no_treino(passo, escolhas, grupo, outro_grupo);

    if (vistos == 0) return 0; // Sem exemplo daquele bairro, a mini-IA não aprova ninguém

    return (registros_no_treino(passo, escolhas, grupo | TRAINING_APPROVED, outro_grupo) * 100) / vistos;
}

// A prova fecha se o conjunto marcado é contraditório, tem uma evidência e nada sobrando nele.
static bool prova_fecha(const DialogStep *passo, const int *escolhas)
{
    const LogicExpression *marcadas[MAX_TRIAGE_CARDS];
    int quantas = 0;
    bool tem_evidencia = false;

    for (int linha = 0; linha < mesa_quantidade_de_linhas(passo); linha++)
    {
        if (escolhas[linha] != 1) continue;

        marcadas[quantas++] = passo->cards[linha].left;
        if (passo->cards[linha].required_evidence != 0) tem_evidencia = true;
    }

    if (!tem_evidencia || logic_is_satisfiable(marcadas, quantas, passo->variable_count)) return false;

    // Tirar qualquer uma precisa desfazer a contradição: senão ela estava sobrando.
    for (int fora = 0; fora < quantas; fora++)
    {
        const LogicExpression *restantes[MAX_TRIAGE_CARDS];
        int quantas_restantes = 0;

        for (int i = 0; i < quantas; i++)
        {
            if (i != fora) restantes[quantas_restantes++] = marcadas[i];
        }

        if (!logic_is_satisfiable(restantes, quantas_restantes, passo->variable_count)) return false;
    }

    return true;
}

int mesa_quantos_erros(const DialogStep *passo, const int *escolhas)
{
    int erros = 0;

    switch (passo->board_mode)
    {
    case BOARD_ASSIGNMENT:
        return clausulas_violadas(passo, escolhas);

    case BOARD_TRAINING:
        return regras_de_treino_violadas(passo, escolhas);

    case BOARD_REFUTATION:
        return prova_fecha(passo, escolhas) ? 0 : 1;

    default:
        for (int linha = 0; linha < mesa_quantidade_de_linhas(passo); linha++)
        {
            if (escolhas[linha] != mesa_escolha_certa_da_linha(passo, linha)) erros++;
        }
        return erros;
    }
}

bool mesa_linha_disponivel(const DialogStep *passo, int linha, unsigned int evidencias)
{
    if (passo->board_mode != BOARD_REFUTATION) return true;

    unsigned int exigida = passo->cards[linha].required_evidence;

    return exigida == 0 || (evidencias & exigida) != 0;
}

void mesa_texto_da_linha(const DialogStep *passo, int linha, unsigned int evidencias,
                         char *destino, size_t tamanho)
{
    const TriageCard *item = &passo->cards[linha];

    if (!mesa_linha_disponivel(passo, linha, evidencias))
    {
        texto_copiar(destino, tamanho, "(evidência não encontrada)");
        return;
    }

    if (passo->board_mode == BOARD_BY_NORM)
    {
        snprintf(destino, tamanho, "%s %s  U: %s", item->id, item->text, item->is_urgent ? "sim" : "não");
        return;
    }

    if (passo->board_mode != BOARD_TRUTH_TABLE)
    {
        texto_copiar(destino, tamanho, item->text);
        return;
    }

    // Na tabela-verdade a linha mostra o valor de cada variável, que é o enunciado dela.
    char valores[MAX_LINE_LEN] = "";

    for (int i = 0; i < passo->variable_count; i++)
    {
        char valor[8];

        snprintf(valor, sizeof(valor), " %c:%c", passo->variable_names[i], ((item->values >> i) & 1) ? 'V' : 'F');
        texto_juntar(valores, sizeof(valores), valor);
    }

    snprintf(destino, tamanho, "%s %s %s", item->id, item->text, valores);
}

void mesa_texto_do_resultado(const DialogStep *passo, int erros, char *destino, size_t tamanho)
{
    if (erros == 0)
    {
        switch (passo->board_mode)
        {
        case BOARD_ASSIGNMENT: texto_copiar(destino, tamanho, "Nenhuma contradição."); return;
        case BOARD_TRAINING: texto_copiar(destino, tamanho, "O treino respeita as regras."); return;
        case BOARD_REFUTATION: texto_copiar(destino, tamanho, "A contradição fecha."); return;
        default: texto_copiar(destino, tamanho, "Todas as linhas estão certas."); return;
        }
    }

    switch (passo->board_mode)
    {
    case BOARD_ASSIGNMENT:
    case BOARD_TRAINING:
        snprintf(destino, tamanho, "Regras violadas: %d de %d.", erros,
                 passo->clause_count > 0 ? passo->clause_count : TRAINING_RULE_COUNT);
        return;

    case BOARD_REFUTATION:
        texto_copiar(destino, tamanho, "A prova não fecha.");
        return;

    default:
        snprintf(destino, tamanho, "Errados: %d de %d.", erros, mesa_quantidade_de_linhas(passo));
        return;
    }
}
