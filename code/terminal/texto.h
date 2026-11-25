#ifndef TEXTO_H
#define TEXTO_H

#include <stdbool.h>
#include <stddef.h>

// Texto do jogo de terminal. Tudo aqui respeita o tamanho do destino: nenhuma função
// desta dupla de arquivos escreve fora do vetor que recebeu, e todas deixam o '\0'.
//
// O roteiro do jogo tem símbolos de lógica (∧ ∨ ¬ → ↔) e acentos, que em UTF-8 ocupam
// mais de um byte por caractere. Por isso a largura de um texto se conta em caracteres,
// com texto_largura(), e nunca com strlen(), que conta bytes e desalinharia as molduras.

// Maior linha que o jogo imprime, em bytes.
#define TEXTO_MAX 512

/**
 * @brief Copia origem para destino sem passar de tamanho, sempre terminando em '\0'.
 *
 * @param tamanho Tamanho do vetor destino, em bytes, contando o '\0'.
 */
void texto_copiar(char *destino, size_t tamanho, const char *origem);

/**
 * @brief Acrescenta extra ao fim de destino sem passar de tamanho.
 */
void texto_juntar(char *destino, size_t tamanho, const char *extra);

/**
 * @brief Largura de um texto em caracteres, não em bytes, para alinhar as molduras.
 */
size_t texto_largura(const char *texto);

/**
 * @brief Monta uma linha com o mesmo caractere repetido, para as bordas das molduras.
 *
 * @param repeticoes Quantas vezes repetir. O que não couber em tamanho é descartado.
 */
void texto_repetir(char *destino, size_t tamanho, const char *caractere, size_t repeticoes);

/**
 * @brief Quantos bytes o primeiro caractere do texto ocupa em UTF-8, de 1 a 4.
 *
 * Serve para andar pelo texto de caractere em caractere, sem partir um símbolo no meio.
 */
size_t texto_bytes_do_caractere(const char *texto);

/**
 * @brief Informa se o texto está vazio ou só tem espaços.
 */
bool texto_esta_vazio(const char *texto);

/**
 * @brief Tira os espaços do começo e do fim, no próprio vetor.
 */
void texto_aparar(char *texto);

#endif // TEXTO_H
