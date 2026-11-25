#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cloud.h"
#include "cloud_json.h"

/* Só Windows aqui: o windows.h não pode se misturar com o raylib.h. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define RENEW_MS_DEFAULT 30000u       // A trava do servidor vence em 2 min
#define RETRY_MS_DEFAULT 15000u
#define PATH_BYTES 600
#define MODE_BYTES 16
#define PASSWORD_BYTES 128
#define RESULT_MAX 32                 // Respostas que ninguém buscou são descartadas, da mais velha
#define NOTICE_MAX 8
#define MATCH_TICKETS 32
#define PENDING_HEADER "IARA_PENDENTE 1\n"

typedef enum { JOB_LIST, JOB_CREATE, JOB_OPEN, JOB_RESOLVE, JOB_CLOSE, JOB_DELETE, JOB_RANKING } job_kind;

typedef struct job
{
    struct job *next;
    job_kind kind;
    int ticket;
    int limit;
    char mode[MODE_BYTES];
    char name[CLOUD_NAME_MAX + 1];
    char password[PASSWORD_BYTES];
    char *local;          // Abrir: o save deste dispositivo
    bool keep_local;      // Decidir a divergência
} job;

typedef struct finished
{
    struct finished *next;
    int ticket;
    cloud_result result;
} finished;

// O save aberto neste dispositivo
typedef struct
{
    bool set;
    bool lost;        // Outro notebook assumiu: nada mais sobe
    bool conflicted;  // Uma gravação foi recusada: nada mais sobe até abrir de novo
    bool stale;       // Houve falha de rede: conferir a versão do servidor antes de gravar
    bool undecided;   // Divergência esperando a escolha do jogador: nada sobe
    bool missing;     // O save não existe no servidor (apagado) enquanto a escolha não vem
    char *held_local, *held_server;   // As duas gravações da divergência
    char mode[MODE_BYTES];
    char name[CLOUD_NAME_MAX + 1];
    int version;
    ULONGLONG next_renew;
} open_save;

typedef struct
{
    int base;         // Versão do servidor em que esta gravação se baseia
    unsigned long long sequence;
    char *content;
} pending_save;

static struct
{
    bool enabled;
    bool stopping;
    cloud_link link;
    char reason[128];      // Motivo de estar offline
    ULONGLONG last_call;
    bool flush_now;
    cloud_transport transport;
    char folder[512];
    char machine[40];
    CRITICAL_SECTION lock;
    CONDITION_VARIABLE wake;
    HANDLE thread;
    job *jobs, *last_job;
    finished *results;
    open_save save;
    cloud_notice notices[NOTICE_MAX];
    int notice_count;
    struct { unsigned long long id; int ticket; } matches[MATCH_TICKETS];
    int next_ticket;
    unsigned long long next_sequence;
    ULONGLONG next_retry;
} cloud;

static unsigned renew_ms = RENEW_MS_DEFAULT;
static unsigned retry_ms = RETRY_MS_DEFAULT;

// -----------------------------------------------------------------------------
// Nomes e arquivos
// -----------------------------------------------------------------------------

static bool valid_name(const char *name)
{
    size_t length = name != NULL ? strlen(name) : 0;

    if (length == 0 || length > CLOUD_NAME_MAX) return false;
    for (size_t i = 0; i < length; i++)
    {
        char c = name[i];

        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ' || c == '-' || c == '_')) return false;
    }
    return true;
}

static bool valid_mode(const char *mode)
{
    return mode != NULL && (strcmp(mode, "normal") == 0 || strcmp(mode, "sobrevivencia") == 0 || strcmp(mode, "dupla") == 0);
}

static bool valid_type(const char *type)
{
    return type != NULL && (strcmp(type, "solo") == 0 || strcmp(type, "dupla") == 0);
}

static bool same_save(const char *mode_a, const char *name_a, const char *mode_b, const char *name_b)
{
    return strcmp(mode_a, mode_b) == 0 && _stricmp(name_a, name_b) == 0;
}

static void pending_path(char *out, size_t size, const char *mode, const char *name)
{
    snprintf(out, size, "%s/save-%s-%s.pend", cloud.folder, mode, name);
}

// Lê o arquivo inteiro (texto); NULL se não existir ou for grande demais
static char *read_file(const char *path)
{
    FILE *file = fopen(path, "rb");
    char *text;
    size_t length;

    if (file == NULL) return NULL;
    text = malloc(CLOUD_CONTENT_MAX + 256);
    if (text == NULL)
    {
        fclose(file);
        return NULL;
    }
    length = fread(text, 1, CLOUD_CONTENT_MAX + 255, file);
    fclose(file);
    text[length] = '\0';
    return text;
}

// Grava num temporário e troca de uma vez: uma queda de energia não deixa o arquivo pela metade
static bool write_file(const char *path, const char *header, const char *content)
{
    char temp[PATH_BYTES + 8];
    FILE *file;
    bool ok = true;

    snprintf(temp, sizeof(temp), "%s.tmp", path);
    file = fopen(temp, "wb");
    if (file == NULL) return false;
    if (header != NULL) ok = fputs(header, file) >= 0;
    if (ok) ok = fwrite(content, 1, strlen(content), file) == strlen(content);
    ok = (fclose(file) == 0) && ok;
    if (ok) ok = MoveFileExA(temp, path, MOVEFILE_REPLACE_EXISTING) != 0;
    if (!ok) remove(temp);
    return ok;
}

static bool write_pending(const char *path, const pending_save *pending)
{
    char header[128];

    snprintf(header, sizeof(header), PENDING_HEADER "base=%d\nseq=%llu\n---\n", pending->base, pending->sequence);
    return write_file(path, header, pending->content);
}

// Chamar com a trava. pending->content é alocado.
static bool read_pending(const char *path, pending_save *pending)
{
    char *text = read_file(path);
    char *body;

    if (text == NULL) return false;
    body = strstr(text, "\n---\n");
    if (strncmp(text, PENDING_HEADER, strlen(PENDING_HEADER)) != 0 || body == NULL ||
        sscanf(text + strlen(PENDING_HEADER), "base=%d\nseq=%llu", &pending->base, &pending->sequence) != 2)
    {
        free(text);
        return false;
    }
    pending->content = _strdup(body + 5);
    free(text);
    return pending->content != NULL;
}

// -----------------------------------------------------------------------------
// Última sincronização de cada save
// -----------------------------------------------------------------------------

static unsigned long long content_hash(const char *content)
{
    unsigned long long hash = 1469598103934665603ULL;   // FNV-1a

    for (; *content != '\0'; content++) hash = (hash ^ (unsigned char)*content) * 1099511628211ULL;
    return hash;
}

static void link_path(char *out, size_t size, const char *mode, const char *name)
{
    snprintf(out, size, "%s/link-%s-%s.sync", cloud.folder, mode, name);
}

// Chamar com a trava
static bool read_link(const char *mode, const char *name, int *version, unsigned long long *hash)
{
    char path[PATH_BYTES];
    char *text;
    bool ok;

    link_path(path, sizeof(path), mode, name);
    text = read_file(path);
    if (text == NULL) return false;
    ok = sscanf(text, "%d %llu", version, hash) == 2;
    free(text);
    return ok;
}

// Chamar com a trava: o servidor e este dispositivo têm "content" na versão "version"
static void write_link(const char *mode, const char *name, int version, const char *content)
{
    char path[PATH_BYTES], line[64];

    link_path(path, sizeof(path), mode, name);
    snprintf(line, sizeof(line), "%d %llu\n", version, content_hash(content));
    write_file(path, NULL, line);
}

// Chamar com a trava
static void forget_files(const char *mode, const char *name)
{
    char path[PATH_BYTES];

    pending_path(path, sizeof(path), mode, name);
    remove(path);
    link_path(path, sizeof(path), mode, name);
    remove(path);
}

// Guarda uma cópia da gravação que não vai valer, para ninguém perder o progresso
static void keep_conflict_copy(const char *mode, const char *name, const char *content)
{
    char path[PATH_BYTES];

    snprintf(path, sizeof(path), "%s/conflito-%s-%s-%llu-%llu.txt", cloud.folder, mode, name,
             (unsigned long long)time(NULL), cloud.next_sequence++);
    write_file(path, NULL, content);
}

// Chamar com a trava
static void push_notice(cloud_notice_kind kind, const char *name)
{
    if (cloud.notice_count == NOTICE_MAX) return;
    cloud.notices[cloud.notice_count].kind = kind;
    snprintf(cloud.notices[cloud.notice_count].name, sizeof(cloud.notices[0].name), "%s", name);
    cloud.notice_count++;
}

// Chamar com a trava: o envio do save aberto para até ele ser aberto de novo. A gravação continua
// na fila, e a próxima abertura pergunta qual vale.
static void block_open_save(cloud_notice_kind kind)
{
    if (!cloud.save.set || cloud.save.lost || cloud.save.conflicted) return;
    if (kind == CLOUD_NOTICE_LOST) cloud.save.lost = true;
    else cloud.save.conflicted = true;
    push_notice(kind, cloud.save.name);
}

// -----------------------------------------------------------------------------
// Chamadas
// -----------------------------------------------------------------------------

// Por que a chamada não deu certo, em palavras. status negativo = erro de rede do Windows.
static void describe_failure(bool answered, int status, char *out, size_t size)
{
    const char *text = NULL;

    if (!answered)
    {
        int error = -status;

        if (error == 12002) text = "o servidor demorou demais para responder";
        else if (error == 12007) text = "endereco do servidor nao encontrado (sem internet ou falha de DNS)";
        else if (error == 12029) text = "nao foi possivel conectar ao servidor (sem internet, firewall ou proxy)";
        else if (error == 12030) text = "a conexao caiu no meio da resposta";
        else if (error == 12175) text = "falha de seguranca na conexao (certificado; confira a data do computador)";
        else if (error > 0) snprintf(out, size, "erro de rede do Windows %d", error);
        else text = "sem resposta do servidor";
    }
    else if (status == 401 || status == 403) snprintf(out, size, "o servidor recusou a chave (HTTP %d)", status);
    else if (status == 404) snprintf(out, size, "funcao nao encontrada no servidor; confira o supabase.sql (HTTP 404)");
    else if (status >= 500) snprintf(out, size, "servidor com erro ou projeto pausado (HTTP %d)", status);
    else snprintf(out, size, "o servidor respondeu HTTP %d", status);
    if (text != NULL) snprintf(out, size, "%s", text);
}

// Faz a chamada e libera o corpo. CLOUD_OK = o servidor respondeu 200 (a resposta ainda será lida).
static cloud_status call(const char *function, char *body, char **response, int *http)
{
    int status = 0;
    char *text = NULL;
    bool answered;

    *response = NULL;
    if (http != NULL) *http = 0;
    if (body == NULL) return CLOUD_SERVER_ERROR;
    answered = cloud.transport(function, body, &status, &text);
    SecureZeroMemory(body, strlen(body));
    free(body);

    EnterCriticalSection(&cloud.lock);
    cloud.last_call = GetTickCount64();
    // HTTP 400 é a recusa de um pedido, com o servidor funcionando. 404 ao registrar uma partida da
    // hora extra é um banco ainda sem o parâmetro novo (supabase.sql): a partida espera na fila.
    if (answered && (status == 200 || status == 400 || (status == 404 && strcmp(function, "iara_registrar_partida") == 0)))
    {
        cloud.link = CLOUD_LINK_ONLINE;
        cloud.reason[0] = '\0';
    }
    else
    {
        cloud.link = CLOUD_LINK_OFFLINE;
        describe_failure(answered, status, cloud.reason, sizeof(cloud.reason));
    }
    LeaveCriticalSection(&cloud.lock);

    if (!answered) return CLOUD_OFFLINE;
    if (http != NULL) *http = status;
    if (status != 200 || text == NULL)
    {
        free(text);
        return status >= 500 ? CLOUD_OFFLINE : CLOUD_SERVER_ERROR;
    }
    *response = text;
    return CLOUD_OK;
}

static cloud_status status_from_word(const char *word)
{
    static const struct { const char *word; cloud_status status; } table[] = {
        {"ok", CLOUD_OK}, {"em_uso", CLOUD_IN_USE}, {"conflito", CLOUD_CONFLICT}, {"existe", CLOUD_EXISTS},
        {"invalido", CLOUD_INVALID}, {"nao_existe", CLOUD_NOT_FOUND}, {"senha", CLOUD_WRONG_PASSWORD}, {"perdido", CLOUD_LOST}};

    for (size_t i = 0; i < sizeof(table) / sizeof(table[0]); i++)
    {
        if (word != NULL && strcmp(word, table[i].word) == 0) return table[i].status;
    }
    return CLOUD_SERVER_ERROR;
}

// Funções que devolvem só o texto do status
static cloud_status call_word(const char *function, char *body)
{
    char *response, *word;
    cj_reader reader;
    cloud_status status = call(function, body, &response, NULL);

    if (status != CLOUD_OK) return status;
    cj_init(&reader, response);
    word = cj_read_string(&reader);
    status = status_from_word(word);
    free(word);
    free(response);
    return status;
}

static char *save_body(const char *mode, const char *name, bool with_machine)
{
    cj_writer writer;

    cj_begin(&writer);
    cj_add_string(&writer, "p_modo", mode);
    cj_add_string(&writer, "p_nome", name);
    if (with_machine) cj_add_string(&writer, "p_maquina", cloud.machine);
    return cj_end(&writer);
}

// [{"status":"ok","conteudo":"...","versao":3}] (abrir e gravar)
static cloud_status call_row(const char *function, char *body, char **content, int *version)
{
    char *response;
    char key[32];
    cj_reader reader;
    cloud_status status = call(function, body, &response, NULL);
    cloud_status row_status = CLOUD_SERVER_ERROR;

    if (content != NULL) *content = NULL;
    *version = 0;
    if (status != CLOUD_OK) return status;

    cj_init(&reader, response);
    cj_expect(&reader, '[');
    cj_expect(&reader, '{');
    do
    {
        if (!cj_read_key(&reader, key, sizeof(key))) break;
        if (strcmp(key, "status") == 0)
        {
            char *word = cj_read_string(&reader);

            row_status = status_from_word(word);
            free(word);
        }
        else if (strcmp(key, "conteudo") == 0 && content != NULL && *content == NULL) *content = cj_read_string(&reader);
        else if (strcmp(key, "versao") == 0)
        {
            long long number;

            cj_read_long(&reader, &number);
            *version = (int)number;
        }
        else cj_skip(&reader);
    } while (cj_consume(&reader, ','));
    cj_expect(&reader, '}');
    cj_expect(&reader, ']');
    free(response);

    if (!reader.error && row_status == CLOUD_OK && content != NULL && *content == NULL) reader.error = true;
    if (reader.error)
    {
        if (content != NULL)
        {
            free(*content);
            *content = NULL;
        }
        return CLOUD_SERVER_ERROR;
    }
    return row_status;
}

static cloud_status remote_open(const char *mode, const char *name, char **content, int *version)
{
    return call_row("iara_abrir_save", save_body(mode, name, true), content, version);
}

static cloud_status remote_write(const char *mode, const char *name, const char *content, int base, int *version)
{
    cj_writer writer;

    cj_begin(&writer);
    cj_add_string(&writer, "p_modo", mode);
    cj_add_string(&writer, "p_nome", name);
    cj_add_string(&writer, "p_maquina", cloud.machine);
    cj_add_string(&writer, "p_conteudo", content);
    cj_add_int(&writer, "p_versao_base", base);
    return call_row("iara_gravar_save", cj_end(&writer), NULL, version);
}

// -----------------------------------------------------------------------------
// Fila de saves
// -----------------------------------------------------------------------------

// Chamar com a trava
static bool is_open_save(const char *mode, const char *name)
{
    return cloud.save.set && same_save(cloud.save.mode, cloud.save.name, mode, name);
}

// A gravação "sent" chegou ao servidor na versão "version"
static void pending_uploaded(const char *mode, const char *name, const pending_save *sent, int version)
{
    char path[PATH_BYTES];
    pending_save now;

    pending_path(path, sizeof(path), mode, name);
    EnterCriticalSection(&cloud.lock);
    if (read_pending(path, &now))
    {
        if (now.sequence == sent->sequence) remove(path);
        else
        {
            // O jogo gravou de novo enquanto esta subia: a mais nova passa a partir da versão atual
            now.base = version;
            write_pending(path, &now);
            cloud.flush_now = true;
        }
        free(now.content);
        write_link(mode, name, version, sent->content);
    }
    if (is_open_save(mode, name))
    {
        cloud.save.version = version;
        cloud.save.stale = false;
    }
    LeaveCriticalSection(&cloud.lock);
}

// A gravação não pode subir agora: fica na fila. Com o save aberto, o jogador é avisado.
static void pending_refused(const char *mode, const char *name, cloud_notice_kind kind)
{
    EnterCriticalSection(&cloud.lock);
    if (is_open_save(mode, name)) block_open_save(kind);
    LeaveCriticalSection(&cloud.lock);
}

static void network_failed(const char *mode, const char *name)
{
    EnterCriticalSection(&cloud.lock);
    if (is_open_save(mode, name)) cloud.save.stale = true;
    LeaveCriticalSection(&cloud.lock);
}

// Envia a gravação que está na fila desse save. false = sem resposta (tentar mais tarde).
static bool flush_save(const char *mode, const char *name)
{
    char path[PATH_BYTES];
    pending_save pending;
    bool open, direct, linked, answered = true;
    cloud_status status;
    int version = 0, link_version = 0;
    unsigned long long link_hash = 0;

    pending_path(path, sizeof(path), mode, name);
    EnterCriticalSection(&cloud.lock);
    open = is_open_save(mode, name);
    if ((open && (cloud.save.undecided || cloud.save.lost || cloud.save.conflicted)) || !read_pending(path, &pending))
    {
        LeaveCriticalSection(&cloud.lock);
        return true;
    }
    direct = open && !cloud.save.stale;
    linked = read_link(mode, name, &link_version, &link_hash);
    LeaveCriticalSection(&cloud.lock);

    if (direct)
    {
        status = remote_write(mode, name, pending.content, pending.base, &version);
        if (status == CLOUD_OK) pending_uploaded(mode, name, &pending, version);
        else if (status == CLOUD_IN_USE) pending_refused(mode, name, CLOUD_NOTICE_LOST);
        else if (status == CLOUD_CONFLICT)
        {
            // Pode ser a própria gravação, cuja resposta se perdeu: a conferência abaixo decide
            direct = false;
        }
        else
        {
            network_failed(mode, name);
            answered = status != CLOUD_OFFLINE;
        }
    }

    if (!direct)
    {
        // Antes de gravar, abre o save: assim uma gravação antiga não recria um save apagado nem
        // passa por cima do que outro dispositivo fez
        char *content = NULL;
        int server_version = 0;
        bool write = false, locked = false;

        status = remote_open(mode, name, &content, &server_version);
        if (status == CLOUD_OK)
        {
            locked = true;
            if (strcmp(content, pending.content) == 0) pending_uploaded(mode, name, &pending, server_version);
            else if (server_version != pending.base) pending_refused(mode, name, CLOUD_NOTICE_CONFLICT);
            else write = true;
            free(content);
        }
        else if (status == CLOUD_NOT_FOUND)
        {
            // Save criado sem conexão: ainda não existe lá. Se já existiu, foi apagado.
            if (!linked && pending.base == 0) write = locked = true;
            else pending_refused(mode, name, CLOUD_NOTICE_LOST);
        }
        else if (status == CLOUD_IN_USE)
        {
            if (open) pending_refused(mode, name, CLOUD_NOTICE_LOST);
        }
        else
        {
            network_failed(mode, name);
            answered = status != CLOUD_OFFLINE;
        }

        if (write)
        {
            status = remote_write(mode, name, pending.content, pending.base, &version);
            if (status == CLOUD_OK) pending_uploaded(mode, name, &pending, version);
            else
            {
                network_failed(mode, name);
                answered = status != CLOUD_OFFLINE;
            }
        }
        // Um save que não está aberto aqui não pode ficar travado
        if (locked && !open) call_word("iara_liberar_save", save_body(mode, name, true));
    }

    free(pending.content);
    return answered;
}

// -----------------------------------------------------------------------------
// Fila de partidas
// -----------------------------------------------------------------------------

static void add_result(int ticket, const cloud_result *result);

// Linha do arquivo: tipo|segundos|abates|nivel|chefes|personagem|venceu|nome|abates da hora extra
static bool flush_match(const char *file_name)
{
    char path[PATH_BYTES], type[8], name[CLOUD_ROW_NAME_BYTES];
    char *text, *response = NULL;
    cloud_match match = {0};
    cloud_result result = {0};
    cj_writer writer;
    unsigned long long id = 0;
    int won = 0, http = 0, ticket = 0;

    snprintf(path, sizeof(path), "%s/%s", cloud.folder, file_name);
    text = read_file(path);
    if (text == NULL) return true;
    if (sscanf(text, "%7[^|]|%d|%d|%d|%d|%d|%d|%63[^|\n]|%d", type, &match.seconds, &match.kills, &match.level,
               &match.bosses, &match.character, &won, name, &match.extra_kills) < 8)
    {
        free(text);
        remove(path);
        return true;
    }
    free(text);
    sscanf(file_name, "partida-%llu", &id);

    cj_begin(&writer);
    cj_add_string(&writer, "p_tipo", type);
    cj_add_string(&writer, "p_nome", name);
    cj_add_int(&writer, "p_segundos", match.seconds);
    cj_add_int(&writer, "p_abates", match.kills);
    cj_add_int(&writer, "p_nivel", match.level);
    cj_add_int(&writer, "p_chefes", match.bosses);
    cj_add_int(&writer, "p_personagem", match.character);
    cj_add_bool(&writer, "p_venceu", won != 0);
    // Só quando há hora extra: assim as outras partidas entram mesmo num banco sem o parâmetro novo
    if (match.extra_kills > 0) cj_add_int(&writer, "p_abates_extra", match.extra_kills);
    result.status = call("iara_registrar_partida", cj_end(&writer), &response, &http);

    if (result.status == CLOUD_OK)
    {
        cj_reader reader;
        long long position = 0;

        cj_init(&reader, response);
        if (cj_read_long(&reader, &position)) result.position = (int)position;
        else result.status = CLOUD_SERVER_ERROR;
        free(response);
    }
    // Respondeu (aceitou ou recusou os dados): sai da fila. Sem resposta: fica para depois.
    if (result.status == CLOUD_OK || http == 400) remove(path);

    EnterCriticalSection(&cloud.lock);
    for (int i = 0; i < MATCH_TICKETS; i++)
    {
        if (cloud.matches[i].ticket != 0 && cloud.matches[i].id == id)
        {
            ticket = cloud.matches[i].ticket;
            cloud.matches[i].ticket = 0;   // O jogo recebe a primeira resposta só
        }
    }
    LeaveCriticalSection(&cloud.lock);
    if (ticket != 0) add_result(ticket, &result);
    return result.status != CLOUD_OFFLINE;
}

// Tenta enviar tudo o que está na fila; para no primeiro pedido sem resposta
static void flush_queue(void)
{
    char pattern[PATH_BYTES];
    WIN32_FIND_DATAA entry;
    HANDLE search;
    bool answered = true;

    snprintf(pattern, sizeof(pattern), "%s/*.pend", cloud.folder);
    search = FindFirstFileA(pattern, &entry);
    if (search == INVALID_HANDLE_VALUE) return;
    do
    {
        const char *file = entry.cFileName;

        if (strncmp(file, "partida-", 8) == 0) answered = flush_match(file);
        else if (strncmp(file, "save-", 5) == 0)
        {
            static const char *modes[] = {"normal", "sobrevivencia", "dupla"};

            for (int i = 0; i < 3; i++)
            {
                char prefix[32], name[CLOUD_NAME_MAX + 1];
                size_t skip, length = strlen(file);

                skip = (size_t)snprintf(prefix, sizeof(prefix), "save-%s-", modes[i]);
                if (strncmp(file, prefix, skip) != 0 || length < skip + 6 || length - skip - 5 > CLOUD_NAME_MAX) continue;
                memcpy(name, file + skip, length - skip - 5);
                name[length - skip - 5] = '\0';
                if (valid_name(name)) answered = flush_save(modes[i], name);
            }
        }
    } while (answered && FindNextFileA(search, &entry));
    FindClose(search);
}

// -----------------------------------------------------------------------------
// Pedidos
// -----------------------------------------------------------------------------

void cloud_result_free(cloud_result *result)
{
    if (result == NULL) return;
    free(result->content);
    free(result->local_content);
    free(result->saves);
    free(result->rows);
    memset(result, 0, sizeof(*result));
}

static void add_result(int ticket, const cloud_result *result)
{
    finished *done = calloc(1, sizeof(*done));
    finished **link;
    int count = 1;

    if (done == NULL) return;
    done->ticket = ticket;
    done->result = *result;

    EnterCriticalSection(&cloud.lock);
    for (link = &cloud.results; *link != NULL; link = &(*link)->next) count++;
    *link = done;
    if (count > RESULT_MAX)
    {
        finished *oldest = cloud.results;

        cloud.results = oldest->next;
        cloud_result_free(&oldest->result);
        free(oldest);
    }
    LeaveCriticalSection(&cloud.lock);
}

// Chamar com a trava
static void drop_held(void)
{
    free(cloud.save.held_local);
    free(cloud.save.held_server);
    cloud.save.held_local = cloud.save.held_server = NULL;
}

static void run_close(void)
{
    char mode[MODE_BYTES], name[CLOUD_NAME_MAX + 1];
    bool release;

    EnterCriticalSection(&cloud.lock);
    if (!cloud.save.set)
    {
        LeaveCriticalSection(&cloud.lock);
        return;
    }
    snprintf(mode, sizeof(mode), "%s", cloud.save.mode);
    snprintf(name, sizeof(name), "%s", cloud.save.name);
    LeaveCriticalSection(&cloud.lock);

    // A gravação sobe antes: gravar renovaria a trava depois de liberada
    flush_save(mode, name);

    EnterCriticalSection(&cloud.lock);
    release = !cloud.save.lost && !cloud.save.missing;
    drop_held();
    cloud.save.set = false;
    LeaveCriticalSection(&cloud.lock);
    if (release) call_word("iara_liberar_save", save_body(mode, name, true));
}

// Chamar com a trava
static void set_open_save(const char *mode, const char *name, int version)
{
    drop_held();
    memset(&cloud.save, 0, sizeof(cloud.save));
    cloud.save.set = true;
    snprintf(cloud.save.mode, sizeof(cloud.save.mode), "%s", mode);
    snprintf(cloud.save.name, sizeof(cloud.save.name), "%s", name);
    cloud.save.version = version;
    cloud.save.next_renew = GetTickCount64() + renew_ms;
}

// Chamar com a trava: vale a gravação deste dispositivo, que sobe em seguida
static void queue_local(const char *mode, const char *name, int base, const char *content)
{
    char path[PATH_BYTES];
    pending_save next;

    pending_path(path, sizeof(path), mode, name);
    next.base = base;
    next.sequence = ++cloud.next_sequence;
    next.content = (char *)content;
    write_pending(path, &next);
    cloud.flush_now = true;
}

// Chamar com a trava: vale a gravação do servidor
static void accept_server(const char *mode, const char *name, int version, const char *content)
{
    char path[PATH_BYTES];

    pending_path(path, sizeof(path), mode, name);
    remove(path);
    write_link(mode, name, version, content);
}

static void run_open(const job *request, cloud_result *result)
{
    const char *mode = request->mode, *name = request->name;
    char path[PATH_BYTES];
    pending_save pending = {0};
    const char *local;
    char *server = NULL;
    bool queued, linked, diverged = false, use_local = false;
    int version = 0, link_version = 0;
    unsigned long long link_hash = 0;

    run_close();
    result->status = remote_open(mode, name, &server, &version);
    if (result->status != CLOUD_OK && result->status != CLOUD_NOT_FOUND) return;

    pending_path(path, sizeof(path), mode, name);
    EnterCriticalSection(&cloud.lock);
    queued = read_pending(path, &pending);
    local = request->local != NULL && request->local[0] != '\0' ? request->local : queued ? pending.content : NULL;
    if (local != NULL && local[0] == '\0') local = NULL;
    linked = read_link(mode, name, &link_version, &link_hash);

    if (result->status == CLOUD_NOT_FOUND)
    {
        if (local != NULL)
        {
            set_open_save(mode, name, 0);
            result->status = CLOUD_OK;
            if (linked)   // Já esteve na nuvem e foi apagado: o jogador decide se envia de novo
            {
                diverged = true;
                cloud.save.missing = result->cloud_missing = true;
                server = _strdup("");
            }
            else use_local = true;   // Criado sem conexão: sobe agora
        }
    }
    else
    {
        set_open_save(mode, name, version);
        if (local == NULL || strcmp(local, server) == 0) accept_server(mode, name, version, server);
        else if (version == 0) use_local = true;   // O servidor nunca teve progresso
        else
        {
            // A fila partindo da versão atual prova que só este dispositivo mudou
            bool server_changed = !(queued && pending.base == version) && !(linked && link_version == version);
            bool local_changed = !linked || content_hash(local) != link_hash;

            if (!server_changed) use_local = true;
            else if (!local_changed) accept_server(mode, name, version, server);
            else diverged = true;
        }
    }

    if (result->status == CLOUD_OK)
    {
        if (use_local) queue_local(mode, name, version, local);
        if (diverged)
        {
            cloud.save.undecided = true;
            cloud.save.held_local = _strdup(local);
            cloud.save.held_server = _strdup(server != NULL ? server : "");
            result->status = CLOUD_DIVERGED;
            result->local_content = _strdup(local);
            result->content = server;
            server = NULL;
        }
        else
        {
            result->content = _strdup(use_local ? local : server);
        }
    }
    LeaveCriticalSection(&cloud.lock);
    free(server);
    if (queued) free(pending.content);
}

// O jogador escolheu qual gravação vale; a outra fica guardada em uma cópia
static void run_resolve(const job *request, cloud_result *result)
{
    const char *mode = request->mode, *name = request->name;

    EnterCriticalSection(&cloud.lock);
    if (!is_open_save(mode, name) || !cloud.save.undecided || cloud.save.held_local == NULL || cloud.save.held_server == NULL ||
        (!request->keep_local && cloud.save.missing))
    {
        result->status = CLOUD_SERVER_ERROR;
    }
    else
    {
        if (request->keep_local)
        {
            if (!cloud.save.missing) keep_conflict_copy(mode, name, cloud.save.held_server);
            else forget_files(mode, name);   // Volta a ser um save novo para o servidor
            queue_local(mode, name, cloud.save.version, cloud.save.held_local);
            result->content = _strdup(cloud.save.held_local);
        }
        else
        {
            keep_conflict_copy(mode, name, cloud.save.held_local);
            accept_server(mode, name, cloud.save.version, cloud.save.held_server);
            result->content = _strdup(cloud.save.held_server);
        }
        cloud.save.undecided = cloud.save.missing = false;
        cloud.save.next_renew = GetTickCount64() + renew_ms;
        drop_held();
        result->status = CLOUD_OK;
    }
    LeaveCriticalSection(&cloud.lock);
}

static void run_list(const job *request, cloud_result *result)
{
    cj_writer writer;
    cj_reader reader;
    char *response;
    int capacity = 0;

    cj_begin(&writer);
    cj_add_string(&writer, "p_modo", request->mode);
    cj_add_string(&writer, "p_maquina", cloud.machine);
    result->status = call("iara_listar_saves", cj_end(&writer), &response, NULL);
    if (result->status != CLOUD_OK) return;

    cj_init(&reader, response);
    cj_expect(&reader, '[');
    if (!cj_consume(&reader, ']'))
    {
        do
        {
            cloud_save_info info = {0};
            char key[32];

            cj_expect(&reader, '{');
            do
            {
                if (!cj_read_key(&reader, key, sizeof(key))) break;
                if (strcmp(key, "nome") == 0 || strcmp(key, "atualizado_em") == 0)
                {
                    char *text = cj_read_string(&reader);

                    if (text != NULL && key[0] == 'n') snprintf(info.name, sizeof(info.name), "%s", text);
                    else if (text != NULL) info.updated = cj_parse_time(text);
                    free(text);
                }
                else if (strcmp(key, "tem_progresso") == 0) cj_read_bool(&reader, &info.has_progress);
                else if (strcmp(key, "em_uso") == 0) cj_read_bool(&reader, &info.in_use);
                else cj_skip(&reader);
            } while (cj_consume(&reader, ','));
            if (!cj_expect(&reader, '}') || !valid_name(info.name)) continue;

            if (result->save_count == capacity)
            {
                cloud_save_info *grown = realloc(result->saves, sizeof(info) * (size_t)(capacity ? capacity * 2 : 32));

                if (grown == NULL) break;
                result->saves = grown;
                capacity = capacity ? capacity * 2 : 32;
            }
            result->saves[result->save_count++] = info;
        } while (cj_consume(&reader, ','));
        cj_expect(&reader, ']');
    }
    free(response);
    if (reader.error) result->status = CLOUD_SERVER_ERROR;
}

static void run_ranking(const job *request, cloud_result *result)
{
    cj_writer writer;
    cj_reader reader;
    char *response;
    int capacity = 0;

    cj_begin(&writer);
    cj_add_string(&writer, "p_tipo", request->mode);
    cj_add_int(&writer, "p_limite", request->limit);
    result->status = call("iara_ler_ranking", cj_end(&writer), &response, NULL);
    if (result->status != CLOUD_OK) return;

    cj_init(&reader, response);
    cj_expect(&reader, '[');
    if (!cj_consume(&reader, ']'))
    {
        do
        {
            cloud_match row = {0};
            char key[32];

            cj_expect(&reader, '{');
            do
            {
                long long number = 0;

                if (!cj_read_key(&reader, key, sizeof(key))) break;
                if (strcmp(key, "nome") == 0)
                {
                    char *text = cj_read_string(&reader);

                    if (text != NULL) snprintf(row.name, sizeof(row.name), "%s", text);
                    free(text);
                }
                else if (strcmp(key, "venceu") == 0) cj_read_bool(&reader, &row.won);
                else
                {
                    int *field = strcmp(key, "segundos") == 0 ? &row.seconds : strcmp(key, "abates") == 0 ? &row.kills
                               : strcmp(key, "nivel") == 0 ? &row.level : strcmp(key, "chefes") == 0 ? &row.bosses
                               : strcmp(key, "personagem") == 0 ? &row.character
                               : strcmp(key, "abates_extra") == 0 ? &row.extra_kills : NULL;

                    if (field == NULL) cj_skip(&reader);
                    else
                    {
                        cj_read_long(&reader, &number);
                        *field = (int)number;
                    }
                }
            } while (cj_consume(&reader, ','));
            if (!cj_expect(&reader, '}')) break;

            if (result->row_count == capacity)
            {
                cloud_match *grown = realloc(result->rows, sizeof(row) * (size_t)(capacity ? capacity * 2 : 32));

                if (grown == NULL) break;
                result->rows = grown;
                capacity = capacity ? capacity * 2 : 32;
            }
            result->rows[result->row_count++] = row;
        } while (cj_consume(&reader, ','));
        cj_expect(&reader, ']');
    }
    free(response);
    if (reader.error) result->status = CLOUD_SERVER_ERROR;
}

static void run_job(job *request)
{
    cloud_result result = {0};

    switch (request->kind)
    {
    case JOB_LIST:    run_list(request, &result); break;
    case JOB_CREATE:  result.status = call_word("iara_criar_save", save_body(request->mode, request->name, false)); break;
    case JOB_OPEN:    run_open(request, &result); break;
    case JOB_RESOLVE: run_resolve(request, &result); break;
    case JOB_CLOSE:   run_close(); break;
    case JOB_RANKING: run_ranking(request, &result); break;
    case JOB_DELETE:
    {
        cj_writer writer;

        cj_begin(&writer);
        cj_add_string(&writer, "p_modo", request->mode);
        cj_add_string(&writer, "p_nome", request->name);
        cj_add_string(&writer, "p_maquina", cloud.machine);
        cj_add_string(&writer, "p_senha", request->password);
        result.status = call_word("iara_apagar_save", cj_end(&writer));
        if (result.status == CLOUD_OK) cloud_forget_save(request->mode, request->name);
        break;
    }
    }
    if (request->ticket != 0) add_result(request->ticket, &result);
    else cloud_result_free(&result);
    SecureZeroMemory(request->password, sizeof(request->password));
    free(request->local);
    free(request);
}

// -----------------------------------------------------------------------------
// Trava do save aberto
// -----------------------------------------------------------------------------

static void renew_open_save(void)
{
    char mode[MODE_BYTES], name[CLOUD_NAME_MAX + 1];
    bool stale, queued;
    cloud_status status;
    char path[PATH_BYTES];

    EnterCriticalSection(&cloud.lock);
    if (!cloud.save.set || cloud.save.lost || cloud.save.missing || GetTickCount64() < cloud.save.next_renew)
    {
        LeaveCriticalSection(&cloud.lock);
        return;
    }
    cloud.save.next_renew = GetTickCount64() + renew_ms;
    snprintf(mode, sizeof(mode), "%s", cloud.save.mode);
    snprintf(name, sizeof(name), "%s", cloud.save.name);
    stale = cloud.save.stale;
    LeaveCriticalSection(&cloud.lock);

    status = call_word("iara_renovar_save", save_body(mode, name, true));
    if (status == CLOUD_LOST)
    {
        EnterCriticalSection(&cloud.lock);
        block_open_save(CLOUD_NOTICE_LOST);
        LeaveCriticalSection(&cloud.lock);
    }
    else if (status != CLOUD_OK) network_failed(mode, name);
    else if (stale)
    {
        // Voltou a rede. Sem nada na fila, confere se alguém gravou nesse meio tempo.
        pending_path(path, sizeof(path), mode, name);
        queued = GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
        if (!queued)
        {
            char *content = NULL;
            int version = 0;

            if (remote_open(mode, name, &content, &version) == CLOUD_OK)
            {
                EnterCriticalSection(&cloud.lock);
                if (is_open_save(mode, name))
                {
                    if (version != cloud.save.version) block_open_save(CLOUD_NOTICE_CONFLICT);
                    else cloud.save.stale = false;
                }
                LeaveCriticalSection(&cloud.lock);
                free(content);
            }
        }
    }
}

// -----------------------------------------------------------------------------
// Thread
// -----------------------------------------------------------------------------

// Sem outros pedidos, pergunta ao servidor de tempos em tempos: é o que mantém o "online" da tela
static void check_connection(void)
{
    cj_writer writer;
    char *response = NULL;
    bool idle;

    EnterCriticalSection(&cloud.lock);
    idle = cloud.last_call == 0 || GetTickCount64() - cloud.last_call >= retry_ms / 2;
    LeaveCriticalSection(&cloud.lock);
    if (!idle) return;

    cj_begin(&writer);
    cj_add_string(&writer, "p_tipo", "solo");
    cj_add_int(&writer, "p_limite", 1);
    call("iara_ler_ranking", cj_end(&writer), &response, NULL);
    free(response);
}

static DWORD WINAPI worker(LPVOID unused)
{
    (void)unused;
    for (;;)
    {
        job *request = NULL;
        bool flush = false, stopping;

        EnterCriticalSection(&cloud.lock);
        for (;;)
        {
            ULONGLONG now = GetTickCount64();
            ULONGLONG due = cloud.next_retry;

            if (cloud.jobs != NULL || cloud.flush_now || cloud.stopping || now >= due) break;
            if (cloud.save.set && !cloud.save.lost && cloud.save.next_renew < due) due = cloud.save.next_renew;
            if (now >= due) break;
            SleepConditionVariableCS(&cloud.wake, &cloud.lock, (DWORD)(due - now));
        }
        stopping = cloud.stopping;
        if (cloud.jobs != NULL)
        {
            request = cloud.jobs;
            cloud.jobs = request->next;
            if (cloud.jobs == NULL) cloud.last_job = NULL;
        }
        else if (cloud.flush_now || GetTickCount64() >= cloud.next_retry)
        {
            flush = true;
            cloud.flush_now = false;
            cloud.next_retry = GetTickCount64() + retry_ms;
        }
        LeaveCriticalSection(&cloud.lock);

        if (request != NULL) run_job(request);
        else if (stopping) break;
        else if (flush)
        {
            flush_queue();
            check_connection();
        }
        renew_open_save();
    }
    run_close();
    flush_queue();
    return 0;
}

// -----------------------------------------------------------------------------
// Pedidos do jogo
// -----------------------------------------------------------------------------

// Código aleatório deste notebook: os de laboratório costumam ter o mesmo nome
static void make_machine_id(char *out, size_t size)
{
    typedef BOOLEAN(WINAPI * random_function)(PVOID, ULONG);
    unsigned char bytes[16] = {0};
    HMODULE library = LoadLibraryA("advapi32.dll");
    random_function fill = library != NULL ? (random_function)(void *)GetProcAddress(library, "SystemFunction036") : NULL;

    if (fill == NULL || !fill(bytes, sizeof(bytes)))
    {
        LARGE_INTEGER counter;
        unsigned long long seed;

        QueryPerformanceCounter(&counter);
        seed = (unsigned long long)counter.QuadPart ^ ((unsigned long long)GetCurrentProcessId() << 32) ^ (unsigned long long)time(NULL);
        for (int i = 0; i < 16; i++)
        {
            seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
            bytes[i] = (unsigned char)(seed >> 56);
        }
    }
    if (library != NULL) FreeLibrary(library);
    for (int i = 0; i < 16 && (size_t)(i * 2 + 3) <= size; i++) snprintf(out + i * 2, 3, "%02x", bytes[i]);
}

static void load_machine_id(void)
{
    char path[PATH_BYTES];
    char *text;
    size_t length = 0;

    snprintf(path, sizeof(path), "%s/notebook.id", cloud.folder);
    text = read_file(path);
    if (text != NULL)
    {
        length = strspn(text, "0123456789abcdef");
        if (length == 32) snprintf(cloud.machine, sizeof(cloud.machine), "%.32s", text);
        free(text);
    }
    if (length != 32)
    {
        make_machine_id(cloud.machine, sizeof(cloud.machine));
        write_file(path, NULL, cloud.machine);
    }
}

bool cloud_start(const char *folder, cloud_transport transport)
{
    if (cloud.enabled || transport == NULL || folder == NULL) return false;
    memset(&cloud, 0, sizeof(cloud));
    snprintf(cloud.folder, sizeof(cloud.folder), "%s", folder);
    CreateDirectoryA(cloud.folder, NULL);
    if (GetFileAttributesA(cloud.folder) == INVALID_FILE_ATTRIBUTES) return false;

    cloud.transport = transport;
    cloud.next_sequence = (unsigned long long)time(NULL) * 1000u;
    load_machine_id();
    InitializeCriticalSection(&cloud.lock);
    InitializeConditionVariable(&cloud.wake);
    cloud.flush_now = true;   // O que ficou da última vez sobe agora
    cloud.next_retry = GetTickCount64() + retry_ms;
    cloud.enabled = true;
    cloud.thread = CreateThread(NULL, 0, worker, NULL, 0, NULL);
    if (cloud.thread == NULL)
    {
        DeleteCriticalSection(&cloud.lock);
        cloud.enabled = false;
        return false;
    }
    return true;
}

bool cloud_enabled(void)
{
    return cloud.enabled;
}

void cloud_stop(unsigned timeout_ms)
{
    if (!cloud.enabled) return;
    EnterCriticalSection(&cloud.lock);
    cloud.stopping = true;
    LeaveCriticalSection(&cloud.lock);
    WakeConditionVariable(&cloud.wake);

    // Se a rede não responder a tempo, o jogo fecha assim mesmo: a fila está no disco
    if (WaitForSingleObject(cloud.thread, timeout_ms) == WAIT_OBJECT_0)
    {
        while (cloud.results != NULL)
        {
            finished *done = cloud.results;

            cloud.results = done->next;
            cloud_result_free(&done->result);
            free(done);
        }
        drop_held();
        CloseHandle(cloud.thread);
        DeleteCriticalSection(&cloud.lock);
    }
    cloud.enabled = false;
}

const char *cloud_machine_id(void)
{
    return cloud.machine;
}

bool cloud_online(void)
{
    bool online;

    if (!cloud.enabled) return false;
    EnterCriticalSection(&cloud.lock);
    online = cloud.link == CLOUD_LINK_ONLINE;
    LeaveCriticalSection(&cloud.lock);
    return online;
}

cloud_link cloud_connection(char *reason, size_t size)
{
    cloud_link link = CLOUD_LINK_UNKNOWN;

    if (reason != NULL && size > 0) reason[0] = '\0';
    if (!cloud.enabled) return link;
    EnterCriticalSection(&cloud.lock);
    link = cloud.link;
    if (reason != NULL && size > 0) snprintf(reason, size, "%s", cloud.reason);
    LeaveCriticalSection(&cloud.lock);
    return link;
}

int cloud_pending_count(void)
{
    char pattern[PATH_BYTES];
    WIN32_FIND_DATAA entry;
    HANDLE search;
    int count = 0;

    if (!cloud.enabled) return 0;
    snprintf(pattern, sizeof(pattern), "%s/*.pend", cloud.folder);
    search = FindFirstFileA(pattern, &entry);
    if (search == INVALID_HANDLE_VALUE) return 0;
    do count++;
    while (FindNextFileA(search, &entry));
    FindClose(search);
    return count;
}

void cloud_set_timing(unsigned renew, unsigned retry)
{
    renew_ms = renew != 0 ? renew : RENEW_MS_DEFAULT;
    retry_ms = retry != 0 ? retry : RETRY_MS_DEFAULT;
}

// Põe o pedido no fim da fila e acorda a thread
static int enqueue(job *request, bool wants_result)
{
    int ticket = 0;

    EnterCriticalSection(&cloud.lock);
    if (wants_result) ticket = request->ticket = ++cloud.next_ticket;
    if (cloud.last_job != NULL) cloud.last_job->next = request;
    else cloud.jobs = request;
    cloud.last_job = request;
    LeaveCriticalSection(&cloud.lock);
    WakeConditionVariable(&cloud.wake);
    return ticket;
}

static job *new_job(job_kind kind, const char *mode, const char *name)
{
    job *request = calloc(1, sizeof(*request));

    if (request == NULL) return NULL;
    request->kind = kind;
    if (mode != NULL) snprintf(request->mode, sizeof(request->mode), "%s", mode);
    if (name != NULL) snprintf(request->name, sizeof(request->name), "%s", name);
    return request;
}

static int submit(job_kind kind, const char *mode, const char *name, const char *password, int limit, bool wants_result)
{
    job *request = new_job(kind, mode, name);

    if (request == NULL) return 0;
    request->limit = limit;
    if (password != NULL) snprintf(request->password, sizeof(request->password), "%s", password);
    return enqueue(request, wants_result);
}

int cloud_list_saves(const char *mode)
{
    return cloud.enabled && valid_mode(mode) ? submit(JOB_LIST, mode, NULL, NULL, 0, true) : 0;
}

int cloud_create_save(const char *mode, const char *name)
{
    return cloud.enabled && valid_mode(mode) && valid_name(name) ? submit(JOB_CREATE, mode, name, NULL, 0, true) : 0;
}

int cloud_open_save(const char *mode, const char *name, const char *local)
{
    job *request;

    if (!cloud.enabled || !valid_mode(mode) || !valid_name(name) || (local != NULL && strlen(local) >= CLOUD_CONTENT_MAX)) return 0;
    request = new_job(JOB_OPEN, mode, name);
    if (request == NULL) return 0;
    if (local != NULL) request->local = _strdup(local);
    return enqueue(request, true);
}

int cloud_resolve_save(const char *mode, const char *name, bool keep_local)
{
    job *request;

    if (!cloud.enabled || !valid_mode(mode) || !valid_name(name)) return 0;
    request = new_job(JOB_RESOLVE, mode, name);
    if (request == NULL) return 0;
    request->keep_local = keep_local;
    return enqueue(request, true);
}

void cloud_forget_save(const char *mode, const char *name)
{
    if (!cloud.enabled || !valid_mode(mode) || !valid_name(name)) return;
    EnterCriticalSection(&cloud.lock);
    forget_files(mode, name);
    LeaveCriticalSection(&cloud.lock);
}

int cloud_delete_save(const char *mode, const char *name, const char *password)
{
    return cloud.enabled && valid_mode(mode) && valid_name(name) && password != NULL
               ? submit(JOB_DELETE, mode, name, password, 0, true) : 0;
}

int cloud_read_ranking(const char *type, int limit)
{
    return cloud.enabled && valid_type(type) ? submit(JOB_RANKING, type, NULL, NULL, limit, true) : 0;
}

void cloud_close_save(void)
{
    if (cloud.enabled) submit(JOB_CLOSE, NULL, NULL, NULL, 0, false);
}

void cloud_store_save(const char *mode, const char *name, const char *content)
{
    char path[PATH_BYTES];
    pending_save old;
    int base = 0;
    unsigned long long hash = 0;

    if (!cloud.enabled || !valid_mode(mode) || !valid_name(name) || content == NULL || strlen(content) >= CLOUD_CONTENT_MAX) return;
    pending_path(path, sizeof(path), mode, name);

    EnterCriticalSection(&cloud.lock);
    // A versão de partida é a da fila, a do save aberto ou a da última sincronização (0 = nunca subiu).
    // O servidor só aceita a gravação se ela ainda for a atual.
    if (read_pending(path, &old))
    {
        base = old.base;
        free(old.content);
    }
    else if (is_open_save(mode, name)) base = cloud.save.version;
    else if (!read_link(mode, name, &base, &hash)) base = 0;
    queue_local(mode, name, base, content);
    LeaveCriticalSection(&cloud.lock);
    WakeConditionVariable(&cloud.wake);
}

int cloud_register_match(const char *type, const cloud_match *match)
{
    char path[PATH_BYTES], line[256];
    unsigned long long id;
    int ticket = 0;

    if (!cloud.enabled || !valid_type(type) || match == NULL || !valid_name(match->name)) return 0;
    snprintf(line, sizeof(line), "%s|%d|%d|%d|%d|%d|%d|%s|%d\n", type, match->seconds, match->kills, match->level,
             match->bosses, match->character, match->won ? 1 : 0, match->name, match->extra_kills);

    EnterCriticalSection(&cloud.lock);
    id = ++cloud.next_sequence;
    snprintf(path, sizeof(path), "%s/partida-%llu.pend", cloud.folder, id);
    if (write_file(path, NULL, line))
    {
        ticket = ++cloud.next_ticket;
        cloud.matches[ticket % MATCH_TICKETS].id = id;
        cloud.matches[ticket % MATCH_TICKETS].ticket = ticket;
        cloud.flush_now = true;
    }
    LeaveCriticalSection(&cloud.lock);
    WakeConditionVariable(&cloud.wake);
    return ticket;
}

bool cloud_poll(int ticket, cloud_result *out)
{
    bool found = false;

    if (!cloud.enabled || ticket == 0 || out == NULL) return false;
    EnterCriticalSection(&cloud.lock);
    for (finished **link = &cloud.results; *link != NULL; link = &(*link)->next)
    {
        finished *done = *link;

        if (done->ticket != ticket) continue;
        *out = done->result;
        *link = done->next;
        free(done);
        found = true;
        break;
    }
    LeaveCriticalSection(&cloud.lock);
    return found;
}

bool cloud_poll_notice(cloud_notice *out)
{
    bool found = false;

    if (!cloud.enabled || out == NULL) return false;
    EnterCriticalSection(&cloud.lock);
    if (cloud.notice_count > 0)
    {
        *out = cloud.notices[0];
        cloud.notice_count--;
        memmove(&cloud.notices[0], &cloud.notices[1], sizeof(cloud.notices[0]) * (size_t)cloud.notice_count);
        found = true;
    }
    LeaveCriticalSection(&cloud.lock);
    return found;
}
