#ifndef SV_RANKING_H
#define SV_RANKING_H

#include <stdbool.h>

/*
 * Ranking do Modo Sobrevivência: cada partida terminada entra com o nome de quem jogou.
 * Pensado para eventos (como a Tech Night), em que várias pessoas jogam no mesmo computador:
 * o ranking é um arquivo só, compartilhado por todos os saves, e não depende de internet.
 *
 * Ordem: quem venceu (desligou a IARA) fica na frente; depois quem sobreviveu mais tempo;
 * no empate, quem derrotou mais inimigos; persistindo o empate, quem chegou primeiro.
 *
 * Regras puras, sem Raylib, para os testes automáticos.
 */

#define SV_RANKING_PATH "iara.ranking"
#define SV_RANKING_MAX 200          // Quantas partidas o arquivo guarda
#define SV_NAME_MAX_CHARS 14        // Letras do nome (acentos contam como uma)
#define SV_NAME_BYTES 64            // Espaço do nome em UTF-8

typedef struct
{
    char name[SV_NAME_BYTES];
    int seconds;        // Tempo sobrevivido
    int kills;          // Inimigos derrotados
    int level;          // Nível alcançado
    int bosses;         // Chefes derrotados
    int character;      // SvCharacterId usado
    bool won;           // Desligou a IARA
    long order;         // Ordem de chegada (desempate)
} SvRankEntry;

void sv_ranking_clear(void);

/** @brief Arquivo em uso. NULL volta ao padrão (SV_RANKING_PATH). */
void sv_ranking_set_path(const char *path);
const char *sv_ranking_path(void);

bool sv_ranking_load(void);
bool sv_ranking_save(void);

/**
 * @brief Coloca uma partida no ranking e grava o arquivo.
 * @return A posição alcançada (1 = primeiro lugar), ou 0 se o ranking estava cheio e ela ficou de fora.
 */
int sv_ranking_add(const char *name, int seconds, int kills, int level, int bosses, int character, bool won);

int sv_ranking_count(void);
const SvRankEntry *sv_ranking_get(int index);   // 0 = primeiro lugar

/** @brief true se a entrada a fica na frente da b. */
bool sv_ranking_better(const SvRankEntry *a, const SvRankEntry *b);

/**
 * @brief Limpa um nome digitado: tira espaços das pontas, junta espaços repetidos, troca caracteres
 * que quebrariam o arquivo e corta em SV_NAME_MAX_CHARS letras. Nome vazio vira "Anônimo".
 */
void sv_ranking_clean_name(const char *in, char *out, int out_size);

#endif // SV_RANKING_H
