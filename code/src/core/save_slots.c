#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
#include <direct.h>
#define REMOVE_FOLDER(path) _rmdir(path)
#else
#include <unistd.h>
#define REMOVE_FOLDER(path) rmdir(path)
#endif

#include "raylib.h"
#include "save_slots.h"

#define PATH_MAX_LENGTH 512

static char saves_root[PATH_MAX_LENGTH] = SAVES_ROOT_DEFAULT;

static char active_name[SAVE_NAME_MAX + 1] = "";
static save_mode active_mode = SAVE_MODE_NORMAL;
static bool active_is_set = false;

// -----------------------------------------------------------------------------
// Nomes e caminhos
// -----------------------------------------------------------------------------

void save_slots_set_root(const char *root)
{
    snprintf(saves_root, sizeof(saves_root), "%s", (root != NULL && root[0] != '\0') ? root : SAVES_ROOT_DEFAULT);
}

const char *save_slots_root(void)
{
    return saves_root;
}

const char *save_mode_folder(save_mode mode)
{
    return mode == SAVE_MODE_SURVIVAL ? "sobrevivencia" : "normal";
}

const char *save_mode_title(save_mode mode)
{
    return mode == SAVE_MODE_SURVIVAL ? "MODO SOBREVIVÊNCIA" : "MODO NORMAL";
}

static const char *progress_file_name(save_mode mode)
{
    return mode == SAVE_MODE_SURVIVAL ? "progresso.survival" : "progresso.save";
}

static bool equal_ignoring_case(const char *a, const char *b)
{
    while (*a != '\0' && *b != '\0')
    {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return false;
        a++;
        b++;
    }
    return *a == *b;
}

static bool is_allowed_character(unsigned char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ' || c == '-' || c == '_';
}

// Só nomes com os caracteres permitidos viram caminho: nada de barras, pontos ou "..".
static bool is_safe_name(const char *name)
{
    size_t length;

    if (name == NULL) return false;
    length = strlen(name);
    if (length == 0 || length > SAVE_NAME_MAX) return false;
    for (size_t i = 0; i < length; i++)
    {
        if (!is_allowed_character((unsigned char)name[i])) return false;
    }
    return true;
}

static bool is_reserved_name(const char *name)
{
    static const char *reserved[] = {"CON", "PRN", "AUX", "NUL", "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
                                     "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"};

    for (size_t i = 0; i < sizeof(reserved) / sizeof(reserved[0]); i++)
    {
        if (equal_ignoring_case(name, reserved[i])) return true;
    }
    return false;
}

static bool mode_folder_path(save_mode mode, char *buffer, size_t size)
{
    return snprintf(buffer, size, "%s/%s", saves_root, save_mode_folder(mode)) < (int)size;
}

static bool slot_folder_path(save_mode mode, const char *name, char *buffer, size_t size)
{
    if (!is_safe_name(name)) return false;
    return snprintf(buffer, size, "%s/%s/%s", saves_root, save_mode_folder(mode), name) < (int)size;
}

bool save_slot_path(save_mode mode, const char *name, char *buffer, size_t size)
{
    if (!is_safe_name(name)) return false;
    return snprintf(buffer, size, "%s/%s/%s/%s", saves_root, save_mode_folder(mode), name, progress_file_name(mode)) < (int)size;
}

void save_name_trim(char *name)
{
    size_t length = strlen(name);
    size_t start = 0;

    while (length > 0 && name[length - 1] == ' ') name[--length] = '\0';
    while (name[start] == ' ') start++;
    if (start > 0) memmove(name, name + start, strlen(name + start) + 1);
}

save_name_check save_slot_check_name(save_mode mode, const char *name)
{
    char trimmed[SAVE_NAME_MAX * 4];

    if (name == NULL) return SAVE_NAME_EMPTY;
    snprintf(trimmed, sizeof(trimmed), "%s", name);
    save_name_trim(trimmed);

    if (trimmed[0] == '\0') return SAVE_NAME_EMPTY;
    if (strlen(trimmed) > SAVE_NAME_MAX) return SAVE_NAME_TOO_LONG;
    for (size_t i = 0; trimmed[i] != '\0'; i++)
    {
        if (!is_allowed_character((unsigned char)trimmed[i])) return SAVE_NAME_INVALID;
    }
    if (is_reserved_name(trimmed)) return SAVE_NAME_RESERVED;
    if (save_slot_exists(mode, trimmed)) return SAVE_NAME_EXISTS;
    return SAVE_NAME_OK;
}

const char *save_name_check_message(save_name_check result)
{
    switch (result)
    {
    case SAVE_NAME_EMPTY:    return "Escreva um nome para o save.";
    case SAVE_NAME_TOO_LONG: return "O nome é grande demais (até 20 letras).";
    case SAVE_NAME_INVALID:  return "Use só letras, números, espaço, - e _.";
    case SAVE_NAME_RESERVED: return "Esse nome não pode ser usado. Escolha outro.";
    case SAVE_NAME_EXISTS:   return "Já existe um save com esse nome.";
    default:                 return "";
    }
}

// -----------------------------------------------------------------------------
// Pastas
// -----------------------------------------------------------------------------

bool save_slots_ensure_folders(void)
{
    bool ok = true;

    for (int mode = 0; mode < SAVE_MODE_COUNT; mode++)
    {
        char folder[PATH_MAX_LENGTH];

        if (!mode_folder_path((save_mode)mode, folder, sizeof(folder))) return false;
        if (!DirectoryExists(folder)) MakeDirectory(folder);
        ok = ok && DirectoryExists(folder);
    }
    return ok;
}

static long slot_modified_time(save_mode mode, const char *name)
{
    char path[PATH_MAX_LENGTH];
    long modified = 0;

    if (save_slot_path(mode, name, path, sizeof(path)) && FileExists(path)) modified = GetFileModTime(path);
    return modified;
}

int save_slots_list(save_mode mode, save_slot_info *out, int max)
{
    char folder[PATH_MAX_LENGTH];
    FilePathList folders;
    int count = 0;

    if (max <= 0 || !mode_folder_path(mode, folder, sizeof(folder)) || !DirectoryExists(folder)) return 0;

    folders = LoadDirectoryFilesEx(folder, "DIRS*", false);
    for (unsigned i = 0; i < folders.count && count < max; i++)
    {
        const char *name = GetFileName(folders.paths[i]);

        // Pastas que não seguem as regras dos nomes (criadas à mão) ficam de fora da lista
        if (!is_safe_name(name)) continue;
        snprintf(out[count].name, sizeof(out[count].name), "%s", name);
        out[count].modified = slot_modified_time(mode, name);
        count++;
    }
    UnloadDirectoryFiles(folders);

    // Mais recente primeiro; quem nunca foi jogado vem depois, por ordem de nome
    for (int i = 1; i < count; i++)
    {
        save_slot_info current = out[i];
        int j = i - 1;

        while (j >= 0 && (out[j].modified < current.modified || (out[j].modified == current.modified && strcmp(out[j].name, current.name) > 0)))
        {
            out[j + 1] = out[j];
            j--;
        }
        out[j + 1] = current;
    }
    return count;
}

bool save_slot_exists(save_mode mode, const char *name)
{
    save_slot_info slots[SAVE_SLOTS_MAX];
    char trimmed[SAVE_NAME_MAX * 4];
    int count;

    if (name == NULL) return false;
    snprintf(trimmed, sizeof(trimmed), "%s", name);
    save_name_trim(trimmed);
    count = save_slots_list(mode, slots, SAVE_SLOTS_MAX);
    for (int i = 0; i < count; i++)
    {
        if (equal_ignoring_case(slots[i].name, trimmed)) return true;
    }
    return false;
}

bool save_slot_create(save_mode mode, const char *name)
{
    char trimmed[SAVE_NAME_MAX * 4];
    char folder[PATH_MAX_LENGTH];

    if (name == NULL) return false;
    snprintf(trimmed, sizeof(trimmed), "%s", name);
    save_name_trim(trimmed);
    if (save_slot_check_name(mode, trimmed) != SAVE_NAME_OK) return false;
    if (!save_slots_ensure_folders()) return false;
    if (!slot_folder_path(mode, trimmed, folder, sizeof(folder))) return false;

    MakeDirectory(folder);
    return DirectoryExists(folder);
}

bool save_slot_delete(save_mode mode, const char *name)
{
    char folder[PATH_MAX_LENGTH];
    FilePathList files;
    bool ok = true;

    if (!slot_folder_path(mode, name, folder, sizeof(folder)) || !DirectoryExists(folder)) return false;

    files = LoadDirectoryFilesEx(folder, "FILES*", false);
    for (unsigned i = 0; i < files.count; i++)
    {
        if (remove(files.paths[i]) != 0) ok = false;
    }
    UnloadDirectoryFiles(files);

    if (REMOVE_FOLDER(folder) != 0) ok = false;
    return ok && !DirectoryExists(folder);
}

bool save_slot_has_progress(save_mode mode, const char *name)
{
    char path[PATH_MAX_LENGTH];

    return save_slot_path(mode, name, path, sizeof(path)) && FileExists(path);
}

// -----------------------------------------------------------------------------
// Resumo para a lista
// -----------------------------------------------------------------------------

// Acha "chave=número" no texto do arquivo, sem validar o resto (a validação é de quem carrega o save)
static bool read_number(const char *text, const char *key, long *value)
{
    char needle[48];
    const char *found;

    snprintf(needle, sizeof(needle), "\n%s=", key);
    found = strstr(text, needle);
    if (found == NULL) return false;
    *value = strtol(found + strlen(needle), NULL, 10);
    return true;
}

void save_slot_summary(save_mode mode, const char *name, char *buffer, size_t size)
{
    char path[PATH_MAX_LENGTH];
    char content[2048];
    FILE *file;
    size_t length;
    long first = 0, second = 0;

    snprintf(buffer, size, "Sem progresso");
    if (!save_slot_path(mode, name, path, sizeof(path))) return;

    file = fopen(path, "rb");
    if (file == NULL) return;
    length = fread(content + 1, 1, sizeof(content) - 2, file);
    fclose(file);
    content[0] = '\n';
    content[length + 1] = '\0';

    if (mode == SAVE_MODE_NORMAL)
    {
        if (read_number(content, "current_day", &first)) snprintf(buffer, size, "Dia %ld", first);
    }
    else
    {
        if (read_number(content, "coins", &first) && read_number(content, "runs", &second)) snprintf(buffer, size, "%ld horas, %ld %s", first, second, second == 1 ? "partida" : "partidas");
    }
}

// -----------------------------------------------------------------------------
// Save ativo
// -----------------------------------------------------------------------------

void active_save_set(save_mode mode, const char *name)
{
    active_mode = mode;
    snprintf(active_name, sizeof(active_name), "%s", name != NULL ? name : "");
    active_is_set = name != NULL && name[0] != '\0';
}

void active_save_clear(void)
{
    active_is_set = false;
    active_name[0] = '\0';
}

bool active_save_is_set(void)
{
    return active_is_set;
}

save_mode active_save_mode(void)
{
    return active_mode;
}

const char *active_save_name(void)
{
    return active_name;
}

bool active_save_path(char *buffer, size_t size)
{
    return active_is_set && save_slot_path(active_mode, active_name, buffer, size);
}

// -----------------------------------------------------------------------------
// Saves antigos
// -----------------------------------------------------------------------------

static bool move_file(const char *from, const char *to)
{
    return rename(from, to) == 0;
}

static int migrate_one(save_mode mode, const char *folder, const char *legacy_name)
{
    char legacy[PATH_MAX_LENGTH];
    char backup[PATH_MAX_LENGTH + 8];
    char destination[PATH_MAX_LENGTH];
    char destination_backup[PATH_MAX_LENGTH + 8];
    save_slot_info existing[1];
    int moved = 0;

    snprintf(legacy, sizeof(legacy), "%s%s%s", folder, folder[0] != '\0' ? "/" : "", legacy_name);
    snprintf(backup, sizeof(backup), "%s.bak", legacy);
    if (!FileExists(legacy) && !FileExists(backup)) return 0;
    if (save_slots_list(mode, existing, 1) > 0) return 0;   // O modo já tem saves: o arquivo antigo fica onde está
    if (!save_slot_create(mode, "Principal")) return 0;
    if (!save_slot_path(mode, "Principal", destination, sizeof(destination))) return 0;
    snprintf(destination_backup, sizeof(destination_backup), "%s.bak", destination);

    if (FileExists(legacy) && move_file(legacy, destination)) moved++;
    if (FileExists(backup) && move_file(backup, destination_backup)) moved++;
    return moved;
}

int save_slots_migrate_legacy_from(const char *folder)
{
    if (folder == NULL) folder = "";
    return migrate_one(SAVE_MODE_NORMAL, folder, "iara.save") + migrate_one(SAVE_MODE_SURVIVAL, folder, "iara.survival");
}

int save_slots_migrate_legacy(void)
{
    return save_slots_migrate_legacy_from("");
}
