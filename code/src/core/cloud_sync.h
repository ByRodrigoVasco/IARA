#ifndef CLOUD_SYNC_H
#define CLOUD_SYNC_H

#include <stdbool.h>
#include "save_slots.h"

/*
 * Ligação entre o jogo e a nuvem (cloud.h): o save baixado vira o arquivo local que save.c e
 * sv_meta.c já leem, cada gravação local sobe, e os avisos da nuvem aparecem na tela.
 * Com a nuvem desligada, nada aqui faz coisa alguma.
 */

#define CLOUD_FOLDER "nuvem"   // Código do dispositivo, fila offline e sincronização, na pasta de dados

/** @brief Liga a nuvem, se o build tem a URL e a chave. Chamar já dentro da pasta de dados. */
void cloud_sync_start(void);

/** @brief Envia o que falta e libera o save, esperando um tempo curto. */
void cloud_sync_stop(void);

/**
 * @brief Grava no arquivo local o conteúdo baixado ("" = save sem progresso: o arquivo sai).
 * Um progresso local diferente, de antes da nuvem, fica guardado em "<arquivo>.anterior".
 */
bool cloud_sync_write_local(save_mode mode, const char *name, const char *content);

/** @brief Pede para abrir o save na nuvem, levando o que há neste dispositivo. Devolve o ticket. */
int cloud_sync_open_save(save_mode mode, const char *name);

/** @brief Manda para a nuvem o progresso do save ativo. Chamar depois de cada gravação local. */
void cloud_sync_active_save(void);

/** @brief Sobe o que falta e libera o save aberto. Chamar ao sair dele. */
void cloud_sync_close_save(void);

/** @brief Um quadro: mostra os avisos de conflito e de save perdido. */
void cloud_sync_update(void);

#endif // CLOUD_SYNC_H
