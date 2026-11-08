#ifndef LOADING_H
#define LOADING_H

#include <stdbool.h>

/*
 * Tela de carregamento com porcentagem.
 *
 * A Raylib só carrega imagens na thread principal, então carregar tudo de uma vez
 * deixa a janela congelada. Aqui o trabalho é dividido em pedaços pequenos: a cada
 * quadro a tela trabalha por alguns milissegundos, desenha a barra e devolve o
 * controle, e a janela continua respondendo.
 */

typedef struct
{
    const char *label;    // O que está sendo carregado, mostrado embaixo da barra
    float weight;         // Peso no total (o tempo estimado, em qualquer unidade)
    float (*step)(void);  // Faz um pedaço pequeno e devolve o andamento deste trabalho, de 0 a 1
} loading_job;

#define LOADING_MAX_JOBS 12
#define LOADING_SLICE_SECONDS 0.010   // Quanto cada quadro trabalha antes de desenhar

/** @brief Começa um carregamento. A tela só avança quando todos os trabalhos acabam. */
void loading_begin(const char *title, const loading_job *jobs, int count, int next_screen);

/** @brief Um quadro: trabalha por até LOADING_SLICE_SECONDS e desenha a barra com a porcentagem. */
void render_loading(void);

/** @brief Se todos os trabalhos terminaram. */
bool is_loading_finished(void);

/** @brief Tela para onde o jogo vai depois de carregar. */
int loading_next_screen(void);

/** @brief Andamento total, de 0 a 100. */
float loading_percent(void);

/** @brief Faz um pedaço do trabalho sem desenhar nada. Devolve se ainda falta trabalho. */
bool loading_work(double max_seconds);

/** @brief Roda todos os trabalhos de uma vez, sem tela (testes e carregamentos curtos). */
void loading_run_all(const loading_job *jobs, int count);

#endif // LOADING_H
