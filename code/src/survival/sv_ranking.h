#ifndef SV_RANKING_H
#define SV_RANKING_H

#include <stdbool.h>
#include "sv_rules.h"

/*
 * Ranking do Modo Sobrevivência. Cada jogador é um save (saves/sobrevivencia/<nome>/), com o seu
 * Banco de Horas e os seus upgrades, e aparece uma vez só no ranking, com o melhor resultado dele.
 * Pensado para eventos (como a Tech Night), em que várias pessoas jogam no mesmo computador:
 * o ranking é um arquivo só (iara.ranking, na pasta do jogo) e não depende de internet.
 *
 * Ordem: pela PONTUAÇÃO da partida (sv_ranking_score), que junta abates, tempo, chefes e vitória.
 * O que se faz depois dos 10 minutos (hora extra) vale o dobro: segundos e abates.
 * Os abates pesam mais que o tempo: quem só foge sem derrotar ninguém não passa na frente de quem
 * sobreviveu um pouco menos mas lutou mais. No empate de pontos, quem derrotou mais inimigos;
 * depois, quem sobreviveu mais tempo; persistindo o empate, quem chegou primeiro.
 *
 * A pontuação não é gravada no arquivo: é calculada a partir dos outros campos. Assim, mudar os
 * pesos abaixo reordena um ranking antigo sozinho, sem precisar apagar o arquivo.
 *
 * Regras puras, sem Raylib, para os testes automáticos.
 */

// Pesos da pontuação. Ritmo de referência: um jogador derruba de 2 a 5 inimigos por segundo,
// então os abates valem cerca de 4 a 10 vezes mais que o tempo na conta final.
#define SV_SCORE_PER_KILL 10        // Cada inimigo derrotado
#define SV_SCORE_PER_SECOND 5       // Cada segundo sobrevivido
#define SV_SCORE_PER_BOSS 500       // Cada chefe derrotado
#define SV_SCORE_WIN_BONUS 2000     // Desligou a IARA (venceu a partida)

#define SV_RANKING_PATH "iara.ranking"
#define SV_RANKING_DUO_PATH "iara_duplas.ranking"   // Ranking da Sobrevivência em dupla: separado do solo
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
    int extra_kills;    // Abates feitos depois dos 10 minutos (contam de novo)
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

/** @brief O mesmo, informando quantos dos abates foram feitos depois dos 10 minutos. */
int sv_ranking_add_run(const char *name, int seconds, int kills, int level, int bosses, int character, bool won, int extra_kills, bool *new_record);

/** @brief Posição do jogador (0 = primeiro lugar), ou -1 se ele não está no ranking. */
int sv_ranking_find(const char *name);

/** @brief Tira o jogador do ranking e grava o arquivo (quando o save dele é apagado). */
bool sv_ranking_remove(const char *name);

int sv_ranking_count(void);
const SvRankEntry *sv_ranking_get(int index);   // 0 = primeiro lugar

/**
 * @brief Pontos de uma partida: abates × SV_SCORE_PER_KILL + segundos × SV_SCORE_PER_SECOND
 * + chefes × SV_SCORE_PER_BOSS + SV_SCORE_WIN_BONUS se venceu. Os segundos além de SV_RUN_SECONDS e
 * os abates da hora extra são somados mais uma vez.
 */
int sv_ranking_score(const SvRankEntry *e);

/** @brief true se a entrada a fica na frente da b. */
bool sv_ranking_better(const SvRankEntry *a, const SvRankEntry *b);

/**
 * @brief Limpa um nome digitado: tira espaços das pontas, junta espaços repetidos, troca caracteres
 * que quebrariam o arquivo e corta em SV_NAME_MAX_CHARS letras. Nome vazio vira "Anônimo".
 */
void sv_ranking_clean_name(const char *in, char *out, int out_size);

#endif // SV_RANKING_H
