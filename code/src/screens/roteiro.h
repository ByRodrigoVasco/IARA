#ifndef ROTEIRO_H
#define ROTEIRO_H

#include <stdbool.h>

#include "logic.h"

// Tipos do roteiro dos dias: os passos de diálogo e as linhas das mesas de triagem.
//
// Ficam num header próprio porque os dois jogos leem o mesmo roteiro (computer_days.h):
// o jogo gráfico, em computer_loop.c, e o jogo de terminal, em code/terminal/jogo.c.
// Nenhum campo daqui usa tipo do raylib, de propósito: é o que torna o roteiro
// aproveitável nos dois. Quem desenha decide como mostrar; o roteiro só guarda o conteúdo.

#define MAX_LINES 4
#define MAX_LINE_LEN 64
#define MAX_OPTIONS 4
#define MAX_OPTION_LEN 256
#define MAX_HINTS 2
#define MAX_SOURCE_LINES 3
#define MAX_AUTHORED_CLAIMS 2 // Afirmações escritas para cada chamado
#define MAX_TRIAGE_CARDS 6

// Destino de uma linha da mesa.
#define TRIAGE_NOT_CHOSEN -1
#define TRIAGE_TO_QUEUE 0
#define TRIAGE_TO_IT 1

// Variáveis da norma da mesa de triagem: U -> TI.
enum { TRIAGE_VARIABLE_URGENT, TRIAGE_VARIABLE_GOES_TO_IT };

// Registro do arquivo morto na mesa de treino.
#define TRAINING_OUTSKIRTS (1u << 0)
#define TRAINING_APPROVED (1u << 1)
#define TRAINING_HAS_CPF (1u << 2)
#define TRAINING_RULE_COUNT 3

// O que as escolhas de um STEP_CHOICE mudam quando o dia é concluído.
#define DAY_EFFECT_NONE 0u
#define DAY_EFFECT_AUTOMATION (1u << 0)
#define DAY_EFFECT_PERCEPTION (1u << 1)
#define DAY_EFFECT_HUMAN_BOND (1u << 2)
#define DAY_EFFECT_NO_LETTER (1u << 3)

// Tipo de passo do diálogo do computador:
// STEP_STATEMENT = apenas texto com botão; STEP_QUIZ = chamado com opções;
// STEP_TRIAGE = mesa em que o jogador decide cada linha, para os puzzles com símbolos lógicos;
// STEP_CHOICE = mensagem ou decisão sem resposta certa; STEP_REPORT = relatório do dia.
typedef enum {
    STEP_STATEMENT,
    STEP_QUIZ,
    STEP_TRIAGE,
    STEP_CHOICE,
    STEP_REPORT
} StepType;

// Como cada mesa confere as escolhas do jogador. O gabarito sempre sai da lógica, não do roteiro.
typedef enum {
    BOARD_BY_NORM,     // Dia 1: a norma obriga ou não o destino de cada linha
    BOARD_TRUTH_TABLE, // Cada linha traz os valores, e a escolha é o valor da fórmula
    BOARD_EQUIVALENCE, // Cada linha traz duas fórmulas: iguais ou diferentes?
    BOARD_ASSIGNMENT,  // As escolhas são os valores das variáveis, e as cláusulas precisam valer
    BOARD_TRAINING,    // As escolhas montam o conjunto de treino da mini-IA
    BOARD_SYLLOGISM,   // Cada linha é uma conclusão: segue das premissas?
    BOARD_REFUTATION   // As escolhas montam o conjunto contraditório da prova final
} BoardMode;

// Uma linha de mesa. O destino certo sai da norma, não fica escrito aqui.
typedef struct {
    const char *id;
    const char *text;
    bool is_urgent;
    unsigned int values;                 // Valores das variáveis, em bits (BOARD_TRUTH_TABLE, BOARD_TRAINING)
    const LogicExpression *left;         // Fórmula da linha (BOARD_EQUIVALENCE, BOARD_REFUTATION)
    const LogicExpression *right;        // NULL compara com "sempre verdade"
    CategoricalStatement conclusion;     // Conclusão a julgar (BOARD_SYLLOGISM)
    unsigned int required_evidence;      // Evidência que o jogador precisa ter reunido (BOARD_REFUTATION)
    int iara_choice;                     // O que a IARA marca nesta linha
    int iara_low_choice;                 // O que ela marca sem memória
} TriageCard;

// Um passo do diálogo (texto simples ou pergunta do quiz).
typedef struct {
    StepType type;

    // Pergunta ou Declaração
    char lines[MAX_LINES][MAX_LINE_LEN];
    int line_count;

    // Opções de Resposta (usado apenas em STEP_QUIZ)
    char options[MAX_OPTIONS][MAX_OPTION_LEN];
    int option_count;
    int correct_option; // Índice da resposta certa (0 a 3)
    char hints[MAX_HINTS][MAX_LINE_LEN]; // Dicas depois do 1º e do 2º erro (usado apenas em STEP_QUIZ)
    char *button_name;  // Nome do botão (usado apenas em STEP_STATEMENT)

    // Sugestão da IARA e o trecho do manual que permite conferi-la (usados apenas em STEP_QUIZ).
    // A IARA acerta a maioria dos chamados, senão o jogador aprenderia a nunca usá-la.
    int iara_option;
    int iara_confidence;
    char iara_reason[MAX_LINE_LEN];
    char source[MAX_SOURCE_LINES][MAX_LINE_LEN];
    int source_count;
    int manual_page;
    char claims[MAX_AUTHORED_CLAIMS][MAX_LINE_LEN];
    bool claim_is_true[MAX_AUTHORED_CLAIMS];

    // O que a IARA diz quando está com pouca memória: sempre errado
    int low_option;
    char low_reason[MAX_LINE_LEN];
    char low_claim[MAX_LINE_LEN];

    // Mesa de triagem (usados apenas em STEP_TRIAGE)
    const TriageCard *cards;
    int card_count;
    const LogicExpression *norm;
    BoardMode board_mode;
    const char *choice_labels[2];     // Rótulo de cada escolha, pelo valor dela
    const char *submit_label;         // Botão de enviar a mesa
    const char *variable_names;       // Letra de cada variável (BOARD_TRUTH_TABLE)
    int variable_count;
    const LogicExpression *const *clauses; // Cláusulas (BOARD_ASSIGNMENT)
    int clause_count;
    const CategoricalStatement *premises;  // Premissas (BOARD_SYLLOGISM)
    int premise_count;

    unsigned int choice_effects[2];   // Efeito de cada opção (usado apenas em STEP_CHOICE)

    bool has_personal_data;           // Pede a tarja antes de enviar à IARA
    char sender[MAX_LINE_LEN];        // Nome no relatório do dia
    char wrong_result[MAX_LINE_LEN];  // O que acontece se a resposta enviada estiver errada
} DialogStep;

#endif // ROTEIRO_H
