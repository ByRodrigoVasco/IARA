#ifndef MENU_H
#define MENU_H

#include <stdbool.h>

// Opções do menu principal
#define NONE 0
#define INICIAR 1
#define SOBREVIVENCIA 2
#define CREDITOS 3
#define CONFIGURACOES 4
#define SAIR 5

extern bool is_menu_to_close; // true quando o jogador escolheu uma opção
extern int next_screen;       // opção escolhida (INICIAR, CREDITOS ou SAIR)

void initialize_menu_variables(void);
void render_menu(void);
void unload_menu(void);

#endif // MENU_H
