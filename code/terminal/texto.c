#include <ctype.h>
#include <string.h>

#include "texto.h"

void texto_copiar(char *destino, size_t tamanho, const char *origem)
{
    if (destino == NULL || tamanho == 0) return;

    if (origem == NULL)
    {
        destino[0] = '\0';
        return;
    }

    // strncpy não garante o '\0' quando a origem enche o destino, por isso o último byte
    // fica reservado e escrito à mão.
    strncpy(destino, origem, tamanho - 1);
    destino[tamanho - 1] = '\0';
}

void texto_juntar(char *destino, size_t tamanho, const char *extra)
{
    if (destino == NULL || tamanho == 0 || extra == NULL) return;

    size_t usado = strlen(destino);

    if (usado + 1 >= tamanho) return; // Já está cheio: não sobra espaço nem para um byte

    strncat(destino, extra, tamanho - usado - 1);
}

size_t texto_largura(const char *texto)
{
    if (texto == NULL) return 0;

    size_t largura = 0;

    for (const unsigned char *byte = (const unsigned char *)texto; *byte != '\0'; byte++)
    {
        // Em UTF-8, os bytes de continuação são 10xxxxxx: não começam caractere novo.
        if ((*byte & 0xC0) != 0x80) largura++;
    }

    return largura;
}

void texto_repetir(char *destino, size_t tamanho, const char *caractere, size_t repeticoes)
{
    if (destino == NULL || tamanho == 0) return;

    destino[0] = '\0';

    for (size_t i = 0; i < repeticoes; i++)
    {
        texto_juntar(destino, tamanho, caractere);
    }
}

size_t texto_bytes_do_caractere(const char *texto)
{
    if (texto == NULL || *texto == '\0') return 0;

    unsigned char primeiro = (unsigned char)*texto;

    if (primeiro < 0x80) return 1;          // 0xxxxxxx: ASCII
    if ((primeiro & 0xE0) == 0xC0) return 2; // 110xxxxx
    if ((primeiro & 0xF0) == 0xE0) return 3; // 1110xxxx: e os simbolos de logica estao aqui
    if ((primeiro & 0xF8) == 0xF0) return 4; // 11110xxx

    return 1; // Byte solto: anda um, para nao travar o laco
}

bool texto_esta_vazio(const char *texto)
{
    if (texto == NULL) return true;

    for (const char *letra = texto; *letra != '\0'; letra++)
    {
        if (!isspace((unsigned char)*letra)) return false;
    }

    return true;
}

void texto_aparar(char *texto)
{
    if (texto == NULL) return;

    size_t inicio = 0;

    while (texto[inicio] != '\0' && isspace((unsigned char)texto[inicio])) inicio++;

    if (inicio > 0) memmove(texto, texto + inicio, strlen(texto + inicio) + 1);

    size_t fim = strlen(texto);

    while (fim > 0 && isspace((unsigned char)texto[fim - 1])) fim--;

    texto[fim] = '\0';
}
