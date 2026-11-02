/*
 * Testes de unidade da dica por item explorável (PI2-203 e PI2-204).
 *
 * Não abrem janela: conferem a zona de interação, o abre e fecha do bilhete e,
 * principalmente, que examinar a dica não mexe em nenhum campo da pontuação.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "raylib.h"
#include "game_state.h"
#include "hints.h"

/* A main.c define estas globais; aqui o teste assume esse papel. */
unsigned int screen_width = 0;
unsigned int screen_height = 0;
unsigned int middle_screen_x = 0;
unsigned int middle_screen_y = 0;

#define VEZES_EXAMINANDO 50

static void test_zone_only_reaches_nearby_hitbox(void)
{
    Vector2 center = {960.0f, 540.0f};
    Rectangle zone = get_hint_zone(1.0f, center);
    Rectangle on_zone = {zone.x + 5.0f, zone.y + 5.0f, 50.0f, 24.0f};
    Rectangle far_away = {center.x, center.y, 50.0f, 24.0f};

    assert(is_hitbox_on_hint(on_zone, 1.0f, center));
    assert(!is_hitbox_on_hint(far_away, 1.0f, center));
}

static void test_zone_follows_window_size(void)
{
    Rectangle big = get_hint_zone(1.0f, (Vector2){960.0f, 540.0f});
    Rectangle small = get_hint_zone(0.5f, (Vector2){480.0f, 270.0f});

    assert(small.x == big.x / 2.0f && small.y == big.y / 2.0f);
    assert(small.width == big.width / 2.0f && small.height == big.height / 2.0f);
}

static void test_note_opens_and_closes(void)
{
    assert(!is_hint_note_open());

    toggle_hint_note();
    assert(is_hint_note_open());

    toggle_hint_note();
    assert(!is_hint_note_open());
}

/* PI2-204: nenhum dos campos de hidden_statistic pode mudar ao examinar a dica. */
static void test_examining_does_not_change_score(void)
{
    initialize_hidden_statistic();

    current_hidden_statistic.current_overall_points = 30;
    current_hidden_statistic.current_automation_points = 7;
    current_hidden_statistic.current_perception_points = 12;
    current_hidden_statistic.current_human_bond_points = 4;

    struct hidden_statistic before = current_hidden_statistic;

    for (int i = 0; i < VEZES_EXAMINANDO; i++)
    {
        toggle_hint_note();
    }

    assert(current_hidden_statistic.current_overall_points == before.current_overall_points);
    assert(current_hidden_statistic.current_automation_points == before.current_automation_points);
    assert(current_hidden_statistic.current_perception_points == before.current_perception_points);
    assert(current_hidden_statistic.current_human_bond_points == before.current_human_bond_points);
}

/* US04: a dica reduz o espaço de busca, mas não pode trazer a resposta pronta. */
static void test_note_does_not_give_the_answer(void)
{
    const char *answer_words[] = {"TI", "chamado", "urgente", "Encaminh", "A)", "B)", "C)"};

    for (int line = 0; line < get_hint_note_line_count(); line++)
    {
        for (int word = 0; word < (int)(sizeof(answer_words) / sizeof(answer_words[0])); word++)
        {
            assert(strstr(get_hint_note_line(line), answer_words[word]) == NULL);
        }
    }
}

int main(void)
{
    test_zone_only_reaches_nearby_hitbox();
    test_zone_follows_window_size();
    test_note_opens_and_closes();
    test_examining_does_not_change_score();
    test_note_does_not_give_the_answer();

    printf("\n  test_hints: todos os testes passaram\n\n");

    return 0;
}
