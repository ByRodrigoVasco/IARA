// Testes de unidade do save local (US01: save automático ao fechar).
// Compilar e rodar: tools/tests/run_tests.ps1
//   ou: gcc code/tests/test_save.c code/src/core/save.c code/src/core/game_state.c -Icode/src/core -o test_save.exe -lraylib -lopengl32 -lgdi32 -lwinmm && ./test_save.exe
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "game_state.h"
#include "save.h"

#define TEST_SAVE_PATH "test_iara.save"

// Grava texto cru num arquivo, para simular saves corrompidos.
static void write_raw(const char *text)
{
    FILE *file = fopen(TEST_SAVE_PATH, "wb");
    assert(file != NULL);
    fputs(text, file);
    fclose(file);
}

// Lê o arquivo de save inteiro para buffer.
static size_t read_raw(char *buffer, size_t size)
{
    FILE *file = fopen(TEST_SAVE_PATH, "rb");
    assert(file != NULL);
    size_t length = fread(buffer, 1, size - 1, file);
    buffer[length] = '\0';
    fclose(file);
    return length;
}

static void reset_state(void)
{
    initialize_game_state_variables();
    initialize_hidden_statistic();
}

int main(void)
{
    remove(TEST_SAVE_PATH);
    reset_state();

    // Sem arquivo: a primeira partida começa no Dia 1, sem alterar nada
    assert(load_game_state(TEST_SAVE_PATH) == SAVE_NOT_FOUND);
    assert(current_game_state_variables.current_day == 1);

    // Ida e volta: tudo o que é gravado volta igual
    current_game_state_variables.current_screen = PLAYING_SCREEN;
    current_game_state_variables.current_day = 12;
    current_game_state_variables.day_type = FLASH_DAY;
    current_game_state_variables.mode_type = true;
    current_game_state_variables.user_investigated = true;
    current_game_state_variables.is_resolved = true;
    current_game_state_variables.resolved_puzzles[1] = true;
    current_game_state_variables.resolved_puzzles[3] = true;
    current_game_state_variables.resolved_puzzles[TOTAL_GAME_DAYS] = true;
    current_hidden_statistic.current_overall_points = -4;
    current_hidden_statistic.current_automation_points = 7;
    current_hidden_statistic.current_perception_points = 3;
    current_hidden_statistic.current_human_bond_points = 9;

    assert(save_game_state(TEST_SAVE_PATH) == SAVE_OK);

    reset_state();
    current_game_state_variables.current_screen = PRE_MENU_SCREEN;
    assert(load_game_state(TEST_SAVE_PATH) == SAVE_OK);

    assert(current_game_state_variables.current_day == 12);
    assert(current_game_state_variables.day_type == FLASH_DAY);
    assert(current_game_state_variables.mode_type == true);
    assert(current_game_state_variables.user_investigated == true);
    assert(current_game_state_variables.is_resolved == true);
    assert(current_game_state_variables.resolved_puzzles[1] == true);
    assert(current_game_state_variables.resolved_puzzles[2] == false);
    assert(current_game_state_variables.resolved_puzzles[3] == true);
    assert(current_game_state_variables.resolved_puzzles[TOTAL_GAME_DAYS] == true);
    assert(current_hidden_statistic.current_overall_points == -4);
    assert(current_hidden_statistic.current_automation_points == 7);
    assert(current_hidden_statistic.current_perception_points == 3);
    assert(current_hidden_statistic.current_human_bond_points == 9);

    // A tela não é restaurada: o jogo sempre reabre pelo pré-menu
    assert(current_game_state_variables.current_screen == PRE_MENU_SCREEN);

    // Gravar de novo sobrescreve o save anterior (no Windows rename não sobrescreve)
    current_game_state_variables.current_day = 13;
    assert(save_game_state(TEST_SAVE_PATH) == SAVE_OK);
    reset_state();
    assert(load_game_state(TEST_SAVE_PATH) == SAVE_OK);
    assert(current_game_state_variables.current_day == 13);

    // O arquivo temporário não fica para trás
    FILE *temp = fopen(TEST_SAVE_PATH ".tmp", "rb");
    assert(temp == NULL);

    // Save truncado (jogo caiu no meio da gravação de outra ferramenta): rejeitado
    char content[8192];
    size_t length = read_raw(content, sizeof(content));
    content[length / 2] = '\0';
    write_raw(content);
    reset_state();
    assert(load_game_state(TEST_SAVE_PATH) == SAVE_CORRUPTED);
    assert(current_game_state_variables.current_day == 1); // nada foi alterado

    // Save editado à mão (checksum não bate): rejeitado
    current_game_state_variables.current_day = 13;
    assert(save_game_state(TEST_SAVE_PATH) == SAVE_OK);
    length = read_raw(content, sizeof(content));
    char *day = strstr(content, "current_day=13");
    assert(day != NULL);
    day[strlen("current_day=")] = '9';
    write_raw(content);
    reset_state();
    assert(load_game_state(TEST_SAVE_PATH) == SAVE_CORRUPTED);
    assert(current_game_state_variables.current_day == 1);

    // Lixo e arquivo vazio: rejeitados sem travar
    write_raw("isso nao e um save");
    assert(load_game_state(TEST_SAVE_PATH) == SAVE_CORRUPTED);
    write_raw("");
    assert(load_game_state(TEST_SAVE_PATH) == SAVE_CORRUPTED);

    // Cabeçalho certo, mas sem checksum: rejeitado
    write_raw(SAVE_FILE_HEADER "\ncurrent_day=5\n");
    assert(load_game_state(TEST_SAVE_PATH) == SAVE_CORRUPTED);
    assert(current_game_state_variables.current_day == 1);

    remove(TEST_SAVE_PATH);

    printf("Todos os testes passaram\n");
    return 0;
}
