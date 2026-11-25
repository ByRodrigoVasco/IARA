#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "entrada.h"
#include "texto.h"

bool entrada_ler_linha(char *destino, size_t tamanho)
{
    if (destino == NULL || tamanho == 0) return false;

    destino[0] = '\0';

    if (fgets(destino, (int)tamanho, stdin) == NULL) return false;

    // fgets guarda o '\n' quando ele cabe; se não couber, o resto da linha fica no buffer
    // e precisa ser descartado, senão a sobra viraria a resposta da pergunta seguinte.
    if (strchr(destino, '\n') == NULL)
    {
        int sobra;

        while ((sobra = fgetc(stdin)) != '\n' && sobra != EOF) { }
    }

    texto_aparar(destino);
    return true;
}

int entrada_ler_inteiro(const char *pergunta, int minimo, int maximo, bool *saiu)
{
    char digitado[64];

    if (saiu != NULL) *saiu = false;

    for (;;)
    {
        printf("%s", pergunta);
        fflush(stdout);

        if (!entrada_ler_linha(digitado, sizeof(digitado)))
        {
            if (saiu != NULL) *saiu = true;
            return minimo;
        }

        if (texto_esta_vazio(digitado))
        {
            printf("  Digite um número de %d a %d.\n", minimo, maximo);
            continue;
        }

        char *resto = NULL;

        errno = 0;
        long valor = strtol(digitado, &resto, 10);

        // ERANGE: cabe em dígitos, mas não cabe num long. Vale avisar com outra frase,
        // senão o jogador leria que 99999999999999999999 "não é um número".
        if (errno == ERANGE)
        {
            printf("  Número grande demais. Digite um número de %d a %d.\n", minimo, maximo);
            continue;
        }

        // resto aponta para o que sobrou depois do número: só vale se nada sobrou,
        // senão "3x" passaria como 3.
        if (errno != 0 || resto == digitado || !texto_esta_vazio(resto))
        {
            printf("  \"%s\" não é um número. Digite um número de %d a %d.\n", digitado, minimo, maximo);
            continue;
        }

        if (valor < minimo || valor > maximo)
        {
            printf("  %ld está fora da faixa. Digite um número de %d a %d.\n", valor, minimo, maximo);
            continue;
        }

        return (int)valor;
    }
}

void entrada_esperar_enter(const char *rotulo)
{
    char descartado[8];

    printf("\n  [Enter] %s ", (rotulo != NULL && rotulo[0] != '\0') ? rotulo : "continuar");
    fflush(stdout);
    entrada_ler_linha(descartado, sizeof(descartado));
}

bool entrada_confirmar(const char *pergunta)
{
    char digitado[16];

    for (;;)
    {
        printf("%s (s/n): ", pergunta);
        fflush(stdout);

        if (!entrada_ler_linha(digitado, sizeof(digitado))) return false;

        if (digitado[0] == 's' || digitado[0] == 'S') return true;
        if (digitado[0] == 'n' || digitado[0] == 'N') return false;

        printf("  Responda com s ou n.\n");
    }
}
