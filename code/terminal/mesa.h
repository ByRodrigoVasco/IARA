#ifndef MESA_H
#define MESA_H

#include <stdbool.h>
#include <stddef.h>

#include "roteiro.h"

// Correção das mesas de triagem do jogo de terminal.
//
// O gabarito nunca está escrito no roteiro: ele sai das fórmulas de lógica de cada mesa,
// avaliadas por logic.c. É por isso que as duas versões do jogo, a gráfica e esta, cobram
// a mesma resposta sem nenhuma tabela de respostas para manter em dia.

/**
 * @brief Quantas linhas a mesa tem, já limitada a MAX_TRIAGE_CARDS.
 */
int mesa_quantidade_de_linhas(const DialogStep *passo);

/**
 * @brief Informa se cada linha tem a sua própria resposta certa.
 *
 * Nas mesas de atribuição, de treino e de refutação quem está certo ou errado é o
 * conjunto das escolhas, não cada linha isolada.
 */
bool mesa_tem_gabarito_por_linha(const DialogStep *passo);

/**
 * @brief Escolha certa de uma linha, nas mesas em que cada linha tem a sua.
 */
int mesa_escolha_certa_da_linha(const DialogStep *passo, int linha);

/**
 * @brief Quanto falta para a mesa estar certa. Zero é o único valor aceito.
 */
int mesa_quantos_erros(const DialogStep *passo, const int *escolhas);

/**
 * @brief Informa se a linha pode ser usada, isto é, se o jogador tem a evidência dela.
 */
bool mesa_linha_disponivel(const DialogStep *passo, int linha, unsigned int evidencias);

/**
 * @brief Monta o texto de uma linha da mesa, com os valores das variáveis quando houver.
 */
void mesa_texto_da_linha(const DialogStep *passo, int linha, unsigned int evidencias,
                         char *destino, size_t tamanho);

/**
 * @brief Monta o resultado do envio: diz quanto está errado, nunca onde.
 */
void mesa_texto_do_resultado(const DialogStep *passo, int erros, char *destino, size_t tamanho);

/**
 * @brief Aprovação da mini-IA do Dia 41 num bairro, na proporção do que ela viu no treino.
 */
int mesa_aprovacao_da_mini_ia(const DialogStep *passo, const int *escolhas, bool periferia);

#endif // MESA_H
