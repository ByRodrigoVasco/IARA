#ifndef SYMPTOMS_H
#define SYMPTOMS_H

#include <stdbool.h>
#include "game_state.h"

// ============================================================================
// SINTOMAS VISUAIS DAS ESTATÍSTICAS OCULTAS (US10, RF11)
//
// O jogador nunca vê os números de hidden_statistic. Cada campo, ao passar de um
// limiar, muda algo no escritório: a luz, os monitores, o mural, o chão.
//
// Regra dos limiares: o nível FORTE é o mesmo limiar que decide o final
// (game_state.h). Assim, o que o jogador vê no cenário avisa para qual final ele
// caminha. O nível LEVE fica na metade desse caminho, arredondada para cima.
//
// Os valores contam só o que veio depois de HIDDEN_STATISTIC_START, como em
// get_game_ending().
// ============================================================================

// Intensidade de um sintoma. A ordem importa: um nível maior inclui os menores.
typedef enum {
    SYMPTOM_NONE,   // Nada muda no cenário
    SYMPTOM_MILD,   // Primeiro sinal, fácil de passar despercebido
    SYMPTOM_STRONG  // Sinal claro: o limiar do final foi alcançado
} SymptomLevel;

// Automação (enviar sem conferir): a IARA vai tomando o escritório.
// Leve: monitores das mesas vazias acendem sozinhos. Forte: luz fria, todos os
// monitores e os servidores pulsando, com a luz falhando de vez em quando.
#define SYMPTOM_AUTOMATION_MILD ((AUTOMATION_HIGH_TOTAL + 1) / 2)    // 3
#define SYMPTOM_AUTOMATION_STRONG AUTOMATION_HIGH_TOTAL              // 5 (final Pamonha)

// Percepção (investigar, pegar erros da IARA): o jogador enxerga o que não bate.
// Leve: um X vermelho sobre o "PESSOAS" do quadro de valores. Forte: também o
// "DADOS", o mural ligado por linhas vermelhas e uma luz vermelha sob a porta do corredor.
#define SYMPTOM_PERCEPTION_MILD ((PERCEPTION_HIGH_TOTAL + 1) / 2)    // 2
#define SYMPTOM_PERCEPTION_STRONG PERCEPTION_HIGH_TOTAL              // 4

// Vínculo humano (conversas opcionais): o escritório ganha calor.
// Leve: luz quente na área do sofá. Forte: também na copa e no bebedouro.
#define SYMPTOM_HUMAN_BOND_MILD ((HUMAN_BOND_HIGH_TOTAL + 1) / 2)    // 1
#define SYMPTOM_HUMAN_BOND_STRONG HUMAN_BOND_HIGH_TOTAL              // 2

// Desempenho (current_overall_points): média de pontos por dia jogável concluído.
// Aqui o sintoma aparece quando a média CAI: papéis de pendência se acumulam no
// chão perto da mesa do jogador e, no nível forte, a impressora pisca em vermelho.
// Leve: menos de um chamado bem resolvido por dia. Forte: menos da metade disso.
#define SYMPTOM_PERFORMANCE_MILD_PER_DAY POINTS_SOLVED_ALONE         // abaixo de 10 por dia
#define SYMPTOM_PERFORMANCE_STRONG_PER_DAY (POINTS_SOLVED_ALONE / 2) // abaixo de 5 por dia

// Estado visual do escritório, calculado a partir das estatísticas ocultas.
typedef struct {
    SymptomLevel automation;
    SymptomLevel perception;
    SymptomLevel human_bond;
    SymptomLevel performance;

    // De 0 a 1: quanto do caminho até o nível forte já foi percorrido. A luz usa
    // estes valores para mudar aos poucos, sem saltos entre um nível e outro.
    float automation_intensity;
    float human_bond_intensity;
} SceneSymptoms;

/**
 * @brief Nível de um sintoma que cresce com o valor (automação, percepção, vínculo).
 *
 * @param earned Pontos acumulados depois de HIDDEN_STATISTIC_START.
 * @param mild Limiar do nível leve.
 * @param strong Limiar do nível forte.
 */
SymptomLevel symptom_level_rising(int earned, int mild, int strong);

/**
 * @brief Nível do sintoma de desempenho, que aparece quando a média por dia cai.
 *
 * @param earned_points Pontos gerais acumulados depois de HIDDEN_STATISTIC_START.
 * @param concluded_days Dias jogáveis já concluídos. Sem nenhum, não há sintoma.
 */
SymptomLevel symptom_level_performance(int earned_points, int concluded_days);

/**
 * @brief De 0 a 1, quanto de earned já cobre o limiar forte.
 */
float symptom_intensity(int earned, int strong);

/**
 * @brief Quantos dias jogáveis o jogador já concluiu, contando o atual se a tarefa dele foi resolvida.
 */
int symptoms_count_concluded_playable_days(void);

/**
 * @brief Calcula os sintomas do escritório a partir de current_hidden_statistic.
 */
SceneSymptoms symptoms_get_scene(void);

#endif // SYMPTOMS_H
