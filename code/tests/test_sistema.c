/*
 * Testes de sistema do IARA - Bloco 1: recursos, personagem, colisoes e movimento.
 *
 * Simula o uso real do jogo: carrega os mesmos arquivos que a main.c carrega,
 * posiciona o jogador no spawn de verdade e o faz andar pelo escritorio usando
 * exatamente as funcoes de colisao do jogo.
 *
 * Compilar e rodar: tools/tests/run_tests.ps1
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "raylib.h"

/* A main.c define estas globais; aqui o teste assume esse papel. */
unsigned int screen_width = 0;
unsigned int screen_height = 0;
unsigned int middle_screen_x = 0;
unsigned int middle_screen_y = 0;

/* Inclui o .c (e nao so o .h) para alcancar as variaveis e funcoes estaticas do
 * game loop: o teste precisa inspecionar o estado real do jogo, nao uma copia. */
#include "game_loop.c"

#define LARGURA_JANELA 1280
#define ALTURA_JANELA 720

#define PASSOS_ALEATORIOS 20000   /* Quantidade de passos da caminhada aleatoria */
#define PASSOS_EMPURRAO 3000      /* Passos empurrando contra a borda do mapa */

/* Grade da busca de caminho: cada celula e exatamente um passo do jogador. */
#define GRADE 501
#define GRADE_CENTRO 250

static int total_casos = 0;
static int casos_ok = 0;

static void checar(const char *id, const char *descricao, bool condicao)
{
    total_casos++;

    if (condicao)
    {
        casos_ok++;
        printf("  [ OK  ]  %-6s  %s\n", id, descricao);
    }
    else
    {
        printf("  [FALHA]  %-6s  %s\n", id, descricao);
    }
}

static void titulo(const char *texto)
{
    printf("\n  --- %s ---\n\n", texto);
}

/* ------------------------------------------------------------------------ */
/* Apoio                                                                      */
/* ------------------------------------------------------------------------ */

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
 * handle_player_input() seguida de handle_map_collisions(). O teste so substitui
 * a leitura do teclado pelos deslocamentos dx e dy recebidos. */
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

/* Gerador pseudoaleatorio proprio, para a caminhada dar sempre o mesmo resultado. */
static unsigned int semente = 20251022u;

static unsigned int sortear(unsigned int limite)
{
    semente = semente * 1103515245u + 12345u;
    return (semente >> 16) % limite;
}

/* ------------------------------------------------------------------------ */
/* Busca do caminho do spawn ate o computador                                 */
/* ------------------------------------------------------------------------ */

static unsigned char veio_de[GRADE][GRADE];   /* 0 = nao visitado, 1..4 = direcao, 5 = origem */
static int fila[GRADE * GRADE];
static int caminho_i[GRADE * GRADE];
static int caminho_j[GRADE * GRADE];
static int tamanho_caminho = 0;

/* Procura um caminho do spawn ate a zona do computador andando um passo por vez.
 * Cada celula da grade e exatamente um passo do jogador em um eixo, entao o
 * caminho encontrado e um percurso que o jogador consegue fazer de verdade. */
static bool existe_caminho_ate_o_computador(Vector2 spawn, int *passos_do_caminho)
{
    float passo = DEFAULT_PLAYER_MOVEMENT_SPEED * get_scale();

    const int di[4] = {1, -1, 0, 0};
    const int dj[4] = {0, 0, 1, -1};

    memset(veio_de, 0, sizeof(veio_de));

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
            tamanho_caminho = 0;

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

/* ------------------------------------------------------------------------ */
/* Evidencia visual                                                           */
/* ------------------------------------------------------------------------ */

/* Desenha o escritorio com as hitboxes e o caminho encontrado, e salva em PNG.
 * Usa uma textura de destino porque a Raylib acumula os desenhos em lote: lendo
 * a tela direto, o PNG sairia com o fundo limpo e nada por cima. */
static void gravar_evidencia(Vector2 spawn, const char *arquivo)
{
    float passo = DEFAULT_PLAYER_MOVEMENT_SPEED * get_scale();

    RenderTexture2D alvo = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

    BeginTextureMode(alvo);
    ClearBackground(BG_COLOR);

    Color guardado = color_to_fade;
    color_to_fade = (Color){255, 255, 255, 255};

    render_game_loop_escritorio();

    /* Obstaculos, limite do mapa e zona do computador, como no modo debug (F1). */
    DrawRectangleLinesEx(map_collision_rec, 2.0f, RED);

    for (int i = 0; i < OBSTACLE_COUNT; i++)
    {
        DrawRectangleLinesEx(office_obstacles[i], 2.0f, ORANGE);
    }

    DrawRectangleLinesEx(estagiario_computer_collision_rectangle, 3.0f, YELLOW);

    /* Caminho simulado do spawn ate o computador. */
    for (int k = 0; k < tamanho_caminho; k++)
    {
        float px = spawn.x + (caminho_i[k] - GRADE_CENTRO) * passo;
        float py = spawn.y + (caminho_j[k] - GRADE_CENTRO) * passo;
        Rectangle hitbox = hitbox_em(px, py);

        DrawCircle((int)(hitbox.x + hitbox.width / 2.0f),
                   (int)(hitbox.y + hitbox.height / 2.0f), 2.0f, SKYBLUE);
    }

    DrawRectangleLinesEx(hitbox_em(spawn.x, spawn.y), 3.0f, GREEN);

    DrawText("IARA - teste de sistema: percurso do spawn ate o computador", 12, 10, 20, RAYWHITE);
    DrawText("verde: spawn   azul: caminho simulado   laranja: obstaculos   amarelo: computador",
             12, 34, 16, LIGHTGRAY);

    color_to_fade = guardado;

    EndTextureMode();

    Image imagem = LoadImageFromTexture(alvo.texture);
    ImageFlipVertical(&imagem); /* textura de destino vem de cabeca para baixo */
    ExportImage(imagem, arquivo);

    UnloadImage(imagem);
    UnloadRenderTexture(alvo);
}

/* ------------------------------------------------------------------------ */
/* Casos de teste                                                             */
/* ------------------------------------------------------------------------ */

static void testar_recursos(void)
{
    titulo("Recursos em disco (CT-01 a CT-03)");

    checar("CT-01", "Janela do jogo inicializa", IsWindowReady());

    /* Acentos do portugues que precisam existir na fonte. */
    const int acentuados[] = {0xC1, 0xC9, 0xCD, 0xD3, 0xDA, 0xC7, 0xC3, 0xD5,
                              0xE1, 0xE9, 0xED, 0xF3, 0xFA, 0xE7, 0xE3, 0xF5};
    int total_acentos = (int)(sizeof(acentuados) / sizeof(acentuados[0]));
    int acentos_ok = 0;

    for (int i = 0; i < total_acentos; i++)
    {
        int indice = GetGlyphIndex(default_game_font, acentuados[i]);

        if (indice >= 0 && indice < default_game_font.glyphCount &&
            default_game_font.glyphs[indice].value == acentuados[i])
        {
            acentos_ok++;
        }
    }

    printf("           fonte VT323 com %d glifos; %d de %d acentos presentes\n",
           default_game_font.glyphCount, acentos_ok, total_acentos);
    checar("CT-02", "Fonte do jogo cobre os acentos do portugues",
           default_game_font.glyphCount > 0 && acentos_ok == total_acentos);

    /* Todo arquivo que o jogo abre em algum momento da partida. */
    const char *arquivos[] = {
        "code/assets/fonts/VT323-Regular.ttf",
        "code/assets/background/second-background.png",
        "code/assets/icons/arrow-icon.png",
        "code/assets/icons/arrow-down.png",
        "code/assets/sprites/tela-inicial/escritorio-tela-inicial.png",
        "code/assets/sprites/tela-inicial/iara-nome.png",
        "code/assets/sprites/environment/office.png",
        ACTION_BUTTONS_SPRITES_PATH,
        MOVE_BUTTONS_SPRITES_PATH,
        MOVE_OR_BUTTONS_SPRITES_PATH,
        COMPUTER_WINDOW_PNG_PATH,
        COMPUTER_CORRECT_ICON,
        COMPUTER_WRONG_ICON,
        ESTAGIARIO_1_EAST_PATH,
        ESTAGIARIO_1_NORTH_PATH,
        ESTAGIARIO_1_NORTHEAST_PATH,
        ESTAGIARIO_1_NORTHWEST_PATH,
        ESTAGIARIO_1_SOUTH_PATH,
        ESTAGIARIO_1_SOUTHEAST_PATH,
        ESTAGIARIO_1_SOUTHWEST_PATH,
        ESTAGIARIO_1_WEST_PATH,
        PERSONAGENS_PATH "Estagiarios/walk/east-0.png",
        PERSONAGENS_PATH "Estagiarios/walk/east-1.png",
        PERSONAGENS_PATH "Estagiarios/walk/east-2.png",
        PERSONAGENS_PATH "Estagiarios/walk/east-3.png"};

    int total_arquivos = (int)(sizeof(arquivos) / sizeof(arquivos[0]));
    int encontrados = 0;

    for (int i = 0; i < total_arquivos; i++)
    {
        if (FileExists(arquivos[i])) encontrados++;
        else printf("           FALTANDO: %s\n", arquivos[i]);
    }

    printf("           %d de %d arquivos de midia encontrados\n", encontrados, total_arquivos);
    checar("CT-03", "Todos os arquivos de midia usados pelo jogo existem",
           encontrados == total_arquivos);
}

static void testar_carregamento_do_jogo(void)
{
    titulo("Carregamento da partida (CT-04 a CT-07)");

    checar("CT-04", "Mapa do escritorio carregado na placa de video", in_game_office.id > 0);

    int direcoes_ok = 0;

    for (int i = 0; i < DIRECTION_COUNT; i++)
    {
        if (estagiario_texture[i].id > 0) direcoes_ok++;
    }

    printf("           %d de %d sprites direcionais do estagiario carregados\n",
           direcoes_ok, DIRECTION_COUNT);
    checar("CT-05", "Sprites das 8 direcoes do jogador carregados",
           direcoes_ok == DIRECTION_COUNT);

    int frames_ok = 0;

    for (int i = 0; i < WALK_FRAME_COUNT; i++)
    {
        if (walk_east_textures[i].id > 0) frames_ok++;
        if (walk_west_textures[i].id > 0) frames_ok++;
    }

    checar("CT-06", "Frames do andar (leste e oeste) carregados",
           frames_ok == WALK_FRAME_COUNT * 2);

    /* Um segundo andando a 60 quadros por segundo. */
    bool ciclo_correto = true;

    for (int quadro = 0; quadro < 60; quadro++)
    {
        float tempo = quadro * (1.0f / 60.0f);
        int frame = get_walk_frame(tempo);
        int esperado = ((int)(tempo / WALK_FRAME_TIME)) % WALK_FRAME_COUNT;

        if (frame < 0 || frame >= WALK_FRAME_COUNT) ciclo_correto = false;
        if (frame != esperado) ciclo_correto = false;
    }

    checar("CT-07", "Animacao do andar percorre os 4 frames em ciclo", ciclo_correto);
}

static void testar_percurso(Vector2 spawn)
{
    titulo("Percurso real do jogador (CT-08)");

    current_estagiario_position = spawn;
    update_collision_rec();

    int passos = 0;
    bool chegou = existe_caminho_ate_o_computador(spawn, &passos);

    if (chegou) printf("           caminho encontrado em %d passos do jogador\n", passos);

    checar("CT-08", "Jogador consegue ir do spawn ate o computador", chegou);
}

static void testar_movimento(Vector2 spawn)
{
    titulo("Movimento e colisoes (CT-09 a CT-13)");

    current_estagiario_position = spawn;
    update_collision_rec();

    bool spawn_livre = !collides_with_any(estagiario_collision_rec, office_obstacles, OBSTACLE_COUNT);
    bool spawn_no_mapa = dentro_do_mapa(estagiario_collision_rec);

    checar("CT-09", "Jogador nasce livre, fora de qualquer obstaculo",
           spawn_livre && spawn_no_mapa);

    /* Caminhada aleatoria: a cada passo o jogador nao pode acabar dentro de um
     * obstaculo nem fora do mapa. */
    int violacoes_obstaculo = 0;
    int violacoes_mapa = 0;

    const int dxs[8] = {1, -1, 0, 0, 1, 1, -1, -1};
    const int dys[8] = {0, 0, 1, -1, 1, -1, 1, -1};

    for (int passo = 0; passo < PASSOS_ALEATORIOS; passo++)
    {
        int direcao = (int)sortear(8);

        simular_passo(dxs[direcao], dys[direcao]);

        if (collides_with_any(estagiario_collision_rec, office_obstacles, OBSTACLE_COUNT))
        {
            violacoes_obstaculo++;
        }

        if (!dentro_do_mapa(estagiario_collision_rec)) violacoes_mapa++;
    }

    printf("           %d passos simulados; %d invasoes de obstaculo, %d saidas do mapa\n",
           PASSOS_ALEATORIOS, violacoes_obstaculo, violacoes_mapa);
    checar("CT-10", "Jogador nunca entra em um obstaculo ao andar", violacoes_obstaculo == 0);
    checar("CT-11", "Jogador nunca sai dos limites do mapa", violacoes_mapa == 0);

    /* Empurra contra cada borda do mapa por muitos quadros seguidos. */
    const int bordas[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    bool preso_no_mapa = true;

    for (int b = 0; b < 4; b++)
    {
        for (int passo = 0; passo < PASSOS_EMPURRAO; passo++)
        {
            simular_passo(bordas[b][0], bordas[b][1]);

            if (!dentro_do_mapa(estagiario_collision_rec)) preso_no_mapa = false;
        }
    }

    checar("CT-12", "Empurrar contra as 4 bordas nao atravessa a parede", preso_no_mapa);

    /* Zona de interacao do computador. */
    current_estagiario_position = spawn;
    update_collision_rec();
    handle_map_interactables();
    bool longe = !is_near_computer;

    Rectangle zona = estagiario_computer_collision_rectangle;
    current_estagiario_position = (Vector2){
        zona.x + zona.width / 2.0f - HITBOX_OFFSET_X_AT_REFERENCE * get_scale(),
        zona.y + zona.height / 2.0f - HITBOX_OFFSET_Y_AT_REFERENCE * get_scale()};
    update_collision_rec();
    handle_map_interactables();
    bool perto = is_near_computer;

    checar("CT-13", "Zona do computador so acende com o jogador em cima", longe && perto);
}

static void testar_redimensionamento(Vector2 spawn)
{
    titulo("Redimensionamento da janela (CT-14)");

    current_estagiario_position = spawn;
    last_screen_center = (Vector2){GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
    last_scale = get_scale();
    update_obstacle_recs();
    update_collision_rec();

    /* Posicao do jogador em relacao ao centro do mapa, na escala de referencia. */
    float ref_x = (spawn.x - last_screen_center.x) / last_scale;
    float ref_y = (spawn.y - last_screen_center.y) / last_scale;

    const int tamanhos[3][2] = {{1024, 576}, {1600, 900}, {LARGURA_JANELA, ALTURA_JANELA}};
    bool posicao_mantida = true;
    bool continua_livre = true;

    for (int t = 0; t < 3; t++)
    {
        SetWindowSize(tamanhos[t][0], tamanhos[t][1]);

        /* Deixa a Raylib processar o novo tamanho antes de medir. */
        for (int quadro = 0; quadro < 3; quadro++)
        {
            BeginDrawing();
            ClearBackground(BG_COLOR);
            EndDrawing();
        }

        keep_player_place_on_resize();
        update_obstacle_recs();
        estagiario_computer_collision_rectangle = get_computer_collision_rectangle();
        update_collision_rec();

        Vector2 centro = {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
        float escala = get_scale();
        float ref_x_agora = (current_estagiario_position.x - centro.x) / escala;
        float ref_y_agora = (current_estagiario_position.y - centro.y) / escala;

        /* Um pixel de referencia de tolerancia, por causa do arredondamento. */
        if (fabsf(ref_x_agora - ref_x) > 1.0f || fabsf(ref_y_agora - ref_y) > 1.0f)
        {
            posicao_mantida = false;
            printf("           em %dx%d o jogador saiu de (%.1f, %.1f) para (%.1f, %.1f)\n",
                   tamanhos[t][0], tamanhos[t][1], ref_x, ref_y, ref_x_agora, ref_y_agora);
        }

        if (collides_with_any(estagiario_collision_rec, office_obstacles, OBSTACLE_COUNT))
        {
            continua_livre = false;
        }
    }

    checar("CT-14", "Redimensionar a janela mantem o jogador no mesmo ponto do mapa",
           posicao_mantida && continua_livre);
}

static void testar_animacoes(void)
{
    titulo("Animacoes de transicao (CT-15 e CT-16)");

    Color cor = {255, 255, 255, 0};
    bool terminou = false;
    int voltas = 0;

    while (!terminou && voltas < 1000)
    {
        fade_in_texture(MEDIUM_FADE_SPEED, &cor, &terminou);
        voltas++;
    }

    printf("           fade in terminou com alpha = %d (esperado 255)\n", cor.a);
    checar("CT-15", "Fade in chega a opacidade total (alpha 255)", terminou && cor.a == 255);

    terminou = false;
    voltas = 0;

    while (!terminou && voltas < 1000)
    {
        fade_out_texture(MEDIUM_FADE_SPEED, &cor, &terminou);
        voltas++;
    }

    printf("           fade out terminou com alpha = %d (esperado 0)\n", cor.a);
    checar("CT-16", "Fade out chega a transparencia total (alpha 0)", terminou && cor.a == 0);
}

int main(void)
{
    /* Sem logs da Raylib: o relatorio dos testes fica limpo para o screencast. */
    SetTraceLogLevel(LOG_WARNING);

    InitWindow(LARGURA_JANELA, ALTURA_JANELA, "IARA - testes de sistema");

    /* Mesmo ajuste que a main.c faz depois de abrir a janela. */
    screen_width = GetScreenWidth();
    screen_height = GetScreenHeight();
    middle_screen_x = screen_width / 2;
    middle_screen_y = screen_height / 2;

    printf("\n========================================================================\n");
    printf(" IARA - TESTES DE SISTEMA | Bloco 1: partida, movimento e colisoes\n");
    printf("========================================================================\n");

    initialize_fonts();
    initialize_game_loop_variables();

    Vector2 spawn = current_estagiario_position;

    testar_recursos();
    testar_carregamento_do_jogo();
    testar_percurso(spawn);
    testar_movimento(spawn);
    testar_redimensionamento(spawn);
    testar_animacoes();

    /* Refaz o percurso na janela original para a imagem de evidencia. */
    SetWindowSize(LARGURA_JANELA, ALTURA_JANELA);

    for (int quadro = 0; quadro < 3; quadro++)
    {
        BeginDrawing();
        ClearBackground(BG_COLOR);
        EndDrawing();
    }

    update_obstacle_recs();
    estagiario_computer_collision_rectangle = get_computer_collision_rectangle();

    int passos = 0;

    if (existe_caminho_ate_o_computador(spawn, &passos))
    {
        gravar_evidencia(spawn, "docs/fds/testes/evidencias/percurso-e-colisoes.png");
        printf("\n  Evidencia salva em docs/fds/testes/evidencias/percurso-e-colisoes.png\n");
    }

    printf("\n------------------------------------------------------------------------\n");
    printf(" Bloco 1: %d de %d casos aprovados", casos_ok, total_casos);

    if (casos_ok != total_casos) printf(" | %d FALHA(S)", total_casos - casos_ok);

    printf("\n========================================================================\n\n");

    unload_fonts();
    CloseWindow();

    return (casos_ok == total_casos) ? 0 : 1;
}
