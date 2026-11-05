#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <stdbool.h>

// Arquivo de preferências, ao lado do save. Fica fora do save de propósito:
// começar uma nova partida ou perder o save não muda as configurações do jogador.
#define PREFERENCES_FILE_PATH "iara.ini"

/**
 * @brief Lê o arquivo e aplica alto contraste, brilho e tela cheia. Exige a janela aberta.
 *
 * Linhas inválidas são ignoradas: o jogo continua com o valor atual daquela opção.
 *
 * @param path Caminho do arquivo.
 * @return true se o arquivo existia e foi lido.
 */
bool load_preferences(const char *path);

/**
 * @brief Grava o alto contraste, o brilho e a tela cheia atuais.
 *
 * @param path Caminho do arquivo.
 * @return true se conseguiu gravar.
 */
bool save_preferences(const char *path);

#endif // PREFERENCES_H
