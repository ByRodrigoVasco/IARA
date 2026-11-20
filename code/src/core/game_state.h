#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "macros.h"

// Quantidade total de dias da campanha (Dia 1 ao Dia 100).
#define TOTAL_GAME_DAYS 100

// Modo de resolução da tarefa do dia (mode_type).
#define MODE_MANUAL false
#define MODE_AUTOMATIC true

// Pontos de cada chamado. Nunca são negativos, e usar a IARA com cuidado vale o mesmo
// que resolver sozinho: o jogo não ensina que usar IA é errado.
#define POINTS_SOLVED_ALONE 10
#define POINTS_SOLVED_WITH_IARA 10
#define POINTS_CAUGHT_IARA_ERROR 5
#define POINTS_WITHOUT_CHECKING 0
#define POINTS_INVESTIGATED 5

// Valor inicial das estatísticas ocultas. O que conta para o final é o que veio depois dele.
#define HIDDEN_STATISTIC_START 1

// Limiares dos finais (GDD 6.5). O GDD não define o do Vínculo: são 3 conversas opcionais na campanha.
#define AUTOMATION_HIGH_TOTAL 5
#define PERCEPTION_HIGH_TOTAL 4
#define HUMAN_BOND_HIGH_TOTAL 2

// Evidências reunidas para a prova do Dia 100.
#define EVIDENCE_NO_RESIGNATION_LETTER (1u << 0) // Dia 12: pediu a revisão da conciliação
#define EVIDENCE_BADGE_AFTER_LEAVING (1u << 1)   // Dia 58: examinou o registro da catraca
#define EVIDENCE_NO_HUMAN_ORDER (1u << 2)        // Dia 74: leu o bilhete da auditoria

// Como um chamado foi concluído.
typedef enum {
    TASK_PENDING,            // Ainda não foi concluído
    TASK_SOLVED_ALONE,       // O jogador resolveu sem abrir a IARA
    TASK_SOLVED_WITH_IARA,   // O jogador conferiu a sugestão da IARA e decidiu
    TASK_SENT_UNCHECKED,     // O jogador enviou a sugestão da IARA sem conferir
    TASK_TAKEN_OVER_BY_IARA  // Failsafe: a IARA assumiu depois de 3 erros seguidos
} TaskOutcome;

// Os 4 finais do jogo (GDD 6.5).
typedef enum {
    ENDING_UNHARMED,   // Final A, "Ileso"
    ENDING_PAMONHA,    // Final B, "Pamonha"
    ENDING_HYPNOTIZED, // Final C, "Hipnotizado"
    ENDING_RUPTURE     // Final D, "Ruptura"
} GameEnding;

// Resultado de um chamado do dia.
typedef struct {
    TaskOutcome outcome;
    bool answer_is_correct;      // A resposta enviada estava certa
    bool caught_iara_error;      // O jogador apontou uma afirmação falsa da IARA
    bool exposed_personal_data;  // Enviou dados pessoais à IARA sem tarja
    bool redacted_personal_data; // Tarjou os dados pessoais antes de enviar à IARA
} TaskResult;

typedef enum { STORY_ARRIVAL, STORY_MIDDLE, STORY_FINISH } StoryPhase;
enum { STORY_ABSENT_JULIO = 1, STORY_ABSENT_ANDERSON = 2 };
#define STORY_LATER_SCENES_MAX ((1u << 21) - 1)

unsigned int story_scene_bit(int day, StoryPhase phase);

// Estado geral da partida.
struct game_state_variables {
    unsigned int story_scenes; // Dia 1: A=1, M=2, F=4; commit apenas no fim da cena.
    unsigned int story_later_scenes; // Três bits por dia jogável a partir do Dia 12.
    unsigned int story_absences;
    int story_presence_day; // Última chegada avaliada ou dia atual de um save legado.
    bool d41_mural_lido;
    bool story_report_closed;
    char computer_checkpoint[1024]; // Snapshot textual validado pelo computador.
    int current_screen;      // Tela atual (PRE_MENU_SCREEN, MENU_SCREEN, ...)
    int current_day;         // Dia atual do jogo
    int day_type;            // Tipo do dia (PLAYABLE_DAY, FLASH_DAY, JUMP_DAY)
    bool mode_type;          // Modo de resolução escolhido (manual ou automático)
    bool user_investigated;  // Se o jogador investigou antes de responder
    bool is_resolved;        // Se a tarefa do dia foi resolvida
    bool has_cheat_sheet;    // Se o jogador já pegou a colinha no mural
    unsigned int evidence_mask; // Evidências reunidas para a prova do Dia 100
    bool found_nicollas_trace;
    bool legacy_mariana_discovery; // Preserva elegibilidade herdada no final Ruptura.
    bool proof_succeeded;    // Se o jogador fechou a prova por contradição do Dia 100

    // Puzzles já resolvidos na campanha. O índice é o número do dia
    // (resolved_puzzles[1] = Dia 1); a posição 0 não é usada.
    bool resolved_puzzles[TOTAL_GAME_DAYS + 1];
};

// Estatísticas ocultas que definem o final do jogo.
struct hidden_statistic {
    signed int current_overall_points;
    unsigned int current_automation_points;
    unsigned int current_perception_points;
    unsigned int current_human_bond_points;
};

typedef struct {
    int automation, perception, bond;
} HiddenStatisticBands;

static inline HiddenStatisticBands hidden_statistic_bands(const struct hidden_statistic *stats)
{
    unsigned int automation = stats->current_automation_points;
    unsigned int perception = stats->current_perception_points;
    unsigned int bond = stats->current_human_bond_points;
    return (HiddenStatisticBands){
        automation >= HIDDEN_STATISTIC_START + AUTOMATION_HIGH_TOTAL ? 3 : automation >= 4 ? 2 : automation >= 2 ? 1 : 0,
        perception >= HIDDEN_STATISTIC_START + PERCEPTION_HIGH_TOTAL ? 3 : perception >= 3 ? 2 : perception >= 2 ? 1 : 0,
        bond >= HIDDEN_STATISTIC_START + HUMAN_BOND_HIGH_TOTAL ? 2 : bond >= 2 ? 1 : 0
    };
}

extern struct game_state_variables current_game_state_variables;
extern struct hidden_statistic current_hidden_statistic;

void initialize_game_state_variables(void);
void initialize_hidden_statistic(void);

/**
 * @brief Conclui a tarefa do dia e soma as estatísticas ocultas. Só vale uma vez por dia.
 */
void resolve_day_tasks(const TaskResult *results, int count);

/**
 * @brief Passa para o dia indicado, com a tarefa do dia ainda por fazer.
 */
void start_game_day(int day);

/**
 * @brief Final da partida, sempre o mesmo para o mesmo padrão de escolhas (GDD 6.5, RF13).
 */
GameEnding get_game_ending(void);

#endif
