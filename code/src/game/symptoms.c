#include "symptoms.h"
#include "calendar.h"
#include "macros.h"

// Pontos acumulados depois do valor inicial. Nunca negativo.
static int earned_after_start(int value)
{
    int earned = value - HIDDEN_STATISTIC_START;

    return earned > 0 ? earned : 0;
}

SymptomLevel symptom_level_rising(int earned, int mild, int strong)
{
    if (earned >= strong) return SYMPTOM_STRONG;
    if (earned >= mild) return SYMPTOM_MILD;

    return SYMPTOM_NONE;
}

SymptomLevel symptom_level_performance(int earned_points, int concluded_days)
{
    if (concluded_days <= 0) return SYMPTOM_NONE;

    // Compara somas em vez de médias, para não depender de divisão com resto.
    if (earned_points < SYMPTOM_PERFORMANCE_STRONG_PER_DAY * concluded_days) return SYMPTOM_STRONG;
    if (earned_points < SYMPTOM_PERFORMANCE_MILD_PER_DAY * concluded_days) return SYMPTOM_MILD;

    return SYMPTOM_NONE;
}

float symptom_intensity(int earned, int strong)
{
    if (strong <= 0 || earned <= 0) return 0.0f;
    if (earned >= strong) return 1.0f;

    return (float)earned / (float)strong;
}

int symptoms_count_concluded_playable_days(void)
{
    int today = current_game_state_variables.current_day;
    int concluded = 0;

    for (int day = 1; day < today && day <= TOTAL_GAME_DAYS; day++)
    {
        if (calendar_get_day_type(day) == PLAYABLE_DAY) concluded++;
    }

    if (current_game_state_variables.is_resolved && calendar_get_day_type(today) == PLAYABLE_DAY) concluded++;

    return concluded;
}

SceneSymptoms symptoms_get_scene(void)
{
    int automation = earned_after_start((int)current_hidden_statistic.current_automation_points);
    int perception = earned_after_start((int)current_hidden_statistic.current_perception_points);
    int human_bond = earned_after_start((int)current_hidden_statistic.current_human_bond_points);
    int overall = earned_after_start(current_hidden_statistic.current_overall_points);

    SceneSymptoms scene;

    scene.automation = symptom_level_rising(automation, SYMPTOM_AUTOMATION_MILD, SYMPTOM_AUTOMATION_STRONG);
    scene.perception = symptom_level_rising(perception, SYMPTOM_PERCEPTION_MILD, SYMPTOM_PERCEPTION_STRONG);
    scene.human_bond = symptom_level_rising(human_bond, SYMPTOM_HUMAN_BOND_MILD, SYMPTOM_HUMAN_BOND_STRONG);
    scene.performance = symptom_level_performance(overall, symptoms_count_concluded_playable_days());

    scene.automation_intensity = symptom_intensity(automation, SYMPTOM_AUTOMATION_STRONG);
    scene.human_bond_intensity = symptom_intensity(human_bond, SYMPTOM_HUMAN_BOND_STRONG);

    return scene;
}
