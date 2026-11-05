#ifndef LOGIC_H
#define LOGIC_H

#include <stdbool.h>

// Maior quantidade de variáveis (P, Q, R...) em uma fórmula.
#define LOGIC_MAX_VARIABLES 8

// Operador de cada nó de uma fórmula de lógica proposicional.
typedef enum {
    LOGIC_VARIABLE,
    LOGIC_NOT,     // usa só left
    LOGIC_AND,
    LOGIC_OR,
    LOGIC_IMPLIES,
    LOGIC_IFF
} LogicOperator;

typedef struct LogicExpression {
    LogicOperator operator;
    int variable; // Índice em values, usado só por LOGIC_VARIABLE
    const struct LogicExpression *left;
    const struct LogicExpression *right;
} LogicExpression;

/**
 * @brief Calcula o valor verdade de uma fórmula.
 *
 * @param values Valor de cada variável, pelo índice.
 */
bool logic_evaluate(const LogicExpression *expression, const bool *values);

/**
 * @brief Informa se a fórmula obriga a variável a ser verdadeira, dados os outros valores.
 *
 * Em P -> Q com P verdadeiro, Q é obrigatório (modus ponens). Com P falso, a fórmula não obriga nada.
 */
bool logic_forces_variable(const LogicExpression *expression, const bool *values, int variable);

/**
 * @brief Informa se duas fórmulas têm o mesmo valor em toda a tabela-verdade.
 */
bool logic_are_equivalent(const LogicExpression *left, const LogicExpression *right, int variable_count);

/**
 * @brief Informa se a fórmula é verdadeira em toda a tabela-verdade.
 */
bool logic_is_tautology(const LogicExpression *expression, int variable_count);

/**
 * @brief Informa se existe uma combinação de valores que torna todas as fórmulas verdadeiras.
 */
bool logic_is_satisfiable(const LogicExpression *const *expressions, int expression_count, int variable_count);

// Formas do silogismo categórico. Os termos são índices de 0 a 2.
typedef enum {
    CATEGORICAL_ALL,     // Todo S é P
    CATEGORICAL_NO,      // Nenhum S é P
    CATEGORICAL_SOME,    // Algum S é P
    CATEGORICAL_SOME_NOT // Algum S não é P
} CategoricalForm;

typedef struct {
    CategoricalForm form;
    int subject;
    int predicate;
} CategoricalStatement;

/**
 * @brief Informa se a conclusão vale em todo diagrama de Venn em que as premissas valem.
 *
 * Usa a leitura moderna: "Todo S é P" não garante que exista algum S.
 */
bool logic_conclusion_follows(const CategoricalStatement *premises, int premise_count, CategoricalStatement conclusion);

#endif // LOGIC_H
