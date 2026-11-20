#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "raylib.h"
#include "game_state.h"
#include "save.h"
unsigned int screen_width, screen_height, middle_screen_x, middle_screen_y;
static bool pressed = false, focused = true;
static bool story_test_key(int key) { return pressed && key == KEY_SPACE; }
#include "computer_loop.c"
#define IsKeyPressed(key) story_test_key(key)
#define IsMouseButtonPressed(button) false
#define IsWindowFocused() focused
#include "game_loop.c"
#undef IsWindowFocused
#undef IsMouseButtonPressed
#undef IsKeyPressed

static void advance_story(void)
{
    story_input_guard = false;
    npc_typed_characters = 1000;
    pressed = true;
    BeginDrawing();
    update_story_scene(0.05f);
    render_office_with_glass();
    render_npc_dialogue();
    EndDrawing();
    pressed = false;
}
static void finish_story(void)
{
    int scene = story_scene;
    for (int i = 0; i < 122 && story_approaching; i++) update_story_scene(0.05f);
    assert(!story_approaching);
    for (int i = 0; i < 8 && scene == story_scene; i++) advance_story();
    assert(scene != story_scene);
}

static Vector2 npc_feet(int id)
{
    Vector2 pos = npc_get_position(id);
    return (Vector2){pos.x + 37.5f * get_scale(), pos.y + 64 * get_scale()};
}

static bool npc_at_mark(int id, float x, float y)
{
    Vector2 feet = npc_feet(id), center = get_map_center();
    return hypotf((feet.x - center.x) / get_scale() - x, (feet.y - center.y) / get_scale() - y) < 3;
}

static int observe_office(unsigned int seed, Rectangle player, unsigned int automation)
{
    float scale = get_scale();
    Vector2 center = get_map_center();
    npc_seed_routines(seed);
    const struct hidden_statistic stats = {
        .current_automation_points = automation, .current_perception_points = 1, .current_human_bond_points = 1
    };
    npc_set_routine_statistics(&stats);
    npc_arrange_for_day(1, scale, center);
    npc_stage_day_one(scale, center);
    npc_end_reception();
    Vector2 starts[NPC_COUNT];
    float first_move[NPC_COUNT] = {0}, first_sit[NPC_COUNT] = {0};
    float first_outing[NPC_COUNT] = {0};
    bool printer = false;
    for (int id = 0; id < NPC_COUNT; id++) starts[id] = npc_get_position(id);
    for (int frame = 1; frame <= 3600; frame++)
    {
        update_npcs(0.05f, scale, center, map_collision_rec, office_obstacles, OBSTACLE_COUNT, player);
        for (int id = 0; id < NPC_COUNT; id++)
        {
            Vector2 position = npc_get_position(id);
            if (first_move[id] == 0 && hypotf(position.x - starts[id].x, position.y - starts[id].y) > 0.01f)
                first_move[id] = frame * 0.05f;
            if (!npc_blocks_player(id))
            {
                if (!first_sit[id]) first_sit[id] = frame * 0.05f;
            }
            else if (first_sit[id] && !first_outing[id]) first_outing[id] = frame * 0.05f;
        }
        printer |= npc_at_mark(NPC_ANDERSON, -365, -100) || npc_at_mark(NPC_ANDERSON, -365, -65);
    }
    int earliest = 0, outings = 0;
    float minimum_work = automation >= 6 ? 25 * 1.35f : 25;
    for (int id = 0; id < NPC_COUNT; id++)
    {
        assert(first_move[id] > 0 && first_sit[id] > first_move[id]);
        if (first_move[id] < first_move[earliest]) earliest = id;
        if (first_outing[id] > 0)
        {
            assert(first_outing[id] - first_sit[id] >= minimum_work - 0.1f);
            outings++;
        }
        for (int other = id + 1; other < NPC_COUNT; other++)
        {
            assert(first_move[id] != first_move[other]);
            assert(fabsf(first_sit[id] - first_sit[other]) >= 0.95f);
        }
    }
    assert(outings >= 3);
    printf("office seed %u: first NPC=%d, NPCs on outings=%d, printer=%d\n", seed, earliest, outings, printer);
    npc_set_routine_statistics(&current_hidden_statistic);
    return earliest;
}

static void expect_approach(int scene, Vector2 target)
{
    Vector2 start = npc_feet(NPC_JV);
    pressed = true;
    for (int i = 0; i < 122 && story_approaching; i++)
    {
        update_story_scene(0.05f);
        assert(story_scene == scene && story_line == 0 && npc_typed_characters == 0);
    }
    pressed = false;
    assert(!story_approaching);
    printf("scene %d: approach %.2fs\n", scene, story_action_time);
    Vector2 jv = npc_feet(NPC_JV);
    assert(hypotf(jv.x - start.x, jv.y - start.y) > 10 * get_scale());
    assert(hypotf(jv.x - target.x, jv.y - target.y) <= 90 * get_scale());
    if (scene == 4)
    {
        Vector2 vasco = npc_feet(NPC_VASCO);
        assert(hypotf(vasco.x - target.x, vasco.y - target.y) <= 90 * get_scale());
        assert(hypotf(vasco.x - jv.x, vasco.y - jv.y) >= 31 * get_scale());
    }
}

static void resize_during_approach(void)
{
    update_story_scene(0.05f);
    assert(story_approaching);
    Vector2 before = npc_feet(NPC_JV), old_center = get_map_center();
    float old_scale = get_scale();
    SetWindowSize(1060, 660);
    BeginDrawing(); EndDrawing();
    keep_player_place_on_resize();
    update_obstacle_recs();
    update_collision_rec();
    Vector2 after = npc_feet(NPC_JV), center = get_map_center();
    assert(get_scale() != old_scale);
    assert(fabsf((after.x - center.x) / get_scale() - (before.x - old_center.x) / old_scale) < 0.1f);
    assert(fabsf((after.y - center.y) / get_scale() - (before.y - old_center.y) / old_scale) < 0.1f);
}

static void verify_chair_timing(void)
{
    const int rates[] = {30, 60, 120};
    float duration = SEAT_PLAYER_FRAME_COUNT / 24.0f;
    const struct hidden_statistic high = {
        .current_automation_points = 6, .current_perception_points = 5, .current_human_bond_points = 3
    };
    HiddenStatisticBands bands = hidden_statistic_bands(&high);
    assert(bands.automation == 3 && bands.perception == 3 && bands.bond == 2);
    for (int i = 0; i < 3; i++)
    {
        float dt = 1.0f / rates[i];
        player_seat = CHAIR_SITTING;
        player_seat_time = 0;
        int frames = 0;
        while (player_seat == CHAIR_SITTING && frames++ < rates[i] * 2) update_player_seating(dt);
        assert(player_seat == CHAIR_SEATED && fabsf(frames * dt - duration) <= dt + 0.001f);
        player_seat = CHAIR_LEAVING;
        frames = 0;
        while (player_seat == CHAIR_LEAVING && frames++ < rates[i] * 2) update_player_seating(dt);
        assert(player_seat == CHAIR_IDLE && fabsf(frames * dt - duration) <= dt + 0.001f);
    }
    enter_computer_screen = false;
}

static void campaign_tick(void)
{
    BeginDrawing();
    handle_game_loop_screens();
    EndDrawing();
}

static void skip_campaign_card(void)
{
    assert(current_screen == SCREEN_NONE);
    campaign_tick();
    assert(current_screen == CALENDAR_DAY_CARD_SCREEN);
    render_calendar_day_card_finished = true;
    campaign_tick();
}

static void reload_campaign(void)
{
    computer_store_checkpoint();
    assert(save_game_state("build/tests/story.save") == SAVE_OK);
    if (computer_loop_loaded)
    {
        unload_computer_loop();
        computer_loop_loaded = false;
    }
    initialize_game_state_variables();
    initialize_hidden_statistic();
    assert(load_game_state("build/tests/story.save") == SAVE_OK);
    place_player_at_entrance();
    update_collision_rec();
    enter_current_day();
    skip_campaign_card();
}

static void mark_board(bool correct)
{
    DialogStep *step = &dialog_sequence[current_step_index];
    for (unsigned mask = 0; mask < (1u << step->card_count); mask++)
    {
        for (int i = 0; i < step->card_count; i++) triage_choices[i] = (mask >> i) & 1u;
        if ((count_board_wrong(step, triage_choices) == 0) == correct) return;
    }
    assert(!"não foi encontrada uma resposta para o teste");
}

static void verify_campaign_scenes(void)
{
    const int days[] = {12, 27, 41, 58, 74, 89, 100};
    // Percursos independentes: acerto direto, retry, envio sem conferir e tomada.
    enum { SOLVED, RETRY, UNCHECKED, TAKEN };
    for (unsigned i = 0; i < sizeof(days) / sizeof(*days); i++)
    {
        int day = days[i];
        for (int route = SOLVED; route <= TAKEN; route++)
        {
            initialize_game_state_variables();
            initialize_hidden_statistic();
            if (day == 74 && route != SOLVED) current_hidden_statistic.current_automation_points = 6;
            if (day == 100) current_game_state_variables.evidence_mask |= EVIDENCE_NO_RESIGNATION_LETTER;
            start_game_day(day);
            place_player_at_entrance();
            update_collision_rec();
            enter_current_day();
            skip_campaign_card();
            assert(story_scene == CAMPAIGN_ARRIVAL);
            if (day == 74) assert(story_first_actor(story_scene) == (route == SOLVED ? NPC_JULIO : NPC_ZITO));
            finish_story();
            assert(campaign_scene_completed(STORY_ARRIVAL));
            if (route == RETRY && (day == 41 || day == 89))
            {
                toggle_mural_note();
                toggle_mural_note();
            }

            initialize_computer_loop();
            computer_loop_loaded = true;
            current_screen = COMPUTER_SCREEN;
            current_step_index = FIRST_TICKET_STEP;
            reset_step_state();
            if (route == UNCHECKED)
            {
                ticket_memory_level = IARA_MEMORY_MEDIUM;
                bool expected = iara_answer_is_correct(current_step_index, ticket_memory_level);
                send_without_checking();
                assert(computer_first_outcome() == TASK_SENT_UNCHECKED &&
                       computer_first_answer_correct() == expected);
            }
            else if (route == TAKEN)
            {
                mark_board(false);
                current_wrong_answers = FAILSAFE_WRONG_ANSWERS - 1;
                submit_triage();
                continue_after_answer();
                assert(current_step_index == REPORT_STEP && computer_failsafe_triggered);
                assert(computer_story_day_choice() == -1);
            }
            else
            {
                mark_board(route == SOLVED);
                submit_triage();
                assert(current_quiz_answered && !story_scene);
                if (route == RETRY)
                {
                    reload_campaign(); // A primeira tentativa ainda espera o comando de continuar.
                    assert(current_screen == COMPUTER_SCREEN && current_quiz_answered);
                }
                continue_after_answer();
            }

            if (route != TAKEN)
            {
                assert(computer_story_middle_pending() && !current_game_state_variables.is_resolved);
                assert(current_step_index == FIRST_TICKET_STEP);
                campaign_tick();
                assert(story_scene == CAMPAIGN_MIDDLE);
                if (route == RETRY)
                {
                    int first_mark = triage_choices[0];
                    reload_campaign();
                    campaign_tick();
                    assert(story_scene == CAMPAIGN_MIDDLE && !campaign_scene_completed(STORY_MIDDLE));
                    assert(current_wrong_answers == 1 && triage_choices[0] == first_mark);
                }
                finish_story();
                assert(campaign_scene_completed(STORY_MIDDLE) && current_screen == COMPUTER_SCREEN);
                if (route == RETRY)
                {
                    start_checking();
                    mark_board(true);
                    submit_triage(); continue_after_answer();
                    campaign_tick();
                    assert(!story_scene && computer_first_outcome() == TASK_SOLVED_WITH_IARA);
                }
            }
            if (dialog_sequence[current_step_index].type == STEP_CHOICE)
            {
                choose_day_option(&dialog_sequence[current_step_index], route == RETRY ? 1 : 0);
                assert(computer_story_day_choice() == (route == RETRY ? 1 : 0));
            }
            assert(current_step_index == REPORT_STEP && current_game_state_variables.is_resolved);
            assert(!can_leave_today());
            if (day == 12)
                assert(!!(current_game_state_variables.evidence_mask & EVIDENCE_NO_RESIGNATION_LETTER) == (route == RETRY));

            advance_step();
            computer_tasks_finished = computer_exit_finished = true;
            int points = current_hidden_statistic.current_overall_points;
            campaign_tick();
            if (route == TAKEN)
            {
                assert(story_scene == CAMPAIGN_MIDDLE);
                finish_story();
                assert(campaign_scene_completed(STORY_MIDDLE));
            }
            assert(story_scene == CAMPAIGN_FINISH);
            reload_campaign();
            assert(story_scene == CAMPAIGN_FINISH);
            assert(points == current_hidden_statistic.current_overall_points);
            if (route == RETRY && day == 41) assert(current_game_state_variables.d41_mural_lido);
            if (route == RETRY && day == 89) assert(strstr(story_text(), "Você achou o mapa"));
            finish_story();
            assert(campaign_scene_completed(STORY_FINISH));
            assert(current_screen == ((route == TAKEN || day == 100) ? DAY_END_CARD_SCREEN : IN_GAME_SCREEN));
            if (day == 100)
            {
                assert(!current_game_state_variables.resolved_puzzles[100]);
                reload_campaign();
                assert(!story_scene && current_screen == DAY_END_CARD_SCREEN);
            }
            if (day == 58 && route == SOLVED)
            {
                assert(!(current_game_state_variables.evidence_mask & EVIDENCE_BADGE_AFTER_LEAVING));
                toggle_mural_note();
                toggle_mural_note();
                assert(current_game_state_variables.evidence_mask & EVIDENCE_BADGE_AFTER_LEAVING);
            }
            if (day == 89 && route == SOLVED)
            {
                assert(!current_game_state_variables.found_nicollas_trace);
                toggle_mural_note();
                assert(current_game_state_variables.found_nicollas_trace);
                toggle_mural_note();
            }
        }
    }

    initialize_game_state_variables(); initialize_hidden_statistic();
    start_game_day(41); enter_current_day(); assert(npc_is_present(NPC_JULIO));
    current_hidden_statistic.current_automation_points = 6;
    enter_current_day(); assert(npc_is_present(NPC_JULIO)); // A faixa cruzou no meio do dia.
    start_game_day(58); enter_current_day();
    assert(!npc_is_present(NPC_JULIO) && !npc_is_present(NPC_ANDERSON));
    assert(!npc_blocks_player(NPC_ANDERSON) && !npc_has_hint(NPC_ANDERSON, 89));
    assert(npc_get_daily_hint_owner(89) == NPC_LAURA && npc_get_daily_hint_owner(100) == NPC_JV);
    assert(save_game_state("build/tests/story.save") == SAVE_OK);
    initialize_game_state_variables();
    assert(load_game_state("build/tests/story.save") == SAVE_OK);
    start_game_day(74); initialize_hidden_statistic(); enter_current_day();
    assert(!npc_is_present(NPC_JULIO) && !npc_is_present(NPC_ANDERSON));
}

int main(void)
{
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(1280, 720, "IARA - cutscenes regression");
    SetTargetFPS(120);
    initialize_game_state_variables(); initialize_hidden_statistic();
    initialize_fonts(); initialize_game_loop_variables();
    verify_chair_timing();
    computer_mouse_capture_enabled = false;
    begin_story_scene(1, false);
    pressed = true;
    update_story_scene(0.05f); // abertura consome a entrada
    assert(story_line == 0);
    pressed = false;
    pause_menu_open();
    float before_pause = npc_typed_characters;
    render_game_loop();
    assert(story_line == 0 && npc_typed_characters == before_pause);
    pause_menu_close();
    focused = false; update_story_scene(0.05f); assert(story_line == 0); focused = true;
    RenderTexture2D target = LoadRenderTexture(1280, 720);
    npc_typed_characters = 1000; npc_balloon_open = 1;
    BeginTextureMode(target); ClearBackground(BLACK);
    render_office_scene(); render_npc_dialogue(); EndTextureMode();
    Image shot = LoadImageFromTexture(target.texture); ImageFlipVertical(&shot);
    ExportImage(shot, "build/scene01a.png"); UnloadImage(shot); UnloadRenderTexture(target);
    finish_story(); assert(current_game_state_variables.story_scenes == 1);
    Rectangle entrance = estagiario_collision_rec;
    current_estagiario_position = player_chair_entry();
    player_seat = CHAIR_SEATED;
    update_collision_rec();
    Vector2 desk = player_chair_anchor();
    // Abertura do computador antes de JV iniciar a ida para a cadeira.
    ticket_results[0].outcome = TASK_SOLVED_ALONE;
    begin_story_scene(2, true);
    expect_approach(2, desk);
    finish_story();

    int first = observe_office(71, entrance, 1);
    assert(observe_office(9001, entrance, 1) != first);
    assert(observe_office(71, entrance, 6) == first);
    npc_arrange_for_day(1, get_scale(), get_map_center());
    npc_end_reception();
    for (int frame = 0; frame < 240; frame++)
        update_npcs(0.05f, get_scale(), get_map_center(), map_collision_rec,
                    office_obstacles, OBSTACLE_COUNT, estagiario_collision_rec);
    assert(!npc_blocks_player(NPC_JV));

    for (int outcome = TASK_SOLVED_ALONE; outcome <= TASK_SENT_UNCHECKED; outcome++)
    {
        current_game_state_variables.story_scenes = 1;
        ticket_results[0].outcome = outcome;
        current_step_index = 3; iara_memory = 4;
        ticket_exposed_data = true; ticket_redacted_data = false;
        current_wrong_answers = 1; triage_choices[0] = 1;
        computer_store_checkpoint();
        assert(save_game_state("build/tests/story.save") == SAVE_OK);
        ticket_results[0].outcome = TASK_PENDING; current_step_index = 0;
        assert(load_game_state("build/tests/story.save") == SAVE_OK);
        reset_computer_progress(); assert(computer_restore_checkpoint());
        assert(current_step_index == 3 && ticket_results[0].outcome == outcome && iara_memory == 4);
        assert(ticket_exposed_data && !ticket_redacted_data && current_wrong_answers == 1 && triage_choices[0] == 1);
        begin_story_scene(2, true);
        story_line = 1;
        const char *expected[] = {"", "Você resolveu direto", "Você conferiu", "Já foi?"};
        assert(strstr(story_text(), expected[outcome]));
        story_line = 0;
        if (outcome == TASK_SOLVED_ALONE)
        {
            focused = false;
            update_story_scene(0.05f);
            assert(story_approaching && story_action_time == 0);
            focused = true;
            resize_during_approach();
            expect_approach(2, player_chair_anchor());
        }
        finish_story();
        assert(current_screen == COMPUTER_SCREEN && (current_game_state_variables.story_scenes & 2));
        assert(current_step_index == 3 && iara_memory == 4);
    }
    npc_arrange_for_day(1, get_scale(), get_map_center());
    npc_end_reception();
    player_seat = CHAIR_IDLE;
    Vector2 player_feet = {estagiario_collision_rec.x + estagiario_collision_rec.width / 2,
                           estagiario_collision_rec.y + estagiario_collision_rec.height / 2};
    begin_story_scene(4, false);
    expect_approach(4, player_feet);
    finish_story();
    assert(current_screen == IN_GAME_SCREEN);

    Vector2 before_block = npc_get_position(NPC_JV);
    office_obstacles[0] = map_collision_rec;
    begin_story_scene(2, true);
    for (int i = 0; i < 122 && story_approaching; i++) update_story_scene(0.05f);
    assert(!story_approaching && story_line == 0 && fabsf(story_action_time - 6) < 0.06f);
    assert(npc_get_position(NPC_JV).x == before_block.x && npc_get_position(NPC_JV).y == before_block.y);
    finish_story();
    update_obstacle_recs();
    // Tomada em Bruna: não conta delegação voluntária, M antes de F, depois encerra.
    current_game_state_variables.story_scenes = 1;
    current_step_index = 2;
    take_over_remaining_tickets();
    assert(computer_failsafe_triggered && current_game_state_variables.is_resolved);
    computer_store_checkpoint();
    unsigned int points = current_hidden_statistic.current_overall_points;
    assert(save_game_state("build/tests/story.save") == SAVE_OK);
    assert(load_game_state("build/tests/story.save") == SAVE_OK);
    reset_computer_progress(); assert(computer_restore_checkpoint());
    assert(current_step_index == REPORT_STEP && computer_failsafe_triggered);
    begin_story_scene(2, false);
    assert(strstr(story_text(), "terminou a fila"));
    finish_story(); assert(story_scene == 4);
    finish_story();
    assert(current_screen == DAY_END_CARD_SCREEN && current_game_state_variables.story_scenes == 7);
    assert(points == current_hidden_statistic.current_overall_points);
    // A cena não pode avançar nem disparar novamente após concluída.
    assert(current_game_state_variables.resolved_puzzles[1]);
    // O failsafe do Dia 3 preserva a escolha, inclusive após salvar e carregar.
    for (int option = 0; option < 2; option++)
    {
        initialize_game_state_variables(); initialize_hidden_statistic();
        start_game_day(3); reset_computer_progress();
        current_step_index = FIRST_TICKET_STEP;
        take_over_remaining_tickets();
        assert(computer_failsafe_triggered && !current_game_state_variables.is_resolved);
        assert(dialog_sequence[current_step_index].type == STEP_CHOICE);
        computer_store_checkpoint();
        assert(save_game_state("build/tests/story.save") == SAVE_OK);
        initialize_game_state_variables(); initialize_hidden_statistic();
        assert(load_game_state("build/tests/story.save") == SAVE_OK);
        reset_computer_progress(); assert(computer_restore_checkpoint());
        assert(computer_failsafe_triggered && dialog_sequence[current_step_index].type == STEP_CHOICE);
        choose_day_option(&dialog_sequence[current_step_index], option);
        assert(current_step_index == REPORT_STEP && current_game_state_variables.is_resolved);
        assert(ticket_results[0].outcome == TASK_TAKEN_OVER_BY_IARA);
        assert(current_hidden_statistic.current_human_bond_points == HIDDEN_STATISTIC_START + (option == 0));
        assert(current_hidden_statistic.current_automation_points == HIDDEN_STATISTIC_START);
        computer_store_checkpoint();
        assert(save_game_state("build/tests/story.save") == SAVE_OK);
        assert(load_game_state("build/tests/story.save") == SAVE_OK);
        reset_computer_progress(); assert(computer_restore_checkpoint());
        assert(current_step_index == REPORT_STEP && current_game_state_variables.is_resolved);
        assert(current_hidden_statistic.current_human_bond_points == HIDDEN_STATISTIC_START + (option == 0));
    }
    // Sair do computador durante o convite não encerra o expediente no Dia 3.
    initialize_game_state_variables(); initialize_hidden_statistic();
    start_game_day(3); reset_computer_progress();
    current_step_index = FIRST_TICKET_STEP; take_over_remaining_tickets();
    initialize_computer_loop(); computer_loop_loaded = true;
    current_screen = COMPUTER_SCREEN; computer_exit_finished = true;
    computer_tasks_finished = false;
    BeginDrawing(); handle_game_loop_screens(); EndDrawing();
    assert(current_screen == IN_GAME_SCREEN && !current_game_state_variables.is_resolved);
    assert(dialog_sequence[current_step_index].type == STEP_CHOICE);
    // Percurso real de telas: primeiro erro, pausa para Zito, convite e encerramento.
    initialize_game_state_variables(); initialize_hidden_statistic();
    start_game_day(3); reset_computer_progress();
    begin_story_scene(DAY3_ARRIVAL, false); finish_story();
    assert(current_game_state_variables.story_scenes & DAY3_ARRIVAL);
    initialize_computer_loop(); computer_loop_loaded = true;
    current_screen = COMPUTER_SCREEN;
    current_step_index = FIRST_TICKET_STEP;
    reset_step_state();
    for (int i = 0; i < MAX_TRIAGE_CARDS; i++) triage_choices[i] = 0;
    submit_triage(); continue_after_answer();
    BeginDrawing(); handle_game_loop_screens(); EndDrawing();
    assert(story_scene == DAY3_TRIAGE && triage_choices[0] == 0);
    finish_story();
    assert(current_screen == COMPUTER_SCREEN && current_wrong_answers == 1);
    take_over_remaining_tickets();
    choose_day_option(&dialog_sequence[current_step_index], 0);
    BeginDrawing(); handle_game_loop_screens(); EndDrawing();
    assert(story_scene == DAY3_LUNCH);
    finish_story();
    advance_step();
    computer_tasks_finished = true;
    computer_exit_finished = true;
    BeginDrawing(); handle_game_loop_screens(); EndDrawing();
    assert(story_scene == DAY3_END);
    finish_story();
    assert(current_screen == DAY_END_CARD_SCREEN);
    assert((current_game_state_variables.story_scenes & 120) == 120);
    assert(current_hidden_statistic.current_human_bond_points == HIDDEN_STATISTIC_START + 1);
    computer_store_checkpoint();
    assert(save_game_state("build/tests/story.save") == SAVE_OK);
    assert(load_game_state("build/tests/story.save") == SAVE_OK);
    assert((current_game_state_variables.story_scenes & 120) == 120);
    verify_campaign_scenes();
    unload_game_loop(); unload_fonts(); CloseWindow();
    puts("test_story: input, focus, variants, checkpoint and failsafe passed");
    return 0;
}
