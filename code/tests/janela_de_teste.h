#ifndef JANELA_DE_TESTE_H
#define JANELA_DE_TESTE_H

#include "raylib.h"

/* Sem FLAG_WINDOW_ALWAYS_RUN a Raylib para o laco com a janela minimizada, e o teste fica parado. */
static void abrir_janela_de_teste(int largura, int altura, const char *titulo)
{
    SetConfigFlags(FLAG_WINDOW_ALWAYS_RUN);
    InitWindow(largura, altura, titulo);
}

#endif /* JANELA_DE_TESTE_H */
