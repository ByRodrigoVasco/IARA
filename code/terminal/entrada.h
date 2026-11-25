#ifndef ENTRADA_H
#define ENTRADA_H

#include <stdbool.h>
#include <stddef.h>

// Leitura do teclado no jogo de terminal.
//
// Nenhuma função daqui devolve valor fora da faixa pedida: o que o jogador digitar de
// errado é recusado com uma explicação e a pergunta volta. O jogo nunca usa scanf("%d"),
// que deixa o resto da linha no buffer e entra em laço infinito quando vem letra; aqui a
// linha é lida inteira com fgets e só depois convertida.

/**
 * @brief Lê uma linha do teclado, sem o '\n' e já aparada.
 *
 * @return false no fim da entrada (Ctrl+Z, ou entrada redirecionada que acabou).
 */
bool entrada_ler_linha(char *destino, size_t tamanho);

/**
 * @brief Pergunta um número inteiro até vir um entre minimo e maximo.
 *
 * @param saiu Recebe true se a entrada acabou antes de um número válido.
 * @return O número lido, ou minimo se a entrada acabou.
 */
int entrada_ler_inteiro(const char *pergunta, int minimo, int maximo, bool *saiu);

/**
 * @brief Espera o Enter, mostrando o rótulo do que vem depois.
 */
void entrada_esperar_enter(const char *rotulo);

/**
 * @brief Pergunta sim ou não até vir uma das duas.
 */
bool entrada_confirmar(const char *pergunta);

#endif // ENTRADA_H
