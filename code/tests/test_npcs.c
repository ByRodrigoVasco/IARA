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
        assert(definition->dialogue_line_count >= 3);

        for (int line = 0; line < definition->dialogue_line_count; line++)
        {
            assert(npc_get_dialogue_line(npc_id, 0, line) != NULL);
        }

        for (int other_id = npc_id + 1; other_id < npc_get_count(); other_id++)
        {
            assert(strcmp(definition->dialogue[0], npc_get_definition(other_id)->dialogue[0]) != 0);
        }
    }

    assert(npc_get_definition(-1) == NULL);
    assert(npc_get_definition(NPC_COUNT) == NULL);
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
        assert(npc_get_dialogue_line_count(owner, day) ==
               npc_get_definition(owner)->dialogue_line_count + npc_get_definition(owner)->hint_line_count);
        assert(npc_get_dialogue_line(owner, day,
               npc_get_definition(owner)->dialogue_line_count) != NULL);
    }

    assert(playable_day_index == 9);
    assert(npc_get_daily_hint_owner(0) == -1);
    assert(npc_get_daily_hint_owner(-1) == -1);
    assert(npc_get_daily_hint_owner(101) == -1);
    assert(!npc_has_hint(-1, 2));
    assert(!npc_has_hint(NPC_ANDERSON, 2));
    assert(npc_get_daily_hint_owner(1) != npc_get_daily_hint_owner(3));
    assert(npc_get_daily_hint_owner(1) == npc_get_daily_hint_owner(89));
}

static void test_non_owner_gets_no_hint_lines(void)
{
    for (int day = 1; day <= 100; day++)
    {
        if (calendar_get_day_type(day) != PLAYABLE_DAY) continue;
        int owner = npc_get_daily_hint_owner(day);

        for (int npc_id = 0; npc_id < npc_get_count(); npc_id++)
        {
            const NpcDefinition *definition = npc_get_definition(npc_id);
            int expected = definition->dialogue_line_count;
            if (npc_id == owner) expected += definition->hint_line_count;

            assert(npc_get_dialogue_line_count(npc_id, day) == expected);
        }
    }
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
    assert(npc_get_definition(NPC_JULIO)->facing == EAST);
    assert(npc_get_definition(NPC_JV)->facing == WEST);
    assert(npc_get_definition(NPC_ZITO)->facing == NORTH);
    assert(npc_get_definition(NPC_JULIO)->position.y == npc_get_definition(NPC_JV)->position.y);
    assert(npc_get_definition(NPC_JULIO)->position.x < npc_get_definition(NPC_JV)->position.x);
    assert(npc_get_definition(NPC_JV)->position.x - npc_get_definition(NPC_JULIO)->position.x <
           75.0f);
    assert(npc_get_definition(NPC_ZITO)->position.y ==
           npc_get_definition(NPC_VASCO)->position.y);
    assert(npc_get_definition(NPC_ZITO)->position.x > npc_get_definition(NPC_VASCO)->position.x);
    assert(npc_get_definition(NPC_ANDERSON)->seated_pose);
    assert(npc_get_definition(NPC_LAURA)->seated_pose);
    assert(npc_get_definition(NPC_NICOLLAS)->seated_pose);
    assert(npc_get_definition(NPC_VASCO)->seated_pose);
    assert(npc_get_definition(NPC_ANDERSON)->position.x == -262.0f);
    assert(npc_get_definition(NPC_ANDERSON)->position.y == -253.0f);
    assert(npc_get_definition(NPC_LAURA)->position.x == -262.0f);
    assert(npc_get_definition(NPC_NICOLLAS)->position.x == 76.0f);
    assert(npc_get_definition(NPC_VASCO)->position.x == 178.0f);
    assert(npc_get_definition(NPC_LAURA)->position.y == 3.0f);
    assert(npc_get_definition(NPC_NICOLLAS)->position.y == 3.0f);
    assert(npc_get_definition(NPC_VASCO)->position.y == 3.0f);
    assert(npc_get_definition(NPC_JULIO)->position.y <
           npc_get_definition(NPC_ZITO)->position.y);
    assert(npc_blocks_player(NPC_JULIO));
    assert(npc_blocks_player(NPC_JV));
    assert(npc_blocks_player(NPC_ZITO));
    assert(!npc_blocks_player(NPC_ANDERSON));
}

int main(void)
{
    test_each_npc_has_its_own_dialogue();
    test_exactly_one_npc_offers_a_hint_each_playable_day();
    test_non_owner_gets_no_hint_lines();
    test_npcs_are_stationary_and_face_the_requested_directions();

    printf("test_npcs: todos os testes passaram\n");
    return 0;
}
