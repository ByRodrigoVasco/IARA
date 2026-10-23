/*
 * Apoio dos testes: movimento do jogador e busca de caminho no escritorio.
 *
 * Este cabecalho so pode ser incluido DEPOIS de "game_loop.c", porque usa as
 * variaveis estaticas do game loop (posicao do jogador, obstaculos, limites do
 * mapa). Tudo aqui e static: cada teste leva a sua copia.
 */

#ifndef PERCURSO_DO_JOGADOR_H
#define PERCURSO_DO_JOGADOR_H

#include <string.h>

/* Grade da busca de caminho: cada celula e exatamente um passo do jogador. */
#define GRADE 501
#define GRADE_CENTRO 250

/* Hitbox do jogador para uma posicao qualquer, na escala atual da tela. */
static Rectangle hitbox_em(float px, float py)
{
    Rectangle referencia = {HITBOX_OFFSET_X_AT_REFERENCE, HITBOX_OFFSET_Y_AT_REFERENCE,
                            HITBOX_WIDTH_AT_REFERENCE, HITBOX_HEIGHT_AT_REFERENCE};

    return scale_reference_rect(referencia, get_scale(), (Vector2){px, py});
}

static bool dentro_do_mapa(Rectangle hitbox)
{
    return hitbox.x >= map_collision_rec.x &&
           hitbox.y >= map_collision_rec.y &&
           hitbox.x + hitbox.width <= map_collision_rec.x + map_collision_rec.width &&
           hitbox.y + hitbox.height <= map_collision_rec.y + map_collision_rec.height;
}

static bool posicao_livre(float px, float py)
{
    Rectangle hitbox = hitbox_em(px, py);

    if (collides_with_any(hitbox, office_obstacles, OBSTACLE_COUNT)) return false;

    return dentro_do_mapa(hitbox);
}

/* Reproduz um quadro de movimento do jogo: a mesma ordem de chamadas de
 * handle_player_input() seguida de handle_map_collisions(). So substitui a
 * leitura do teclado pelos deslocamentos dx e dy recebidos. */
static void simular_passo(int dx, int dy)
{
    float speed = DEFAULT_PLAYER_MOVEMENT_SPEED * get_scale();

    if (dx != 0 && dy != 0) speed *= 0.7071f;

    update_collision_rec();
    current_estagiario_position.x = move_axis(current_estagiario_position.x, dx * speed,
                                              estagiario_collision_rec, true,
                                              office_obstacles, OBSTACLE_COUNT);

    update_collision_rec();
    current_estagiario_position.y = move_axis(current_estagiario_position.y, dy * speed,
                                              estagiario_collision_rec, false,
                                              office_obstacles, OBSTACLE_COUNT);

    update_collision_rec();
    handle_map_collisions();
}

/* ------------------------------------------------------------------------ */
/* Caminho do spawn ate o computador                                          */
/* ------------------------------------------------------------------------ */

static unsigned char veio_de[GRADE][GRADE];   /* 0 = nao visitado, 1..4 = direcao, 5 = origem */
static int fila[GRADE * GRADE];
static int caminho_i[GRADE * GRADE];
static int caminho_j[GRADE * GRADE];
static int tamanho_caminho = 0;

/* Posicao na tela do passo k do caminho (0 = junto ao computador). */
static Vector2 posicao_do_caminho(Vector2 spawn, int k)
{
    float passo = DEFAULT_PLAYER_MOVEMENT_SPEED * get_scale();

    return (Vector2){spawn.x + (caminho_i[k] - GRADE_CENTRO) * passo,
                     spawn.y + (caminho_j[k] - GRADE_CENTRO) * passo};
}

/* Procura um caminho do spawn ate a zona do computador andando um passo por vez.
 * Cada celula da grade e exatamente um passo do jogador em um eixo, entao o
 * caminho encontrado e um percurso que o jogador consegue fazer de verdade.
 *
 * O caminho fica em caminho_i/caminho_j, do computador (indice 0) ate o spawn. */
static bool calcular_caminho_ate_o_computador(Vector2 spawn, int *passos_do_caminho)
{
    float passo = DEFAULT_PLAYER_MOVEMENT_SPEED * get_scale();

    const int di[4] = {1, -1, 0, 0};
    const int dj[4] = {0, 0, 1, -1};

    memset(veio_de, 0, sizeof(veio_de));
    tamanho_caminho = 0;

    int inicio = 0;
    int fim = 0;

    veio_de[GRADE_CENTRO][GRADE_CENTRO] = 5; /* marca a origem */
    fila[fim++] = GRADE_CENTRO * GRADE + GRADE_CENTRO;

    while (inicio < fim)
    {
        int atual = fila[inicio++];
        int i = atual / GRADE;
        int j = atual % GRADE;

        float px = spawn.x + (i - GRADE_CENTRO) * passo;
        float py = spawn.y + (j - GRADE_CENTRO) * passo;

        /* Chegou: a hitbox encosta na zona de interacao do computador. */
        if (CheckCollisionRecs(hitbox_em(px, py), estagiario_computer_collision_rectangle))
        {
            while (veio_de[i][j] != 5)
            {
                caminho_i[tamanho_caminho] = i;
                caminho_j[tamanho_caminho] = j;
                tamanho_caminho++;

                int origem = veio_de[i][j] - 1;
                i -= di[origem];
                j -= dj[origem];
            }

            caminho_i[tamanho_caminho] = GRADE_CENTRO;
            caminho_j[tamanho_caminho] = GRADE_CENTRO;
            tamanho_caminho++;

            *passos_do_caminho = tamanho_caminho - 1;
            return true;
        }

        for (int d = 0; d < 4; d++)
        {
            int ni = i + di[d];
            int nj = j + dj[d];

            if (ni < 0 || ni >= GRADE || nj < 0 || nj >= GRADE) continue;
            if (veio_de[ni][nj] != 0) continue;

            float nx = spawn.x + (ni - GRADE_CENTRO) * passo;
            float ny = spawn.y + (nj - GRADE_CENTRO) * passo;

            if (!posicao_livre(nx, ny)) continue;

            veio_de[ni][nj] = (unsigned char)(d + 1);
            fila[fim++] = ni * GRADE + nj;
        }
    }

    return false;
}

#endif // PERCURSO_DO_JOGADOR_H
