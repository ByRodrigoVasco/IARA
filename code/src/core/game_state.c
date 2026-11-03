#include "game_state.h"
#include "calendar.h"

// Variável de estado atual do jogo
struct game_state_variables current_game_state_variables;

// Variável de estatísticas ocultas do jogo
struct hidden_statistic current_hidden_statistic;

void initialize_game_state_variables(void)
{
    current_game_state_variables.current_day = 1;
    current_game_state_variables.day_type = PLAYABLE_DAY;
    current_game_state_variables.mode_type = false;
    current_game_state_variables.user_investigated = false;
    current_game_state_variables.is_resolved = false;
    current_game_state_variables.has_cheat_sheet = false;
    current_game_state_variables.evidence_mask = 0;
    current_game_state_variables.found_mariana = false;
    current_game_state_variables.proof_succeeded = false;

    for (int day = 0; day <= TOTAL_GAME_DAYS; day++)
    {
        current_game_state_variables.resolved_puzzles[day] = false;
    }
}

void initialize_hidden_statistic(void)
{
    current_hidden_statistic.current_overall_points = HIDDEN_STATISTIC_START;
    current_hidden_statistic.current_automation_points = HIDDEN_STATISTIC_START;
    current_hidden_statistic.current_perception_points = HIDDEN_STATISTIC_START;
    current_hidden_statistic.current_human_bond_points = HIDDEN_STATISTIC_START;
}

void resolve_day_tasks(const TaskResult *results, int count)
{
    if (current_game_state_variables.is_resolved) return;

    int unchecked = 0;
    bool solved_by_player = false;
    bool caught_error = false;
    bool taken_over = false;

    for (int i = 0; i < count; i++)
    {
        switch (results[i].outcome)
        {
        case TASK_SOLVED_ALONE:
            current_hidden_statistic.current_overall_points += POINTS_SOLVED_ALONE;
            solved_by_player = true;
            break;

        case TASK_SOLVED_WITH_IARA:
            current_hidden_statistic.current_overall_points += POINTS_SOLVED_WITH_IARA;
            solved_by_player = true;
            break;

        case TASK_SENT_UNCHECKED:
            current_hidden_statistic.current_overall_points += POINTS_WITHOUT_CHECKING;
            unchecked++;
            break;

        case TASK_TAKEN_OVER_BY_IARA:
            taken_over = true;
            break;

        default:
            break;
        }

        if (results[i].caught_iara_error)
        {
            current_hidden_statistic.current_overall_points += POINTS_CAUGHT_IARA_ERROR;
            caught_error = true;
        }
    }

    bool mostly_unchecked = (unchecked * 2) > count;
    bool investigated = current_game_state_variables.user_investigated;

    current_game_state_variables.is_resolved = true;
    current_game_state_variables.mode_type = (mostly_unchecked || taken_over) ? MODE_AUTOMATIC : MODE_MANUAL;

    if (investigated && solved_by_player) current_hidden_statistic.current_overall_points += POINTS_INVESTIGATED;

    // O GDD conta Automação e Percepção por dia. Só enviar sem conferir é Automação: usar a
    // IARA com cuidado não conta, e o failsafe não foi escolha do jogador.
    if (mostly_unchecked) current_hidden_statistic.current_automation_points++;
    if (investigated || caught_error) current_hidden_statistic.current_perception_points++;
}

void start_game_day(int day)
{
    current_game_state_variables.current_day = day;
    current_game_state_variables.day_type = calendar_get_day_type(day);
    current_game_state_variables.mode_type = MODE_MANUAL;
    current_game_state_variables.user_investigated = false;
    current_game_state_variables.is_resolved = false;
}

GameEnding get_game_ending(void)
{
    bool automation_high = current_hidden_statistic.current_automation_points >= HIDDEN_STATISTIC_START + AUTOMATION_HIGH_TOTAL;
    bool perception_high = current_hidden_statistic.current_perception_points >= HIDDEN_STATISTIC_START + PERCEPTION_HIGH_TOTAL;
    bool human_bond_high = current_hidden_statistic.current_human_bond_points >= HIDDEN_STATISTIC_START + HUMAN_BOND_HIGH_TOTAL;

    if (!perception_high) return automation_high ? ENDING_PAMONHA : ENDING_UNHARMED;

    if (!automation_high && human_bond_high && current_game_state_variables.found_mariana && current_game_state_variables.proof_succeeded)
    {
        return ENDING_RUPTURE;
    }

    return ENDING_HYPNOTIZED;
}
