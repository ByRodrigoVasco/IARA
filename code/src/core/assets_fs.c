#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "assets_fs.h"

/*
 * Formato do pacote (gerado por tools/release/build_release.ps1), tudo em little endian:
 *   "IARAPAK1"                                  8 bytes
 *   quantidade de arquivos                      uint32
 *   para cada arquivo: tamanho do caminho       uint32
 *                      caminho (UTF-8, com /)   bytes, sem o \0
 *                      início dos dados         uint32, contado do começo do pacote
 *                      tamanho dos dados        uint32
 *   dados dos arquivos, um depois do outro
 * O caminho é o mesmo que o jogo usa, a partir da raiz do repositório: "code/assets/...".
 */

#ifdef IARA_EMBED_ASSETS
// Definidos em tools/release/iara_pack.S, que embute o pacote com .incbin
extern const unsigned char iara_pack_start[];
extern const unsigned char iara_pack_end[];
#endif

#define PACK_MAGIC "IARAPAK1"

typedef struct
{
    const char *path;            // Aponta para dentro do pacote; não termina em \0
    unsigned int path_length;
    const unsigned char *data;
    unsigned int size;
} PackEntry;

static PackEntry *entries = NULL;
static int entry_count = 0;

static unsigned int read_u32(const unsigned char *p)
{
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8) | ((unsigned int)p[2] << 16) | ((unsigned int)p[3] << 24);
}

static char lower_ascii(char c)
{
    return (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
}

// Compara como o Windows: sem diferenciar maiúsculas, com \ e / iguais, ignorando um "./" no começo
static bool same_path(const PackEntry *entry, const char *path)
{
    if (path[0] == '.' && (path[1] == '/' || path[1] == '\\')) path += 2;
    for (unsigned int i = 0; i < entry->path_length; i++)
    {
        char a = entry->path[i], b = path[i];

        if (b == '\0') return false;
        if (b == '\\') b = '/';
        if (lower_ascii(a) != lower_ascii(b)) return false;
    }
    return path[entry->path_length] == '\0';
}

static const PackEntry *find_entry(const char *path)
{
    if (path == NULL) return NULL;
    for (int i = 0; i < entry_count; i++)
    {
        if (same_path(&entries[i], path)) return &entries[i];
    }
    return NULL;
}

// Lê do disco, como o LoadFileData do raylib faz quando ninguém troca o leitor (saves, arquivos fora do pacote)
static unsigned char *read_from_disk(const char *path, int *size)
{
    FILE *file = fopen(path, "rb");
    unsigned char *data = NULL;
    long length;

    *size = 0;
    if (file == NULL)
    {
        TraceLog(LOG_WARNING, "FILEIO: [%s] Failed to open file", path);
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    length = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (length > 0)
    {
        data = (unsigned char *)MemAlloc((unsigned int)length);
        if (data != NULL && fread(data, 1, (size_t)length, file) == (size_t)length) *size = (int)length;
        else
        {
            MemFree(data);
            data = NULL;
        }
    }
    fclose(file);
    return data;
}

// Leitor de arquivos do raylib: primeiro o pacote, depois o disco. O raylib libera o retorno com MemFree.
static unsigned char *load_file_data(const char *path, int *size)
{
    const PackEntry *entry = find_entry(path);

    if (entry == NULL) return read_from_disk(path, size);

    unsigned char *copy = (unsigned char *)MemAlloc(entry->size > 0 ? entry->size : 1);

    if (copy == NULL)
    {
        *size = 0;
        return NULL;
    }
    memcpy(copy, entry->data, entry->size);
    *size = (int)entry->size;
    return copy;
}

// Monta o índice a partir do pacote. Um pacote estragado fica de fora inteiro (o jogo tenta o disco).
static bool open_pack(const unsigned char *pack, size_t pack_size)
{
    size_t at = 8 + 4;
    unsigned int count;

    if (pack_size < at || memcmp(pack, PACK_MAGIC, 8) != 0) return false;
    count = read_u32(pack + 8);
    entries = (PackEntry *)calloc(count > 0 ? count : 1, sizeof(PackEntry));
    if (entries == NULL) return false;

    for (unsigned int i = 0; i < count; i++)
    {
        unsigned int length, offset, size;

        if (at + 4 > pack_size) break;
        length = read_u32(pack + at);
        at += 4;
        if (at + length + 8 > pack_size) break;
        entries[i].path = (const char *)(pack + at);
        entries[i].path_length = length;
        at += length;
        offset = read_u32(pack + at);
        size = read_u32(pack + at + 4);
        at += 8;
        if ((size_t)offset + size > pack_size) break;
        entries[i].data = pack + offset;
        entries[i].size = size;
        entry_count = (int)i + 1;
    }

    if (entry_count != (int)count)
    {
        TraceLog(LOG_ERROR, "ASSETS: pacote embutido estragado (%d de %u arquivos lidos)", entry_count, count);
        free(entries);
        entries = NULL;
        entry_count = 0;
        return false;
    }
    return true;
}

void assets_fs_init(void)
{
#ifdef IARA_EMBED_ASSETS
    if (entries != NULL) return;
    if (open_pack(iara_pack_start, (size_t)(iara_pack_end - iara_pack_start)))
    {
        SetLoadFileDataCallback(load_file_data);
        TraceLog(LOG_INFO, "ASSETS: %d arquivos embutidos no executavel", entry_count);
    }
#endif
}

bool assets_fs_embedded(void)
{
    return entry_count > 0;
}

bool asset_exists(const char *path)
{
    return find_entry(path) != NULL || FileExists(path);
}

Music asset_load_music(const char *path)
{
    const PackEntry *entry = find_entry(path);

    // Os dados ficam no executável a partida toda, então o stream pode ler direto deles
    if (entry != NULL) return LoadMusicStreamFromMemory(GetFileExtension(path), entry->data, (int)entry->size);
    return LoadMusicStream(path);
}
