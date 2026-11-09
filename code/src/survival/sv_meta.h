#ifndef SV_META_H
#define SV_META_H

#include <stdbool.h>
#include "sv_rules.h"

/*
 * Progresso permanente do Modo Sobrevivência: Banco de Horas e upgrades.
 * Fica em um arquivo de texto próprio ("chave=valor"), separado do save da campanha.
 */

#define SV_SAVE_PATH "iara.survival"

typedef struct
{
    int coins;                  // Banco de Horas
    int upgrades[UPG_COUNT];    // Nível de cada upgrade
    int runs;                   // Partidas jogadas
    int wins;                   // Vezes que a IARA foi derrotada
    int best_seconds;           // Maior tempo de sobrevivência
    int kills;                  // Inimigos eliminados no total
    int character;              // Personagem escolhido (SvCharacterId)
} SvMeta;

extern SvMeta sv_meta;

void sv_meta_reset(void);
/** @brief Arquivo de progresso em uso: o do save escolhido, ou SV_SAVE_PATH quando não há save (testes). */
const char *sv_meta_path(void);

/** @brief Escolhe o arquivo de progresso. NULL volta ao padrão. */
void sv_meta_set_path(const char *path);

bool sv_meta_load(const char *path);
bool sv_meta_save(const char *path);
bool sv_meta_last_save_ok(void);   // A última gravação deu certo? (para avisar o jogador se não deu)

// Compra o próximo nível. Retorna false se faltou saldo ou já está no máximo.
bool sv_meta_buy(SvUpgradeId id);

#endif // SV_META_H
