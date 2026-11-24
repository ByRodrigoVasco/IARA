#ifndef DATA_DIR_H
#define DATA_DIR_H

#include <stdbool.h>
#include <stddef.h>

/*
 * Pasta dos dados do jogador: Documentos\IARA.
 *
 * Saves, preferências (iara.ini), rankings e o log de desempenho usam caminhos relativos, então
 * basta o jogo trabalhar dentro desta pasta para tudo ir para lá. O main.c faz isso no executável
 * de release; no build de desenvolvimento a pasta de trabalho continua sendo a do repositório.
 */

#define DATA_DIR_NAME "IARA"

/**
 * @brief Monta (e cria, se faltar) Documentos\IARA. Usa a pasta Documentos do Windows, inclusive
 * quando ela foi movida (OneDrive, outro disco).
 *
 * @return false se não deu para achar ou criar a pasta (o jogo segue na pasta do executável).
 */
bool data_dir_prepare(char *out, size_t size);

/**
 * @brief Na primeira vez que a pasta é usada (sem saves nem iara.ini), copia para ela os dados
 * que já existiam na pasta do executável: a pasta saves e os arquivos iara.ini, iara.save,
 * iara.survival, iara.ranking e iara_duplas.ranking. Só copia, nunca apaga.
 *
 * @return Quantos arquivos foram copiados.
 */
int data_dir_migrate_from_exe_folder(const char *data_dir);

#endif // DATA_DIR_H
