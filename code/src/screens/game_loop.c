#include <time.h>
#include <math.h> // Necessário para operações matemáticas avançadas, como a função sin() usada na animação do indicador

#include "raylib.h"
#include "game_loop.h"
#include "macros.h"
#include "fonts.h"
#include "animations.h"
#include "characters.h"
#include "collisions.h"
#include "computer_loop.h"

// Valor de sentinela para indicar que o temporizador do cartão de introdução ainda não começou a contar
#define DID_NOT_START -30

// ============================================================================
// IDENTIFICADORES DE TELA DO GAME LOOP (MÁQUINA DE ESTADOS PRINCIPAL)
// ============================================================================
#define SCREEN_NONE 0                  // Estado inicial ou nulo antes da definição da tela
#define CALENDAR_DAY_CARD_SCREEN 1     // Tela do cartão narrativo do dia ("Dia 1...")
#define IN_GAME_SCREEN 2               // Tela do jogo onde o jogador anda pelo escritório
#define COMPUTER_SCREEN 3              // Tela interativa do computador de trabalho

// ============================================================================
// ESTADOS DA ANIMAÇÃO DO CARTÃO DO DIA (TRANSIÇÃO FADE IN / AWAIT / FADE OUT)
// ============================================================================
#define NONE 0          // Nenhum estado atribuído
#define FADING_IN 1     // O texto e o fundo estão surgindo gradualmente (Aumentando Alpha)
#define AWAIT 2         // O texto fica 100% visível aguardando o tempo limite especificado
#define FADING_OUT 3    // O texto está desaparecendo gradualmente (Reduzindo Alpha)

// ============================================================================
// VALORES DE REFERÊNCIA PARA SUPORTE A MÚLTIPLAS RESOLUÇÕES (RESPONSIVIDADE)
// Todos os cálculos de tamanho e offset tomam como base a resolução 1920x1080 (1080p)
// ============================================================================
#define REFERENCE_HEIGHT 1080.0f               // Altura padrão de projeto para cálculo de proporções
#define PLAYER_SIZE_AT_REFERENCE 75.0f         // Largura e altura base do sprite do jogador em pixels
#define COMPUTER_OFFSET_X_AT_REFERENCE 170.0f  // Deslocamento X do computador a partir do centro da tela
#define COMPUTER_OFFSET_Y_AT_REFERENCE 93.0f   // Deslocamento Y do computador a partir do centro da tela
#define COMPUTER_SIZE_AT_REFERENCE 100.0f      // Dimensão base do retângulo de interação do computador

// ============================================================================
// DIMENSÕES E OFFSETS DA HITBOX (CAIXA DE COLISÃO DO JOGADOR)
// Ajustados para que a colisão ocorra apenas no corpo do personagem e não no espaço transparente do sprite
// ============================================================================
#define HITBOX_OFFSET_X_AT_REFERENCE 10.0f  // Distância do topo-esquerdo do sprite até o início da hitbox (X)
#define HITBOX_OFFSET_Y_AT_REFERENCE 0.1f   // Distância do topo-esquerdo do sprite até o início da hitbox (Y)
#define HITBOX_WIDTH_AT_REFERENCE    50.0f  // Largura efetiva da área de colisão do jogador
#define HITBOX_HEIGHT_AT_REFERENCE   72.0f  // Altura efetiva da área de colisão do jogador

// ============================================================================
// VARIÁVEIS ESTÁTICAS DE CONTROLE E ESTADO
// ============================================================================
static int current_calendar_day_state = NONE;             // Estado atual da animação da tela do cartão narrativo
static bool render_calendar_day_card_finished = false;   // Indica se toda a sequência do cartão narrativo terminou
static int current_screen = SCREEN_NONE;                  // Armazena qual tela do jogo está sendo processada no momento
static bool enter_computer_screen = false;               // Flag acionada ao interagir com o computador para mudar de tela
static bool is_near_computer = false;                     // Flag que indica se o jogador está sobre a zona de interação do computador

// Texturas gerais de interface e mapas
static Texture2D second_background_texture;              // Textura do fundo do cartão do dia
static Color color_to_fade = {255, 255, 255, 0};          // Cor base de renderização (RGBA) usada para aplicar efeito de opacidade/fade

// Textos e tamanhos pré-calculados para otimização de renderização na tela de transição
static char *current_card_title = "Dia 1";
static Vector2 calendar_card_title_text_size;

static char *current_card_subtitle_line_1 = "Sua mesa fica na salinha ao lado do corredor, um pouco afastada do resto.";
static Vector2 calendar_card_subtitle_text_size_line_1;

static char *current_card_subtitle_line_2 = "Ninguém te recebe. O computador já está ligado, esperando.";
static Vector2 calendar_card_subtitle_text_size_line_2;

static char *next_day_subtitle_card = "Em breve, novos dias...";
static Vector2 next_day_subtitle_text_size;

// Texturas do ambiente de jogo (Escritório e Dicas de Teclado)
static Texture2D in_game_office;        // Mapa do escritório
static Texture2D action_tip_texture;    // Textura da dica visual de ação (Espaço)
static Texture2D move_tip_texture;      // Textura da dica visual de movimento (WASD)
static Texture2D move_or_tip_texture;   // Textura da dica visual alternativa de movimento (Setas)
static Texture2D arrow_down_texture;     // Seta indicativa flutuante sobre objetos interativos

// Estruturas do personagem e caixas de colisão
static char_textures estagiario_texture;                 // Array/Estrutura com as texturas de todas as direções do jogador
static Vector2 current_estagiario_position;               // Posição cartesiana (X, Y) do jogador na tela
static Texture2D current_estagiario_texture_direction;   // Textura atual baseada na direção para onde o jogador está olhando

static Rectangle map_collision_rec;                      // Limites físicos das paredes do mapa
static Rectangle estagiario_computer_collision_rectangle; // Área de gatilho para interagir com o computador
static Rectangle estagiario_collision_rec;               // Hitbox atualizada dinamicamente do jogador
static bool show_debug_hitboxes = false;                 // Modo debug (F1): desenha as hitboxes de colisão

// Obstáculos medidos no mapa de referência (office.png), relativos ao centro da tela
#define OBSTACLE_COUNT 15  // Quantidade de obstáculos medidos; atualizar ao incluir mais
static Rectangle office_obstacles_reference[OBSTACLE_COUNT] = {
    {67.0f, 47.0f, 192.0f, 125.0f},      // mesas do grupo inferior direito (a do jogador)
    {-271.0f, 49.0f, 192.0f, 125.0f},    // mesas do grupo inferior esquerdo
    {-271.0f, -206.0f, 192.0f, 136.0f},  // mesas do grupo superior esquerdo
    {67.0f, -206.0f, 192.0f, 136.0f},    // mesas do grupo superior direito
    {-21.0f, -163.0f, 32.0f, 85.0f},     // planta grande do meio, em cima
    {-21.0f, 92.0f, 32.0f, 82.0f},       // planta grande do meio, embaixo
    {-356.0f, -438.0f, 730.0f, 115.0f},  // parede do topo (cozinha, quadro e bebedouro)
    {-531.0f, -438.0f, 175.0f, 322.0f},  // banheiros e sala do mural
    {374.0f, -438.0f, 157.0f, 715.0f},   // sala de servidores e salas de vidro da direita
    {-531.0f, 310.0f, 1062.0f, 129.0f},  // parede de baixo, porta e jardineiras
    {-476.0f, -133.0f, 85.0f, 87.0f},    // impressora
    {-529.0f, 52.0f, 103.0f, 87.0f},     // estante de livros
    {-491.0f, 167.0f, 200.0f, 127.0f},   // sofá, poltronas e mesinha
    {-526.0f, -63.0f, 33.0f, 58.0f},     // planta da esquerda, em cima
    {-526.0f, 157.0f, 33.0f, 83.0f}};    // planta da esquerda, embaixo
static Rectangle office_obstacles[OBSTACLE_COUNT];  // Mesmos obstáculos em coordenadas da tela

// ---------------------------------------------------------------------------
// FUNÇÕES DE REDIMENSIONAMENTO E ESCALA DINÂMICA
// ---------------------------------------------------------------------------

/**
 * @brief Calcula o fator de escala dinâmico proporcional à resolução atual da tela.
 * 
 * Toma como base a altura de 1080p (REFERENCE_HEIGHT).
 * Exemplo: 
 *   - Em 1920x1080 -> Retorna 1.0f
 *   - Em 1280x720  -> Retorna 0.667f (720 / 1080)
 *   - Em 3840x2160 -> Retorna 2.0f (2160 / 1080)
 * 
 * @return float Fator multiplicador de escala.
 */
static float get_scale(void)
{
    return GetScreenHeight() / REFERENCE_HEIGHT;
}

/**
 * @brief Obtém a dimensão (largura/altura) do sprite do jogador redimensionada para a resolução atual.
 * 
 * @return float Tamanho do jogador ajustado pela escala da janela.
 */
static float get_player_size(void)
{
    return PLAYER_SIZE_AT_REFERENCE * get_scale();
}

/**
 * @brief Calcula dinamicamente o retângulo de colisão/interação do computador no mapa.
 * 
 * Posiciona o retângulo em relação ao centro da tela, aplicando o deslocamento (offset)
 * de referência e ajustando o tamanho conforme a escala atual da tela.
 * 
 * @return Rectangle Área delimitada do computador em coordenadas do mundo.
 */
static Rectangle get_computer_collision_rectangle(void)
{
    Rectangle reference = {COMPUTER_OFFSET_X_AT_REFERENCE, COMPUTER_OFFSET_Y_AT_REFERENCE,
                           COMPUTER_SIZE_AT_REFERENCE, COMPUTER_SIZE_AT_REFERENCE};
    Vector2 screen_center = {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};

    return scale_reference_rect(reference, get_scale(), screen_center);
}

/**
 * @brief Atualiza a caixa de colisão (hitbox) do jogador conforme sua posição atual na tela.
 * 
 * Aplica os offsets X e Y escalados à posição do sprite do jogador para garantir
 * que a caixa de colisão acompanhe a movimentação exata do personagem.
 */
void update_collision_rec(void)
{
    Rectangle reference = {HITBOX_OFFSET_X_AT_REFERENCE, HITBOX_OFFSET_Y_AT_REFERENCE,
                           HITBOX_WIDTH_AT_REFERENCE, HITBOX_HEIGHT_AT_REFERENCE};

    estagiario_collision_rec = scale_reference_rect(reference, get_scale(), current_estagiario_position);
}

/**
 * @brief Converte os obstáculos de referência para a tela atual (a janela pode mudar de tamanho).
 */
static void update_obstacle_recs(void)
{
    Vector2 screen_center = {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};

    for (int i = 0; i < OBSTACLE_COUNT; i++)
    {
        office_obstacles[i] = scale_reference_rect(office_obstacles_reference[i], get_scale(), screen_center);
    }
}

/**
 * @brief Inicializa todas as variáveis, carrega imagens da memória de disco para a GPU,
 * redimensiona ativos visuais, calcula tamanhos de fontes e define os estados iniciais.
 * 
 * Deve ser chamada uma única vez antes de iniciar a execução das telas do jogo.
 */
void initialize_game_loop_variables(void)
{
    // Carrega a textura de fundo da tela de introdução
    second_background_texture = LoadTexture("code/assets/background/second-background.png");
    
    // Pré-calcula as dimensões dos textos usando a fonte padrão para posterior centralização perfeita
    calendar_card_title_text_size = MeasureTextEx(default_game_font, current_card_title, TITLE_FONT_SIZE, DEFAULT_LETTER_SPACING);
    calendar_card_subtitle_text_size_line_1 = MeasureTextEx(default_game_font, current_card_subtitle_line_1, SUBTITLE_FONT_SIZE, DEFAULT_LETTER_SPACING);
    calendar_card_subtitle_text_size_line_2 = MeasureTextEx(default_game_font, current_card_subtitle_line_2, SUBTITLE_FONT_SIZE, DEFAULT_LETTER_SPACING);

    next_day_subtitle_text_size = MeasureTextEx(default_game_font, next_day_subtitle_card, SUBTITLE_FONT_SIZE, DEFAULT_LETTER_SPACING);

    // Carrega o mapa do escritório
    in_game_office = LoadTexture("code/assets/sprites/environment/office.png");

    // Carregamento e redimensionamento na memória RAM (Image) antes de enviar para a VRAM (Texture2D) da seta de indicação
    float rescaling_factor_arrow = 0.2f;
    Image arrow_image = LoadImage(ARROW_DOWN_PNG_PATH);
    ImageResize(&arrow_image, (float)arrow_image.width * rescaling_factor_arrow, (float)arrow_image.height * rescaling_factor_arrow);
    arrow_down_texture = LoadTextureFromImage(arrow_image);

    // Carregamento e redimensionamento das imagens das dicas de controle na tela
    float rescaling_factor_tip = 1.3f;
    Image action_image = LoadImage(ACTION_BUTTONS_SPRITES_PATH);
    ImageResize(&action_image, (float)action_image.width * rescaling_factor_tip, (float)action_image.height * rescaling_factor_tip);
    action_tip_texture = LoadTextureFromImage(action_image);

    Image move_image = LoadImage(MOVE_BUTTONS_SPRITES_PATH);
    ImageResize(&move_image, (float)move_image.width * rescaling_factor_tip, (float)move_image.height * rescaling_factor_tip);
    move_tip_texture = LoadTextureFromImage(move_image);

    Image move_or_image = LoadImage(MOVE_OR_BUTTONS_SPRITES_PATH);
    ImageResize(&move_or_image, (float)move_or_image.width * rescaling_factor_tip, (float)move_or_image.height * rescaling_factor_tip);
    move_or_tip_texture = LoadTextureFromImage(move_or_image);
    
    // Libera a memória RAM das imagens intermediárias após o upload para a placa de vídeo
    UnloadImage(arrow_image);
    UnloadImage(action_image); 
    UnloadImage(move_image);
    UnloadImage(move_or_image);

    // Inicializa o conjunto de sprites direcionais do personagem estagiário
    initialize_estagiario_1_textures(estagiario_texture);

    // Define o ponto inicial de surgimento (spawn) do jogador no centro da tela
    current_estagiario_position = (Vector2){middle_screen_x, middle_screen_y};
    current_estagiario_texture_direction = estagiario_texture[SOUTH]; // Inicia olhando para o sul (frente)

    // Define a área delimitadora de colisão do cenário/escritório centralizado na tela
    map_collision_rec = (Rectangle){
        middle_screen_x - (in_game_office.width / 2), middle_screen_y - (in_game_office.height / 2),
        in_game_office.width, in_game_office.height
    };

    // Obtém o retângulo do computador e alinha a caixa de colisão inicial do jogador
    estagiario_computer_collision_rectangle = get_computer_collision_rectangle();
    update_collision_rec();
}

/**
 * @brief Desenha os textos do cartão do dia centralizados na tela, aplicando a cor de fade atual.
 */
void render_calendar_day_card(void)
{
    // Desenha o Título ("Dia 1")
    DrawTextEx(default_game_font, current_card_title,
               (Vector2){middle_screen_x - (calendar_card_title_text_size.x / 2), middle_screen_y - (calendar_card_subtitle_text_size_line_1.y * 5)},
               TITLE_FONT_SIZE, DEFAULT_LETTER_SPACING, color_to_fade);

    // Desenha a primeira linha do subtítulo narrativa
    DrawTextEx(default_game_font, current_card_subtitle_line_1, (Vector2){middle_screen_x - (calendar_card_subtitle_text_size_line_1.x / 2), middle_screen_y - (calendar_card_subtitle_text_size_line_1.y / 2)},
               SUBTITLE_FONT_SIZE, DEFAULT_LETTER_SPACING, color_to_fade);

    // Desenha a segunda linha do subtítulo narrativa
    DrawTextEx(default_game_font, current_card_subtitle_line_2, (Vector2){middle_screen_x - (calendar_card_subtitle_text_size_line_2.x / 2), middle_screen_y + (calendar_card_subtitle_text_size_line_1.y / 2)},
               SUBTITLE_FONT_SIZE, DEFAULT_LETTER_SPACING, color_to_fade);
}

/**
 * @brief Gerencia a máquina de estados do cartão do dia, controlando o aparecimento (fade in),
 * o tempo de leitura visível e o desaparecimento (fade out).
 */
void handle_calendar_day_card(void)
{
    bool fading_in_finished = false;
    static double timer = DID_NOT_START;
    double amount_of_seconds_to_wait = 5.0; // Tempo em segundos que a mensagem permanece visível na tela

    switch (current_calendar_day_state)
    {
    case NONE:
        // Transiciona para o estado de entrada (surgimento gradual)
        current_calendar_day_state = FADING_IN;
        break;

    case FADING_IN:
        // Aumenta gradualmente a opacidade (Alpha) de color_to_fade
        fade_in_texture(MEDIUM_FADE_SPEED, &color_to_fade, &fading_in_finished);
        render_calendar_day_card();

        // Quando atingir 100% de opacidade, passa para o estado de espera
        if (fading_in_finished)
        {
            current_calendar_day_state = AWAIT;
        }
        break;

    case AWAIT:
        render_calendar_day_card();

        // Marca o tempo exato de início da espera
        if (timer == DID_NOT_START)
        {
            timer = GetTime();
        }

        // Verifica se o tempo decorrido desde o início superou a meta estabelecida
        if (GetTime() - timer > amount_of_seconds_to_wait)
        {
            timer = DID_NOT_START; // Reseta o temporizador
            current_calendar_day_state = FADING_OUT; // Avança para o desaparecimento
        }
        break;

    case FADING_OUT:
        // Reduz a opacidade até 0 e ativa render_calendar_day_card_finished quando concluído
        fade_out_texture(DEFAULT_FADE_SPEED, &color_to_fade, &render_calendar_day_card_finished);
        render_calendar_day_card();
        break;

    default:
        // Estado inválido do cartão do dia: nada a desenhar
        break;
    }
}

/**
 * @brief Descarrega da memória GPU os ativos visuais exclusivos da tela do cartão narrativo.
 */
void unload_calendar_day_textures(void)
{
    UnloadTexture(second_background_texture);
}

/**
 * @brief Verifica interações do jogador com objetos do cenário (computador).
 * 
 * Checa a sobreposição entre a caixa de colisão do jogador e a área do computador.
 * Caso esteja próximo e a tecla ESPAÇO seja pressionada, solicita a transição para a tela do computador.
 */
void handle_map_interactables(void)
{
    // Checa colisão AABB (Axis-Aligned Bounding Box) entre o jogador e o computador
    is_near_computer = CheckCollisionRecs(estagiario_collision_rec, estagiario_computer_collision_rectangle);

    if (is_near_computer)
    {
        // Se estiver ao alcance e pressionar ESPAÇO, ativa a flag para abrir o computador
        if (IsKeyPressed(KEY_SPACE))
        {
            enter_computer_screen = true;
        }
    }
}

/**
 * @brief Controla e limita a movimentação do jogador para mantê-lo estritamente dentro das barreiras do mapa.
 * 
 * Esta função utiliza colisão AABB. Se a hitbox do jogador ultrapassar qualquer uma das bordas
 * do mapa (esquerda, direita, cima, baixo), a posição do personagem é imediatamente corrigida/travada.
 */
void handle_map_collisions(void)
{
    float scale = get_scale();
    float offset_x = HITBOX_OFFSET_X_AT_REFERENCE * scale;
    float offset_y = HITBOX_OFFSET_Y_AT_REFERENCE * scale;

    // Garante que a hitbox esteja na posição atual antes de checar as restrições
    update_collision_rec();

    if (CheckCollisionRecs(estagiario_collision_rec, map_collision_rec))
    {
        // Trava a posição caso tente ultrapassar a borda esquerda do mapa
        if (estagiario_collision_rec.x < map_collision_rec.x)
        {
            current_estagiario_position.x = map_collision_rec.x - offset_x;
        }
        // Trava a posição caso tente ultrapassar a borda direita do mapa
        else if (estagiario_collision_rec.x + estagiario_collision_rec.width > map_collision_rec.x + map_collision_rec.width)
        {
            current_estagiario_position.x = map_collision_rec.x + map_collision_rec.width - estagiario_collision_rec.width - offset_x;
        }

        // Trava a posição caso tente ultrapassar a borda superior (topo) do mapa
        if (estagiario_collision_rec.y < map_collision_rec.y)
        {
            current_estagiario_position.y = map_collision_rec.y - offset_y;
        }
        // Trava a posição caso tente ultrapassar a borda inferior (base) do mapa
        else if (estagiario_collision_rec.y + estagiario_collision_rec.height > map_collision_rec.y + map_collision_rec.height)
        {
            current_estagiario_position.y = map_collision_rec.y + map_collision_rec.height - estagiario_collision_rec.height - offset_y;
        }

        // Recalcula o retângulo final com as coordenadas ajustadas e corrigidas
        update_collision_rec();
    }
}

/**
 * @brief Captura os comandos do teclado (WASD / Setas), atualiza a velocidade, direção do sprite e
 * posição do jogador no espaço 2D.
 * 
 * Suporta movimentação em 8 direções. Aplica normalização escalar (0.7071 ≈ 1/√2) para evitar que
 * a movimentação na diagonal seja mais rápida do que a movimentação em eixos únicos.
 */
void handle_player_input(void)
{
    float speed = DEFAULT_PLAYER_MOVEMENT_SPEED * get_scale();

    // Modificadores vetoriais de direção (-1, 0, 1)
    int dx = 0;
    int dy = 0;

    // F1 liga e desliga o modo debug das hitboxes
    if (IsKeyPressed(KEY_F1)) show_debug_hitboxes = !show_debug_hitboxes;

    // Leitura das teclas de navegação (Suporta tanto WASD quanto as Setas Direcionais)
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    dy -= 1;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  dy += 1;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  dx -= 1;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) dx += 1;

    // Se nenhuma tecla direcional estiver pressionada, interrompe o processamento mantendo a posição
    if (dx == 0 && dy == 0)
    {
        update_collision_rec();
        return;
    }

    // Seleção da textura direcional correta com base no vetor combinado (dx, dy)
    if      (dx == 0 && dy < 0) current_estagiario_texture_direction = estagiario_texture[NORTH];
    else if (dx == 0 && dy > 0) current_estagiario_texture_direction = estagiario_texture[SOUTH];
    else if (dx < 0  && dy == 0) current_estagiario_texture_direction = estagiario_texture[WEST];
    else if (dx > 0  && dy == 0) current_estagiario_texture_direction = estagiario_texture[EAST];
    else if (dx < 0  && dy < 0) current_estagiario_texture_direction = estagiario_texture[NORTHWEST];
    else if (dx > 0  && dy < 0) current_estagiario_texture_direction = estagiario_texture[NORTHEAST];
    else if (dx < 0  && dy > 0) current_estagiario_texture_direction = estagiario_texture[SOUTHWEST];
    else                        current_estagiario_texture_direction = estagiario_texture[SOUTHEAST];

    // Correção de velocidade diagonal (Evita o efeito de ser mais rápido na diagonal devido ao Teorema de Pitágoras)
    if (dx != 0 && dy != 0)
    {
        speed *= 0.7071f;
    }

    // Move um eixo por vez: se bater num obstáculo, desfaz só aquele eixo e o jogador desliza na parede
    current_estagiario_position.x += dx * speed;
    update_collision_rec();
    if (collides_with_any(estagiario_collision_rec, office_obstacles, OBSTACLE_COUNT))
    {
        current_estagiario_position.x -= dx * speed;
    }

    current_estagiario_position.y += dy * speed;
    update_collision_rec();
    if (collides_with_any(estagiario_collision_rec, office_obstacles, OBSTACLE_COUNT))
    {
        current_estagiario_position.y -= dy * speed;
    }

    update_collision_rec();
}

/**
 * @brief Desenha o sprite do jogador na tela utilizando as dimensões e rotação adequadas.
 */
void render_player(void)
{
    float size = get_player_size();

    // Retângulo de origem recortando toda a textura direcional atual
    Rectangle source = {0, 0,
                        (float)current_estagiario_texture_direction.width,
                        (float)current_estagiario_texture_direction.height};
    
    // Retângulo de destino definindo onde e com qual tamanho o sprite será desenhado no mundo
    Rectangle dest = {current_estagiario_position.x,
                      current_estagiario_position.y,
                      size, size};

    // Desenha a textura na tela com suporte a tint de opacidade (color_to_fade)
    DrawTexturePro(current_estagiario_texture_direction, source, dest,
                   (Vector2){0, 0}, 0.0f, color_to_fade);
}

/**
 * @brief Desenha o contorno das hitboxes de colisão quando o modo debug (F1) está ligado.
 */
void render_debug_hitboxes(void)
{
    if (!show_debug_hitboxes) return;

    DrawRectangleLinesEx(map_collision_rec, 2.0f, RED);                           // Limite do mapa
    DrawRectangleLinesEx(estagiario_computer_collision_rectangle, 2.0f, YELLOW);  // Zona do computador
    DrawRectangleLinesEx(estagiario_collision_rec, 2.0f, GREEN);                  // Jogador

    for (int i = 0; i < OBSTACLE_COUNT; i++)
    {
        DrawRectangleLinesEx(office_obstacles[i], 2.0f, ORANGE);                  // Obstáculos
    }
}

/**
 * @brief Renderiza o cenário/fundo do escritório centralizado na janela do jogo.
 */
void render_game_loop_escritorio(void)
{
    float scale = get_scale();
    float office_width = in_game_office.width * scale;
    float office_height = in_game_office.height * scale;

    Rectangle source = {0, 0, (float)in_game_office.width, (float)in_game_office.height};
    Rectangle dest = {(GetScreenWidth() - office_width) / 2.0f,
                      (GetScreenHeight() - office_height) / 2.0f,
                      office_width, office_height};

    DrawTexturePro(in_game_office, source, dest, (Vector2){0, 0}, 0.0f, color_to_fade);
}

/**
 * @brief Renderiza os ícones de dica de controle (WASD, Setas, Espaço) de forma alinhada e
 * centralizada no espaço livre da margem inferior da tela (abaixo do mapa do escritório).
 */
void render_control_tips(void)
{
    float scale = get_scale();

    // Calcula a margem inferior do mapa em relação à janela do jogo
    float office_height = in_game_office.height * scale;
    float office_top_y = (GetScreenHeight() - office_height) / 2.0f;
    float office_bottom_y = office_top_y + office_height;

    // Espaço vertical restante abaixo do escritório
    float available_space_y = GetScreenHeight() - office_bottom_y;

    // Fator de escala menor para os ícones caberem harmonicamente no rodapé
    float tip_scale = scale * 0.40f;

    // Dimensões escaladas das três texturas de dicas
    float move_w = move_tip_texture.width * tip_scale;
    float move_h = move_tip_texture.height * tip_scale;

    float move_or_w = move_or_tip_texture.width * tip_scale;
    float move_or_h = move_or_tip_texture.height * tip_scale;

    float action_w = action_tip_texture.width * tip_scale;
    float action_h = action_tip_texture.height * tip_scale;

    float gap = 20.0f * scale; // Espaçamento horizontal entre cada dica

    // Cálculo da largura total necessária para centralizar o conjunto de dicas horizontalmente
    float total_width = move_w + gap + move_or_w + gap + action_w;
    float start_x = (GetScreenWidth() - total_width) / 2.0f;

    // Linha central vertical da margem inferior
    float center_y = office_bottom_y + (available_space_y / 2.0f);

    // 1. Dica de Movimento Principal (WASD)
    Rectangle move_src = {0, 0, (float)move_tip_texture.width, (float)move_tip_texture.height};
    Rectangle move_dest = {start_x, center_y - (move_h / 2.0f), move_w, move_h};
    DrawTexturePro(move_tip_texture, move_src, move_dest, (Vector2){0, 0}, 0.0f, color_to_fade);

    // 2. Dica de Movimento Secundário (Setas / OU)
    float move_or_x = start_x + move_w + gap;
    Rectangle move_or_src = {0, 0, (float)move_or_tip_texture.width, (float)move_or_tip_texture.height};
    Rectangle move_or_dest = {move_or_x, center_y - (move_or_h / 2.0f), move_or_w, move_or_h};
    DrawTexturePro(move_or_tip_texture, move_or_src, move_or_dest, (Vector2){0, 0}, 0.0f, color_to_fade);

    // 3. Dica de Ação de Interação (Espaço)
    float action_x = move_or_x + move_or_w + gap;
    Rectangle action_src = {0, 0, (float)action_tip_texture.width, (float)action_tip_texture.height};
    Rectangle action_dest = {action_x, center_y - (action_h / 2.0f), action_w, action_h};
    DrawTexturePro(action_tip_texture, action_src, action_dest, (Vector2){0, 0}, 0.0f, color_to_fade);
}

/**
 * @brief Desenha a seta amarela indicativa flutuando acima do computador.
 * 
 * Utiliza a função matemática harmônica `sinf()` multiplicada pelo tempo do jogo (`GetTime()`)
 * para criar um efeito senoidal fluido de oscilação ("quicar"/bounce para cima e para baixo).
 */
void render_interact_arrow(void)
{
    float scale = get_scale();
    
    // Cálculo do desvio de flutuação vertical usando onda senoidal
    float bounce_offset = sinf(GetTime() * 6.0f) * (10.0f * scale);

    float arrow_width  = arrow_down_texture.width  * scale;
    float arrow_height = arrow_down_texture.height * scale;

    // Posição centralizada horizontalmente no topo da caixa de colisão do computador
    float arrow_x = estagiario_computer_collision_rectangle.x
                  + estagiario_computer_collision_rectangle.width / 2.0f
                  - arrow_width / 2.0f;
    
    // Posição vertical logo acima do computador + offset animado da onda senoidal
    float arrow_y = estagiario_computer_collision_rectangle.y
                  - arrow_height - (10.0f * scale) + bounce_offset;

    Rectangle source = {0, 0, (float)arrow_down_texture.width, (float)arrow_down_texture.height};
    Rectangle dest   = {arrow_x, arrow_y, arrow_width, arrow_height};

    DrawTexturePro(arrow_down_texture, source, dest, (Vector2){0, 0}, 0.0f, color_to_fade);
}

/**
 * @brief Descarrega da memória de vídeo (GPU) todas as texturas pertencentes ao ambiente de jogo do escritório.
 */
void unload_office(void)
{
    UnloadTexture(current_estagiario_texture_direction);
    UnloadTexture(in_game_office);
    UnloadTexture(arrow_down_texture); // Liberação da textura da seta flutuante

    UnloadTexture(action_tip_texture);
    UnloadTexture(move_tip_texture);
    UnloadTexture(move_or_tip_texture);

    // Iteração para liberar cada um dos sprites direcionais do jogador
    for (int i = 0; i < DIRECTION_COUNT; i++)
    {
        UnloadTexture(estagiario_texture[i]);
    }
}

/**
 * @brief Gerencia e executa a máquina de estados principal que controla qual tela do jogo
 * deve ser atualizada e renderizada a cada quadro (frame).
 */
void handle_game_loop_screens(void)
{
    switch (current_screen)
    {
    case SCREEN_NONE:
        // Transição inicial automática para o cartão narrativo do dia
        current_screen = CALENDAR_DAY_CARD_SCREEN;
        break;

    case CALENDAR_DAY_CARD_SCREEN:
        // Executa a lógica e renderização da apresentação do dia
        handle_calendar_day_card();

        // Quando a animação do cartão for totalmente concluída, descarrega a tela e passa para a fase in-game
        if (render_calendar_day_card_finished)
        {
            unload_calendar_day_textures();
            current_screen = IN_GAME_SCREEN;
        }
        break;

    case IN_GAME_SCREEN:
        // Mantém a área de colisão do computador atualizada caso haja redimensionamento de janela
        estagiario_computer_collision_rectangle = get_computer_collision_rectangle();
        update_obstacle_recs();

        // Processa lógica de comandos do jogador, interações e colisões com o mapa
        handle_player_input();
        handle_map_interactables();
        handle_map_collisions();

        // ORDEM DE RENDERIZAÇÃO (Layering / Z-Index):
        // 1º O mapa de fundo (Escritório)
        // 2º O sprite do Jogador
        // 3º As dicas de controle no rodapé
        // 4º A seta indicativa por cima do cenário
        render_game_loop_escritorio();
        render_player();
        render_control_tips();
        render_interact_arrow();
        render_debug_hitboxes();

        // Se a interação com o computador foi acionada, realiza a transição por Fade Out
        if (enter_computer_screen)
        {   
            static bool animation_finished = false;
            fade_out_texture(MEDIUM_FADE_SPEED, &color_to_fade, &animation_finished);

            // Quando terminar o fade out, descarrega o escritório e abre o loop do computador
            if (animation_finished)
            {
                unload_office();
                initialize_computer_loop();
                current_screen = COMPUTER_SCREEN;
                enter_computer_screen = false;
            }
        }
        else
        {
            // Aplica efeito de Fade In ao entrar no jogo
            bool trash;
            fade_in_texture(FAST_FADE_SPEED, &color_to_fade, &trash);
        }
        break;

    case COMPUTER_SCREEN:
        // Delega a atualização e desenho para a sub-rotina/módulo do computador
        handle_computer_loop();
        break;

    default:
        break;
    }
}

/**
 * @brief Função principal do quadro de renderização. Delimita o ciclo de desenho da Raylib
 * (BeginDrawing / EndDrawing) e limpa o fundo a cada frame.
 */
void render_game_loop(void)
{
    BeginDrawing();
    ClearBackground(BG_COLOR); // Limpa o buffer de cor com a cor padrão do jogo

    handle_game_loop_screens(); // Processa a tela ativa

    EndDrawing(); // Finaliza a renderização do frame e envia para o monitor (Swap Buffers)
}

/**
 * @brief Função global de encerramento do game loop.
 * Garante a liberação de todos os recursos de memória para evitar Memory Leaks ao fechar o jogo.
 */
void unload_game_loop(void)
{
    // Limpeza de segurança das texturas do escritório e elementos gráficos
    UnloadTexture(in_game_office);
    UnloadTexture(arrow_down_texture);

    UnloadTexture(action_tip_texture);
    UnloadTexture(move_tip_texture);
    UnloadTexture(move_or_tip_texture);

    // Garante o descarregamento das texturas direcionais do jogador
    for (int i = 0; i < DIRECTION_COUNT; i++)
    {
        UnloadTexture(estagiario_texture[i]);
    }
}