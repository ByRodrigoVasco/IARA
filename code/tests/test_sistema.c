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
#include <math.h>

#include "raylib.h"
#include "janela_de_teste.h"
#include "game_state.h"

/* A main.c define estas globais; aqui o teste assume esse papel. */
unsigned int screen_width = 0;
unsigned int screen_height = 0;
unsigned int middle_screen_x = 0;
unsigned int middle_screen_y = 0;

/* Inclui o .c (e nao so o .h) para alcancar as variaveis e funcoes estaticas do
 * game loop: o teste precisa inspecionar o estado real do jogo, nao uma copia. */
#include "game_loop.c"

/* Movimento do jogador e busca de caminho; precisa vir depois do game_loop.c. */
#include "percurso_do_jogador.h"

#define LARGURA_JANELA 1280
#define ALTURA_JANELA 720

#define PASSOS_ALEATORIOS 20000   /* Quantidade de passos da caminhada aleatoria */
#define PASSOS_EMPURRAO 3000      /* Passos empurrando contra a borda do mapa */

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

/* Gerador pseudoaleatorio proprio, para a caminhada dar sempre o mesmo resultado. */
static unsigned int semente = 20251022u;

static unsigned int sortear(unsigned int limite)
{
    semente = semente * 1103515245u + 12345u;
    return (semente >> 16) % limite;
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

    for (int d = 0; d < DIRECTION_COUNT; d++)
    {
        for (int i = 0; i < WALK_FRAME_COUNT; i++)
        {
            if (walk_textures[d][i].id > 0) frames_ok++;
        }
    }

    checar("CT-06", "Frames do andar das 8 direcoes carregados",
           frames_ok == WALK_FRAME_COUNT * DIRECTION_COUNT);

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

static void testar_npcs(Vector2 player_spawn)
{
    titulo("NPCs, movimentacao e colisoes");

    bool sprites_carregados = initialize_npcs(get_scale(), get_map_center());
    checar("CT-71", "Sprites e animacoes dos NPCs carregados", sprites_carregados);

    current_estagiario_position = player_spawn;
    update_collision_rec();
    bool posicoes_livres = true;

    for (int npc = 0; npc < npc_get_count(); npc++)
    {
        Rectangle hitbox = npc_get_hitbox(npc, get_scale());
        if (npc_is_moving(npc) &&
            (collides_with_any(hitbox, office_obstacles, OBSTACLE_COUNT) ||
             CheckCollisionRecs(hitbox, estagiario_collision_rec) ||
             !dentro_do_mapa(hitbox)))
        {
            printf("           NPC %d inicia sobreposto a movel, jogador ou limite do mapa\n", npc);
            posicoes_livres = false;
        }

        for (int other = npc + 1; npc_is_moving(npc) && other < npc_get_count(); other++)
        {
            if (npc_is_moving(other) && CheckCollisionRecs(hitbox, npc_get_hitbox(other, get_scale())))
            {
                printf("           NPCs %d e %d iniciam sobrepostos\n", npc, other);
                posicoes_livres = false;
            }
        }
    }

    checar("CT-72", "NPCs nascem sem sobreposicao com o mapa, moveis ou jogador", posicoes_livres);

    Rectangle zone = npc_get_interaction_zone(NPC_ANDERSON, get_scale());
    Rectangle player_near_npc = {zone.x + zone.width / 2.0f - 5.0f,
                                  zone.y + zone.height / 2.0f - 5.0f, 10.0f, 10.0f};
    checar("CT-73", "Jogador pode alcancar a zona de conversa de um NPC",
           npc_get_nearby(player_near_npc, get_scale()) == NPC_ANDERSON);

    Rectangle player_obstacles[OBSTACLE_COUNT + NPC_COUNT];
    int player_obstacle_count = get_player_movement_obstacles(player_obstacles);
    Rectangle npc_hitbox = npc_get_hitbox(NPC_ZITO, get_scale());
    Rectangle player_hitbox = {npc_hitbox.x - npc_hitbox.width, npc_hitbox.y,
                               npc_hitbox.width, npc_hitbox.height};
    float blocked_position = move_axis(player_hitbox.x, 2.0f, player_hitbox, true,
                                      player_obstacles, player_obstacle_count);
    checar("CT-74", "Colisao do jogador inclui a hitbox dos NPCs",
           blocked_position == player_hitbox.x);

    Rectangle initial_hitboxes[NPC_COUNT];
    bool movement_and_collisions_hold = true;
    bool every_npc_moved[NPC_COUNT] = {false};

    for (int npc = 0; npc < npc_get_count(); npc++)
    {
        initial_hitboxes[npc] = npc_get_hitbox(npc, get_scale());
    }

    SetRandomSeed(20261107u);
    for (int frame = 0; frame < 300; frame++)
    {
        update_npcs(0.05f, get_scale(), get_map_center(), map_collision_rec,
                    office_obstacles, OBSTACLE_COUNT, estagiario_collision_rec);

        for (int npc = 0; npc < npc_get_count(); npc++)
        {
            Rectangle hitbox = npc_get_hitbox(npc, get_scale());
            if (hitbox.x != initial_hitboxes[npc].x || hitbox.y != initial_hitboxes[npc].y)
            {
                every_npc_moved[npc] = true;
            }

            if (npc_is_moving(npc) &&
                (collides_with_any(hitbox, office_obstacles, OBSTACLE_COUNT) ||
                 CheckCollisionRecs(hitbox, estagiario_collision_rec) ||
                 !dentro_do_mapa(hitbox)))
            {
                if (movement_and_collisions_hold)
                {
                    printf("           NPC %d colidiu com movel, jogador ou mapa no quadro %d\n", npc, frame);
                }
                movement_and_collisions_hold = false;
            }

            for (int other = npc + 1; npc_is_moving(npc) && other < npc_get_count(); other++)
            {
                if (npc_is_moving(other) &&
                    CheckCollisionRecs(hitbox, npc_get_hitbox(other, get_scale())))
                {
                    if (movement_and_collisions_hold)
                    {
                        printf("           NPCs %d e %d colidiram no quadro %d\n", npc, other, frame);
                    }
                    movement_and_collisions_hold = false;
                }
            }
        }
    }

    for (int npc = 0; npc < npc_get_count(); npc++)
    {
        if (every_npc_moved[npc] != npc_is_moving(npc))
        {
            printf("           NPC %d movimento inesperado\n", npc);
            movement_and_collisions_hold = false;
        }
    }

    checar("CT-75", "NPCs permanecem parados e colidem apenas quando estao em pe",
           movement_and_collisions_hold);
}

static void testar_percurso(Vector2 spawn)
{
    titulo("Percurso real do jogador (CT-08)");

    current_estagiario_position = spawn;
    update_collision_rec();

    int passos = 0;
    bool chegou = calcular_caminho_ate_o_computador(spawn, &passos);

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

/* A dica fica em um item do cenario: o jogador precisa conseguir chegar ate ele,
 * e examina-lo nao pode mexer na pontuacao nem soltar o jogador (PI2-203/204). */
static void testar_dica(Vector2 spawn)
{
    titulo("Dica no mural (CT-38 e CT-39)");

    Rectangle zona = get_hint_zone(get_scale(), get_map_center());
    int passos = 0;
    bool alcancavel = calcular_caminho_ate(zona, spawn, &passos);

    printf("           caminho do spawn ate o mural: %d passos\n", passos);
    checar("CT-38", "Jogador consegue ir do spawn ate o item da dica", alcancavel);

    initialize_hidden_statistic();
    struct hidden_statistic antes = current_hidden_statistic;

    /* O mural so tem o que examinar nos dias com investigacao. */
    current_game_state_variables.current_day = 1;
    current_estagiario_position = posicao_do_caminho(spawn, 0);
    update_collision_rec();
    handle_map_interactables();
    bool perto = is_near_hint;

    toggle_hint_note();
    bool abriu = is_hint_note_open();
    toggle_hint_note();

    bool mesmos_pontos = current_hidden_statistic.current_overall_points == antes.current_overall_points &&
                         current_hidden_statistic.current_automation_points == antes.current_automation_points &&
                         current_hidden_statistic.current_perception_points == antes.current_perception_points &&
                         current_hidden_statistic.current_human_bond_points == antes.current_human_bond_points;

    checar("CT-39", "Examinar a dica abre o bilhete e nao altera a pontuacao",
           perto && abriu && !is_hint_note_open() && mesmos_pontos);

    current_estagiario_position = spawn;
    update_collision_rec();
}

/* Simula a visao com daltonismo sobre um pixel (matrizes de Machado et al., 2009,
 * severidade total). */
static Color simular_daltonismo(Color c, const float m[3][3])
{
    float r = c.r, g = c.g, b = c.b;
    float saida[3];

    for (int l = 0; l < 3; l++)
    {
        saida[l] = m[l][0] * r + m[l][1] * g + m[l][2] * b;
        if (saida[l] < 0.0f) saida[l] = 0.0f;
        if (saida[l] > 255.0f) saida[l] = 255.0f;
    }

    return (Color){(unsigned char)saida[0], (unsigned char)saida[1], (unsigned char)saida[2], 255};
}

/* Razao de contraste da WCAG entre duas cores. */
static float luminancia_relativa(Color c)
{
    float canais[3] = {c.r / 255.0f, c.g / 255.0f, c.b / 255.0f};

    for (int i = 0; i < 3; i++)
    {
        canais[i] = (canais[i] <= 0.03928f) ? canais[i] / 12.92f : powf((canais[i] + 0.055f) / 1.055f, 2.4f);
    }

    return 0.2126f * canais[0] + 0.7152f * canais[1] + 0.0722f * canais[2];
}

static float razao_de_contraste(Color a, Color b)
{
    float la = luminancia_relativa(a) + 0.05f;
    float lb = luminancia_relativa(b) + 0.05f;

    return (la > lb) ? la / lb : lb / la;
}

/* Desenha o escritorio com o modo ligado e confere, com protanopia e deuteranopia
 * simuladas, se o contorno de cada elemento continua se destacando (CT-40). */
static void testar_alto_contraste(void)
{
    titulo("Alto contraste com daltonismo simulado (CT-40)");

    const float protanopia[3][3] = {{0.152286f, 1.052583f, -0.204868f}, {0.114503f, 0.786281f, 0.099216f}, {-0.003882f, -0.048116f, 1.051998f}};
    const float deuteranopia[3][3] = {{0.367322f, 0.860646f, -0.227968f}, {0.280085f, 0.672501f, 0.047413f}, {-0.011820f, 0.042940f, 0.968881f}};

    toggle_high_contrast();
    color_to_fade = WHITE;

    RenderTexture2D alvo = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

    BeginTextureMode(alvo);
    begin_high_contrast_filter();
    ClearBackground(BG_COLOR);
    render_game_loop_escritorio();
    render_player();
    render_high_contrast_outlines();
    end_high_contrast_filter();
    EndTextureMode();

    Image quadro = LoadImageFromTexture(alvo.texture);
    ImageFlipVertical(&quadro);

    float espessura = HIGH_CONTRAST_LINE_AT_REFERENCE * get_scale();
    if (espessura < 2.0f) espessura = 2.0f;

    /* Em cada elemento, compara a linha clara com a escura logo ao lado. */
    Rectangle elementos[] = {estagiario_computer_collision_rectangle, hint_item_rectangle};
    int fracos = 0;

    for (int e = 0; e < 2; e++)
    {
        int meio_y = (int)(elementos[e].y + elementos[e].height / 2.0f);
        Color clara = GetImageColor(quadro, (int)(elementos[e].x + espessura / 2.0f), meio_y);
        Color escura = GetImageColor(quadro, (int)(elementos[e].x - espessura / 2.0f), meio_y);

        if (razao_de_contraste(simular_daltonismo(clara, protanopia), simular_daltonismo(escura, protanopia)) < 3.0f) fracos++;
        if (razao_de_contraste(simular_daltonismo(clara, deuteranopia), simular_daltonismo(escura, deuteranopia)) < 3.0f) fracos++;
    }

    /* Vidro: traco claro sobre a base escura, comparado com o intervalo entre tracos. */
    Rectangle vidro = office_obstacles[GLASS_ROOMS_OBSTACLE];
    Color traco = GetImageColor(quadro, (int)(vidro.x + espessura * 2.0f), (int)(vidro.y + espessura / 2.0f));
    Color intervalo = GetImageColor(quadro, (int)(vidro.x + espessura * 7.0f), (int)(vidro.y + espessura / 2.0f));

    if (razao_de_contraste(simular_daltonismo(traco, deuteranopia), simular_daltonismo(intervalo, deuteranopia)) < 3.0f) fracos++;
    if (razao_de_contraste(simular_daltonismo(traco, protanopia), simular_daltonismo(intervalo, protanopia)) < 3.0f) fracos++;

    checar("CT-40", "Contornos seguem distinguiveis com protanopia e deuteranopia simuladas", fracos == 0);

    /* Evidencia: o quadro como uma pessoa com deuteranopia o veria. */
    for (int y = 0; y < quadro.height; y++)
    {
        for (int x = 0; x < quadro.width; x++)
        {
            ImageDrawPixel(&quadro, x, y, simular_daltonismo(GetImageColor(quadro, x, y), deuteranopia));
        }
    }

    ExportImage(quadro, "docs/fds/testes/evidencias/alto-contraste-deuteranopia.png");

    UnloadImage(quadro);
    UnloadRenderTexture(alvo);
    toggle_high_contrast();
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

    abrir_janela_de_teste(LARGURA_JANELA, ALTURA_JANELA, "IARA - testes de sistema");

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
    testar_npcs(spawn);
    testar_percurso(spawn);
    testar_movimento(spawn);
    testar_dica(spawn);
    testar_alto_contraste();
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

    if (calcular_caminho_ate_o_computador(spawn, &passos))
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
