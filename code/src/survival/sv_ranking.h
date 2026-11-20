#ifndef SV_RANKING_H
#define SV_RANKING_H

#include <stdbool.h>

/*
 * Ranking do Modo Sobrevivência. Cada jogador é um save (saves/sobrevivencia/<nome>/), com o seu
 * Banco de Horas e os seus upgrades, e aparece uma vez só no ranking, com o melhor resultado dele.
 * Pensado para eventos (como a Tech Night), em que várias pessoas jogam no mesmo computador:
 * o ranking é um arquivo só (iara.ranking, na pasta do jogo) e não depende de internet.
 *
 * Ordem: quem venceu (desligou a IARA) fica na frente; depois quem sobreviveu mais tempo;
 * no empate, quem derrotou mais inimigos; persistindo o empate, quem chegou primeiro.
 *
 * Regras puras, sem Raylib, para os testes automáticos.
 */

#define SV_RANKING_PATH "iara.ranking"
#define SV_RANKING_MAX 200          // Quantas partidas o arquivo guarda
#define SV_NAME_MAX_CHARS 20        // Letras do nome: o mesmo limite do nome do save (SAVE_NAME_MAX)
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
 * @brief Registra uma partida do jogador e grava o arquivo.
 *
 * Cada nome aparece uma vez só (sem diferenciar maiúsculas): se o jogador já está no ranking, a
 * partida só entra se for melhor que o recorde dele.
 * @param new_record Se não for NULL, recebe true quando a partida virou o novo recorde do jogador.
 * @return A posição do jogador depois da partida (1 = primeiro lugar), ou 0 se ficou de fora.
 */
int sv_ranking_add(const char *name, int seconds, int kills, int level, int bosses, int character, bool won, bool *new_record);

/** @brief Posição do jogador (0 = primeiro lugar), ou -1 se ele não está no ranking. */
int sv_ranking_find(const char *name);

/** @brief Tira o jogador do ranking e grava o arquivo (quando o save dele é apagado). */
bool sv_ranking_remove(const char *name);

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
