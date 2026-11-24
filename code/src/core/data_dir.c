#include <stdio.h>
#include <string.h>

#include "data_dir.h"

/* Só Windows aqui: o windows.h não pode se misturar com o raylib.h (CloseWindow, DrawText...). */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define CSIDL_PERSONAL_ID 0x0005       // Pasta Documentos
#define CSIDL_FLAG_CREATE_ID 0x8000    // Cria a pasta Documentos se ela ainda não existir

typedef HRESULT(WINAPI *GetFolderPathFn)(HWND, int, HANDLE, DWORD, LPSTR);

// SHGetFolderPathA vem do shell32, carregado na hora: assim nenhum build precisa ligar -lshell32
static bool documents_folder(char *out, size_t size)
{
    char path[MAX_PATH];
    HMODULE shell = LoadLibraryA("shell32.dll");
    GetFolderPathFn get_folder_path;
    bool ok = false;

    if (shell == NULL) return false;
    get_folder_path = (GetFolderPathFn)(void *)GetProcAddress(shell, "SHGetFolderPathA");
    if (get_folder_path != NULL && get_folder_path(NULL, CSIDL_PERSONAL_ID | CSIDL_FLAG_CREATE_ID, NULL, 0, path) == S_OK)
    {
        ok = snprintf(out, size, "%s", path) < (int)size;
    }
    FreeLibrary(shell);
    return ok;
}

// pasta\nome, só se couber inteiro: um caminho cortado apontaria para outro lugar
static bool join_path(char *out, size_t size, const char *folder, const char *name)
{
    size_t a = strlen(folder), b = strlen(name);

    if (a + 1 + b + 1 > size) return false;
    memcpy(out, folder, a);
    out[a] = '\\';
    memcpy(out + a + 1, name, b + 1);
    return true;
}

static bool is_directory(const char *path)
{
    DWORD attributes = GetFileAttributesA(path);
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY);
}

static bool is_file(const char *path)
{
    DWORD attributes = GetFileAttributesA(path);
    return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
}

bool data_dir_prepare(char *out, size_t size)
{
    char documents[MAX_PATH];

    if (!documents_folder(documents, sizeof(documents))) return false;
    if (snprintf(out, size, "%s\\%s", documents, DATA_DIR_NAME) >= (int)size) return false;
    if (!is_directory(out) && !CreateDirectoryA(out, NULL)) return false;
    return is_directory(out);
}

// Copia a pasta inteira, sem sobrescrever o que já existe no destino
static int copy_tree(const char *from, const char *to)
{
    char pattern[MAX_PATH], source[MAX_PATH], target[MAX_PATH];
    WIN32_FIND_DATAA entry;
    HANDLE search;
    int copied = 0;

    if (!is_directory(to) && !CreateDirectoryA(to, NULL)) return 0;
    if (snprintf(pattern, sizeof(pattern), "%s\\*", from) >= (int)sizeof(pattern)) return 0;
    search = FindFirstFileA(pattern, &entry);
    if (search == INVALID_HANDLE_VALUE) return 0;
    do
    {
        if (strcmp(entry.cFileName, ".") == 0 || strcmp(entry.cFileName, "..") == 0) continue;
        if (!join_path(source, sizeof(source), from, entry.cFileName)) continue;
        if (!join_path(target, sizeof(target), to, entry.cFileName)) continue;
        if (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) copied += copy_tree(source, target);
        else if (CopyFileA(source, target, TRUE)) copied++;
    } while (FindNextFileA(search, &entry));
    FindClose(search);
    return copied;
}

int data_dir_migrate_from_exe_folder(const char *data_dir)
{
    static const char *files[] = {"iara.ini", "iara.save", "iara.survival", "iara.ranking", "iara_duplas.ranking"};
    char old_dir[MAX_PATH], source[MAX_PATH], target[MAX_PATH];
    char *slash;
    int copied = 0;

    // Pasta do executável, pela mesma API (ANSI) que monta o caminho de Documentos
    DWORD length = GetModuleFileNameA(NULL, old_dir, sizeof(old_dir));
    if (length == 0 || length >= sizeof(old_dir) || data_dir == NULL) return 0;
    slash = strrchr(old_dir, '\\');
    if (slash == NULL) return 0;
    *slash = '\0';
    if (_stricmp(old_dir, data_dir) == 0) return 0;

    // Só na primeira vez: se a pasta nova já tem dados, ela vale (nada é misturado nem sobrescrito).
    // Caminho que não cabe em MAX_PATH não é usado cortado: a cópia daquele item é pulada.
    if (!join_path(target, sizeof(target), data_dir, "saves") || is_directory(target)) return 0;
    if (!join_path(target, sizeof(target), data_dir, "iara.ini") || is_file(target)) return 0;

    if (join_path(source, sizeof(source), old_dir, "saves") && join_path(target, sizeof(target), data_dir, "saves") &&
        is_directory(source))
    {
        copied += copy_tree(source, target);
    }

    for (size_t i = 0; i < sizeof(files) / sizeof(files[0]); i++)
    {
        if (!join_path(source, sizeof(source), old_dir, files[i]) || !join_path(target, sizeof(target), data_dir, files[i])) continue;
        if (is_file(source) && CopyFileA(source, target, TRUE)) copied++;
    }
    return copied;
}

#else

bool data_dir_prepare(char *out, size_t size)
{
    (void)out;
    (void)size;
    return false;
}

int data_dir_migrate_from_exe_folder(const char *data_dir)
{
    (void)data_dir;
    return 0;
}

#endif
