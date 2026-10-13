#ifndef MENU_H
#define MENU_H

#define NONE 0
#define INICIAR 1
#define CREDITOS 2
#define SAIR 3

extern bool is_menu_to_close;
extern int next_screen;

void initialize_menu_variables(void);
void render_menu(void);
void unload_menu(void);

#endif // MENU_H