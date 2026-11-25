#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "raylib.h"
#include "cloud.h"
#include "cloud_http.h"
#include "cloud_sync.h"
#include "notice.h"

#define PATH_BYTES 600
#define STOP_TIMEOUT_MS 6000
#define CLOUD_LOG_PATH "iara-desempenho.log"   // O log do jogo, na pasta de dados
#define CLOUD_LOG_MAX_LINES 100

void cloud_sync_start(void)
{
    if (cloud_start(CLOUD_FOLDER, cloud_http_transport())) TraceLog(LOG_INFO, "NUVEM: ligada");
    else TraceLog(LOG_INFO, "NUVEM: desligada, os saves e os rankings ficam neste computador");
}

void cloud_sync_stop(void)
{
    cloud_stop(STOP_TIMEOUT_MS);
}

// Lê o arquivo inteiro como está no disco; NULL se não existir ou passar do limite da nuvem
static char *read_whole_file(const char *path)
{
    FILE *file = fopen(path, "rb");
    char *content;
    size_t length;

    if (file == NULL) return NULL;
    content = malloc(CLOUD_CONTENT_MAX + 1);
    if (content == NULL)
    {
        fclose(file);
        return NULL;
    }
    length = fread(content, 1, CLOUD_CONTENT_MAX, file);
    fclose(file);
    content[length] = '\0';
    if (length >= CLOUD_CONTENT_MAX)
    {
        free(content);
        return NULL;
    }
    return content;
}

static bool write_whole_file(const char *path, const char *content)
{
    FILE *file = fopen(path, "wb");
    size_t length = strlen(content);
    bool ok;

    if (file == NULL) return false;
    ok = fwrite(content, 1, length, file) == length;
    return (fclose(file) == 0) && ok;
}

bool cloud_sync_write_local(save_mode mode, const char *name, const char *content)
{
    char path[PATH_BYTES], other[PATH_BYTES + 16], temp[PATH_BYTES + 16];
    char *current;

    if (content == NULL || !save_slot_ensure(mode, name) || !save_slot_path(mode, name, path, sizeof(path))) return false;

    // Progresso que só existe neste computador não se perde: a primeira versão dele fica guardada
    current = read_whole_file(path);
    snprintf(other, sizeof(other), "%s.anterior", path);
    if (current != NULL && strcmp(current, content) != 0 && !FileExists(other)) write_whole_file(other, current);
    free(current);

    // Sobras de uma gravação antiga seriam lidas no lugar do que veio da nuvem
    snprintf(other, sizeof(other), "%s.bak", path);
    remove(other);

    if (content[0] == '\0')
    {
        remove(path);
        return !FileExists(path);
    }
    snprintf(temp, sizeof(temp), "%s.tmp", path);
    if (!write_whole_file(temp, content))
    {
        remove(temp);
        return false;
    }
    remove(path);
    return rename(temp, path) == 0;
}

int cloud_sync_open_save(save_mode mode, const char *name)
{
    char path[PATH_BYTES];
    char *content = NULL;
    int ticket;

    if (save_slot_path(mode, name, path, sizeof(path))) content = read_whole_file(path);
    ticket = cloud_open_save(save_mode_folder(mode), name, content);
    free(content);
    return ticket;
}

void cloud_sync_active_save(void)
{
    char path[PATH_BYTES];
    char *content;

    if (!cloud_enabled() || !active_save_path(path, sizeof(path))) return;
    content = read_whole_file(path);
    if (content == NULL) return;
    cloud_store_save(save_mode_folder(active_save_mode()), active_save_name(), content);
    free(content);
}

void cloud_sync_close_save(void)
{
    cloud_close_save();
}

// Registra no log cada mudança da conexão, com o motivo quando cai
static void log_connection(void)
{
    static cloud_link last_link = CLOUD_LINK_UNKNOWN;
    static char last_reason[128] = "";
    static int lines = 0;
    char reason[128];
    cloud_link link = cloud_connection(reason, sizeof(reason));
    FILE *file;

    if (link == CLOUD_LINK_UNKNOWN || (link == last_link && strcmp(reason, last_reason) == 0)) return;
    last_link = link;
    snprintf(last_reason, sizeof(last_reason), "%s", reason);

    if (link == CLOUD_LINK_ONLINE) TraceLog(LOG_INFO, "NUVEM: online");
    else TraceLog(LOG_WARNING, "NUVEM: offline: %s", reason);
    if (lines >= CLOUD_LOG_MAX_LINES) return;
    file = fopen(CLOUD_LOG_PATH, "a");
    if (file == NULL) return;
    fprintf(file, "%.1f s  nuvem %s%s%s\n", GetTime(), link == CLOUD_LINK_ONLINE ? "online" : "offline", reason[0] != '\0' ? ": " : "", reason);
    fclose(file);
    lines++;
}

void cloud_sync_update(void)
{
    cloud_notice notice;
    char text[256];

    log_connection();
    if (!cloud_poll_notice(&notice)) return;
    if (notice.kind == CLOUD_NOTICE_LOST)
        snprintf(text, sizeof(text), "O save \"%s\" foi assumido ou apagado na nuvem. O jogo continua salvando neste dispositivo; ao abrir o save de novo, você escolhe qual fica.", notice.name);
    else
        snprintf(text, sizeof(text), "A nuvem tem outra gravação do save \"%s\". O jogo continua salvando neste dispositivo; ao abrir o save de novo, você escolhe qual fica.", notice.name);
    TraceLog(LOG_WARNING, "NUVEM: %s", text);
    notice_show(text, NOTICE_ERROR);
}
