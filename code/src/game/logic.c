#include "logic.h"

bool logic_evaluate(const LogicExpression *expression, const bool *values)
{
    switch (expression->operator)
    {
    case LOGIC_VARIABLE:
        return values[expression->variable];

    case LOGIC_NOT:
        return !logic_evaluate(expression->left, values);

    case LOGIC_AND:
        return logic_evaluate(expression->left, values) && logic_evaluate(expression->right, values);

    case LOGIC_OR:
        return logic_evaluate(expression->left, values) || logic_evaluate(expression->right, values);

    case LOGIC_IMPLIES:
        return !logic_evaluate(expression->left, values) || logic_evaluate(expression->right, values);

    case LOGIC_IFF:
        return logic_evaluate(expression->left, values) == logic_evaluate(expression->right, values);
    }

    return false;
}

bool logic_forces_variable(const LogicExpression *expression, const bool *values, int variable)
{
    bool without_variable[LOGIC_MAX_VARIABLES];

    for (int i = 0; i < LOGIC_MAX_VARIABLES; i++)
    {
        without_variable[i] = values[i];
    }

    // Se negar a variável viola a fórmula, ela é obrigatória
    without_variable[variable] = false;

    return !logic_evaluate(expression, without_variable);
}

// Monta os valores das variáveis a partir dos bits de uma combinação da tabela-verdade.
static void fill_values(bool *values, int combination, int variable_count)
{
    for (int i = 0; i < LOGIC_MAX_VARIABLES; i++)
    {
        values[i] = (i < variable_count) && ((combination >> i) & 1);
    }
}

bool logic_are_equivalent(const LogicExpression *left, const LogicExpression *right, int variable_count)
{
    bool values[LOGIC_MAX_VARIABLES];

    for (int combination = 0; combination < (1 << variable_count); combination++)
    {
        fill_values(values, combination, variable_count);

        if (logic_evaluate(left, values) != logic_evaluate(right, values)) return false;
    }

    return true;
}

bool logic_is_tautology(const LogicExpression *expression, int variable_count)
{
    bool values[LOGIC_MAX_VARIABLES];

    for (int combination = 0; combination < (1 << variable_count); combination++)
    {
        fill_values(values, combination, variable_count);

        if (!logic_evaluate(expression, values)) return false;
    }

    return true;
}

bool logic_is_satisfiable(const LogicExpression *const *expressions, int expression_count, int variable_count)
{
    bool values[LOGIC_MAX_VARIABLES];

    for (int combination = 0; combination < (1 << variable_count); combination++)
    {
        bool all_true = true;

        fill_values(values, combination, variable_count);

        for (int i = 0; i < expression_count && all_true; i++)
        {
            all_true = logic_evaluate(expressions[i], values);
        }

        if (all_true) return true;
    }

    return false;
}

// As 8 regiões do diagrama de Venn de 3 termos: cada bit do índice diz se a região fica dentro do termo.
#define VENN_REGION_COUNT 8

static bool categorical_holds(CategoricalStatement statement, int occupied_regions)
{
    for (int region = 0; region < VENN_REGION_COUNT; region++)
    {
        bool in_subject = (region >> statement.subject) & 1;
        bool in_predicate = (region >> statement.predicate) & 1;
        bool occupied = (occupied_regions >> region) & 1;

        if (statement.form == CATEGORICAL_ALL && occupied && in_subject && !in_predicate) return false;
        if (statement.form == CATEGORICAL_NO && occupied && in_subject && in_predicate) return false;
        if (statement.form == CATEGORICAL_SOME && occupied && in_subject && in_predicate) return true;
        if (statement.form == CATEGORICAL_SOME_NOT && occupied && in_subject && !in_predicate) return true;
    }

    return statement.form == CATEGORICAL_ALL || statement.form == CATEGORICAL_NO;
}

bool logic_conclusion_follows(const CategoricalStatement *premises, int premise_count, CategoricalStatement conclusion)
{
    // Testa todos os diagramas possíveis: basta um contraexemplo para a conclusão não seguir
    for (int occupied_regions = 0; occupied_regions < (1 << VENN_REGION_COUNT); occupied_regions++)
    {
        bool premises_hold = true;

        for (int i = 0; i < premise_count && premises_hold; i++)
        {
            premises_hold = categorical_holds(premises[i], occupied_regions);
        }

        if (premises_hold && !categorical_holds(conclusion, occupied_regions)) return false;
    }

    return true;
}
