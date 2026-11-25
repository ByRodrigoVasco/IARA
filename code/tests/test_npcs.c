#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "npcs.h"
#include "calendar.h"
#include "characters.h"
#include "macros.h"

static void test_each_npc_has_its_own_dialogue(void)
{
    assert(npc_get_count() == NPC_COUNT);

    for (int npc_id = 0; npc_id < npc_get_count(); npc_id++)
    {
        const NpcDefinition *definition = npc_get_definition(npc_id);
        assert(definition != NULL);
        assert(definition->name != NULL);
        assert(definition->sprite_folder != NULL);

        // O cumprimento do Dia 1 é de boas-vindas e o dos outros dias é outro
        assert(npc_get_greeting(npc_id, 1) != NULL);
        assert(npc_get_greeting(npc_id, 3) != NULL);
        assert(strcmp(npc_get_greeting(npc_id, 1), npc_get_greeting(npc_id, 3)) != 0);

        for (int other_id = npc_id + 1; other_id < npc_get_count(); other_id++)
        {
            assert(strcmp(npc_get_greeting(npc_id, 1), npc_get_greeting(other_id, 1)) != 0);
            assert(strcmp(npc_get_greeting(npc_id, 3), npc_get_greeting(other_id, 3)) != 0);
        }
    }

    assert(npc_get_definition(-1) == NULL);
    assert(npc_get_definition(NPC_COUNT) == NULL);
    assert(npc_get_greeting(-1, 1) == NULL);
}

static void test_every_npc_has_a_different_line_for_each_playable_day(void)
{
    int days[NPC_PLAYABLE_DAY_COUNT];
    int count = 0;

    for (int day = 1; day <= 100 && count < NPC_PLAYABLE_DAY_COUNT; day++)
    {
        if (calendar_get_day_type(day) == PLAYABLE_DAY) days[count++] = day;
    }
    assert(count == NPC_PLAYABLE_DAY_COUNT);

    for (int npc_id = 0; npc_id < npc_get_count(); npc_id++)
    {
        for (int i = 0; i < count; i++)
        {
            assert(npc_get_playable_day_index(days[i]) == i);

            for (int line = 0; line < NPC_DAY_LINE_COUNT; line++)
            {
                assert(npc_get_day_line(npc_id, days[i], line) != NULL);
                assert(strlen(npc_get_day_line(npc_id, days[i], line)) > 10);
            }

            // Cada dia tem a sua fala: nada repetido nos outros dias do mesmo NPC
            for (int j = i + 1; j < count; j++)
            {
                assert(strcmp(npc_get_day_line(npc_id, days[i], 0), npc_get_day_line(npc_id, days[j], 0)) != 0);
            }

            // E cada NPC fala diferente dos colegas no mesmo dia
            for (int other_id = npc_id + 1; other_id < npc_get_count(); other_id++)
            {
                assert(strcmp(npc_get_day_line(npc_id, days[i], 0), npc_get_day_line(other_id, days[i], 0)) != 0);
            }
        }
    }

    assert(npc_get_day_line(NPC_ANDERSON, 2, 0) == NULL);   // O Dia 2 é um flash, ninguém conversa
    assert(npc_get_day_line(NPC_ANDERSON, 0, 0) == NULL);
    assert(npc_get_playable_day_index(2) == -1);
}

static void test_exactly_one_npc_offers_a_hint_each_playable_day(void)
{
    int playable_day_index = 0;
    for (int day = 1; day <= 100; day++)
    {
        int owner = npc_get_daily_hint_owner(day);
        int owners = 0;

        if (calendar_get_day_type(day) != PLAYABLE_DAY)
        {
            assert(owner == -1);
            continue;
        }

        assert(owner == playable_day_index % npc_get_count());
        assert(owner >= 0 && owner < npc_get_count());
        playable_day_index++;

        for (int npc_id = 0; npc_id < npc_get_count(); npc_id++)
        {
            if (npc_has_hint(npc_id, day)) owners++;
        }

        assert(owners == 1);

        // A dica vem depois da fala do dia, e a dica de cada dia é uma só
        assert(npc_get_hint_line(day, 0) != NULL);
        assert(npc_get_hint_line(day, NPC_HINT_LINE_COUNT - 1) != NULL);
        assert(npc_get_hint_line(day, NPC_HINT_LINE_COUNT) == NULL);
        assert(strcmp(npc_get_dialogue_line(owner, day, false, NPC_DAY_LINE_COUNT), npc_get_hint_line(day, 0)) == 0);
    }

    assert(playable_day_index == 9);
    assert(npc_get_daily_hint_owner(0) == -1);
    assert(npc_get_daily_hint_owner(-1) == -1);
    assert(npc_get_daily_hint_owner(101) == -1);
    assert(!npc_has_hint(-1, 2));
    assert(!npc_has_hint(NPC_ANDERSON, 2));
    assert(npc_get_daily_hint_owner(1) != npc_get_daily_hint_owner(3));
    assert(npc_get_daily_hint_owner(1) == npc_get_daily_hint_owner(89));

    // Cada dica é diferente das outras
    for (int day = 1; day <= 100; day++)
    {
        if (calendar_get_day_type(day) != PLAYABLE_DAY) continue;
        for (int other = day + 1; other <= 100; other++)
        {
            if (calendar_get_day_type(other) != PLAYABLE_DAY) continue;
            assert(strcmp(npc_get_hint_line(day, 0), npc_get_hint_line(other, 0)) != 0);
        }
    }
}

static void test_conversation_starts_with_a_greeting_only_the_first_time_of_the_day(void)
{
    for (int day = 1; day <= 100; day++)
    {
        if (calendar_get_day_type(day) != PLAYABLE_DAY) continue;
        int owner = npc_get_daily_hint_owner(day);

        for (int npc_id = 0; npc_id < npc_get_count(); npc_id++)
        {
            int hint_lines = (npc_id == owner) ? NPC_HINT_LINE_COUNT : 0;

            // Só o dono da dica do dia tem as linhas da dica
            assert(npc_get_dialogue_line_count(npc_id, day, true) == 1 + NPC_DAY_LINE_COUNT + hint_lines);
            assert(npc_get_dialogue_line_count(npc_id, day, false) == NPC_DAY_LINE_COUNT + hint_lines);

            // Primeira vez: cumprimento e depois a fala. Depois: só a fala do dia
            assert(strcmp(npc_get_dialogue_line(npc_id, day, true, 0), npc_get_greeting(npc_id, day)) == 0);
            assert(strcmp(npc_get_dialogue_line(npc_id, day, true, 1), npc_get_day_line(npc_id, day, 0)) == 0);
            assert(strcmp(npc_get_dialogue_line(npc_id, day, false, 0), npc_get_day_line(npc_id, day, 0)) == 0);
            assert(npc_get_dialogue_line(npc_id, day, true, npc_get_dialogue_line_count(npc_id, day, true)) == NULL);
            assert(npc_get_dialogue_line(npc_id, day, false, -1) == NULL);
        }
    }

    // O Dia 1 recebe o jogador com as boas-vindas
    assert(strstr(npc_get_dialogue_line(NPC_LAURA, 1, true, 0), "bem-vind") != NULL ||
           strstr(npc_get_dialogue_line(NPC_LAURA, 1, true, 0), "Bem-vind") != NULL);
    assert(npc_get_dialogue_line_count(NPC_LAURA, 2, true) == 0);
}

static void test_seats_change_every_playable_day_and_never_repeat(void)
{
    int previous[NPC_COUNT];

    for (int npc_id = 0; npc_id < NPC_COUNT; npc_id++) previous[npc_id] = npc_get_seat_for_day(npc_id, 1);

    // No Dia 1 cada um fica no seu lugar de origem
    for (int npc_id = 0; npc_id < NPC_COUNT; npc_id++) assert(previous[npc_id] == npc_id);

    for (int day = 2; day <= 100; day++)
    {
        int seats[NPC_COUNT];
        bool used[NPC_COUNT] = {false};

        if (calendar_get_day_type(day) != PLAYABLE_DAY) continue;

        for (int npc_id = 0; npc_id < NPC_COUNT; npc_id++)
        {
            seats[npc_id] = npc_get_seat_for_day(npc_id, day);
            assert(seats[npc_id] >= 0 && seats[npc_id] < NPC_COUNT);
            assert(!used[seats[npc_id]]);                       // Dois NPCs nunca dividem o mesmo lugar
            used[seats[npc_id]] = true;
            assert(seats[npc_id] != previous[npc_id]);          // Ninguém repete o lugar do dia jogável anterior
            assert(npc_get_seat_for_day(npc_id, day) == seats[npc_id]);   // O mesmo dia sempre arruma igual
        }

        for (int npc_id = 0; npc_id < NPC_COUNT; npc_id++) previous[npc_id] = seats[npc_id];
    }

    assert(npc_get_seat_for_day(-1, 3) == -1);
}

static void test_npcs_are_stationary_and_face_the_requested_directions(void)
{
    assert(npc_get_count() == 7);
    assert(!npc_is_moving(NPC_ANDERSON));
    assert(!npc_is_moving(NPC_JULIO));
    assert(!npc_is_moving(NPC_JV));
    assert(!npc_is_moving(NPC_LAURA));
    assert(!npc_is_moving(NPC_NICOLLAS));
    assert(!npc_is_moving(NPC_VASCO));
    assert(!npc_is_moving(NPC_ZITO));
    assert(npc_get_definition(NPC_JULIO)->facing == SOUTHEAST);
    assert(npc_get_definition(NPC_JV)->facing == SOUTHWEST);
    assert(npc_get_definition(NPC_ZITO)->facing == SOUTHWEST);
    assert(npc_get_definition(NPC_ANDERSON)->facing == SOUTH);
    assert(npc_get_definition(NPC_LAURA)->facing == SOUTH);
    assert(npc_get_definition(NPC_NICOLLAS)->facing == SOUTH);
    assert(npc_get_definition(NPC_VASCO)->facing == SOUTH);
    assert(npc_get_definition(NPC_JULIO)->position.x < npc_get_definition(NPC_JV)->position.x);
    assert(npc_get_definition(NPC_JV)->position.x - npc_get_definition(NPC_JULIO)->position.x <
           75.0f);
    assert(npc_get_definition(NPC_JV)->position.y < npc_get_definition(NPC_JULIO)->position.y);
    assert(npc_get_definition(NPC_ZITO)->position.y == npc_get_definition(NPC_JULIO)->position.y);
    assert(npc_get_definition(NPC_ZITO)->position.x > npc_get_definition(NPC_VASCO)->position.x);
    assert(npc_get_definition(NPC_ANDERSON)->seated_pose);
    assert(npc_get_definition(NPC_LAURA)->seated_pose);
    assert(npc_get_definition(NPC_NICOLLAS)->seated_pose);
    assert(npc_get_definition(NPC_VASCO)->seated_pose);
    assert(!npc_get_definition(NPC_JULIO)->seated_pose);
    assert(npc_get_definition(NPC_ANDERSON)->position.x == -258.0f);
    assert(npc_get_definition(NPC_ANDERSON)->position.y == -263.0f);
    assert(npc_get_definition(NPC_LAURA)->position.x == -262.0f);
    assert(npc_get_definition(NPC_LAURA)->position.y == 0.0f);
    assert(npc_get_definition(NPC_NICOLLAS)->position.x == 76.0f);
    assert(npc_get_definition(NPC_VASCO)->position.x == 178.0f);
    assert(npc_get_definition(NPC_NICOLLAS)->position.y == -7.0f);
    assert(npc_get_definition(NPC_VASCO)->position.y == -7.0f);
    assert(npc_blocks_player(NPC_JULIO));
    assert(npc_blocks_player(NPC_JV));
    assert(npc_blocks_player(NPC_ZITO));
    assert(!npc_blocks_player(NPC_ANDERSON));
}

int main(void)
{
    test_each_npc_has_its_own_dialogue();
    test_every_npc_has_a_different_line_for_each_playable_day();
    test_conversation_starts_with_a_greeting_only_the_first_time_of_the_day();
    test_seats_change_every_playable_day_and_never_repeat();
    test_exactly_one_npc_offers_a_hint_each_playable_day();
    test_npcs_are_stationary_and_face_the_requested_directions();

    printf("test_npcs: todos os testes passaram\n");
    return 0;
}
