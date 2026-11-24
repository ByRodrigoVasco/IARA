#ifndef ASSETS_FS_H
#define ASSETS_FS_H

#include <stdbool.h>
#include "raylib.h"

/*
 * Assets embutidos no executável.
 *
 * No build de release (tools/release/build_release.ps1, com IARA_EMBED_ASSETS), todo o code/assets
 * vai para dentro do .exe num pacote só, e o jogo roda sem a pasta ao lado. Imagens, fontes e
 * efeitos sonoros passam pelo leitor de arquivos do raylib, que assets_fs_init() troca por um que
 * procura primeiro no pacote. As músicas (LoadMusicStream) e as checagens de arquivo não passam
 * por esse leitor, por isso usam as funções abaixo.
 *
 * No build de desenvolvimento e nos testes não há pacote: tudo continua vindo do disco.
 */

/** @brief Abre o pacote embutido, se houver, e liga o leitor de arquivos do raylib a ele. Chamar logo no começo do main. */
void assets_fs_init(void);

/** @brief O executável traz os assets dentro dele. */
bool assets_fs_embedded(void);

/** @brief O asset existe (no pacote ou no disco). Use no lugar de FileExists para arquivos de code/assets. */
bool asset_exists(const char *path);

/** @brief Abre uma música do pacote ou do disco. Use no lugar de LoadMusicStream para arquivos de code/assets. */
Music asset_load_music(const char *path);

#endif // ASSETS_FS_H
