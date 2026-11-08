#ifndef MENU_H
#define MENU_H

#include <stdbool.h>

// Opções do menu principal
#define NONE 0
#define INICIAR 1
#define CREDITOS 2
#define CONFIGURACOES 3
#define SAIR 4

extern bool is_menu_to_close; // true quando o jogador escolheu uma opção
extern int next_screen;       // opção escolhida (INICIAR, CREDITOS ou SAIR)

/** @brief Carrega a próxima imagem do menu e devolve o andamento, de 0 a 1. A abertura chama uma vez por quadro. */
float menu_preload_step(void);

/** @brief Se todas as imagens do menu já estão carregadas. */
bool menu_preload_done(void);

/** @brief Libera as imagens do menu. Só ao encerrar o jogo. */
void unload_menu_textures(void);

void initialize_menu_variables(void);
void render_menu(void);
void unload_menu(void);

#endif // MENU_H
