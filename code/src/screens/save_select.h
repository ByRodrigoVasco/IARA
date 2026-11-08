#ifndef SAVE_SELECT_H
#define SAVE_SELECT_H

#include "save_slots.h"

/*
 * Tela de escolha do save, aberta depois do modo de jogo: a lista dos saves do modo,
 * a opção de criar um save novo (com nome que ainda não exista) e a de apagar um save.
 */

#define SAVE_SELECT_NONE 0     // Continua na tela
#define SAVE_SELECT_BACK 1     // Voltar para a escolha do modo
#define SAVE_SELECT_CHOSEN 2   // Um save foi escolhido: ele já é o save ativo (active_save_*)

/** @brief Prepara a tela para os saves do modo. Chamar sempre que ela for aberta. */
void initialize_save_select(save_mode mode);

/** @brief Um quadro da tela. Devolve SAVE_SELECT_NONE, SAVE_SELECT_BACK ou SAVE_SELECT_CHOSEN. */
int render_save_select(void);

/** @brief Escolhe um save que já existe, como se o jogador o tivesse confirmado. Devolve falso se ele não existir. */
bool save_select_choose_existing(const char *name);

/**
 * @brief Cria um save novo e o escolhe, como se o jogador tivesse digitado o nome e confirmado.
 * Devolve SAVE_NAME_OK se deu certo, ou o motivo da recusa (nome repetido, inválido...).
 */
save_name_check save_select_create_named(const char *name);

/** @brief Apaga um save da lista, como se o jogador tivesse confirmado a exclusão. */
bool save_select_delete_named(const char *name);

/** @brief Volta à escolha do modo, como se o jogador tivesse escolhido VOLTAR. Usado pelos testes. */
void save_select_go_back(void);

#endif // SAVE_SELECT_H
