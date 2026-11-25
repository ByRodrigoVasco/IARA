// Testes de unidade da nuvem: leitura das respostas, fila offline, trava e conflito. Sem rede: um
// servidor falso, na memória, imita as funções de tools/nuvem/supabase.sql.
// Compilar e rodar: tools/tests/run_tests.ps1
//   ou: gcc code/tests/test_cloud.c code/src/core/cloud.c code/src/core/cloud_json.c -Icode/src/core -o test_cloud.exe && ./test_cloud.exe
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "cloud.h"
#include "cloud_json.h"

#define TEST_FOLDER "build/tests/nuvem_teste"
#define MAX_SAVES 64

static int total = 0;
static int passed = 0;

static void check(const char *id, const char *description, bool condition)
{
    total++;
    if (condition) passed++;
    printf("  [%s] %s: %s\n", condition ? "OK" : "FALHA", id, description);
}

// -----------------------------------------------------------------------------
// Servidor falso
// -----------------------------------------------------------------------------

typedef struct
{
    bool used;
    char mode[16], name[32], content[4096], holder[48];
    int version;
} fake_save;

static struct
{
    CRITICAL_SECTION lock;
    fake_save saves[MAX_SAVES];
    bool offline;
    bool drop_next_write_answer;   // Grava, mas a resposta "se perde"
    int match_http;                // Código devolvido por registrar partida (200 por padrão)
    int matches;
    char last_match_name[32];
    long long last_extra;
    int releases;
} server;

typedef struct
{
    char mode[16], name[32], machine[48], content[4096], password[64];
    long long base, seconds, extra;
} fake_args;

static void read_args(const char *body, fake_args *args)
{
    cj_reader reader;
    char key[32];

    memset(args, 0, sizeof(*args));
    cj_init(&reader, body);
    cj_expect(&reader, '{');
    do
    {
        char *text = NULL;

        if (!cj_read_key(&reader, key, sizeof(key))) break;
        if (!strcmp(key, "p_versao_base")) cj_read_long(&reader, &args->base);
        else if (!strcmp(key, "p_segundos")) cj_read_long(&reader, &args->seconds);
        else if (!strcmp(key, "p_abates_extra")) cj_read_long(&reader, &args->extra);
        else if (!strcmp(key, "p_modo") || !strcmp(key, "p_tipo")) { text = cj_read_string(&reader); snprintf(args->mode, sizeof(args->mode), "%s", text ? text : ""); }
        else if (!strcmp(key, "p_nome")) { text = cj_read_string(&reader); snprintf(args->name, sizeof(args->name), "%s", text ? text : ""); }
        else if (!strcmp(key, "p_maquina")) { text = cj_read_string(&reader); snprintf(args->machine, sizeof(args->machine), "%s", text ? text : ""); }
        else if (!strcmp(key, "p_conteudo")) { text = cj_read_string(&reader); snprintf(args->content, sizeof(args->content), "%s", text ? text : ""); }
        else if (!strcmp(key, "p_senha")) { text = cj_read_string(&reader); snprintf(args->password, sizeof(args->password), "%s", text ? text : ""); }
        else cj_skip(&reader);
        free(text);
    } while (cj_consume(&reader, ','));
}

static fake_save *find_save(const char *mode, const char *name)
{
    for (int i = 0; i < MAX_SAVES; i++)
    {
        if (server.saves[i].used && !strcmp(server.saves[i].mode, mode) && !_stricmp(server.saves[i].name, name)) return &server.saves[i];
    }
    return NULL;
}

static fake_save *add_save(const char *mode, const char *name)
{
    for (int i = 0; i < MAX_SAVES; i++)
    {
        if (server.saves[i].used) continue;
        memset(&server.saves[i], 0, sizeof(server.saves[i]));
        server.saves[i].used = true;
        snprintf(server.saves[i].mode, sizeof(server.saves[i].mode), "%s", mode);
        snprintf(server.saves[i].name, sizeof(server.saves[i].name), "%s", name);
        return &server.saves[i];
    }
    return NULL;
}

static bool held_by_other(const fake_save *save, const char *machine)
{
    return save->holder[0] != '\0' && strcmp(save->holder, machine) != 0;
}

static char *row_answer(const char *status, const char *content, int version)
{
    cj_writer writer;
    char *object, *array;
    size_t size;

    cj_begin(&writer);
    cj_add_string(&writer, "status", status);
    if (content != NULL) cj_add_string(&writer, "conteudo", content);
    cj_add_int(&writer, "versao", version);
    object = cj_end(&writer);
    size = strlen(object) + 3;
    array = malloc(size);
    snprintf(array, size, "[%s]", object);
    free(object);
    return array;
}

static char *word_answer(const char *word)
{
    char *answer = malloc(strlen(word) + 3);

    sprintf(answer, "\"%s\"", word);
    return answer;
}

static bool fake_transport(const char *function, const char *body, int *http, char **response)
{
    fake_args args;
    fake_save *save;
    bool answered = true;

    EnterCriticalSection(&server.lock);
    if (server.offline)
    {
        LeaveCriticalSection(&server.lock);
        return false;
    }
    read_args(body, &args);
    save = find_save(args.mode, args.name);
    *http = 200;
    *response = NULL;

    if (!strcmp(function, "iara_criar_save"))
    {
        if (save != NULL) *response = word_answer("existe");
        else { add_save(args.mode, args.name); *response = word_answer("ok"); }
    }
    else if (!strcmp(function, "iara_abrir_save"))
    {
        if (save == NULL) *response = row_answer("nao_existe", "", 0);
        else if (held_by_other(save, args.machine)) *response = row_answer("em_uso", "", save->version);
        else
        {
            snprintf(save->holder, sizeof(save->holder), "%s", args.machine);
            *response = row_answer("ok", save->content, save->version);
        }
    }
    else if (!strcmp(function, "iara_gravar_save"))
    {
        if (save == NULL) save = add_save(args.mode, args.name);
        if (held_by_other(save, args.machine)) *response = row_answer("em_uso", NULL, save->version);
        else if (save->version != args.base) *response = row_answer("conflito", NULL, save->version);
        else
        {
            snprintf(save->content, sizeof(save->content), "%s", args.content);
            snprintf(save->holder, sizeof(save->holder), "%s", args.machine);
            save->version++;
            *response = row_answer("ok", NULL, save->version);
            if (server.drop_next_write_answer)
            {
                server.drop_next_write_answer = false;
                free(*response);
                *response = NULL;
                answered = false;
            }
        }
    }
    else if (!strcmp(function, "iara_renovar_save"))
    {
        if (save == NULL || held_by_other(save, args.machine)) *response = word_answer("perdido");
        else { snprintf(save->holder, sizeof(save->holder), "%s", args.machine); *response = word_answer("ok"); }
    }
    else if (!strcmp(function, "iara_liberar_save"))
    {
        if (save != NULL && !held_by_other(save, args.machine)) save->holder[0] = '\0';
        server.releases++;
        *response = word_answer("ok");
    }
    else if (!strcmp(function, "iara_apagar_save"))
    {
        if (strcmp(args.password, "certa \"1\"") != 0) *response = word_answer("senha");
        else if (save == NULL) *response = word_answer("nao_existe");
        else { save->used = false; *response = word_answer("ok"); }
    }
    else if (!strcmp(function, "iara_listar_saves"))
    {
        size_t size = 16384, length = 1;

        *response = malloc(size);
        strcpy(*response, "[");
        for (int i = 0; i < MAX_SAVES; i++)
        {
            fake_save *each = &server.saves[i];

            if (!each->used || strcmp(each->mode, args.mode) != 0) continue;
            length += (size_t)snprintf(*response + length, size - length,
                                       "%s{\"nome\":\"%s\",\"atualizado_em\":\"2026-11-25T12:00:00.5-03:00\",\"tem_progresso\":%s,\"em_uso\":%s}",
                                       length > 1 ? "," : "", each->name, each->version > 0 ? "true" : "false",
                                       held_by_other(each, args.machine) ? "true" : "false");
        }
        snprintf(*response + length, size - length, "]");
    }
    else if (!strcmp(function, "iara_registrar_partida"))
    {
        *http = server.match_http != 0 ? server.match_http : 200;
        if (*http == 200)
        {
            server.matches++;
            server.last_extra = strstr(body, "p_abates_extra") != NULL ? args.extra : -1;
            snprintf(server.last_match_name, sizeof(server.last_match_name), "%s", args.name);
        }
        *response = _strdup(*http == 200 ? (args.seconds > 7200 ? "0" : "3") : "{\"message\":\"erro\"}");
    }
    else if (!strcmp(function, "iara_ler_ranking"))
    {
        *response = _strdup("[{\"nome\":\"Ana\",\"segundos\":300,\"abates\":900,\"nivel\":10,\"chefes\":2,\"personagem\":4,\"venceu\":false,\"pontos\":11500},"
                            "{\"nome\":\"B\\u00e9\",\"segundos\":10,\"abates\":1,\"nivel\":1,\"chefes\":0,\"personagem\":0,\"venceu\":true,\"pontos\":2060}]");
    }
    else *http = 404, *response = _strdup("{}");
    LeaveCriticalSection(&server.lock);
    return answered;
}

// -----------------------------------------------------------------------------
// Apoio
// -----------------------------------------------------------------------------

static void set_offline(bool offline)
{
    EnterCriticalSection(&server.lock);
    server.offline = offline;
    LeaveCriticalSection(&server.lock);
}

// Cópia do estado de um save no servidor
static fake_save snapshot(const char *mode, const char *name)
{
    fake_save copy = {0};
    fake_save *save;

    EnterCriticalSection(&server.lock);
    save = find_save(mode, name);
    if (save != NULL) copy = *save;
    LeaveCriticalSection(&server.lock);
    return copy;
}

// Outro notebook mexe no save
static void other_machine(const char *mode, const char *name, const char *content, const char *holder)
{
    fake_save *save;

    EnterCriticalSection(&server.lock);
    save = find_save(mode, name);
    if (save == NULL) save = add_save(mode, name);
    if (content != NULL)
    {
        snprintf(save->content, sizeof(save->content), "%s", content);
        save->version++;
    }
    snprintf(save->holder, sizeof(save->holder), "%s", holder);
    LeaveCriticalSection(&server.lock);
}

static bool wait_result(int ticket, cloud_result *result)
{
    memset(result, 0, sizeof(*result));
    for (int i = 0; ticket != 0 && i < 600; i++)
    {
        if (cloud_poll(ticket, result)) return true;
        Sleep(5);
    }
    return false;
}

static cloud_status status_of(int ticket)
{
    cloud_result result;
    cloud_status status = wait_result(ticket, &result) ? result.status : (cloud_status)-1;

    cloud_result_free(&result);
    return status;
}

static bool wait_content(const char *mode, const char *name, const char *content)
{
    for (int i = 0; i < 600; i++)
    {
        if (strcmp(snapshot(mode, name).content, content) == 0) return true;
        Sleep(5);
    }
    return false;
}

static bool wait_pending(int count)
{
    for (int i = 0; i < 600; i++)
    {
        if (cloud_pending_count() == count) return true;
        Sleep(5);
    }
    return false;
}

static bool wait_notice(cloud_notice_kind kind)
{
    cloud_notice notice;

    for (int i = 0; i < 600; i++)
    {
        if (cloud_poll_notice(&notice)) return notice.kind == kind;
        Sleep(5);
    }
    return false;
}

static int count_files(const char *pattern)
{
    char full[600];
    WIN32_FIND_DATAA entry;
    HANDLE search;
    int count = 0;

    snprintf(full, sizeof(full), TEST_FOLDER "/%s", pattern);
    search = FindFirstFileA(full, &entry);
    if (search == INVALID_HANDLE_VALUE) return 0;
    do count++;
    while (FindNextFileA(search, &entry));
    FindClose(search);
    return count;
}

static void clean_folder(void)
{
    char full[600];
    WIN32_FIND_DATAA entry;
    HANDLE search = FindFirstFileA(TEST_FOLDER "/*", &entry);

    if (search == INVALID_HANDLE_VALUE) return;
    do
    {
        snprintf(full, sizeof(full), TEST_FOLDER "/%s", entry.cFileName);
        if (!(entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) remove(full);
    } while (FindNextFileA(search, &entry));
    FindClose(search);
}

static cloud_status open_save(const char *name)
{
    return status_of(cloud_open_save("sobrevivencia", name, NULL));
}

// -----------------------------------------------------------------------------
// Testes
// -----------------------------------------------------------------------------

static void test_json(void)
{
    cj_reader reader;
    cj_writer writer;
    char *text, *body;
    long long number = 0;
    bool flag = false;

    printf("\n  --- Leitura e escrita das mensagens ---\n\n");

    cj_init(&reader, " \"a\\n\\r\\t\\\"\\\\\\/\\u00e9\\ud83d\\ude00\" ");
    text = cj_read_string(&reader);
    check("NV-01", "os escapes e o Unicode viram UTF-8", text != NULL && strcmp(text, "a\n\r\t\"\\/\xc3\xa9\xf0\x9f\x98\x80") == 0);
    free(text);

    cj_init(&reader, "{\"x\":[1,{\"y\":\"]\"},true],\"n\":-42.5,\"b\":true,\"s\":null}");
    {
        char key[8];
        bool ok = cj_expect(&reader, '{') && cj_read_key(&reader, key, sizeof(key)) && cj_skip(&reader) && cj_consume(&reader, ',') &&
                  cj_read_key(&reader, key, sizeof(key)) && cj_read_long(&reader, &number) && cj_consume(&reader, ',') &&
                  cj_read_key(&reader, key, sizeof(key)) && cj_read_bool(&reader, &flag) && cj_consume(&reader, ',') &&
                  cj_read_key(&reader, key, sizeof(key));

        text = cj_read_string(&reader);
        check("NV-02", "valores desconhecidos são pulados; número, booleano e null são lidos", ok && number == -42 && flag && text != NULL && text[0] == '\0' && cj_expect(&reader, '}'));
        free(text);
    }

    {
        const char *broken[] = {"\"sem fim", "\"\\", "\"\\u12\"", "\"\\ud800\"", "\"\\q\"", "[1,", "{\"a\"1}", ""};
        bool all = true;

        for (int i = 0; i < 8; i++)
        {
            cj_init(&reader, broken[i]);
            all = !cj_skip(&reader) && reader.error && all;
        }
        check("NV-03", "texto cortado ou fora do formato é recusado sem passar do fim", all);
    }
    {
        char deep[400];

        memset(deep, '[', sizeof(deep) - 1);
        deep[sizeof(deep) - 1] = '\0';
        cj_init(&reader, deep);
        check("NV-04", "listas aninhadas demais são recusadas", !cj_skip(&reader));
    }

    cj_begin(&writer);
    cj_add_string(&writer, "p", "linha 1\r\n\"aspas\" \\ \x01 fim");
    cj_add_int(&writer, "n", 7);
    cj_add_bool(&writer, "b", false);
    body = cj_end(&writer);
    check("NV-05", "o corpo escrito escapa aspas, barras e quebras de linha",
          body != NULL && strcmp(body, "{\"p\":\"linha 1\\r\\n\\\"aspas\\\" \\\\ \\u0001 fim\",\"n\":7,\"b\":false}") == 0);
    free(body);

    check("NV-06", "a data do servidor vira segundos em UTC, com fuso", cj_parse_time("1970-01-02T00:00:01+00:00") == 86401 &&
                                                                          cj_parse_time("2026-11-25T12:00:00.123-03:00") == cj_parse_time("2026-11-25T15:00:00Z") &&
                                                                          cj_parse_time("ontem") == 0);
}

static void test_cloud(void)
{
    cloud_result result;
    cloud_match match = {"Ana", 300, 900, 10, 2, 4, false};
    char machine[40];
    int ticket;

    printf("\n  --- Nuvem desligada ---\n\n");
    check("NV-07", "sem transporte a nuvem fica desligada e não faz pedidos",
          !cloud_start(TEST_FOLDER, NULL) && !cloud_enabled() && cloud_list_saves("normal") == 0 && cloud_register_match("solo", &match) == 0 && cloud_pending_count() == 0);

    cloud_set_timing(60, 40);
    cloud_start(TEST_FOLDER, fake_transport);

    printf("\n  --- Saves ---\n\n");
    snprintf(machine, sizeof(machine), "%s", cloud_machine_id());
    check("NV-08", "o notebook ganha um código aleatório de 32 caracteres", strlen(machine) == 32 && strspn(machine, "0123456789abcdef") == 32);
    check("NV-09", "nome ou modo fora das regras não vira pedido", cloud_create_save("sobrevivencia", "../x") == 0 && cloud_create_save("outro", "Ana") == 0 &&
                                                                      cloud_open_save("normal", "abcdefghijklmnopqrstu", NULL) == 0);
    check("NV-10", "criar um save novo dá ok", status_of(cloud_create_save("sobrevivencia", "Ana")) == CLOUD_OK);
    check("NV-11", "nome repetido, sem diferenciar maiúsculas, dá existe", status_of(cloud_create_save("sobrevivencia", "ANA")) == CLOUD_EXISTS);

    other_machine("sobrevivencia", "Bia", "coins=1\n", "outro-notebook");
    ticket = cloud_list_saves("sobrevivencia");
    wait_result(ticket, &result);
    check("NV-12", "a lista traz nome, data, progresso e quem está em uso",
          result.status == CLOUD_OK && result.save_count == 2 && strcmp(result.saves[0].name, "Ana") == 0 && !result.saves[0].in_use && !result.saves[0].has_progress &&
              result.saves[1].in_use && result.saves[1].has_progress && result.saves[1].updated == cj_parse_time("2026-11-25T15:00:00Z"));
    cloud_result_free(&result);

    check("NV-13", "save aberto em outro notebook responde em uso", open_save("Bia") == CLOUD_IN_USE);
    check("NV-14", "save que não existe responde nao_existe", open_save("Ninguem") == CLOUD_NOT_FOUND);

    ticket = cloud_open_save("sobrevivencia", "Ana", NULL);
    wait_result(ticket, &result);
    check("NV-15", "abrir trava o save para este notebook", result.status == CLOUD_OK && result.content != NULL && result.content[0] == '\0' &&
                                                             strcmp(snapshot("sobrevivencia", "Ana").holder, machine) == 0);
    cloud_result_free(&result);

    cloud_store_save("sobrevivencia", "Ana", "coins=10\r\nruns=1\r\n");
    check("NV-16", "a gravação sobe com o conteúdo idêntico e sai da fila", wait_content("sobrevivencia", "Ana", "coins=10\r\nruns=1\r\n") && wait_pending(0));
    cloud_store_save("sobrevivencia", "Ana", "coins=20\n");
    check("NV-17", "a gravação seguinte parte da versão nova", wait_content("sobrevivencia", "Ana", "coins=20\n") && snapshot("sobrevivencia", "Ana").version == 2);

    printf("\n  --- Fila offline ---\n\n");
    set_offline(true);
    cloud_store_save("sobrevivencia", "Ana", "coins=30\n");
    Sleep(150);
    cloud_store_save("sobrevivencia", "Ana", "coins=40\n");
    Sleep(150);
    check("NV-18", "sem rede, só a gravação mais nova fica na fila", cloud_pending_count() == 1 && !cloud_online() && snapshot("sobrevivencia", "Ana").version == 2);
    set_offline(false);
    check("NV-19", "quando a rede volta, a fila sobe sozinha", wait_content("sobrevivencia", "Ana", "coins=40\n") && wait_pending(0));
    {
        char reason[128];
        bool online = cloud_connection(reason, sizeof(reason)) == CLOUD_LINK_ONLINE && reason[0] == '\0';
        bool offline = false;

        set_offline(true);
        // Sem nenhum pedido do jogo, a própria nuvem percebe a queda
        for (int i = 0; i < 600 && !offline; i++, Sleep(5)) offline = cloud_connection(reason, sizeof(reason)) == CLOUD_LINK_OFFLINE;
        check("NV-43", "a conexão passa de online para offline, com o motivo", online && offline && reason[0] != '\0');
        set_offline(false);
        for (int i = 0; i < 600 && !cloud_online(); i++) Sleep(5);
    }

    EnterCriticalSection(&server.lock);
    server.drop_next_write_answer = true;
    LeaveCriticalSection(&server.lock);
    cloud_store_save("sobrevivencia", "Ana", "coins=50\n");
    {
        cloud_notice notice;
        bool cleared = wait_pending(0);

        Sleep(100);
        check("NV-20", "resposta perdida de uma gravação aceita não vira conflito", cleared && !cloud_poll_notice(&notice) && snapshot("sobrevivencia", "Ana").version == 4);
    }
    cloud_store_save("sobrevivencia", "Ana", "coins=55\n");
    check("NV-21", "e as gravações seguintes continuam subindo", wait_content("sobrevivencia", "Ana", "coins=55\n"));

    {
        bool warned;

        Sleep(100);
        other_machine("sobrevivencia", "Ana", "coins=999\n", machine);
        cloud_store_save("sobrevivencia", "Ana", "coins=60\n");
        warned = wait_notice(CLOUD_NOTICE_CONFLICT);
        check("NV-22", "gravação que parte de uma versão antiga não sobrescreve e avisa", warned && strcmp(snapshot("sobrevivencia", "Ana").content, "coins=999\n") == 0);
        cloud_store_save("sobrevivencia", "Ana", "coins=61\n");
        Sleep(150);
        check("NV-23", "depois do conflito, nada sobe e a gravação mais nova fica na fila",
              cloud_pending_count() == 1 && strcmp(snapshot("sobrevivencia", "Ana").content, "coins=999\n") == 0);
    }
    ticket = cloud_open_save("sobrevivencia", "Ana", "coins=61\n");
    wait_result(ticket, &result);
    check("NV-24", "abrir de novo mostra a divergência, com as duas gravações",
          result.status == CLOUD_DIVERGED && !result.cloud_missing && result.content != NULL && strcmp(result.content, "coins=999\n") == 0 &&
              result.local_content != NULL && strcmp(result.local_content, "coins=61\n") == 0);
    cloud_result_free(&result);
    Sleep(150);
    check("NV-44", "enquanto o jogador não escolhe, nada sobe", cloud_pending_count() == 1 && strcmp(snapshot("sobrevivencia", "Ana").content, "coins=999\n") == 0);
    ticket = cloud_resolve_save("sobrevivencia", "Ana", false);
    wait_result(ticket, &result);
    check("NV-45", "escolher o online entrega o da nuvem e guarda uma cópia do local",
          result.status == CLOUD_OK && result.content != NULL && strcmp(result.content, "coins=999\n") == 0 && cloud_pending_count() == 0 &&
              count_files("conflito-sobrevivencia-Ana-*") == 1);
    cloud_result_free(&result);

    printf("\n  --- Trava ---\n\n");
    open_save("Ana");
    other_machine("sobrevivencia", "Ana", NULL, "outro-notebook");
    check("NV-25", "se outro notebook assume o save, a renovação avisa", wait_notice(CLOUD_NOTICE_LOST));
    cloud_close_save();
    Sleep(150);
    check("NV-26", "fechar um save perdido não tira a trava do outro", strcmp(snapshot("sobrevivencia", "Ana").holder, "outro-notebook") == 0);

    other_machine("sobrevivencia", "Ana", NULL, "");
    open_save("Ana");
    cloud_store_save("sobrevivencia", "Ana", "coins=70\n");
    cloud_close_save();
    {
        bool released = false;

        for (int i = 0; i < 600 && !released; i++, Sleep(5)) released = snapshot("sobrevivencia", "Ana").holder[0] == '\0';
        check("NV-27", "fechar o save sobe o que falta e libera a trava", released && strcmp(snapshot("sobrevivencia", "Ana").content, "coins=70\n") == 0);
    }

    printf("\n  --- Entre uma execução e outra ---\n\n");
    open_save("Ana");
    set_offline(true);
    cloud_store_save("sobrevivencia", "Ana", "coins=80\n");
    cloud_stop(3000);
    check("NV-28", "fechar o jogo sem rede deixa a gravação no disco", count_files("*.pend") == 1);
    EnterCriticalSection(&server.lock);
    server.offline = false;
    find_save("sobrevivencia", "Ana")->holder[0] = '\0';   // A trava venceu
    LeaveCriticalSection(&server.lock);
    cloud_start(TEST_FOLDER, fake_transport);
    check("NV-29", "o código do notebook é o mesmo na execução seguinte", strcmp(machine, cloud_machine_id()) == 0);
    {
        bool uploaded = wait_content("sobrevivencia", "Ana", "coins=80\n") && wait_pending(0);

        Sleep(100);
        check("NV-30", "a fila da execução anterior sobe e o save fica livre", uploaded && snapshot("sobrevivencia", "Ana").holder[0] == '\0');
    }

    // Gravação na fila de um save que alguém apagou
    open_save("Ana");
    cloud_store_save("sobrevivencia", "Ana", "coins=85\n");
    wait_content("sobrevivencia", "Ana", "coins=85\n");
    wait_pending(0);
    set_offline(true);
    cloud_store_save("sobrevivencia", "Ana", "coins=90\n");
    cloud_stop(3000);
    EnterCriticalSection(&server.lock);
    server.offline = false;
    find_save("sobrevivencia", "Ana")->used = false;
    LeaveCriticalSection(&server.lock);
    cloud_start(TEST_FOLDER, fake_transport);
    Sleep(200);
    check("NV-31", "a fila não recria um save apagado, e o progresso continua guardado", cloud_pending_count() == 1 && !snapshot("sobrevivencia", "Ana").used);
    ticket = cloud_open_save("sobrevivencia", "Ana", "coins=90\n");
    wait_result(ticket, &result);
    check("NV-46", "abrir um save apagado na nuvem pergunta antes", result.status == CLOUD_DIVERGED && result.cloud_missing);
    cloud_result_free(&result);
    check("NV-47", "para um save apagado não há online para escolher", status_of(cloud_resolve_save("sobrevivencia", "Ana", false)) == CLOUD_SERVER_ERROR);
    check("NV-48", "escolher o local envia o save de novo",
          status_of(cloud_resolve_save("sobrevivencia", "Ana", true)) == CLOUD_OK && wait_content("sobrevivencia", "Ana", "coins=90\n") && wait_pending(0));
    cloud_close_save();
    Sleep(150);

    // Abrir com uma gravação ainda na fila, mais nova e mais velha que a do servidor
    status_of(cloud_create_save("sobrevivencia", "Caio"));
    open_save("Caio");
    set_offline(true);
    cloud_store_save("sobrevivencia", "Caio", "coins=1\n");
    cloud_stop(3000);
    {
        // Sem a thread correr entre o início e o pedido, abre antes de a fila subir
        EnterCriticalSection(&server.lock);
        server.offline = false;
        find_save("sobrevivencia", "Caio")->holder[0] = '\0';
        cloud_start(TEST_FOLDER, fake_transport);
        ticket = cloud_open_save("sobrevivencia", "Caio", NULL);
        LeaveCriticalSection(&server.lock);
        wait_result(ticket, &result);
        check("NV-32", "abrir entrega a gravação da fila quando ela é a mais nova",
              result.status == CLOUD_OK && result.content != NULL && strcmp(result.content, "coins=1\n") == 0 && wait_content("sobrevivencia", "Caio", "coins=1\n"));
        cloud_result_free(&result);
    }
    wait_pending(0);
    set_offline(true);
    cloud_store_save("sobrevivencia", "Caio", "coins=2\n");
    cloud_stop(3000);
    EnterCriticalSection(&server.lock);
    server.offline = false;
    LeaveCriticalSection(&server.lock);
    other_machine("sobrevivencia", "Caio", "coins=500\n", "");
    cloud_start(TEST_FOLDER, fake_transport);
    ticket = cloud_open_save("sobrevivencia", "Caio", "coins=2\n");
    wait_result(ticket, &result);
    check("NV-33", "se os dois lados mudaram sem conexão, abrir pergunta qual fica",
          result.status == CLOUD_DIVERGED && result.content != NULL && strcmp(result.content, "coins=500\n") == 0 &&
              strcmp(snapshot("sobrevivencia", "Caio").content, "coins=500\n") == 0);
    cloud_result_free(&result);
    ticket = cloud_resolve_save("sobrevivencia", "Caio", true);
    wait_result(ticket, &result);
    check("NV-49", "escolher o local sobe o deste dispositivo e guarda o da nuvem",
          result.status == CLOUD_OK && strcmp(result.content, "coins=2\n") == 0 && wait_content("sobrevivencia", "Caio", "coins=2\n") && wait_pending(0) &&
              count_files("conflito-sobrevivencia-Caio-*") == 1);
    cloud_result_free(&result);
    cloud_close_save();
    Sleep(150);

    printf("\n  --- Jogar sem conexão ---\n\n");
    // Save criado sem conexão, que nunca foi aberto na nuvem
    set_offline(true);
    cloud_store_save("sobrevivencia", "Duda", "coins=3\n");
    Sleep(100);
    cloud_store_save("sobrevivencia", "Duda", "coins=4\n");
    check("NV-50", "o save criado sem conexão fica na fila", cloud_pending_count() == 1);
    set_offline(false);
    check("NV-51", "e é criado na nuvem quando a rede volta, sem ficar travado",
          wait_content("sobrevivencia", "Duda", "coins=4\n") && wait_pending(0) && (Sleep(100), snapshot("sobrevivencia", "Duda").holder[0] == '\0'));

    // Já sincronizado, jogado sem conexão sem ninguém mexer na nuvem: sobe sem perguntar
    set_offline(true);
    cloud_store_save("sobrevivencia", "Duda", "coins=5\n");
    Sleep(100);
    set_offline(false);
    check("NV-52", "um save já sincronizado sobe sozinho se a nuvem não mudou", wait_content("sobrevivencia", "Duda", "coins=5\n") && wait_pending(0));

    // Só a nuvem mudou: vale a dela, sem perguntar
    Sleep(100);
    other_machine("sobrevivencia", "Duda", "coins=600\n", "");
    ticket = cloud_open_save("sobrevivencia", "Duda", "coins=5\n");
    wait_result(ticket, &result);
    check("NV-53", "se só a nuvem mudou, abrir traz a dela sem perguntar", result.status == CLOUD_OK && strcmp(result.content, "coins=600\n") == 0);
    cloud_result_free(&result);
    cloud_close_save();
    Sleep(150);

    // Só o dispositivo mudou, sem nada na fila (jogado numa versão sem nuvem)
    ticket = cloud_open_save("sobrevivencia", "Duda", "coins=601\n");
    wait_result(ticket, &result);
    check("NV-54", "se só este dispositivo mudou, abrir mantém o local e envia",
          result.status == CLOUD_OK && strcmp(result.content, "coins=601\n") == 0 && wait_content("sobrevivencia", "Duda", "coins=601\n"));
    cloud_result_free(&result);
    cloud_close_save();
    Sleep(150);

    // Mesmo nome criado sem conexão e já usado na nuvem: nada é sobrescrito
    other_machine("sobrevivencia", "Eva", "coins=700\n", "");
    set_offline(true);
    cloud_store_save("sobrevivencia", "Eva", "coins=7\n");
    Sleep(100);
    set_offline(false);
    Sleep(200);
    check("NV-55", "um nome que já tem progresso na nuvem não é sobrescrito",
          cloud_pending_count() == 1 && strcmp(snapshot("sobrevivencia", "Eva").content, "coins=700\n") == 0 && snapshot("sobrevivencia", "Eva").holder[0] == '\0');
    ticket = cloud_open_save("sobrevivencia", "Eva", "coins=7\n");
    wait_result(ticket, &result);
    check("NV-56", "e abrir pergunta qual fica", result.status == CLOUD_DIVERGED);
    cloud_result_free(&result);
    cloud_close_save();
    Sleep(150);
    check("NV-57", "sair sem escolher mantém os dois e libera o save",
          cloud_pending_count() == 1 && strcmp(snapshot("sobrevivencia", "Eva").content, "coins=700\n") == 0 && snapshot("sobrevivencia", "Eva").holder[0] == '\0');
    cloud_forget_save("sobrevivencia", "Eva");
    check("NV-58", "apagar o save deste dispositivo tira o que estava na fila", cloud_pending_count() == 0);

    printf("\n  --- Apagar ---\n\n");
    check("NV-34", "senha errada não apaga", status_of(cloud_delete_save("sobrevivencia", "Caio", "errada")) == CLOUD_WRONG_PASSWORD && snapshot("sobrevivencia", "Caio").used);
    cloud_close_save();
    check("NV-35", "a senha certa apaga", status_of(cloud_delete_save("sobrevivencia", "Caio", "certa \"1\"")) == CLOUD_OK && !snapshot("sobrevivencia", "Caio").used);

    printf("\n  --- Ranking ---\n\n");
    ticket = cloud_register_match("solo", &match);
    wait_result(ticket, &result);
    check("NV-36", "a partida sobe e devolve a posição", result.status == CLOUD_OK && result.position == 3 && strcmp(server.last_match_name, "Ana") == 0 && wait_pending(0));
    cloud_result_free(&result);
    check("NV-59", "sem hora extra o parâmetro novo não vai ao servidor", server.last_extra == -1);
    {
        cloud_match overtime = {"Ana", 700, 1500, 20, 5, 4, true, 321};

        wait_result(cloud_register_match("solo", &overtime), &result);
        check("NV-60", "os abates da hora extra vão junto da partida", result.status == CLOUD_OK && server.last_extra == 321 && wait_pending(0));
        cloud_result_free(&result);
        server.matches--;
    }

    set_offline(true);
    ticket = cloud_register_match("dupla", &match);
    wait_result(ticket, &result);
    check("NV-37", "sem rede, a partida responde offline e fica na fila", result.status == CLOUD_OFFLINE && cloud_pending_count() == 1);
    cloud_result_free(&result);
    set_offline(false);
    check("NV-38", "e sobe quando a rede volta", wait_pending(0) && server.matches == 2);

    EnterCriticalSection(&server.lock);
    server.match_http = 400;
    LeaveCriticalSection(&server.lock);
    ticket = cloud_register_match("solo", &match);
    check("NV-39", "partida que o servidor não aceita sai da fila", status_of(ticket) == CLOUD_SERVER_ERROR && wait_pending(0));

    ticket = cloud_read_ranking("solo", 10);
    wait_result(ticket, &result);
    check("NV-40", "o ranking é lido na ordem, com acentos", result.status == CLOUD_OK && result.row_count == 2 && strcmp(result.rows[0].name, "Ana") == 0 &&
                                                              result.rows[0].kills == 900 && result.rows[0].character == 4 && strcmp(result.rows[1].name, "B\xc3\xa9") == 0 && result.rows[1].won);
    cloud_result_free(&result);

    set_offline(true);
    check("NV-41", "pedidos sem rede respondem offline", status_of(cloud_list_saves("normal")) == CLOUD_OFFLINE && status_of(cloud_read_ranking("solo", 10)) == CLOUD_OFFLINE &&
                                                              status_of(cloud_create_save("normal", "Ze")) == CLOUD_OFFLINE);
    cloud_stop(3000);
    check("NV-42", "a nuvem encerra", !cloud_enabled());
}

int main(void)
{
    InitializeCriticalSection(&server.lock);
    CreateDirectoryA("build", NULL);
    CreateDirectoryA("build/tests", NULL);
    CreateDirectoryA(TEST_FOLDER, NULL);
    clean_folder();

    test_json();
    test_cloud();
    clean_folder();

    printf("\n  %d de %d verificações passaram.\n\n", passed, total);
    return passed == total ? 0 : 1;
}
