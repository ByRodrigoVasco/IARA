/*
 * Testes de unidade dos sintomas visuais das estatísticas ocultas (US10, RF11).
 *
 * Não abrem janela: conferem os limiares de cada campo de hidden_statistic e que
 * o nível forte de cada sintoma coincide com o limiar do final correspondente.
 */

#include <assert.h>
#include <stdio.h>

#include "game_state.h"
#include "symptoms.h"

static void reset_state(void)
{
    initialize_game_state_variables();
    initialize_hidden_statistic();
}

// Coloca a estatística em START + earned.
#define SET_EARNED(field, earned) (current_hidden_statistic.field = HIDDEN_STATISTIC_START + (earned))

static void test_new_game_has_no_symptoms(void)
{
    reset_state();

    SceneSymptoms scene = symptoms_get_scene();

    assert(scene.automation == SYMPTOM_NONE);
    assert(scene.perception == SYMPTOM_NONE);
    assert(scene.human_bond == SYMPTOM_NONE);
    assert(scene.performance == SYMPTOM_NONE);
    assert(scene.automation_intensity == 0.0f);
    assert(scene.human_bond_intensity == 0.0f);
}

static void test_rising_thresholds(void)
{
    assert(symptom_level_rising(0, 2, 4) == SYMPTOM_NONE);
    assert(symptom_level_rising(1, 2, 4) == SYMPTOM_NONE);
    assert(symptom_level_rising(2, 2, 4) == SYMPTOM_MILD);
    assert(symptom_level_rising(3, 2, 4) == SYMPTOM_MILD);
    assert(symptom_level_rising(4, 2, 4) == SYMPTOM_STRONG);
    assert(symptom_level_rising(9, 2, 4) == SYMPTOM_STRONG);
}

// O nível forte avisa o final: tem que bater exatamente com get_game_ending().
static void test_strong_matches_ending_thresholds(void)
{
    assert(SYMPTOM_AUTOMATION_STRONG == AUTOMATION_HIGH_TOTAL);
    assert(SYMPTOM_PERCEPTION_STRONG == PERCEPTION_HIGH_TOTAL);
    assert(SYMPTOM_HUMAN_BOND_STRONG == HUMAN_BOND_HIGH_TOTAL);

    assert(SYMPTOM_AUTOMATION_MILD > 0 && SYMPTOM_AUTOMATION_MILD < SYMPTOM_AUTOMATION_STRONG);
    assert(SYMPTOM_PERCEPTION_MILD > 0 && SYMPTOM_PERCEPTION_MILD < SYMPTOM_PERCEPTION_STRONG);
    assert(SYMPTOM_HUMAN_BOND_MILD > 0 && SYMPTOM_HUMAN_BOND_MILD < SYMPTOM_HUMAN_BOND_STRONG);

    // Um ponto abaixo do limiar do final: sem final Pamonha e sem sintoma forte
    reset_state();
    SET_EARNED(current_automation_points, AUTOMATION_HIGH_TOTAL - 1);
    assert(get_game_ending() != ENDING_PAMONHA);
    assert(symptoms_get_scene().automation == SYMPTOM_MILD);

    // No limiar: o final muda e o sintoma fica forte junto
    SET_EARNED(current_automation_points, AUTOMATION_HIGH_TOTAL);
    assert(get_game_ending() == ENDING_PAMONHA);
    assert(symptoms_get_scene().automation == SYMPTOM_STRONG);

    reset_state();
    SET_EARNED(current_perception_points, PERCEPTION_HIGH_TOTAL - 1);
    assert(symptoms_get_scene().perception == SYMPTOM_MILD);
    SET_EARNED(current_perception_points, PERCEPTION_HIGH_TOTAL);
    assert(get_game_ending() == ENDING_HYPNOTIZED);
    assert(symptoms_get_scene().perception == SYMPTOM_STRONG);

    reset_state();
    SET_EARNED(current_human_bond_points, HUMAN_BOND_HIGH_TOTAL - 1);
    assert(symptoms_get_scene().human_bond == SYMPTOM_MILD);
    SET_EARNED(current_human_bond_points, HUMAN_BOND_HIGH_TOTAL);
    assert(symptoms_get_scene().human_bond == SYMPTOM_STRONG);
}

static void test_intensity_grows_and_stops_at_one(void)
{
    assert(symptom_intensity(0, 5) == 0.0f);
    assert(symptom_intensity(-3, 5) == 0.0f);
    assert(symptom_intensity(1, 5) < symptom_intensity(2, 5));
    assert(symptom_intensity(5, 5) == 1.0f);
    assert(symptom_intensity(9, 5) == 1.0f);
    assert(symptom_intensity(3, 0) == 0.0f);
}

static void test_performance_follows_average_per_day(void)
{
    // Sem dia concluído, não há média: nenhum sintoma
    assert(symptom_level_performance(0, 0) == SYMPTOM_NONE);

    // 2 dias: leve abaixo de 20 pontos, forte abaixo de 10
    assert(symptom_level_performance(20, 2) == SYMPTOM_NONE);
    assert(symptom_level_performance(19, 2) == SYMPTOM_MILD);
    assert(symptom_level_performance(10, 2) == SYMPTOM_MILD);
    assert(symptom_level_performance(9, 2) == SYMPTOM_STRONG);
    assert(symptom_level_performance(0, 2) == SYMPTOM_STRONG);
}

static void test_concluded_days_count_only_playable_days(void)
{
    reset_state();

    // Dia 1 ainda em andamento: nada concluído
    assert(symptoms_count_concluded_playable_days() == 0);

    current_game_state_variables.is_resolved = true;
    assert(symptoms_count_concluded_playable_days() == 1);

    // No Dia 12, os dias jogáveis anteriores são o 1 e o 3; os de flash e salto não contam
    start_game_day(12);
    assert(symptoms_count_concluded_playable_days() == 2);

    current_game_state_variables.is_resolved = true;
    assert(symptoms_count_concluded_playable_days() == 3);
}

static void test_careless_player_sees_pending_papers(void)
{
    reset_state();

    // Dia 3 concluído sem ganhar nenhum ponto nos dois dias
    start_game_day(3);
    current_game_state_variables.is_resolved = true;
    assert(symptoms_get_scene().performance == SYMPTOM_STRONG);

    // Resolveu bem os 5 chamados do Dia 1: média alta, nenhum papel no chão
    SET_EARNED(current_overall_points, 5 * POINTS_SOLVED_ALONE);
    assert(symptoms_get_scene().performance == SYMPTOM_NONE);
}

static void test_symptoms_do_not_change_score(void)
{
    reset_state();
    SET_EARNED(current_automation_points, 3);
    SET_EARNED(current_perception_points, 2);
    SET_EARNED(current_human_bond_points, 1);
    SET_EARNED(current_overall_points, 40);

    struct hidden_statistic before = current_hidden_statistic;

    for (int i = 0; i < 100; i++) (void)symptoms_get_scene();

    assert(current_hidden_statistic.current_overall_points == before.current_overall_points);
    assert(current_hidden_statistic.current_automation_points == before.current_automation_points);
    assert(current_hidden_statistic.current_perception_points == before.current_perception_points);
    assert(current_hidden_statistic.current_human_bond_points == before.current_human_bond_points);
}

int main(void)
{
    test_new_game_has_no_symptoms();
    test_rising_thresholds();
    test_strong_matches_ending_thresholds();
    test_intensity_grows_and_stops_at_one();
    test_performance_follows_average_per_day();
    test_concluded_days_count_only_playable_days();
    test_careless_player_sees_pending_papers();
    test_symptoms_do_not_change_score();

    printf("\n  test_symptoms: todos os testes passaram\n\n");

    return 0;
}
