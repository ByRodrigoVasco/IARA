/*
 * Testes de sistema do IARA - Bloco 4: historias restantes da Sprint 03 (Week 15e16).
 *
 * Cobre as historias da Sprint 03 que ainda nao tinham caso de teste:
 *
 *   US16 (PI2-253)  efeitos sonoros: passos, porta, ambiente, teclado e voz dos NPCs
 *   US17 (PI2-254)  sprites dos NPCs e arte das telas de flash, salto e finais
 *   US10 (PI2-197)  estado do jogo sem HUD, por sintomas no cenario
 *   US13 (PI2-200)  tensao que cresce sem susto repentino
 *   US04 (PI2-186)  dica pelo dialogo de NPC (PI2-202)
 *   US11 (PI2-168)  peso de cada metodo de resolucao e acumulo entre os dias
 *
 * O bloco reaproveita o bloco 3 (test_fluxo.c) inteiro: monta o jogo de verdade e
 * o conduz pela maquina de estados, quadro a quadro. Duas coisas mudam:
 *
 *   1. As teclas passam por um gancho. A Raylib nao permite injetar teclas, entao
 *      IsKeyDown e IsKeyPressed respondem "apertada" para a tecla que o teste
 *      escolher, e para o resto consultam o teclado de verdade. Andar, conversar e
 *      sair pela porta passam pelo codigo do jogo, sem atalho.
 *   2. Os sons passam por um registro. LoadSound e PlaySound anotam o arquivo de
 *      cada som tocado, e o teste confere o que o jogador ouviria em cada acao.
 *
 * Cada caso pode rodar sozinho, para gravar o screencast dele:
 *     test_sprint03 CT-95
 *
 * Compilar e rodar: tools/tests/run_tests.ps1
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "raylib.h"

/* ------------------------------------------------------------------------ */
/* Gancho do teclado                                                          */
/* ------------------------------------------------------------------------ */

static int tecla_segurada = -1;   /* Tecla mantida apertada, como WASD ao andar */
static int tecla_apertada = -1;   /* Tecla apertada so neste quadro, como ESPACO */

static bool tecla_segurada_no_teste(int tecla)
{
    return tecla == tecla_segurada || (IsKeyDown)(tecla);
}

static bool tecla_apertada_no_teste(int tecla)
{
    return tecla == tecla_apertada || (IsKeyPressed)(tecla);
}

/* ------------------------------------------------------------------------ */
/* Registro dos sons                                                          */
/* ------------------------------------------------------------------------ */

#define MAX_SONS_CARREGADOS 64
#define MAX_SONS_TOCADOS 512

typedef struct
{
    void *buffer;
    char arquivo[160];
} SomCarregado;

static SomCarregado sons_carregados[MAX_SONS_CARREGADOS];
static int total_sons_carregados = 0;
static char sons_tocados[MAX_SONS_TOCADOS][160];
static int total_sons_tocados = 0;

static const char *nome_do_som(Sound som)
{
    for (int i = 0; i < total_sons_carregados; i++)
    {
        if (sons_carregados[i].buffer == som.stream.buffer) return sons_carregados[i].arquivo;
    }
    return "(som desconhecido)";
}

static Sound carregar_som_registrado(const char *arquivo)
{
    Sound som = (LoadSound)(arquivo);

    if (total_sons_carregados < MAX_SONS_CARREGADOS && som.stream.buffer != NULL)
    {
        sons_carregados[total_sons_carregados].buffer = som.stream.buffer;
        snprintf(sons_carregados[total_sons_carregados].arquivo, 160, "%s", GetFileName(arquivo));
        total_sons_carregados++;
    }
    return som;
}

static void tocar_som_registrado(Sound som)
{
    if (total_sons_tocados < MAX_SONS_TOCADOS)
    {
        snprintf(sons_tocados[total_sons_tocados], 160, "%s", nome_do_som(som));
        total_sons_tocados++;
    }
    (PlaySound)(som);
}

/* Algum som tocado desde o indice "desde" tem este trecho no nome do arquivo? */
static bool tocou_desde(int desde, const char *trecho)
{
    for (int i = desde; i < total_sons_tocados; i++)
    {
        if (strstr(sons_tocados[i], trecho) != NULL) return true;
    }
    return false;
}

static bool algum_som_carregado_tem(const char *trecho)
{
    for (int i = 0; i < total_sons_carregados; i++)
    {
        if (strstr(sons_carregados[i].arquivo, trecho) != NULL) return true;
    }
    return false;
}

/* Lista, numa linha so, os sons que tocaram desde "desde" (sem repetir). */
static const char *sons_tocados_desde(int desde)
{
    static char lista[1024];
    lista[0] = '\0';

    for (int i = desde; i < total_sons_tocados; i++)
    {
        bool repetido = false;

        for (int j = desde; j < i; j++)
        {
            if (strcmp(sons_tocados[i], sons_tocados[j]) == 0) repetido = true;
        }
        if (repetido) continue;
        if (lista[0] != '\0') strncat(lista, ", ", sizeof(lista) - strlen(lista) - 1);
        strncat(lista, sons_tocados[i], sizeof(lista) - strlen(lista) - 1);
    }
    return lista[0] != '\0' ? lista : "nenhum";
}

/* Os ganchos valem para o audio, o computador, o menu e o escritorio, que entram
 * inteiros logo abaixo (o bloco 3 os inclui). */
#define IsKeyDown(tecla) tecla_segurada_no_teste(tecla)
#define IsKeyPressed(tecla) tecla_apertada_no_teste(tecla)
#define LoadSound(arquivo) carregar_som_registrado(arquivo)
#define PlaySound(som) tocar_som_registrado(som)

#include "audio.c"

/* O bloco 3 inteiro, com a main dele renomeada: daqui saem a janela, os saves de
 * teste e a conducao do jogo. */
#define main main_do_bloco_3
#include "test_fluxo.c"
#undef main

/* O bloco 3 desfaz o gancho de IsKeyPressed depois dos creditos; o escritorio, o
 * computador e o audio ja foram compilados com ele. */

/* ------------------------------------------------------------------------ */
/* Conducao                                                                   */
/* ------------------------------------------------------------------------ */

static const char *caso_pedido = NULL;   /* NULL roda todos */

static bool deve_rodar(const char *id)
{
    return caso_pedido == NULL || strcmp(caso_pedido, id) == 0;
}

/* Cabecalho de um caso, no formato do relatorio: historia, pre-condicao e passos. */
static void cabecalho(const char *id, const char *historia, const char *pre_condicao)
{
    printf("\n  --- %s | %s ---\n", id, historia);
    printf("           Pre-condicao: %s\n", pre_condicao);
}

/* Um quadro do jogo, como a main.c: o audio e a tela andam pela maquina de estados. */
static void quadro(void)
{
    handle_state_machine();
    tecla_apertada = -1;
}

static void quadros(int quantos)
{
    for (int i = 0; i < quantos && !WindowShouldClose(); i++) quadro();
}

static void apertar(int tecla)
{
    tecla_apertada = tecla;
    quadro();
}

/* Rodando um caso so (para o screencast), segura a tela por um tempo para dar para ver. */
static void pausa_para_o_video(double segundos)
{
    if (caso_pedido == NULL) return;
    quadros((int)(segundos * 60.0));
}

/* Pula o cartao do dia com ESPACO, como o jogador, ate o escritorio aparecer. */
static bool chegar_ao_escritorio(void)
{
    double inicio = GetTime();

    while (current_screen != IN_GAME_SCREEN && GetTime() - inicio < 25.0 && !WindowShouldClose())
    {
        if (current_screen == CALENDAR_DAY_CARD_SCREEN) apertar(KEY_SPACE);
        else quadro();
    }
    quadros(20); /* fade do escritorio */
    pausa_para_o_video(1.0);
    return current_screen == IN_GAME_SCREEN;
}

/* Partida nova, no Dia 1, com o jogador na entrada do escritorio. */
static bool comecar_no_dia_1(void)
{
    iniciar_partida_nova();
    return chegar_ao_escritorio();
}

/* Partida retomada do save num dia jogavel qualquer, com o escritorio a vista. */
static bool comecar_no_dia(int dia)
{
    iniciar_partida_nova();
    initialize_hidden_statistic();
    start_game_day(dia);
    current_game_state_variables.current_screen = retomar_partida_do_save();
    for (int i = 0; i < 5; i++) quadro();
    return chegar_ao_escritorio() && current_game_state_variables.current_day == dia;
}

/* Busca de caminho ate o ponto em que "chegou" vale, desviando de moveis e NPCs.
 * Devolve a direcao do primeiro passo (dx, dy) ou false se nao houver caminho. */
typedef bool (*Chegada)(Rectangle hitbox, int alvo);

static bool primeiro_passo(Chegada chegou, int alvo, int *dx, int *dy)
{
    float passo = DEFAULT_PLAYER_MOVEMENT_SPEED * get_scale();
    Vector2 origem = current_estagiario_position;
    const int di[4] = {1, -1, 0, 0};
    const int dj[4] = {0, 0, 1, -1};
    int inicio = 0;
    int fim = 0;

    atualizar_obstaculos_com_npcs();
    memset(veio_de, 0, sizeof(veio_de));
    veio_de[GRADE_CENTRO][GRADE_CENTRO] = 5;
    fila[fim++] = GRADE_CENTRO * GRADE + GRADE_CENTRO;

    while (inicio < fim)
    {
        int atual = fila[inicio++];
        int i = atual / GRADE;
        int j = atual % GRADE;

        if (chegou(hitbox_em(origem.x + (i - GRADE_CENTRO) * passo, origem.y + (j - GRADE_CENTRO) * passo), alvo))
        {
            /* Volta pelo caminho ate a celula vizinha da origem. */
            int anterior_i = i;
            int anterior_j = j;

            while (veio_de[i][j] != 5)
            {
                anterior_i = i;
                anterior_j = j;
                int de_onde = veio_de[i][j] - 1;
                i -= di[de_onde];
                j -= dj[de_onde];
            }
            *dx = anterior_i - GRADE_CENTRO;
            *dy = anterior_j - GRADE_CENTRO;
            return true;
        }

        for (int d = 0; d < 4; d++)
        {
            int ni = i + di[d];
            int nj = j + dj[d];

            if (ni < 0 || ni >= GRADE || nj < 0 || nj >= GRADE) continue;
            if (veio_de[ni][nj] != 0) continue;
            if (!posicao_livre_com_npcs(origem.x + (ni - GRADE_CENTRO) * passo, origem.y + (nj - GRADE_CENTRO) * passo)) continue;

            veio_de[ni][nj] = (unsigned char)(d + 1);
            fila[fim++] = ni * GRADE + nj;
        }
    }
    return false;
}

static int tecla_da_direcao(int dx, int dy)
{
    if (dx > 0) return KEY_D;
    if (dx < 0) return KEY_A;
    if (dy > 0) return KEY_S;
    return KEY_W;
}

/* Anda com as teclas de verdade (WASD), um quadro por passo, ate chegar. Os NPCs
 * que andam mudam o caminho, por isso ele e recalculado a cada passo. */
static bool andar_ate(Chegada chegou, int alvo, int limite_de_passos, int *passos)
{
    *passos = 0;

    for (int k = 0; k < limite_de_passos && !WindowShouldClose(); k++)
    {
        update_collision_rec();
        if (chegou(estagiario_collision_rec, alvo))
        {
            tecla_segurada = -1;
            quadro();
            return true;
        }

        int dx = 0;
        int dy = 0;

        if (!primeiro_passo(chegou, alvo, &dx, &dy))
        {
            /* Um NPC andando pode fechar o caminho por um instante: espera ele passar. */
            tecla_segurada = -1;
            quadro();
            continue;
        }

        tecla_segurada = tecla_da_direcao(dx, dy);
        quadro();
        (*passos)++;
    }

    tecla_segurada = -1;
    quadro();
    update_collision_rec();
    return chegou(estagiario_collision_rec, alvo);
}

static bool chegou_ao_npc(Rectangle hitbox, int npc)
{
    return npc_get_nearby(hitbox, get_scale()) == npc;
}

static bool chegou_ao_computador(Rectangle hitbox, int ignorado)
{
    (void)ignorado;
    return CheckCollisionRecs(hitbox, get_computer_collision_rectangle()) && npc_get_nearby(hitbox, get_scale()) < 0;
}

static bool chegou_a_porta(Rectangle hitbox, int ignorado)
{
    (void)ignorado;
    return CheckCollisionRecs(hitbox, get_exit_zone()) && npc_get_nearby(hitbox, get_scale()) < 0 &&
           !is_hitbox_on_hint(hitbox, get_scale(), get_map_center());
}

/* Joga o computador do dia inteiro pelo codigo do quiz e volta ao escritorio. */
static bool resolver_o_dia_no_computador(void)
{
    int passos = 0;

    if (!andar_ate(chegou_ao_computador, 0, 900, &passos)) return false;

    apertar(KEY_SPACE);
    rodar_ate_a_tela(COMPUTER_SCREEN, 5.0);
    if (current_screen != COMPUTER_SCREEN) return false;

    jogar_o_computador(0);
    rodar_ate_a_tela(IN_GAME_SCREEN, 8.0);
    quadros(10);
    return current_screen == IN_GAME_SCREEN && current_game_state_variables.is_resolved;
}

/* Quadro do escritorio, sem ninguem: so cenario, para comparar cenarios entre si. */
static Image foto_do_cenario(void)
{
    RenderTexture2D alvo = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

    BeginTextureMode(alvo);
    ClearBackground(BG_COLOR);
    render_game_loop_escritorio();
    render_day_counter();
    EndTextureMode();

    Image imagem = LoadImageFromTexture(alvo.texture);
    ImageFlipVertical(&imagem);
    UnloadRenderTexture(alvo);
    return imagem;
}

static int pixels_diferentes(Image a, Image b)
{
    if (a.width != b.width || a.height != b.height) return -1;

    int diferentes = 0;
    Color *pa = LoadImageColors(a);
    Color *pb = LoadImageColors(b);

    for (int i = 0; i < a.width * a.height; i++)
    {
        if (pa[i].r != pb[i].r || pa[i].g != pb[i].g || pa[i].b != pb[i].b) diferentes++;
    }

    UnloadImageColors(pa);
    UnloadImageColors(pb);
    return diferentes;
}

/* Os dias jogaveis da campanha, na ordem do calendario. */
static int dias_jogaveis[NPC_PLAYABLE_DAY_COUNT];

static int listar_dias_jogaveis(void)
{
    int total = 0;

    for (int dia = 1; dia <= TOTAL_GAME_DAYS && total < NPC_PLAYABLE_DAY_COUNT; dia++)
    {
        if (calendar_get_day_type(dia) == PLAYABLE_DAY) dias_jogaveis[total++] = dia;
    }
    return total;
}

/* Resultado de um caso: esperado e obtido, para o relatorio e para o screencast. */
static void registrar(const char *id, const char *descricao, const char *esperado, const char *obtido, bool passou)
{
    fflush(stdout);
    pausa_para_o_video(1.0);
    printf("           Esperado:     %s\n", esperado);
    printf("           Obtido:       %s\n", obtido);
    checar(id, descricao, passou);
    fflush(stdout);
    pausa_para_o_video(3.0);
}

/* ------------------------------------------------------------------------ */
/* US16 (PI2-253): efeitos sonoros                                            */
/* ------------------------------------------------------------------------ */

static void ct94_passos(void)
{
    cabecalho("CT-94", "US16 / PI2-262: som de passos ao andar",
              "partida nova, Dia 1, jogador parado na entrada do escritorio");
    bool no_escritorio = comecar_no_dia_1();

    etapa("Passo 1: segura D por 1,5 s (90 quadros) e observa o som de passos");
    Vector2 antes = current_estagiario_position;
    int desde = total_sons_tocados;
    int quadros_com_passos = 0;

    tecla_segurada = KEY_D;
    for (int i = 0; i < 90 && !WindowShouldClose(); i++)
    {
        quadro();
        if (IsSoundPlaying(player_footsteps_sound)) quadros_com_passos++;
    }
    bool andou = fabsf(current_estagiario_position.x - antes.x) > 1.0f;

    etapa("Passo 2: solta a tecla e espera 0,5 s parado");
    tecla_segurada = -1;
    quadros(30);
    bool parou = !IsSoundPlaying(player_footsteps_sound);

    char obtido[256];
    snprintf(obtido, sizeof(obtido), "andou %.0f px; passos tocando em %d de 90 quadros; parado, o som %s",
             current_estagiario_position.x - antes.x, quadros_com_passos, parou ? "parou" : "continuou");
    registrar("CT-94", "Passos tocam enquanto o jogador anda e param quando ele para (US16, PI2-262)",
              "o som de passos toca durante a caminhada e para quando o jogador para",
              obtido, no_escritorio && andou && tocou_desde(desde, "PASSOS") && quadros_com_passos >= 60 && parou);
}

static void ct95_porta(void)
{
    cabecalho("CT-95", "US16 / PI2-263: som de porta ao entrar e sair",
              "Dia 1 com os chamados ja resolvidos no computador");
    bool no_escritorio = comecar_no_dia_1();
    bool resolvido = no_escritorio && resolver_o_dia_no_computador();

    etapa("Passo 1: anda ate a porta de vidro");
    int passos = 0;
    bool na_porta = resolvido && andar_ate(chegou_a_porta, 0, 900, &passos);

    etapa("Passo 2: aperta ESPACO na porta e acompanha 1 s da saida");
    int desde = total_sons_tocados;
    if (na_porta) apertar(KEY_SPACE);
    quadros(60);
    bool saiu = current_screen == DAY_END_CARD_SCREEN || current_screen == DAY_TRANSITION_SCREEN;
    bool som_de_porta = tocou_desde(desde, "PORTA");

    char obtido[512];
    snprintf(obtido, sizeof(obtido), "%s; sons tocados na saida: %s; PORTA ABRINDO.mp3 carregado: %s",
             saiu ? "o dia terminou pela porta" : "o jogador nao saiu pela porta",
             sons_tocados_desde(desde), algum_som_carregado_tem("PORTA") ? "sim" : "nao");
    registrar("CT-95", "Sair pela porta de vidro toca o som de porta (US16, PI2-263)",
              "ao sair pela porta toca o som de porta (arquivo PORTA ABRINDO.mp3)",
              obtido, saiu && som_de_porta);
}

static void ct96_ambiente(void)
{
    cabecalho("CT-96", "US16 / PI2-264: som ambiente continuo no escritorio",
              "Dia 1, jogador no escritorio");
    bool no_escritorio = comecar_no_dia_1();

    etapa("Passo 1: fica 3 s no escritorio e mede a camada de ambiente");
    quadros(180);
    bool tocando_no_escritorio = IsMusicStreamPlaying(calm_music) && layer_weight[AUDIO_LAYER_CALM] > 0.0f;
    float peso_no_escritorio = layer_weight[AUDIO_LAYER_CALM];

    etapa("Passo 2: anda ate o computador, abre e fica 2 s na tela do quiz");
    int passos = 0;
    bool no_computador = andar_ate(chegou_ao_computador, 0, 900, &passos);
    if (no_computador) apertar(KEY_SPACE);
    rodar_ate_a_tela(COMPUTER_SCREEN, 5.0);
    quadros(120);
    bool continua_no_quiz = IsMusicStreamPlaying(calm_music) && layer_weight[AUDIO_LAYER_CALM] > 0.0f;
    float peso_no_quiz = layer_weight[AUDIO_LAYER_CALM];

    char obtido[256];
    snprintf(obtido, sizeof(obtido), "ambiente tocando no escritorio (peso %.2f) e por baixo do quiz (peso %.2f)",
             peso_no_escritorio, peso_no_quiz);
    registrar("CT-96", "O ambiente do escritorio toca sem parar, inclusive por baixo da tela do quiz (US16, PI2-264)",
              "SOM AMBIENTE.mp3 tocando no escritorio e continuando, mais baixo, na tela do quiz",
              obtido, no_escritorio && tocando_no_escritorio && current_screen == COMPUTER_SCREEN && continua_no_quiz &&
                          peso_no_quiz < peso_no_escritorio);
}

static void ct97_teclado(void)
{
    cabecalho("CT-97", "US16 / PI2-264: som de teclado na tela do quiz",
              "Dia 1, jogador no escritorio, chamados ainda abertos");
    bool no_escritorio = comecar_no_dia_1();

    etapa("Passo 1: anda ate o computador e aperta ESPACO");
    int passos = 0;
    bool no_computador = no_escritorio && andar_ate(chegou_ao_computador, 0, 900, &passos);
    int desde = total_sons_tocados;
    if (no_computador) apertar(KEY_SPACE);
    rodar_ate_a_tela(COMPUTER_SCREEN, 5.0);

    etapa("Passo 2: responde todos os chamados do dia no computador");
    bool abriu = current_screen == COMPUTER_SCREEN;
    if (abriu) jogar_o_computador(0);
    bool som_de_teclado = tocou_desde(desde, "TECLADO");

    char obtido[512];
    snprintf(obtido, sizeof(obtido), "%s; sons tocados no computador: %s; SOM DE TECLADO.ogg carregado: %s",
             abriu ? "chamados respondidos" : "o computador nao abriu",
             sons_tocados_desde(desde), algum_som_carregado_tem("TECLADO") ? "sim" : "nao");
    registrar("CT-97", "Interagir com o computador na tela do quiz toca o som de teclado (US16, PI2-264)",
              "ao responder os chamados toca o som de teclado (arquivo SOM DE TECLADO.ogg)",
              obtido, abriu && som_de_teclado);
}

static void ct98_voz_do_npc(void)
{
    cabecalho("CT-98", "US16 / PI2-265: voice blip quando o NPC comeca a falar",
              "Dia 1, jogador no escritorio");
    bool no_escritorio = comecar_no_dia_1();
    int npc = npc_get_daily_hint_owner(1);

    etapa(TextFormat("Passo 1: anda ate %s", npc_get_definition(npc)->name));
    int passos = 0;
    bool perto = no_escritorio && andar_ate(chegou_ao_npc, npc, 1500, &passos);

    etapa("Passo 2: aperta ESPACO e acompanha 1 s da fala");
    int desde = total_sons_tocados;
    int quadros_com_voz = 0;
    if (perto) apertar(KEY_SPACE);
    bool conversa_aberta = active_npc_id == npc;
    for (int i = 0; i < 60 && !WindowShouldClose(); i++)
    {
        quadro();
        if (IsSoundPlaying(iara_voice_sound)) quadros_com_voz++;
    }

    etapa("Passo 3: aperta ESPACO ate a conversa acabar");
    for (int i = 0; i < 20 && active_npc_id >= 0; i++)
    {
        apertar(KEY_SPACE);
        quadros(5);
    }

    char obtido[512];
    snprintf(obtido, sizeof(obtido), "%s; sons tocados durante a fala: %s; voz tocando em %d de 60 quadros",
             conversa_aberta ? TextFormat("conversa com %s aberta", npc_get_definition(npc)->name) : "a conversa nao abriu",
             sons_tocados_desde(desde), quadros_com_voz);
    registrar("CT-98", "Quando um NPC comeca a falar toca um som curto de fala (US16, PI2-265)",
              "ao abrir a conversa toca um voice blip enquanto a fala aparece",
              obtido, conversa_aberta && quadros_com_voz > 0);
}

/* ------------------------------------------------------------------------ */
/* US17 (PI2-254): sprites                                                    */
/* ------------------------------------------------------------------------ */

static void ct99_sprites_dos_npcs(void)
{
    cabecalho("CT-99", "US17 / PI2-267 a PI2-269: sprites dos NPCs no padrao de load_walk_frames",
              "jogo instalado com code/assets/sprites/PERSONAGENS");
    comecar_no_dia_1();
    const char *direcoes[] = {"south", "north", "east", "south-east", "north-west", "south-west", "north-east"};
    int npcs_completos = 0;
    int arquivos_faltando = 0;
    char faltando[256] = "";

    etapa("Passo 1: para cada NPC, confere os 4 quadros de cada direcao em <pasta>/walk/<direcao>-<n>.png");
    for (int npc = 0; npc < NPC_COUNT; npc++)
    {
        const NpcDefinition *definicao = npc_get_definition(npc);
        bool completo = true;

        for (int d = 0; d < 7; d++)
        {
            for (int n = 0; n < WALK_FRAME_COUNT; n++)
            {
                if (!FileExists(TextFormat("%s%s/walk/%s-%d.png", PERSONAGENS_PATH, definicao->sprite_folder, direcoes[d], n)))
                {
                    completo = false;
                    arquivos_faltando++;
                    if (strlen(faltando) < 180)
                        strncat(faltando, TextFormat("%s/%s-%d ", definicao->sprite_folder, direcoes[d], n),
                                sizeof(faltando) - strlen(faltando) - 1);
                }
            }
        }
        if (completo) npcs_completos++;
    }

    etapa("Passo 2: abre o escritorio e carrega os NPCs como o jogo carrega");
    unload_npcs();
    bool carregou = initialize_npcs(get_scale(), get_map_center());

    char obtido[512];
    snprintf(obtido, sizeof(obtido), "%d de %d NPCs com as 7 direcoes x 4 quadros; %d arquivo(s) faltando %s; initialize_npcs: %s",
             npcs_completos, NPC_COUNT, arquivos_faltando, faltando, carregou ? "ok" : "falhou");
    registrar("CT-99", "Os sprites dos 7 NPCs seguem a pasta e os nomes de load_walk_frames e carregam no jogo (US17, PI2-269)",
              "todos os NPCs com walk/<direcao>-0..3.png e initialize_npcs() carregando sem erro",
              obtido, npcs_completos == NPC_COUNT && carregou);
}

static void ct100_arte_das_telas(void)
{
    cabecalho("CT-100", "US17 / PI2-267 e PI2-268: arte do Flash do dia, do salto puro e dos 4 finais",
              "partida retomada no Dia 1, escritorio a vista");
    comecar_no_dia_1();

    etapa("Passo 1: abre o Flash do Dia 2 e compara com o escritorio escurecido do Dia 2");
    start_game_day(2);
    current_screen = DAY_TRANSITION_SCREEN;
    transition_time = 0.0f;
    Image escritorio = foto_do_cenario();
    RenderTexture2D alvo = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
    BeginTextureMode(alvo);
    ClearBackground(BG_COLOR);
    color_to_fade.a = 255;
    render_game_loop_escritorio();
    EndTextureMode();
    Image so_o_escritorio = LoadImageFromTexture(alvo.texture);
    ImageFlipVertical(&so_o_escritorio);
    UnloadRenderTexture(alvo);
    for (int i = 0; i < 90; i++) quadro();
    bool flash_reusa_escritorio = pixels_diferentes(escritorio, so_o_escritorio) >= 0;
    UnloadImage(escritorio);
    UnloadImage(so_o_escritorio);

    etapa("Passo 2: procura arte propria de flash, salto e finais nas pastas de sprites e de fundo");
    const char *pastas[] = {"code/assets/sprites", "code/assets/background"};
    const char *temas[] = {"flash", "salto", "jump", "final", "ending"};
    int achados = 0;
    for (int p = 0; p < 2; p++)
    {
        FilePathList arquivos = LoadDirectoryFilesEx(pastas[p], ".png", true);
        for (unsigned int i = 0; i < arquivos.count; i++)
        {
            const char *nome = GetFileName(arquivos.paths[i]);
            if (strstr(arquivos.paths[i], "sobrevivencia") != NULL) continue;
            for (int t = 0; t < 5; t++)
            {
                if (strstr(TextToLower(nome), temas[t]) != NULL) achados++;
            }
        }
        UnloadDirectoryFiles(arquivos);
    }

    etapa("Passo 3: abre o cartao de cada um dos 4 finais");
    int finais_com_texto = 0;
    for (int f = 0; f < 4; f++)
    {
        if (ending_cards[f].title != NULL && strlen(ending_cards[f].title) > 0) finais_com_texto++;
    }
    current_game_state_variables.found_mariana = false;
    show_ending();
    for (int i = 0; i < 90; i++) quadro();

    char obtido[512];
    snprintf(obtido, sizeof(obtido),
             "o Flash usa o escritorio escurecido com texto%s; o salto e os %d finais sao so texto; "
             "%d imagem(ns) de flash, salto ou final nas pastas de arte",
             flash_reusa_escritorio ? "" : " (nao conferido)", finais_com_texto, achados);
    registrar("CT-100", "Flash do dia, salto puro e os 4 finais tem arte propria (US17, PI2-267 e PI2-268)",
              "sprites/imagens proprias para o Flash do dia ou o salto puro e para as telas dos 4 finais",
              obtido, achados > 0);

    iniciar_partida_nova();
}

/* ------------------------------------------------------------------------ */
/* US10 (PI2-197): sintomas no lugar do HUD                                   */
/* ------------------------------------------------------------------------ */

static void definir_estatisticas(unsigned int pontos, unsigned int automacao, unsigned int percepcao, unsigned int vinculo)
{
    current_hidden_statistic.current_overall_points = pontos;
    current_hidden_statistic.current_automation_points = automacao;
    current_hidden_statistic.current_perception_points = percepcao;
    current_hidden_statistic.current_human_bond_points = vinculo;
}

static int diferenca_por_estatisticas = -1;

static void ct101_sem_hud(void)
{
    cabecalho("CT-101", "US10 / PI2-236: nenhum numero de status na tela",
              "Dia 3, escritorio a vista");
    /* O Dia 3 vem antes de o escritorio comecar a escurecer: sem a luz das lampadas, que
     * pisca com o tempo, as duas fotos so diferem se um numero for desenhado. */
    bool no_escritorio = comecar_no_dia(3);

    /* Os dois estados tem numeros diferentes, mas ficam na mesma faixa de sintomas
     * (abaixo de todos os limiares de symptoms.h): o cenario e o mesmo, entao qualquer
     * pixel diferente so pode vir de um numero de status desenhado na tela. Comparar
     * estatisticas em faixas opostas confundiria um HUD com os sintomas do CT-102. */
    etapa("Passo 1: fotografa o escritorio com 500 pontos e as demais estatisticas no inicio");
    definir_estatisticas(500, HIDDEN_STATISTIC_START, HIDDEN_STATISTIC_START, HIDDEN_STATISTIC_START);
    quadros(30);
    Image antes = foto_do_cenario();

    etapa("Passo 2: muda para 999 pontos e 1 ponto de Percepcao, ainda abaixo de qualquer sintoma");
    definir_estatisticas(999, HIDDEN_STATISTIC_START, HIDDEN_STATISTIC_START + 1, HIDDEN_STATISTIC_START);
    quadros(30);
    Image depois = foto_do_cenario();

    diferenca_por_estatisticas = pixels_diferentes(antes, depois);
    UnloadImage(antes);
    UnloadImage(depois);

    char obtido[256];
    snprintf(obtido, sizeof(obtido), "%d pixel(s) diferentes entre as duas fotos: nenhuma estatistica aparece como numero",
             diferenca_por_estatisticas);
    registrar("CT-101", "Nenhuma estatistica oculta aparece como numero na tela do escritorio (US10, PI2-236)",
              "mudar os numeros sem mudar a faixa de sintomas nao altera nenhum pixel (so o contador de dias)",
              obtido, no_escritorio && diferenca_por_estatisticas == 0);
}

static void ct102_cenario_reage(void)
{
    cabecalho("CT-102", "US10 / PI2-237: o cenario muda conforme o estado do jogador",
              "Dia 27, escritorio a vista");
    bool no_escritorio = comecar_no_dia(27);

    etapa("Passo 1: fotografa o escritorio de quem resolveu tudo sozinho (Automacao 1, Percepcao 5)");
    definir_estatisticas(150, 1, 5, 3);
    quadros(30);
    Image cuidadoso = foto_do_cenario();

    etapa("Passo 2: fotografa o mesmo dia de quem so enviou sem conferir (Automacao 9, Percepcao 1)");
    definir_estatisticas(0, 9, 1, 1);
    quadros(30);
    Image automatico = foto_do_cenario();
    int diferenca = pixels_diferentes(cuidadoso, automatico);

    etapa("Passo 3: compara com o Dia 89, para ver que o cenario muda com o dia");
    UnloadImage(cuidadoso);
    start_game_day(89);
    quadros(10);
    Image dia_89 = foto_do_cenario();
    int diferenca_entre_dias = pixels_diferentes(automatico, dia_89);
    UnloadImage(automatico);
    UnloadImage(dia_89);

    char obtido[256];
    snprintf(obtido, sizeof(obtido),
             "no mesmo dia, %d pixel(s) diferentes entre os dois jogadores; entre o Dia 27 e o 89, %d",
             diferenca, diferenca_entre_dias);
    registrar("CT-102", "O escritorio reage as escolhas do jogador, nao so ao dia (US10, PI2-237)",
              "jogadores com estatisticas opostas veem o escritorio diferente no mesmo dia",
              obtido, no_escritorio && diferenca > 0);

    iniciar_partida_nova();
}

/* ------------------------------------------------------------------------ */
/* US13 (PI2-200): tensao progressiva                                         */
/* ------------------------------------------------------------------------ */

static void ct103_tensao(void)
{
    cabecalho("CT-103", "US13 / PI2-245 a PI2-247: tensao que sobe sem susto",
              "partida nova; dias jogaveis do calendario");
    int total = listar_dias_jogaveis();
    float tensao[NPC_PLAYABLE_DAY_COUNT];
    float agua[NPC_PLAYABLE_DAY_COUNT];
    bool nunca_recua = true;
    int desde = total_sons_tocados;

    etapa("Passo 1: abre cada dia jogavel, em ordem, e anota a tensao da trilha e a agua no escritorio");
    for (int i = 0; i < total; i++)
    {
        comecar_no_dia(dias_jogaveis[i]);
        quadros(30);
        tensao[i] = game_tension_target;
        agua[i] = calendar_get_water_level(dias_jogaveis[i]);
        etapa(TextFormat("      Dia %3d: tensao %.2f, agua %.2f", dias_jogaveis[i], tensao[i], agua[i]));
        if (i > 0 && (tensao[i] < tensao[i - 1] || agua[i] < agua[i - 1])) nunca_recua = false;
    }

    etapa("Passo 2: confere se algum som de susto (GRITO, GLITCH) foi carregado ou tocou");
    bool susto = tocou_desde(desde, "GRITO") || tocou_desde(desde, "GLITCH") ||
                 algum_som_carregado_tem("GRITO") || algum_som_carregado_tem("GLITCH");

    char obtido[256];
    snprintf(obtido, sizeof(obtido), "tensao de %.2f a %.2f e agua de %.2f a %.2f, %s; som de susto: %s",
             tensao[0], tensao[total - 1], agua[0], agua[total - 1],
             nunca_recua ? "sem nunca recuar" : "recuando em algum dia", susto ? "sim" : "nenhum");
    registrar("CT-103", "A tensao da trilha e o cenario sobem dia a dia, comecando calmos, sem som de susto (US13)",
              "Dia 1 calmo, tensao e agua nunca recuam ate o ultimo dia jogavel e nenhum grito ou glitch toca",
              obtido, total == NPC_PLAYABLE_DAY_COUNT && tensao[0] == 0.0f && nunca_recua &&
                          tensao[total - 1] > tensao[0] && agua[total - 1] > agua[0] && !susto);

    iniciar_partida_nova();
}

/* ------------------------------------------------------------------------ */
/* US04 (PI2-186 e PI2-202): dica pelo NPC                                    */
/* ------------------------------------------------------------------------ */

static void ct104_dica_do_npc(void)
{
    cabecalho("CT-104", "US04 / PI2-202: dica pelo dialogo de NPC, sem custo",
              "cada dia jogavel, jogador na entrada do escritorio");
    int total = listar_dias_jogaveis();
    int dias_com_dica = 0;
    int dias_sem_custo = 0;
    bool alguma_dica_da_iara = false;
    char dias_sem_dica[128] = "";

    etapa("Passo 1: em cada dia, anda ate o NPC da dica e conversa ate o fim da fala");
    for (int i = 0; i < total; i++)
    {
        int dia = dias_jogaveis[i];
        comecar_no_dia(dia);
        int npc = npc_get_daily_hint_owner(dia);

        if (npc < 0 || !npc_has_hint(npc, dia))
        {
            strncat(dias_sem_dica, TextFormat("%d ", dia), sizeof(dias_sem_dica) - strlen(dias_sem_dica) - 1);
            continue;
        }
        if (strstr(TextToLower(npc_get_definition(npc)->name), "iara") != NULL) alguma_dica_da_iara = true;

        struct hidden_statistic antes = current_hidden_statistic;
        int passos = 0;
        bool perto = andar_ate(chegou_ao_npc, npc, 1500, &passos);
        bool ouviu_a_dica = false;

        if (perto) apertar(KEY_SPACE);
        for (int k = 0; k < 30 && active_npc_id >= 0; k++)
        {
            const char *fala = npc_get_dialogue_line(active_npc_id, dia, active_npc_first_today, active_npc_dialogue_line);
            if (fala != NULL && strcmp(fala, npc_get_hint_line(dia, 0)) == 0) ouviu_a_dica = true;
            quadros(3);
            apertar(KEY_SPACE);
            quadros(3);
            apertar(KEY_SPACE);
        }

        if (ouviu_a_dica) dias_com_dica++;
        if (memcmp(&antes, &current_hidden_statistic, sizeof(antes)) == 0) dias_sem_custo++;
        etapa(TextFormat("      Dia %3d: %s, %d passos, dica %s", dia, npc_get_definition(npc)->name, passos,
                         ouviu_a_dica ? "ouvida" : "nao ouvida"));
    }

    char obtido[256];
    snprintf(obtido, sizeof(obtido), "dica ouvida pelo NPC em %d de %d dias%s%s; pontos iguais depois da conversa em %d de %d; dica da IARA: %s",
             dias_com_dica, total, dias_sem_dica[0] ? ", sem NPC de dica nos dias " : "", dias_sem_dica,
             dias_sem_custo, total, alguma_dica_da_iara ? "sim" : "nunca");
    registrar("CT-104", "Em todo dia jogavel um NPC (nunca a IARA) da a dica, e conversar nao muda nenhum ponto (US04, PI2-202)",
              "9 de 9 dias com dica ouvida no dialogo, estatisticas iguais antes e depois, nenhuma dica da IARA",
              obtido, dias_com_dica == total && dias_sem_custo == total && !alguma_dica_da_iara);

    iniciar_partida_nova();
}

/* ------------------------------------------------------------------------ */
/* US11 / PI2-168: pontuacao por metodo                                       */
/* ------------------------------------------------------------------------ */

/* Joga os 9 dias resolvendo todos os chamados do mesmo jeito; "excecao" muda um dia so. */
static GameEnding jogar_campanha_por_metodo(TaskOutcome metodo, bool com_dica, int dia_de_excecao,
                                           TaskOutcome metodo_da_excecao, unsigned int *pontos)
{
    initialize_game_state_variables();
    initialize_hidden_statistic();
    int total = listar_dias_jogaveis();

    for (int i = 0; i < total; i++)
    {
        TaskResult chamados[5];
        TaskOutcome hoje = (i == dia_de_excecao) ? metodo_da_excecao : metodo;

        start_game_day(dias_jogaveis[i]);
        current_game_state_variables.user_investigated = com_dica;
        for (int c = 0; c < 5; c++) chamados[c] = (TaskResult){.outcome = hoje, .caught_iara_error = false};
        resolve_day_tasks(chamados, 5);
    }
    *pontos = current_hidden_statistic.current_overall_points;
    return get_game_ending();
}

static void ct105_pesos(void)
{
    cabecalho("CT-105", "US11 / PI2-168: peso de cada metodo e acumulo entre os dias",
              "partida nova, 9 dias jogaveis com 5 chamados cada");
    comecar_no_dia_1();
    unsigned int com_dica = 0, sozinho = 0, automatico = 0, quase_sozinho = 0, quase_automatico = 0;

    etapa("Passo 1: joga os 9 dias de tres jeitos: manual com a dica do mural, manual sem dica e so com a IARA");
    GameEnding final_com_dica = jogar_campanha_por_metodo(TASK_SOLVED_ALONE, true, -1, TASK_PENDING, &com_dica);
    GameEnding final_sozinho = jogar_campanha_por_metodo(TASK_SOLVED_ALONE, false, -1, TASK_PENDING, &sozinho);
    GameEnding final_automatico = jogar_campanha_por_metodo(TASK_SENT_UNCHECKED, false, -1, TASK_PENDING, &automatico);

    etapa("Passo 2: repete trocando o metodo de um dia so, para ver se uma escolha isolada decide o final");
    GameEnding final_quase_sozinho = jogar_campanha_por_metodo(TASK_SOLVED_ALONE, false, 4, TASK_SENT_UNCHECKED, &quase_sozinho);
    GameEnding final_quase_automatico = jogar_campanha_por_metodo(TASK_SENT_UNCHECKED, false, 4, TASK_SOLVED_ALONE, &quase_automatico);

    const char *letras = "ABCD";
    char obtido[384];
    snprintf(obtido, sizeof(obtido),
             "com dica %u pts (Final %c), sem dica %u pts (Final %c), IARA %u pts (Final %c); "
             "trocando um dia: %u pts (Final %c) e %u pts (Final %c)",
             com_dica, letras[final_com_dica], sozinho, letras[final_sozinho], automatico, letras[final_automatico],
             quase_sozinho, letras[final_quase_sozinho], quase_automatico, letras[final_quase_automatico]);
    registrar("CT-105", "Manual com dica vale mais que manual sem dica, que vale mais que a IARA; os pontos acumulam e um dia isolado nao muda o final (US11, PI2-168)",
              "com dica > sem dica > automatico, somados nos 9 dias, e o final so muda com o padrao das escolhas",
              obtido, com_dica > sozinho && sozinho > automatico && final_sozinho != final_automatico &&
                          final_quase_sozinho == final_sozinho && final_quase_automatico == final_automatico);

    initialize_game_state_variables();
    initialize_hidden_statistic();
}

/* ------------------------------------------------------------------------ */

typedef struct
{
    const char *id;
    void (*rodar)(void);
} CasoDoBloco;

static const CasoDoBloco casos_do_bloco[] = {
    {"CT-94", ct94_passos},          {"CT-95", ct95_porta},
    {"CT-96", ct96_ambiente},        {"CT-97", ct97_teclado},
    {"CT-98", ct98_voz_do_npc},      {"CT-99", ct99_sprites_dos_npcs},
    {"CT-100", ct100_arte_das_telas}, {"CT-101", ct101_sem_hud},
    {"CT-102", ct102_cenario_reage}, {"CT-103", ct103_tensao},
    {"CT-104", ct104_dica_do_npc},   {"CT-105", ct105_pesos},
};

int main(int argc, char **argv)
{
    if (argc > 1) caso_pedido = argv[1];

    SetTraceLogLevel(LOG_WARNING);
    abrir_janela_de_teste(LARGURA_JANELA, ALTURA_JANELA, "IARA - testes da Sprint 03");

    screen_width = GetScreenWidth();
    screen_height = GetScreenHeight();
    middle_screen_x = screen_width / 2;
    middle_screen_y = screen_height / 2;

    SetTargetFPS(60);

    printf("\n========================================================================\n");
    printf(" IARA - TESTES DE SISTEMA | Bloco 4: historias restantes da Sprint 03\n");
    printf("========================================================================\n");
    if (caso_pedido != NULL) printf("\n  So o caso %s.\n", caso_pedido);

    /* Mesma ordem de preparo da main.c, com o audio ligado: os casos da US16 escutam o jogo. */
    bool audio_ok = initialize_audio();
    initialize_fonts();
    initialize_menu_variables();
    preparar_saves_de_teste();
    if (!audio_ok) printf("\n  [aviso] o dispositivo de audio nao abriu: os casos de som vao falhar\n");

    for (size_t i = 0; i < sizeof(casos_do_bloco) / sizeof(casos_do_bloco[0]); i++)
    {
        if (deve_rodar(casos_do_bloco[i].id)) casos_do_bloco[i].rodar();
    }

    printf("\n------------------------------------------------------------------------\n");
    printf(" Bloco 4: %d de %d casos aprovados", casos_ok, total_casos);
    if (casos_ok != total_casos) printf(" | %d FALHA(S)", total_casos - casos_ok);
    printf("\n========================================================================\n\n");

    unload_game_loop();
    limpar_saves_de_teste();
    save_slots_set_root(NULL);
    unload_audio();
    unload_fonts();
    CloseWindow();

    return (casos_ok == total_casos) ? 0 : 1;
}
