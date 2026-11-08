// Testes de unidade dos saves em pastas: nomes, nomes repetidos, listar, criar, apagar e migrar o save antigo.
// Compilar e rodar: tools/tests/run_tests.ps1
//   ou: gcc code/tests/test_save_slots.c code/src/core/save_slots.c -Icode/src/core -o test_save_slots.exe -lraylib -lopengl32 -lgdi32 -lwinmm && ./test_save_slots.exe
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "raylib.h"
#include "save_slots.h"

#define TEST_ROOT "build/tests/saves_teste"
#define TEST_OUTSIDE "build/tests/fora_dos_saves"
#define TEST_LEGACY "build/tests/legado"

static int total = 0;
static int passed = 0;

static void check(const char *id, const char *description, bool condition)
{
    total++;
    if (condition) passed++;
    printf("  [%s] %s: %s\n", condition ? "OK" : "FALHA", id, description);
}

static void write_file(const char *path, const char *text)
{
    FILE *file = fopen(path, "wb");

    if (file != NULL)
    {
        fputs(text, file);
        fclose(file);
    }
}

static bool list_has(save_mode mode, const char *name)
{
    save_slot_info slots[SAVE_SLOTS_MAX];
    int count = save_slots_list(mode, slots, SAVE_SLOTS_MAX);

    for (int i = 0; i < count; i++)
    {
        if (strcmp(slots[i].name, name) == 0) return true;
    }
    return false;
}

// Apaga o que sobrou de uma execução anterior
static void clean(void)
{
    for (int mode = 0; mode < SAVE_MODE_COUNT; mode++)
    {
        save_slot_info slots[SAVE_SLOTS_MAX];
        int count = save_slots_list((save_mode)mode, slots, SAVE_SLOTS_MAX);

        for (int i = 0; i < count; i++) save_slot_delete((save_mode)mode, slots[i].name);
    }
    remove(TEST_LEGACY "/iara.save");
    remove(TEST_LEGACY "/iara.survival");
    remove(TEST_LEGACY "/iara.save.bak");
}

int main(void)
{
    char path[512];
    char summary[128];
    save_slot_info slots[SAVE_SLOTS_MAX];

    MakeDirectory(TEST_LEGACY);
    MakeDirectory(TEST_OUTSIDE);
    save_slots_set_root(TEST_ROOT);
    save_slots_ensure_folders();
    clean();

    printf("\n  --- Saves em pastas ---\n\n");

    check("SL-01", "as pastas dos dois modos são criadas dentro de saves", DirectoryExists(TEST_ROOT "/normal") && DirectoryExists(TEST_ROOT "/sobrevivencia"));
    check("SL-02", "sem saves, a lista vem vazia", save_slots_list(SAVE_MODE_NORMAL, slots, SAVE_SLOTS_MAX) == 0 && save_slots_list(SAVE_MODE_SURVIVAL, slots, SAVE_SLOTS_MAX) == 0);

    // Regras do nome
    check("SL-03", "nome vazio ou só com espaços é recusado", save_slot_check_name(SAVE_MODE_NORMAL, "") == SAVE_NAME_EMPTY && save_slot_check_name(SAVE_MODE_NORMAL, "   ") == SAVE_NAME_EMPTY);
    check("SL-04", "nome grande demais é recusado", save_slot_check_name(SAVE_MODE_NORMAL, "abcdefghijklmnopqrstu") == SAVE_NAME_TOO_LONG && save_slot_check_name(SAVE_MODE_NORMAL, "abcdefghijklmnopqrst") == SAVE_NAME_OK);
    check("SL-05", "caracteres que não servem para pasta são recusados", save_slot_check_name(SAVE_MODE_NORMAL, "a/b") == SAVE_NAME_INVALID && save_slot_check_name(SAVE_MODE_NORMAL, "..") == SAVE_NAME_INVALID &&
                                                                       save_slot_check_name(SAVE_MODE_NORMAL, "Jo\xc3\xa3o") == SAVE_NAME_INVALID && save_slot_check_name(SAVE_MODE_NORMAL, "a:b") == SAVE_NAME_INVALID);
    check("SL-06", "nomes que o Windows não aceita como pasta são recusados", save_slot_check_name(SAVE_MODE_NORMAL, "CON") == SAVE_NAME_RESERVED && save_slot_check_name(SAVE_MODE_NORMAL, "nul") == SAVE_NAME_RESERVED &&
                                                                              save_slot_check_name(SAVE_MODE_NORMAL, "com3") == SAVE_NAME_RESERVED);
    check("SL-07", "letras, números, espaço, - e _ são aceitos", save_slot_check_name(SAVE_MODE_NORMAL, "Ana 2-b_c") == SAVE_NAME_OK);

    // Criar e nomes repetidos
    check("SL-08", "um save novo cria a própria pasta", save_slot_create(SAVE_MODE_NORMAL, "Ana") && DirectoryExists(TEST_ROOT "/normal/Ana"));
    check("SL-09", "o nome não pode se repetir, sem diferenciar maiúsculas de minúsculas",
          save_slot_exists(SAVE_MODE_NORMAL, "ana") && save_slot_check_name(SAVE_MODE_NORMAL, "ANA") == SAVE_NAME_EXISTS && save_slot_check_name(SAVE_MODE_NORMAL, " ana ") == SAVE_NAME_EXISTS &&
              !save_slot_create(SAVE_MODE_NORMAL, "ANA"));
    check("SL-10", "cada modo tem os seus nomes: o mesmo nome vale nos dois", save_slot_check_name(SAVE_MODE_SURVIVAL, "Ana") == SAVE_NAME_OK && save_slot_create(SAVE_MODE_SURVIVAL, "Ana") && DirectoryExists(TEST_ROOT "/sobrevivencia/Ana"));
    check("SL-11", "o save criado com espaços nas pontas usa o nome sem eles", save_slot_create(SAVE_MODE_NORMAL, "  Bia  ") && list_has(SAVE_MODE_NORMAL, "Bia"));
    check("SL-12", "a lista mostra só os saves do modo", list_has(SAVE_MODE_NORMAL, "Ana") && list_has(SAVE_MODE_NORMAL, "Bia") && !list_has(SAVE_MODE_SURVIVAL, "Bia") && save_slots_list(SAVE_MODE_NORMAL, slots, SAVE_SLOTS_MAX) == 2);
    check("SL-13", "sem progresso gravado, a lista segue a ordem dos nomes", save_slots_list(SAVE_MODE_NORMAL, slots, SAVE_SLOTS_MAX) == 2 && strcmp(slots[0].name, "Ana") == 0 && strcmp(slots[1].name, "Bia") == 0);

    // Caminho e resumo
    check("SL-14", "o progresso fica dentro da pasta do save, com um arquivo por modo",
          save_slot_path(SAVE_MODE_NORMAL, "Ana", path, sizeof(path)) && strcmp(path, TEST_ROOT "/normal/Ana/progresso.save") == 0 &&
              save_slot_path(SAVE_MODE_SURVIVAL, "Ana", path, sizeof(path)) && strcmp(path, TEST_ROOT "/sobrevivencia/Ana/progresso.survival") == 0);
    check("SL-15", "nome com caracteres perigosos não vira caminho", !save_slot_path(SAVE_MODE_NORMAL, "../fora", path, sizeof(path)) && !save_slot_path(SAVE_MODE_NORMAL, "a/b", path, sizeof(path)));

    save_slot_summary(SAVE_MODE_NORMAL, "Ana", summary, sizeof(summary));
    check("SL-16", "save sem arquivo de progresso aparece como \"Sem progresso\"", strcmp(summary, "Sem progresso") == 0 && !save_slot_has_progress(SAVE_MODE_NORMAL, "Ana"));
    write_file(TEST_ROOT "/normal/Ana/progresso.save", "IARA_SAVE v1\ncurrent_day=12\nday_type=1\n");
    save_slot_summary(SAVE_MODE_NORMAL, "Ana", summary, sizeof(summary));
    check("SL-17", "o resumo do modo normal mostra o dia", strcmp(summary, "Dia 12") == 0 && save_slot_has_progress(SAVE_MODE_NORMAL, "Ana"));
    write_file(TEST_ROOT "/sobrevivencia/Ana/progresso.survival", "coins=340\nruns=6\nwins=0\n");
    save_slot_summary(SAVE_MODE_SURVIVAL, "Ana", summary, sizeof(summary));
    check("SL-18", "o resumo do modo sobrevivência mostra as horas e as partidas", strcmp(summary, "340 horas, 6 partidas") == 0);
    check("SL-19", "o save com progresso gravado ganha a data da última vez", (save_slots_list(SAVE_MODE_NORMAL, slots, SAVE_SLOTS_MAX), slots[0].modified > 0 || slots[1].modified > 0));

    // Save ativo
    active_save_clear();
    check("SL-20", "sem save ativo não há caminho", !active_save_is_set() && !active_save_path(path, sizeof(path)));
    active_save_set(SAVE_MODE_SURVIVAL, "Ana");
    check("SL-21", "o save ativo aponta para o arquivo do modo dele", active_save_is_set() && active_save_mode() == SAVE_MODE_SURVIVAL && strcmp(active_save_name(), "Ana") == 0 &&
                                                                         active_save_path(path, sizeof(path)) && strcmp(path, TEST_ROOT "/sobrevivencia/Ana/progresso.survival") == 0);
    active_save_clear();

    // Apagar
    write_file(TEST_ROOT "/normal/Ana/progresso.save.bak", "x");
    check("SL-22", "apagar remove os arquivos e a pasta do save", save_slot_delete(SAVE_MODE_NORMAL, "Ana") && !DirectoryExists(TEST_ROOT "/normal/Ana") && !list_has(SAVE_MODE_NORMAL, "Ana"));
    check("SL-23", "apagar um save não mexe no do outro modo nem nos outros saves", DirectoryExists(TEST_ROOT "/sobrevivencia/Ana") && list_has(SAVE_MODE_NORMAL, "Bia"));
    check("SL-24", "apagar um save que não existe falha sem estragar nada", !save_slot_delete(SAVE_MODE_NORMAL, "Ana") && list_has(SAVE_MODE_NORMAL, "Bia"));
    write_file(TEST_OUTSIDE "/importante.txt", "nao apagar");
    check("SL-25", "apagar não sai da pasta dos saves", !save_slot_delete(SAVE_MODE_NORMAL, "../../fora_dos_saves") && !save_slot_delete(SAVE_MODE_NORMAL, "..") && FileExists(TEST_OUTSIDE "/importante.txt"));
    check("SL-26", "depois de apagado, o nome pode ser usado de novo", save_slot_check_name(SAVE_MODE_NORMAL, "Ana") == SAVE_NAME_OK && save_slot_create(SAVE_MODE_NORMAL, "Ana"));

    // Saves antigos
    clean();
    write_file(TEST_LEGACY "/iara.save", "IARA_SAVE v1\ncurrent_day=7\n");
    write_file(TEST_LEGACY "/iara.survival", "coins=50\nruns=2\n");
    check("SL-27", "os dois saves antigos viram o save \"Principal\" de cada modo", save_slots_migrate_legacy_from(TEST_LEGACY) == 2 && list_has(SAVE_MODE_NORMAL, "Principal") && list_has(SAVE_MODE_SURVIVAL, "Principal"));
    save_slot_summary(SAVE_MODE_NORMAL, "Principal", summary, sizeof(summary));
    check("SL-28", "o progresso antigo segue valendo depois da migração", strcmp(summary, "Dia 7") == 0 && !FileExists(TEST_LEGACY "/iara.save") && !FileExists(TEST_LEGACY "/iara.survival"));
    write_file(TEST_LEGACY "/iara.save", "IARA_SAVE v1\ncurrent_day=99\n");
    check("SL-29", "se o modo já tem saves, o arquivo antigo não é mexido", save_slots_migrate_legacy_from(TEST_LEGACY) == 0 && FileExists(TEST_LEGACY "/iara.save"));

    clean();
    remove(TEST_OUTSIDE "/importante.txt");
    save_slots_set_root(NULL);

    printf("\n  %d de %d verificações passaram.\n\n", passed, total);
    return passed == total ? 0 : 1;
}
