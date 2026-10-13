#ifndef GAME_LOOP_H
#define GAME_LOOP_H

#include "raylib.h" // Inclui a biblioteca Raylib para disponibilizar os tipos gráficos, vetores e estruturas base do motor

/**
 * @brief Inicializa todas as variáveis globais/estáticas, texturas, estados e caixas de colisão do Game Loop.
 * 
 * Esta função é responsável por carregar os ativos visuais (sprites do ambiente, do jogador e elementos
 * de interface) do disco para a memória da GPU, pré-calcular proporções e posições responsivas com base
 * na resolução da tela, e definir o estado inicial do jogador e do cartão narrativo.
 * 
 * Deve ser invocada uma única vez durante a fase de preparação (setup), antes de entrar no ciclo principal de quadros.
 */
void initialize_game_loop_variables(void);

/**
 * @brief Executa o ciclo contínuo de atualização de lógica e renderização gráfica a cada quadro (frame).
 * 
 * Controla a máquina de estados das telas (Cartão Narrativo do Dia, Exploração do Escritório e Tela do Computador).
 * A cada frame, processa as entradas do teclado (WASD/Setas/Espaço), atualiza a movimentação e física de colisões,
 * delimita a renderização entre `BeginDrawing()` e `EndDrawing()`, e desenha os elementos na ordem correta de camadas (Z-index).
 */
void render_game_loop(void);

/**
 * @brief Descarrega e libera todos os recursos de memória de vídeo (GPU) alocados para o Game Loop.
 * 
 * Realiza a faxina final da memória liberando as texturas do mapa do escritório, dicas visuais de controle,
 * indicadores flutuantes e o array de sprites direcionais do personagem.
 * 
 * Deve ser chamada obrigatoriamente no encerramento do jogo ou na transição definitiva de módulos para evitar vazamentos de memória (Memory Leaks).
 */
void unload_game_loop(void);

#endif // GAME_LOOP_H