#include <time.h>
#include <math.h> // Necessário para operações matemáticas avançadas, como a função sin() usada na animação do indicador
#include <stdio.h>

#include "raylib.h"
#include "rlgl.h"
#include "game_loop.h"
#include "game_state.h"
#include "macros.h"
#include "fonts.h"
#include "animations.h"
#include "characters.h"
#include "npcs.h"
#include "campaign_scenes.h"
#include "collisions.h"
#include "computer_loop.h"
#include "helper.h"
#include "hints.h"
#include "accessibility.h"
#include "calendar.h"
#include "audio.h"
#include "pause_menu.h"
#include "symptoms.h"
#include "office_symptoms.h"
#include "notice.h"

// Valor de sentinela para indicar que o temporizador do cartão de introdução ainda não começou a contar
#define DID_NOT_START -30

// ============================================================================
// IDENTIFICADORES DE TELA DO GAME LOOP (MÁQUINA DE ESTADOS PRINCIPAL)
// ============================================================================
#define SCREEN_NONE 0                  // Estado inicial ou nulo antes da definição da tela
#define CALENDAR_DAY_CARD_SCREEN 1     // Tela do cartão narrativo do dia ("Dia 1...")
#define IN_GAME_SCREEN 2               // Tela do jogo onde o jogador anda pelo escritório
#define COMPUTER_SCREEN 3              // Tela interativa do computador de trabalho
#define DAY_END_CARD_SCREEN 4          // Cartão de fim do dia, depois de sair pela porta
#define DAY_TRANSITION_SCREEN 5        // Flash do dia e salto puro, até o próximo dia jogável
#define ENDING_CARD_SCREEN 6           // Cartão do final, depois do Dia 100

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
#define REFERENCE_MIN_WIDTH 1200.0f            // Largura em que o mapa (1062 de largura) ainda cabe com folga
#define PLAYER_SIZE_AT_REFERENCE 75.0f         // Largura e altura base do sprite do jogador em pixels
#define COMPUTER_OFFSET_X_AT_REFERENCE 170.0f  // Deslocamento X do computador a partir do centro da tela
#define COMPUTER_OFFSET_Y_AT_REFERENCE 155.0f   // Deslocamento Y do computador a partir do centro da tela
#define COMPUTER_SIZE_AT_REFERENCE 100.0f      // Dimensão base do retângulo de interação do computador
#define COMPUTER_SCENE_ZOOM 1.25f              // Aproximação do escritório atrás do monitor, além do necessário para cobrir a janela
#define COMPUTER_SCENE_DIM_ALPHA 120           // Escurecimento do escritório atrás do monitor (0 a 255)
#define HIGH_CONTRAST_LINE_AT_REFERENCE 4.0f   // Espessura dos contornos do alto contraste
#define CARD_TEXT_MAX_WIDTH_RATIO 0.92f        // Largura máxima do texto do cartão do dia (largura da janela)
#define DAY_COUNTER_FONT_AT_REFERENCE 44.0f    // Tamanho do contador de dias, na margem de cima do mapa

// Tempos da passagem entre os dias, em segundos (GDD 2.7).
#define FLASH_MIN_TIME 3.0f                    // O flash fica na tela ao menos isso antes de aceitar o toque
#define JUMP_BLOCK_TIME 0.8f                   // Um bloco de salto puro inteiro passa em menos de 1s
#define TRANSITION_TEXT_WIDTH_RATIO 0.88f      // Largura máxima da frase do flash (largura da janela)
#define FLASH_DIM_ALPHA 190                    // Escurecimento da imagem atrás da frase do flash

// Sintoma visual do avanço da campanha: o escritório escurece aos poucos (GDD 6.1).
#define OFFICE_MAX_DARKENING 150.0f

// ============================================================================
// DIMENSÕES E OFFSETS DA HITBOX (CAIXA DE COLISÃO DO JOGADOR)
// A colisão ocorre só nos pés do personagem, para ele poder passar atrás dos móveis (a cabeça fica por cima)
// ============================================================================
#define HITBOX_OFFSET_X_AT_REFERENCE 19.5f  // Distância do topo-esquerdo do sprite até o início da hitbox (X): centrada, como o corpo
#define HITBOX_OFFSET_Y_AT_REFERENCE 48.0f  // Distância do topo-esquerdo do sprite até o início da hitbox (Y)
#define HITBOX_WIDTH_AT_REFERENCE    36.0f  // Largura efetiva da área de colisão do jogador (cabe nos corredores estreitos entre móveis)
#define HITBOX_HEIGHT_AT_REFERENCE   24.0f  // Altura da área de colisão do jogador (só os pés)
#define PLAYER_SPAWN_OFFSET_X_AT_REFERENCE -37.0f  // Deslocamento X do spawn a partir do centro da tela
#define PLAYER_SPAWN_OFFSET_Y_AT_REFERENCE 208.0f  // Deslocamento Y do spawn (logo acima da porta de entrada)

// Zona da porta de vidro, por onde o jogador encerra o dia: do começo da porta até a soleira, a faixa inteira por onde ele consegue passar
static const Rectangle EXIT_ZONE_REFERENCE = {-91.0f, 282.0f, 184.0f, 86.0f};

// ============================================================================
// VARIÁVEIS ESTÁTICAS DE CONTROLE E ESTADO
// ============================================================================
static int current_calendar_day_state = NONE;             // Estado atual da animação da tela do cartão narrativo
static bool game_loop_loaded = false;                    // Indica se as texturas da partida estão carregadas
static bool computer_loop_loaded = false;                // Indica se as texturas da tela do computador estão carregadas
static bool render_calendar_day_card_finished = false;   // Indica se toda a sequência do cartão narrativo terminou
static int current_screen = SCREEN_NONE;                  // Armazena qual tela do jogo está sendo processada no momento
static bool game_loop_finished = false;                   // Indica que o dia terminou e a partida volta ao menu
static bool game_loop_quit_requested = false;              // O jogador escolheu SAIR DO JOGO na pausa; a máquina de estados fecha o jogo
static bool leave_office = false;                        // Flag acionada ao interagir com a porta depois de concluir a tarefa
static bool is_near_exit = false;                        // Flag que indica se o jogador alcança a porta de saída
static bool enter_computer_screen = false;               // Flag acionada ao interagir com o computador para mudar de tela
static bool is_near_hint = false;                         // Flag que indica se o jogador alcança o bilhete do mural
static bool is_near_computer = false;                     // Flag que indica se o jogador está sobre a zona de interação do computador
static int nearby_npc_id = -1;
static bool resume_computer_checkpoint = false;
enum { DAY3_ARRIVAL = 8, DAY3_TRIAGE = 16, DAY3_END = 32, DAY3_LUNCH = 64 };
enum {
    CAMPAIGN_ARRIVAL = 128 + STORY_ARRIVAL,
    CAMPAIGN_MIDDLE = 128 + STORY_MIDDLE,
    CAMPAIGN_FINISH = 128 + STORY_FINISH
};
static const CampaignSceneDay *campaign_scene_day;
static int story_scene = 0; // bits 1/2/4 pertencem ao Dia 1
static HiddenStatisticBands story_bands;
static int story_line = 0;
static bool story_return_computer = false;
static bool story_input_guard = false;
static float story_action_time = 0.0f;
static bool story_approaching = false;
#define STORY_APPROACH_LIMIT 6.0f
#define STORY_SECOND_ACTOR_DELAY 0.4f
static const char *story_text(void);
static void begin_story_scene(int scene, bool return_computer);
static bool campaign_scene_completed(StoryPhase phase);
static int active_npc_id = -1;
static int active_npc_dialogue_line = 0;
static bool active_npc_first_today = false;               // A conversa em curso começou com o cumprimento do dia
static const char *active_npc_variant = NULL;
static bool npc_greeted_today[NPC_COUNT];                 // Quem já cumprimentou o jogador no dia atual
static float npc_typed_characters = 0.0f;                 // Quantos caracteres da fala já foram "digitados" na tela
static float npc_balloon_open = 0.0f;                     // 0 = balão fechado, 1 = aberto (anima a abertura e o fechamento)
static int npc_balloon_npc = -1;                          // Quem fala no balão, mesmo durante o fechamento
static float skip_guard_time = 0.0f;                      // Segundos desde que o cartão do dia abriu: evita pular sem querer

#define NPC_TYPE_SPEED 48.0f                              // Caracteres por segundo na fala dos NPCs
#define NPC_BALLOON_SPEED 7.0f                            // Velocidade da abertura do balão (1/s)
#define CARD_SKIP_GUARD_TIME 0.4f                         // O pulo só vale depois disso, para o toque que abriu o cartão não pular

// Texturas gerais de interface e mapas
static Color color_to_fade = {255, 255, 255, 0};          // Cor base de renderização (RGBA) usada para aplicar efeito de opacidade/fade

// Textos da tela de transição
static const char *current_card_title;
static const char *current_card_subtitle_line_1;
static const char *current_card_subtitle_line_2;
static char card_title_text[32];                // Título do cartão com o número do dia

// Passagem entre os dias
static float transition_time = 0.0f;            // Segundos no dia de flash ou de salto atual
static int jump_block_total_days = 0;             // Tamanho do bloco de salto puro atual, para a trilha perder uma fatia por dia
static float jump_day_time = 0.0f;              // Quanto dura cada dia do bloco de salto atual

#define DAY_END_CARD_FIRST_LINE_1 "Você encerra seu primeiro expediente e deixa o escritório."
#define DAY_END_CARD_LINE_1 "Mais um expediente encerrado."
#define DAY_END_CARD_DELEGATED_LINE_1 "A IARA fechou seus chamados em segundos. Você só assinou embaixo."
#define DAY_END_CARD_FAILSAFE_LINE_1 "O expediente acaba. A IARA conclui o que faltava, em silêncio."

// Os 4 finais (GDD 6.5). O GDD define as condições; os textos são proposta.
typedef struct {
    const char *title;
    const char *line_1;
    const char *line_2;
} EndingCard;

static const EndingCard ending_cards[] = {
    [ENDING_UNHARMED] = {"Final A: Ileso", "Você é efetivado(a). Nunca viu nada de estranho, e nada viu você.", "Na sexta tem happy hour. A água nunca chegou à sua mesa."},
    [ENDING_PAMONHA] = {"Final B: Pamonha", "A IARA fez tudo, e você assinou embaixo. Promoção garantida.", "Sua mesa nova fica lá embaixo. É bem tranquilo."},
    [ENDING_HYPNOTIZED] = {"Final C: Hipnotizado", "Você ouviu o canto, foi atrás dele e chegou sozinho(a).", "O seu crachá abre uma última porta, no porão."},
    [ENDING_RUPTURE] = {"Final D: Ruptura", "A prova fecha, e a versão da IARA não se sustenta.", "As portas destravam. Ao longe, a água volta a correr."}};

// Texturas do ambiente de jogo (Escritório e Dicas de Teclado)
static const char *const campaign_art_paths[] = {
    CAMPAIGN_ART_PATH "flash.png", CAMPAIGN_ART_PATH "salto.png",
    CAMPAIGN_ART_PATH "final-ileso.png", CAMPAIGN_ART_PATH "final-pamonha.png",
    CAMPAIGN_ART_PATH "final-hipnotizado.png", CAMPAIGN_ART_PATH "final-ruptura.png"
};
enum { ART_FLASH, ART_JUMP, ART_FIRST_ENDING };
static Texture2D campaign_art[sizeof(campaign_art_paths) / sizeof(campaign_art_paths[0])];
static GameEnding shown_ending;
static bool replay_requested;

static Texture2D in_game_office;        // Mapa do escritório
static Texture2D chair_floor, player_chair, player_seat_frames[SEAT_PLAYER_FRAME_COUNT];
static enum {CHAIR_IDLE, CHAIR_APPROACH, CHAIR_SITTING, CHAIR_SEATED, CHAIR_LEAVING} player_seat;
static float player_seat_time;
static Texture2D action_tip_texture;    // Textura da dica visual de ação (Espaço)
static Texture2D move_tip_texture;      // Textura da dica visual de movimento (WASD)
static Texture2D move_or_tip_texture;   // Textura da dica visual alternativa de movimento (Setas)

// Estruturas do personagem e caixas de colisão
static char_textures estagiario_texture;                 // Array/Estrutura com as texturas de todas as direções do jogador
static Vector2 current_estagiario_position;               // Posição cartesiana (X, Y) do jogador na tela
static Texture2D current_estagiario_texture_direction;   // Textura atual baseada na direção para onde o jogador está olhando
static int current_direction;                            // Direção atual do jogador (EAST, NORTH...)
static Texture2D walk_textures[DIRECTION_COUNT][WALK_FRAME_COUNT]; // Frames do andar de cada direção
static bool walk_loaded[DIRECTION_COUNT];                 // true se os frames da direção foram carregados
static float walk_time = 0.0f;                           // Segundos andando sem parar (0 = parado)

static Rectangle map_collision_rec;                      // Limites físicos das paredes do mapa
static Vector2 last_screen_center;                        // Centro da tela no quadro anterior (detecta redimensionamento)
static float last_scale;                                  // Escala no quadro anterior
static Rectangle estagiario_computer_collision_rectangle; // Área de gatilho para interagir com o computador
static Rectangle estagiario_collision_rec;               // Hitbox atualizada dinamicamente do jogador
static Rectangle hint_item_rectangle;                     // Mural onde fica o bilhete com a dica
static bool show_debug_hitboxes = false;                 // Modo debug (F1): desenha as hitboxes de colisão

// Obstáculos medidos no mapa de referência (office.png), relativos ao centro da tela
// Linha inferior da porta no asset: y = 808 px (centro vertical do mapa = 438,5).
#define OFFICE_FRONT_BOUNDARY_Y 369.5f
#define OBSTACLE_COUNT 32  // Quantidade de obstáculos medidos; atualizar ao incluir mais
#define GLASS_ROOMS_OBSTACLE 7   // Índice das salas de vidro em office_obstacles
#define GLASS_DOOR_OBSTACLE 21   // Índice da porta de vidro em office_obstacles: a passagem atrás do vidro é livre, então o retângulo é {0}
static Rectangle office_obstacles_reference[OBSTACLE_COUNT] = {
    {-531.0f, -438.0f, 1062.0f, 114.0f},  // parede do topo (banheiros, cozinha, quadro, servidores)
    {-531.0f, -317.0f, 175.0f, 67.0f},    // banheiros, parte de baixo
    {-189.0f, -317.0f, 31.0f, 12.0f},     // lixeira
    {16.0f, -356.5f, 37.0f, 52.0f},       // planta embaixo do quadro
    {185.0f, -317.0f, 23.0f, 12.0f},      // bebedouro
    {364.0f, -317.0f, 167.0f, 94.0f},     // sala de servidores, parte de baixo
    {-531.0f, -242.0f, 140.0f, 106.0f},   // sala do mural
    {403.0f, -215.0f, 128.0f, 487.0f},    // salas de vidro da direita
    {-272.0f, -197.0f, 190.0f, 78.0f},    // mesas superior esquerda
    {63.0f, -197.0f, 198.0f, 78.0f},      // mesas superior direita
    {-28.0f, -171.0f, 48.0f, 89.0f},      // planta do meio, em cima
    {-473.0f, -128.0f, 81.0f, 86.0f},     // impressora
    {-531.0f, -65.0f, 43.0f, 63.0f},      // planta da esquerda, em cima
    {-276.0f, 59.0f, 198.0f, 82.0f},      // mesas inferior esquerda
    {63.0f, 59.0f, 198.0f, 78.0f},        // mesas inferior direita
    {-531.0f, 57.0f, 105.0f, 82.0f},      // estante de livros
    {-32.0f, 82.0f, 52.0f, 96.0f},       // planta do meio, embaixo
    {-531.0f, 167.0f, 39.0f, 78.0f},      // planta da esquerda, no meio
    {-422.0f, 253.0f, 66.0f, 43.0f},      // mesinha do sofá
    {475.0f, 247.0f, 50.0f, 60.0f},       // planta da direita, embaixo
    {-531.0f, 292.0f, 43.0f, 31.0f},      // planta da esquerda, embaixo
    {0},                              // porta: passagem atras do vidro
    {-175.0f, 307.5f, 84.0f, 54.0f},      // jardineira esquerda
    {93.0f, 307.5f, 84.0f, 54.0f},        // jardineira direita
    {-531.0f, OFFICE_FRONT_BOUNDARY_Y, 1062.0f, 68.0f}, // linha inferior da porta
    {0}, {0},                         // pilares: oclusao visual, passagem livre
    {-531.0f, 331.5f, 4.0f, 106.0f},    // lateral esquerda
    {524.0f, 331.5f, 7.0f, 106.0f},      // lateral direita
    {0}, {0}, {0}};                    // bases dos tres tiles de vidro
static Rectangle office_obstacles[OBSTACLE_COUNT];  // Mesmos obstáculos em coordenadas da tela

// Paineis inferiores: coordenadas do mesmo office.png, relativas ao centro.
#define GLASS_COUNT 3
static const collision_tile glass_reference[GLASS_COUNT] = {
    {{-527.0f, 331.5f, 210.0f, OFFICE_FRONT_BOUNDARY_Y - 331.5f + 4.0f}, TILE_GLASS, 4.0f},
    {{317.0f, 331.5f, 207.0f, OFFICE_FRONT_BOUNDARY_Y - 331.5f + 4.0f}, TILE_GLASS, 4.0f},
    {{-58.0f, 308.5f, 109.0f, OFFICE_FRONT_BOUNDARY_Y - 308.5f + 4.0f}, TILE_GLASS, 4.0f}
};
// Superficie visual independente; a base fisica nunca ultrapassa a linha da porta.
static const Rectangle glass_surface_reference[GLASS_COUNT] = {
    {-527.0f, 331.5f, 210.0f, 49.0f},
    {317.0f, 331.5f, 207.0f, 49.0f},
    {-58.0f, 308.5f, 109.0f, 58.0f}
};
static const Rectangle office_foreground_reference[] = {
    {-245.0f, 369.5f, 181.0f, 68.0f},  // parede esquerda: sem incluir o piso
    {58.0f, 369.5f, 185.0f, 68.0f},    // parede direita: sem incluir o piso
    {-64.0f, 369.5f, 122.0f, 68.0f},    // soleira e placa abaixo da porta
    {-314.0f, 324.5f, 69.0f, 113.0f},   // pilar esquerdo
    {243.0f, 324.5f, 74.0f, 113.0f},    // pilar direito
    {-7.0f, 296.5f, 10.0f, 73.0f}      // divisoria central da porta
};
static collision_tile office_glass[GLASS_COUNT];
static Rectangle office_glass_surface[GLASS_COUNT];
static RenderTexture2D glass_scene;
static Shader glass_shader;
static int glass_panel_loc, glass_size_loc;

// Cena intermediaria invertida em Y; o recorte usa coordenadas de tela.
static const char *glass_fragment_shader =
    "#version 330\n"
    "in vec2 fragTexCoord; in vec4 fragColor; out vec4 finalColor;\n"
    "uniform sampler2D texture0; uniform vec4 colDiffuse;\n"
    "uniform vec4 panel; uniform vec2 size;\n"
    "void main() {\n"
    " vec2 pixel = vec2(fragTexCoord.x, 1.0-fragTexCoord.y)*size;\n"
    " if(any(lessThan(pixel,panel.xy)) || any(greaterThanEqual(pixel,panel.xy+panel.zw))) discard;\n"
    " vec4 c = texture(texture0, fragTexCoord);\n"
    " c.rgb = mix(c.rgb,vec3(0.45,0.80,0.95),0.12);\n"
    " finalColor = c*colDiffuse*fragColor; }\n";

static void unload_glass(void)
{
    if (glass_scene.id) UnloadRenderTexture(glass_scene);
    if (glass_shader.id) UnloadShader(glass_shader);
    glass_scene = (RenderTexture2D){0};
    glass_shader = (Shader){0};
}

// ---------------------------------------------------------------------------
// FUNÇÕES DE REDIMENSIONAMENTO E ESCALA DINÂMICA
// ---------------------------------------------------------------------------

static Vector2 get_map_center(void);

/**
 * @brief Calcula o fator de escala dinâmico proporcional à resolução atual da tela.
 * 
 * Toma como base a altura de 1080p (REFERENCE_HEIGHT). Em janelas estreitas, a largura
 * passa a limitar a escala, para o mapa nunca sair da janela.
 * Exemplo: 
 *   - Em 1920x1080 -> Retorna 1.0f
 *   - Em 1280x720  -> Retorna 0.667f (720 / 1080)
 *   - Em 3840x2160 -> Retorna 2.0f (2160 / 1080)
 * 
 * @return float Fator multiplicador de escala.
 */
static float get_scale(void)
{
    float by_height = GetScreenHeight() / REFERENCE_HEIGHT;
    float by_width = GetScreenWidth() / REFERENCE_MIN_WIDTH;

    return (by_width < by_height) ? by_width : by_height;
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

// Cadeira à frente do computador, na fileira inferior direita do escritório.
static Vector2 player_chair_anchor(void)
{
    Vector2 center = get_map_center();
    float scale = get_scale();
    return (Vector2){center.x + 213.0f * scale, center.y + 196.5f * scale};
}

static Vector2 player_chair_entry(void)
{
    Vector2 anchor = player_chair_anchor();
    float pixel = get_player_size() / CHARACTER_SPRITE_SIZE;
    return (Vector2){anchor.x - 16 * pixel, anchor.y + (SEAT_ENTRY_OFFSET - 31) * pixel};
}

/** Área de interação com o computador, relativa ao centro do mapa. */
static Rectangle get_computer_collision_rectangle(void)
{
    Rectangle reference = {COMPUTER_OFFSET_X_AT_REFERENCE, COMPUTER_OFFSET_Y_AT_REFERENCE,
                           COMPUTER_SIZE_AT_REFERENCE, COMPUTER_SIZE_AT_REFERENCE};
    Vector2 screen_center = {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};

    return scale_reference_rect(reference, get_scale(), screen_center);
}

/**
 * @brief Prepara o cartão narrativo com os textos indicados, para aparecer do início.
 */
static void start_day_card(const char *title, const char *line_1, const char *line_2)
{
    current_card_title = title;
    current_card_subtitle_line_1 = line_1;
    current_card_subtitle_line_2 = line_2;
    current_calendar_day_state = NONE;
    render_calendar_day_card_finished = false;
    skip_guard_time = 0.0f;
}

/**
 * @brief Calcula a zona em frente à porta de vidro, em coordenadas da tela.
 */
static Rectangle get_exit_zone(void)
{
    Vector2 screen_center = {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};

    return scale_reference_rect(EXIT_ZONE_REFERENCE, get_scale(), screen_center);
}

// Porta de vidro: o retângulo só serve à seta e ao contorno, a passagem atrás do vidro é livre
static const Rectangle glass_door_reference = {-60.0f, 304.0f, 116.0f, 27.0f};

static Rectangle get_glass_door_rectangle(void)
{
    Vector2 screen_center = {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};

    return scale_reference_rect(glass_door_reference, get_scale(), screen_center);
}

/**
 * @brief Objeto indicado pela seta: primeiro o computador e, com a tarefa resolvida, a porta.
 */
static bool can_leave_today(void)
{
    return current_game_state_variables.is_resolved &&
        (!campaign_scene_day || campaign_scene_completed(STORY_FINISH));
}

static Rectangle get_next_objective_rectangle(void)
{
    return can_leave_today() ? get_glass_door_rectangle() : estagiario_computer_collision_rectangle;
}

/**
 * @brief Encerra o dia e mostra o cartão final, com o texto de como a tarefa foi resolvida.
 */
static void finish_day(void)
{
    int day = current_game_state_variables.current_day;
    const char *line_1 = (day == 1) ? DAY_END_CARD_FIRST_LINE_1 : DAY_END_CARD_LINE_1;

    if (computer_failsafe_triggered) line_1 = DAY_END_CARD_FAILSAFE_LINE_1;
    else if (current_game_state_variables.mode_type == MODE_AUTOMATIC) line_1 = DAY_END_CARD_DELEGATED_LINE_1;

    walk_time = 0.0f;
    color_to_fade.a = 0;
    if (day != TOTAL_GAME_DAYS) current_game_state_variables.resolved_puzzles[day] = true;
    snprintf(card_title_text, sizeof(card_title_text), "Fim do Dia %d", day);
    start_day_card(card_title_text, line_1, calendar_get_day_end_line(day));
    current_screen = DAY_END_CARD_SCREEN;
}

/**
 * @brief Coloca o jogador na porta de entrada, olhando para o escritório.
 */
static void place_player_at_entrance(void)
{
    current_estagiario_position = (Vector2){
        (GetScreenWidth() / 2.0f) + PLAYER_SPAWN_OFFSET_X_AT_REFERENCE * get_scale(),
        (GetScreenHeight() / 2.0f) + PLAYER_SPAWN_OFFSET_Y_AT_REFERENCE * get_scale()};
    current_direction = NORTH;
    current_estagiario_texture_direction = estagiario_texture[NORTH];
    player_seat = CHAIR_IDLE;
    player_seat_time = 0.0f;
    walk_time = 0.0f;
}

/**
 * @brief Abre um dia jogável: chamados do dia, bilhete do mural e cartão de chegada.
 */
static void open_playable_day(void)
{
    int day = current_game_state_variables.current_day;

    struct game_state_variables *state = &current_game_state_variables;
    campaign_scene_day = get_campaign_scene_day(day);
    if (day >= 41 && state->story_presence_day != day)
    {
        HiddenStatisticBands bands = hidden_statistic_bands(&current_hidden_statistic);
        if (bands.automation >= 2) state->story_absences |= STORY_ABSENT_JULIO;
        if (day >= 58 && bands.automation >= 3) state->story_absences |= STORY_ABSENT_ANDERSON;
        state->story_presence_day = day;
        computer_checkpoint_dirty = true;
    }
    npc_set_story_absences(state->story_absences);

    // Cada dia jogável traz os colegas em lugares novos, e todos cumprimentam de novo
    npc_arrange_for_day(day, get_scale(), get_map_center());
    story_scene = 0;
    if (day == 1)
    {
        if (current_game_state_variables.story_scenes & 1) npc_end_reception();
        else npc_stage_day_one(get_scale(), get_map_center());
    }
    for (int npc = 0; npc < NPC_COUNT; npc++) npc_greeted_today[npc] = false;
    active_npc_id = -1;
    active_npc_variant = NULL;
    npc_balloon_open = 0.0f;
    audio_set_game_tension((float)(day - 1) / 80.0f);   // Calmo nos primeiros dias; a tensão da trilha sobe com a história
    computer_set_hint_giver(npc_has_hint(npc_get_daily_hint_owner(day), day)
                                ? npc_get_definition(npc_get_daily_hint_owner(day))->name
                                : NULL);

    // Quem perdeu o bilhete do Dia 1 não fica sem a colinha nos puzzles seguintes (RF07)
    if (day > 1) current_game_state_variables.has_cheat_sheet = true;

    reset_computer_progress();
    bool restored = computer_restore_checkpoint();
    resume_computer_checkpoint = day != 1 && restored;
    if (!restored) state->computer_checkpoint[0] = '\0';
    set_hint_note_lines(calendar_get_note_lines(day), CALENDAR_NOTE_LINE_COUNT);
    color_to_fade.a = 0;
    snprintf(card_title_text, sizeof(card_title_text), "Dia %d", day);
    const char *first_line = calendar_get_arrival_line(day, 0);
    const char *second_line = calendar_get_arrival_line(day, 1);
    HiddenStatisticBands bands = hidden_statistic_bands(&current_hidden_statistic);
    if (day == 3 && bands.automation >= 1)
        first_line = "O Suporte está vazio. A luz da fila já está acesa no seu monitor.";
    if (day == 12 && bands.bond >= 1)
        second_line = "A IARA reescreveu as regras. No quadro, deixaram um recado oferecendo ajuda.";
    if (day == 74 && bands.perception == 3)
        second_line = "O corredor está molhado. O risco do mural reapareceu ao lado do seu teclado.";
    start_day_card(card_title_text, first_line, second_line);
    current_screen = SCREEN_NONE;
}

/**
 * @brief Entra no dia atual da campanha: dia jogável abre o escritório, os outros passam rápido.
 */
static void enter_current_day(void)
{
    int day = current_game_state_variables.current_day;

    current_game_state_variables.day_type = calendar_get_day_type(day);
    transition_time = 0.0f;

    if (current_game_state_variables.day_type == PLAYABLE_DAY)
    {
        jump_block_total_days = 0;
        audio_set_jump_progress(0.0f);
        open_playable_day();
        return;
    }

    if (current_game_state_variables.day_type == JUMP_DAY)
    {
        jump_day_time = JUMP_BLOCK_TIME / calendar_get_jump_block_length(day);

        int days_left = calendar_get_jump_block_length(day);
        if (days_left > jump_block_total_days) jump_block_total_days = days_left;
        audio_set_jump_progress((float)(jump_block_total_days - days_left) / (float)jump_block_total_days);
    }
    else
    {
        jump_block_total_days = 0;
        audio_set_jump_progress(0.0f);
    }

    current_screen = DAY_TRANSITION_SCREEN;
}

/**
 * @brief Passa para o dia seguinte da campanha. Num salto, mantém o ritmo do bloco.
 */
static void advance_to_next_day(void)
{
    float block_day_time = jump_day_time;
    bool was_jumping = current_game_state_variables.day_type == JUMP_DAY;

    start_game_day(current_game_state_variables.current_day + 1);
    enter_current_day();

    if (was_jumping && current_game_state_variables.day_type == JUMP_DAY) jump_day_time = block_day_time;
}

/**
 * @brief Mostra o final da partida, calculado das escolhas de toda a campanha.
 */
static void show_ending(void)
{
    shown_ending = get_game_ending();
    const EndingCard *card = &ending_cards[shown_ending];

    color_to_fade.a = 0;
    start_day_card(card->title, card->line_1, card->line_2);
    current_screen = ENDING_CARD_SCREEN;
}

/**
 * @brief Abre ou fecha o bilhete do mural. É nele que o jogador pega a colinha. Ler o bilhete antes
 * de concluir a tarefa conta como investigar; os pontos só entram quando a tarefa é concluída,
 * então ler de novo não soma nada.
 */
static void toggle_mural_note(void)
{
    toggle_hint_note();

    if (!is_hint_note_open()) return;

    int day = current_game_state_variables.current_day;

    current_game_state_variables.has_cheat_sheet = true;

    // O que o jogador examina vira evidência para a prova do Dia 100
    unsigned int evidence = day == 58 ? EVIDENCE_BADGE_AFTER_LEAVING :
                            day == 74 ? EVIDENCE_NO_HUMAN_ORDER : 0;
    if (evidence && !(current_game_state_variables.evidence_mask & evidence))
    {
        current_game_state_variables.evidence_mask |= evidence;
        notice_show("Registro guardado.", NOTICE_SUCCESS);
    }
    if (day == 41) current_game_state_variables.d41_mural_lido = true;
    if (day == 89 && !current_game_state_variables.found_nicollas_trace)
    {
        current_game_state_variables.found_nicollas_trace = true;
        notice_show("Mapa examinado.", NOTICE_INFO);
    }
    if (day >= 12) computer_checkpoint_dirty = true;

    if (!current_game_state_variables.is_resolved)
    {
        current_game_state_variables.user_investigated = true;
    }
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
 * @brief Mantém o jogador no mesmo ponto do mapa quando a janela muda de tamanho.
 */
static void keep_player_place_on_resize(void)
{
    Vector2 screen_center = {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
    float scale = get_scale();

    // Janela minimizada: altura 0 daria divisão por zero, então espera a janela voltar
    if (scale <= 0) return;

    if (screen_center.x != last_screen_center.x || screen_center.y != last_screen_center.y || scale != last_scale)
    {
        // Distância do jogador ao centro do mapa, reescalada, somada ao novo centro
        resize_npcs(last_scale, scale, last_screen_center, screen_center);
        current_estagiario_position.x = screen_center.x + (current_estagiario_position.x - last_screen_center.x) * scale / last_scale;
        current_estagiario_position.y = screen_center.y + (current_estagiario_position.y - last_screen_center.y) * scale / last_scale;
        last_screen_center = screen_center;
        last_scale = scale;
    }
}

/**
 * @brief Converte o limite do mapa e os obstáculos de referência para a tela atual (a janela pode mudar de tamanho).
 */
static void update_obstacle_recs(void)
{
    Vector2 screen_center = {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};

    // O mapa é desenhado com get_scale() centralizado, então o limite usa a mesma escala e centro
    Rectangle map_reference = {-in_game_office.width / 2.0f, -in_game_office.height / 2.0f,
                               (float)in_game_office.width, (float)in_game_office.height};
    map_collision_rec = scale_reference_rect(map_reference, get_scale(), screen_center);

    for (int i = 0; i < OBSTACLE_COUNT; i++)
    {
        office_obstacles[i] = scale_reference_rect(office_obstacles_reference[i], get_scale(), screen_center);
    }
    for (int i = 0; i < GLASS_COUNT; i++)
    {
        office_glass[i] = glass_reference[i];
        office_glass[i].bounds = scale_reference_rect(glass_reference[i].bounds, get_scale(), screen_center);
        office_glass[i].base_height *= get_scale();
        office_glass_surface[i] = scale_reference_rect(glass_surface_reference[i], get_scale(), screen_center);
        office_obstacles[OBSTACLE_COUNT - GLASS_COUNT + i] = tile_collision_rectangle(office_glass[i]);
    }
}

/**
 * @brief Centro atual do mapa na tela.
 */
static Vector2 get_map_center(void)
{
    return (Vector2){GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
}

/**
 * @brief Inicializa todas as variáveis, carrega imagens da memória de disco para a GPU,
 * redimensiona ativos visuais, calcula tamanhos de fontes e define os estados iniciais.
 * 
 * Deve ser chamada uma única vez antes de iniciar a execução das telas do jogo.
 */
void initialize_game_loop_variables(void)
{
    game_loop_loaded = true;

    // A partida pode recomeçar depois de voltar ao menu
    game_loop_finished = false;
    replay_requested = false;
    game_loop_quit_requested = false;
    leave_office = false;
    enter_computer_screen = false;
    nearby_npc_id = -1;
    active_npc_id = -1;
    active_npc_dialogue_line = 0;
    walk_time = 0.0f;
    color_to_fade.a = 0;

    unload_glass();
    glass_shader = LoadShaderFromMemory(NULL, glass_fragment_shader);
    glass_panel_loc = GetShaderLocation(glass_shader, "panel");
    glass_size_loc = GetShaderLocation(glass_shader, "size");

    // Carrega o mapa do escritório
    in_game_office = LoadTexture("code/assets/sprites/environment/office.png");
    chair_floor = LoadTexture(OFFICE_CHAIR_FLOOR_PATH);
    for (unsigned i = 0; i < sizeof(campaign_art) / sizeof(campaign_art[0]); i++)
        campaign_art[i] = LoadTexture(campaign_art_paths[i]);
    player_chair = LoadTexture(OFFICE_CHAIR_NORTH_PATH);
    for (int i = 0; i < SEAT_PLAYER_FRAME_COUNT; i++)
    {
        player_seat_frames[i] = LoadTexture(TextFormat(SEAT_PLAYER_FRAME_PATH, i));
        if (!player_seat_frames[i].id) break;
    }

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
    UnloadImage(action_image); 
    UnloadImage(move_image);
    UnloadImage(move_or_image);

    // Inicializa o conjunto de sprites direcionais do personagem estagiário
    initialize_estagiario_1_textures(estagiario_texture);
    for (int i = 0; i < DIRECTION_COUNT; i++)
    {
        walk_loaded[i] = load_walk_frames("Estagiarios", i, walk_textures[i]);
    }

    if (!initialize_npcs(get_scale(), get_map_center()))
    {
        TraceLog(LOG_ERROR, "Nao foi possivel carregar os sprites dos NPCs.");
    }

    // Define o ponto inicial de surgimento (spawn) do jogador, logo acima da porta de entrada
    place_player_at_entrance();

    // Guarda centro e escala iniciais para detectar redimensionamento depois
    last_screen_center = (Vector2){GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
    last_scale = get_scale();

    // Calcula o limite do mapa e os obstáculos já no início (depois são recalculados a cada quadro)
    update_obstacle_recs();

    // Obtém o retângulo do computador e alinha a caixa de colisão inicial do jogador
    estagiario_computer_collision_rectangle = get_computer_collision_rectangle();
    hint_item_rectangle = get_hint_item_rectangle(get_scale(), get_map_center());
    update_collision_rec();

    // Retoma de onde o save parou. Fechado no cartão de fim do dia, o dia já estava encerrado
    int day = current_game_state_variables.current_day;

    if (current_game_state_variables.resolved_puzzles[day] && day < TOTAL_GAME_DAYS) start_game_day(day + 1);

    enter_current_day();
}

/**
 * @brief Desenha os textos do cartão do dia centralizados na tela, aplicando a cor de fade atual.
 */
static void draw_campaign_art(Texture2D art, unsigned char alpha)
{
    if (!art.id) return;
    float scale = fmaxf((float)GetScreenWidth() / art.width, (float)GetScreenHeight() / art.height);
    float width = art.width * scale, height = art.height * scale;
    DrawTexturePro(art, (Rectangle){0, 0, (float)art.width, (float)art.height},
                   (Rectangle){(GetScreenWidth() - width) / 2, (GetScreenHeight() - height) / 2, width, height},
                   (Vector2){0}, 0, (Color){255, 255, 255, alpha});
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){8, 9, 16, (unsigned char)(alpha * 0.38f)});
}

void render_calendar_day_card(void)
{
    if (current_screen == ENDING_CARD_SCREEN)
        draw_campaign_art(campaign_art[ART_FIRST_ENDING + shown_ending], color_to_fade.a);
    float scale = get_ui_scale();
    float title_size = TITLE_FONT_SIZE * scale;
    float title_spacing = DEFAULT_LETTER_SPACING * scale;
    float subtitle_size = SUBTITLE_FONT_SIZE * scale;
    float subtitle_spacing = DEFAULT_LETTER_SPACING * scale;

    // As duas linhas do subtítulo usam a mesma fonte, reduzida até a maior caber na janela
    float max_width = GetScreenWidth() * CARD_TEXT_MAX_WIDTH_RATIO;
    float line_1_width = MeasureTextEx(default_game_font, current_card_subtitle_line_1, subtitle_size, subtitle_spacing).x;
    float line_2_width = MeasureTextEx(default_game_font, current_card_subtitle_line_2, subtitle_size, subtitle_spacing).x;
    float longest_width = (line_1_width > line_2_width) ? line_1_width : line_2_width;

    if (longest_width > max_width)
    {
        subtitle_size *= max_width / longest_width;
        subtitle_spacing *= max_width / longest_width;
    }

    Vector2 title_text_size = MeasureTextEx(default_game_font, current_card_title, title_size, title_spacing);
    Vector2 line_1_text_size = MeasureTextEx(default_game_font, current_card_subtitle_line_1, subtitle_size, subtitle_spacing);
    Vector2 line_2_text_size = MeasureTextEx(default_game_font, current_card_subtitle_line_2, subtitle_size, subtitle_spacing);
    float center_x = GetScreenWidth() / 2.0f;
    float center_y = GetScreenHeight() / 2.0f;

    // Desenha o Título ("Dia 1")
    DrawTextEx(default_game_font, current_card_title,
               (Vector2){center_x - (title_text_size.x / 2), center_y - (SUBTITLE_FONT_SIZE * scale * 5)},
               title_size, title_spacing, color_to_fade);

    // Desenha a primeira linha do subtítulo narrativa
    DrawTextEx(default_game_font, current_card_subtitle_line_1, (Vector2){center_x - (line_1_text_size.x / 2), center_y - (line_1_text_size.y / 2)},
               subtitle_size, subtitle_spacing, color_to_fade);

    // Desenha a segunda linha do subtítulo narrativa
    DrawTextEx(default_game_font, current_card_subtitle_line_2, (Vector2){center_x - (line_2_text_size.x / 2), center_y + (line_1_text_size.y / 2)},
               subtitle_size, subtitle_spacing, color_to_fade);
}

// Botão PULAR do cartão do dia: quem lê rápido não precisa esperar o tempo todo. ESPAÇO, ENTER ou um
// clique em qualquer lugar também pulam. O botão aparece de leve no canto de baixo.
static bool card_skip_requested(void)
{
    float scale = get_ui_scale();
    const char *label = "PULAR  [ESPAÇO]";
    float font_size = 22.0f * scale;
    float width = MeasureTextEx(default_game_font, label, font_size, 0.0f).x + (36.0f * scale);
    float height = font_size + (18.0f * scale);
    Rectangle button = {GetScreenWidth() - width - (28.0f * scale), GetScreenHeight() - height - (24.0f * scale), width, height};
    bool hovered = CheckCollisionPointRec(GetMousePosition(), button) && IsWindowFocused();
    unsigned char alpha = (unsigned char)(color_to_fade.a * 0.85f);

    DrawRectangleRounded(button, 0.35f, 6, (Color){18, 10, 20, (unsigned char)(alpha * 0.7f)});
    DrawRectangleRoundedLinesEx(button, 0.35f, 6, 2.0f, hovered ? (Color){255, 214, 70, alpha} : (Color){200, 190, 210, (unsigned char)(alpha * 0.8f)});
    DrawTextEx(default_game_font, label, (Vector2){button.x + (18.0f * scale), button.y + ((height - font_size) / 2.0f)},
               font_size, 0.0f, (Color){245, 240, 248, alpha});

    if (skip_guard_time < CARD_SKIP_GUARD_TIME) return false;
    return IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

/**
 * @brief Gerencia a máquina de estados do cartão do dia, controlando o aparecimento (fade in),
 * o tempo de leitura visível e o desaparecimento (fade out).
 */
static bool ending_action_requested(void)
{
    float scale = get_ui_scale();
    float width = 255 * scale, height = 44 * scale;
    float y = GetScreenHeight() * 0.76f;
    float center = GetScreenWidth() / 2.0f;
    Rectangle replay = {center - width - 8 * scale, y, width, height};
    Rectangle menu = {center + 8 * scale, y, width, height};
    const char *labels[] = {"JOGAR DE NOVO [R]", "MENU [ENTER]"};
    Rectangle buttons[] = {replay, menu};
    bool click = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    bool replay_clicked = false, menu_clicked = false;

    for (int i = 0; i < 2; i++)
    {
        Rectangle bounds = buttons[i];
        bool hovered = CheckCollisionPointRec(GetMousePosition(), bounds);
        DrawRectangleRounded(bounds, 0.24f, 6, (Color){17, 20, 29, 245});
        DrawRectangleRoundedLinesEx(bounds, 0.24f, 6, 2, hovered ? GOLD : LIGHTGRAY);
        float font_size = 21 * scale;
        float text_width = MeasureTextEx(default_game_font, labels[i], font_size, 0).x;
        DrawTextEx(default_game_font, labels[i], (Vector2){bounds.x + (width - text_width) / 2,
                   bounds.y + (height - font_size) / 2}, font_size, 0, WHITE);
        if (click && hovered)
        {
            if (i == 0) replay_clicked = true;
            else menu_clicked = true;
        }
    }

    if (skip_guard_time < CARD_SKIP_GUARD_TIME) return false;
    replay_requested = replay_clicked || IsKeyPressed(KEY_R);
    return replay_requested || menu_clicked || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE);
}

void handle_calendar_day_card(void)
{
    bool fading_in_finished = false;
    static double timer = DID_NOT_START;
    skip_guard_time += GetFrameTime();
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
        if (current_screen != ENDING_CARD_SCREEN && card_skip_requested())
        {
            timer = DID_NOT_START;
            current_calendar_day_state = FADING_OUT;
            break;
        }

        // Quando atingir 100% de opacidade, passa para o estado de espera
        if (fading_in_finished)
        {
            current_calendar_day_state = AWAIT;
        }
        break;

    case AWAIT:
        render_calendar_day_card();
        if (current_screen == ENDING_CARD_SCREEN)
        {
            if (ending_action_requested()) current_calendar_day_state = FADING_OUT;
            break;
        }
        if (card_skip_requested())
        {
            timer = DID_NOT_START;
            current_calendar_day_state = FADING_OUT;
            break;
        }

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

// Quantos caracteres (e não bytes) o texto UTF-8 tem: a fala digitada conta letras, não bytes dos acentos.
static int utf8_length(const char *text)
{
    int length = 0;

    if (text == NULL) return 0;
    for (; *text != '\0'; text++)
    {
        if (((unsigned char)*text & 0xC0) != 0x80) length++;
    }
    return length;
}

// Quantos bytes ocupam os primeiros `characters` caracteres do texto UTF-8.
static int utf8_prefix_bytes(const char *text, int characters)
{
    int bytes = 0;

    while (text[bytes] != '\0')
    {
        if (((unsigned char)text[bytes] & 0xC0) != 0x80)
        {
            if (characters == 0) break;
            characters--;
        }
        bytes++;
    }
    return bytes;
}

/**
 * @brief Verifica interações do jogador com objetos do cenário (computador).
 * 
 * Checa a sobreposição entre a caixa de colisão do jogador e cada objeto. Com a tecla ESPAÇO,
 * o computador abre a tela do quiz e o mural abre o bilhete com a dica; com o bilhete aberto,
 * ESPAÇO apenas o fecha. Examinar o bilhete não reduz a pontuação (US04). Depois de concluir
 * a tarefa, o computador deixa de abrir e a porta de vidro encerra o dia.
 */
static const char *npc_conversation_line(void)
{
    if (active_npc_variant && active_npc_dialogue_line == (int)active_npc_first_today)
        return active_npc_variant;
    int statistic_line = active_npc_dialogue_line - (active_npc_first_today ? 1 : 0);
    if (statistic_line == 0)
    {
        const char *variant = npc_get_statistic_day_line(active_npc_id, current_game_state_variables.current_day,
                                                         &current_hidden_statistic);
        if (variant) return variant;
    }
    return npc_get_dialogue_line(active_npc_id, current_game_state_variables.current_day,
                                 active_npc_first_today, active_npc_dialogue_line);
}

void handle_map_interactables(void)
{
    // Checa colisão AABB (Axis-Aligned Bounding Box) entre o jogador e o computador
    is_near_computer = CheckCollisionRecs(estagiario_collision_rec, estagiario_computer_collision_rectangle);
    is_near_hint = calendar_has_investigation(current_game_state_variables.current_day) &&
                   is_hitbox_on_hint(estagiario_collision_rec, get_scale(), get_map_center());
    is_near_exit = can_leave_today() && CheckCollisionRecs(estagiario_collision_rec, get_exit_zone());
    nearby_npc_id = npc_get_nearby(estagiario_collision_rec, get_scale());
    if (is_near_hint || is_near_exit || (is_near_computer && !can_leave_today()))
        nearby_npc_id = -1;

    if (active_npc_id >= 0)
    {
        int day = current_game_state_variables.current_day;
        const char *line = npc_conversation_line();
        float line_length = (float)utf8_length(line);

        // A fala aparece letra por letra; ESPAÇO, ENTER ou clique completam a fala na hora e, com ela
        // completa, passam para a próxima
        npc_typed_characters += GetFrameTime() * NPC_TYPE_SPEED;
        if (npc_typed_characters > line_length) npc_typed_characters = line_length;

        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            if (npc_typed_characters < line_length)
            {
                npc_typed_characters = line_length;
            }
            else
            {
                active_npc_dialogue_line++;
                npc_typed_characters = 0.0f;
                if (active_npc_dialogue_line >= npc_get_dialogue_line_count(active_npc_id, day, active_npc_first_today))
                {
                    active_npc_id = -1;
                    active_npc_dialogue_line = 0;
                }
            }
        }
        // Enquanto a fala aparece letra por letra, toca o som curto de fala (o mesmo blip da IARA)
        update_iara_voice_typing(active_npc_id >= 0 && npc_typed_characters < line_length);
        return;
    }

    if (player_seat != CHAIR_IDLE || !IsKeyPressed(KEY_SPACE)) return;

    if (is_hint_note_open())
    {
        toggle_mural_note();
    }
    else if (nearby_npc_id >= 0)
    {
        // Na primeira conversa do dia o NPC cumprimenta (no Dia 1, dá as boas-vindas); depois só fala do dia
        active_npc_id = nearby_npc_id;
        npc_face_player(active_npc_id, get_scale(), estagiario_collision_rec);
        active_npc_first_today = !npc_greeted_today[nearby_npc_id];
        active_npc_variant = NULL;
        int day = current_game_state_variables.current_day;
        HiddenStatisticBands bands = hidden_statistic_bands(&current_hidden_statistic);
        if (day == 3 && bands.perception >= 1 && active_npc_id == NPC_NICOLLAS && active_npc_first_today)
        {
            Rectangle mural = get_hint_item_rectangle(get_scale(), get_map_center());
            Rectangle feet = npc_get_hitbox(active_npc_id, get_scale());
            Vector2 npc = {feet.x + feet.width / 2, feet.y + feet.height / 2};
            if (hypotf(npc.x - mural.x - mural.width, npc.y - mural.y - mural.height / 2) <= 150 * get_scale())
                active_npc_variant = "Tem um risco novo na moldura. Jurava que eu tinha limpado isso ontem.";
        }
        if (day == 41 && bands.perception >= 2 && active_npc_id == NPC_NICOLLAS)
        {
            Vector2 center = get_map_center(), npc = npc_get_position(active_npc_id);
            float scale = get_scale();
            if (hypotf(npc.x - center.x + 408 * scale, npc.y - center.y - 94 * scale) <= 125 * scale)
                active_npc_variant = "Esse risco perto do arquivo é igual ao do mural. Você viu também?";
        }
        if (day == 58 && bands.bond == 2 && active_npc_id == NPC_LAURA)
            active_npc_variant = "Se quiser, a gente confere esses nomes juntos.";
        npc_greeted_today[nearby_npc_id] = true;
        npc_balloon_npc = active_npc_id;
        active_npc_dialogue_line = 0;
        npc_typed_characters = 0.0f;
    }
    else if (is_near_hint)
    {
        toggle_mural_note();
    }
    else if (is_near_exit)
    {
        leave_office = true;
    }
    else if (is_near_computer && !can_leave_today())
    {
        if (player_seat_frames[SEAT_PLAYER_FRAME_COUNT - 1].id) player_seat = CHAIR_APPROACH;
        else enter_computer_screen = true;
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

static int get_player_movement_obstacles(Rectangle obstacles[])
{
    int movement_obstacle_count = 0;

    for (int i = 0; i < OBSTACLE_COUNT; i++)
    {
        obstacles[movement_obstacle_count++] = office_obstacles[i];
    }
    for (int i = 0; i < NPC_COUNT; i++)
    {
        if (npc_blocks_player(i))
        {
            obstacles[movement_obstacle_count++] = npc_get_hitbox(i, get_scale());
        }
    }
    return movement_obstacle_count;
}

/**
 * @brief Captura os comandos do teclado (WASD / Setas), atualiza a velocidade, direção do sprite e
 * posição do jogador no espaço 2D.
 * 
 * Suporta movimentação em 8 direções. Aplica normalização escalar (0.7071 ≈ 1/√2) para evitar que
 * a movimentação na diagonal seja mais rápida do que a movimentação em eixos únicos.
 */
static void update_player_seating(float dt)
{
    if (player_seat == CHAIR_SITTING)
    {
        player_seat_time += dt;
        if (player_seat_time >= SEAT_PLAYER_FRAME_COUNT * SEAT_FRAME_SECONDS)
        {
            player_seat_time = SEAT_PLAYER_FRAME_COUNT * SEAT_FRAME_SECONDS;
            player_seat = CHAIR_SEATED;
            enter_computer_screen = true;
        }
        return;
    }
    if (player_seat == CHAIR_LEAVING)
    {
        player_seat_time = fmaxf(0.0f, player_seat_time - dt);
        if (player_seat_time == 0.0f) player_seat = CHAIR_IDLE;
        return;
    }
    if (player_seat != CHAIR_APPROACH) return;

    // Alcança a faixa livre ao sul do móvel, depois centraliza na cadeira.
    Vector2 target = player_chair_entry();
    float dx = target.x - current_estagiario_position.x;
    float dy = target.y - current_estagiario_position.y;
    bool horizontal = fabsf(dy) < 0.1f;
    if (horizontal && fabsf(dx) < 0.1f)
    {
        current_estagiario_position = target;
        current_direction = NORTH;
        current_estagiario_texture_direction = estagiario_texture[NORTH];
        player_seat_time = 0.0f;
        player_seat = CHAIR_SITTING;
        walk_time = 0.0f;
        update_collision_rec();
        return;
    }
    Rectangle obstacles[OBSTACLE_COUNT + NPC_COUNT];
    int count = get_player_movement_obstacles(obstacles);
    float remaining = horizontal ? dx : dy;
    float step = copysignf(fminf(fabsf(remaining), 160.0f * get_scale() * fminf(dt, 0.05f)), remaining);
    float *axis = horizontal ? &current_estagiario_position.x : &current_estagiario_position.y;
    update_collision_rec();
    float next = move_axis(*axis, step, estagiario_collision_rec, horizontal, obstacles, count);
    if (step != 0.0f && next == *axis)
    {
        player_seat = CHAIR_IDLE;
        walk_time = 0.0f;
        return;
    }
    *axis = next;
    walk_time += dt;
    current_direction = horizontal ? (dx < 0 ? WEST : EAST) : (dy < 0 ? NORTH : SOUTH);
    current_estagiario_texture_direction = estagiario_texture[current_direction];
    update_collision_rec();
}

void handle_player_input(void)
{
    float speed = DEFAULT_PLAYER_MOVEMENT_SPEED * get_scale();
    Rectangle movement_obstacles[OBSTACLE_COUNT + NPC_COUNT];
    int movement_obstacle_count = get_player_movement_obstacles(movement_obstacles);

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
        walk_time = 0.0f;
        update_collision_rec();
        return;
    }

    walk_time += GetFrameTime(); // Usa o tempo real do quadro para não depender do FPS

    // Seleção da direção correta com base no vetor combinado (dx, dy)
    if      (dx == 0 && dy < 0) current_direction = NORTH;
    else if (dx == 0 && dy > 0) current_direction = SOUTH;
    else if (dx < 0  && dy == 0) current_direction = WEST;
    else if (dx > 0  && dy == 0) current_direction = EAST;
    else if (dx < 0  && dy < 0) current_direction = NORTHWEST;
    else if (dx > 0  && dy < 0) current_direction = NORTHEAST;
    else if (dx < 0  && dy > 0) current_direction = SOUTHWEST;
    else                        current_direction = SOUTHEAST;
    current_estagiario_texture_direction = estagiario_texture[current_direction];

    // Correção de velocidade diagonal (Evita o efeito de ser mais rápido na diagonal devido ao Teorema de Pitágoras)
    if (dx != 0 && dy != 0)
    {
        speed *= 0.7071f;
    }

    // Move um eixo por vez: se bater num obstáculo, desfaz só aquele eixo e o jogador desliza na parede
    update_collision_rec();
    current_estagiario_position.x = move_axis(current_estagiario_position.x, dx * speed, estagiario_collision_rec, true,
                                              movement_obstacles, movement_obstacle_count);

    update_collision_rec();
    current_estagiario_position.y = move_axis(current_estagiario_position.y, dy * speed, estagiario_collision_rec, false,
                                              movement_obstacles, movement_obstacle_count);
    update_collision_rec();
}

// Luz do escritório no último quadro desenhado: os personagens usam a mesma luz do cenário,
// senão brilham sobre o escritório escurecido.
static Color office_scene_ambient = {255, 255, 255, 255};
static Rectangle office_scene_rect = {0, 0, 0, 0};
static float office_scene_progress = 0.0f;

static Color character_light(Vector2 foot)
{
    return office_symptoms_light_at(office_scene_ambient, office_scene_rect, foot, office_scene_progress, GetTime());
}

/**
 * @brief Desenha o sprite do jogador na tela utilizando as dimensões e rotação adequadas.
 */
void render_player(void)
{
    float size = get_player_size();
    float pixel = size / CHARACTER_SPRITE_SIZE;
    bool seated = player_seat >= CHAIR_SITTING;
    int frame = (int)(player_seat_time / SEAT_FRAME_SECONDS);
    if (frame >= SEAT_PLAYER_FRAME_COUNT) frame = SEAT_PLAYER_FRAME_COUNT - 1;
    Texture2D chair = seated ? player_seat_frames[frame] : player_chair;
    if (chair.id)
    {
        Rectangle source = {0, 0, chair.width, chair.height};
        Rectangle target = scale_reference_rect((Rectangle){
            seated ? -SEAT_ORIGIN_X : -CHAIR_ORIGIN_X, seated ? -SEAT_ORIGIN_Y : -CHAIR_ORIGIN_Y,
            source.width, source.height}, pixel, player_chair_anchor());
        DrawTexturePro(chair, source, target, (Vector2){0}, 0.0f, color_to_fade);
        if (seated) return;
    }

    // Parado desenha a textura da direção; andando troca pelo frame do andar
    Texture2D texture = current_estagiario_texture_direction;

    if (walk_time > 0 && walk_loaded[current_direction])
    {
        texture = walk_textures[current_direction][get_walk_frame(walk_time)];
    }

    // Retângulo de origem recortando toda a textura atual
    Rectangle source = {0, 0, (float)texture.width, (float)texture.height};
    
    // Retângulo de destino definindo onde e com qual tamanho o sprite será desenhado no mundo
    Rectangle dest = {current_estagiario_position.x,
                      current_estagiario_position.y,
                      size, size};

    // Desenha a textura com a luz do ponto onde o jogador pisa (inclui a opacidade do fade)
    Color light = character_light((Vector2){dest.x + (size * 0.5f), dest.y + (size * 0.9f)});
    light.a = color_to_fade.a;

    DrawTexturePro(texture, source, dest, (Vector2){0, 0}, 0.0f, light);
}

/**
 * @brief Desenha o contorno das hitboxes de colisão quando o modo debug (F1) está ligado.
 */
void render_debug_hitboxes(void)
{
    if (!show_debug_hitboxes) return;

    // Desempenho: para medir antes e depois de mexer no desenho do escritório
    DrawRectangle(8, 8, 330, 30, (Color){0, 0, 0, 170});
    DrawText(TextFormat("FPS %d   quadro %.1f ms", GetFPS(), GetFrameTime() * 1000.0f), 16, 14, 20, LIME);

    DrawRectangleLinesEx(map_collision_rec, 2.0f, RED);                           // Limite do mapa
    DrawRectangleLinesEx(estagiario_computer_collision_rectangle, 2.0f, YELLOW);  // Zona do computador
    DrawRectangleLinesEx(estagiario_collision_rec, 2.0f, GREEN);                  // Jogador
    DrawRectangleLinesEx(get_exit_zone(), 2.0f, SKYBLUE);                         // Zona de saída (porta)

    for (int i = 0; i < OBSTACLE_COUNT; i++)
    {
        DrawRectangleLinesEx(office_obstacles[i], 2.0f, ORANGE);                  // Obstáculos
    }

    for (int i = 0; i < NPC_COUNT; i++)
    {
        DrawRectangleLinesEx(npc_get_hitbox(i, get_scale()), 2.0f, PURPLE);       // NPCs
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

    // Sem nenhum número na tela: quanto mais tarde na campanha, mais escuro,
    // e as estatísticas ocultas mudam a luz e os elementos do mapa (US10, RF11)
    SceneSymptoms symptoms = symptoms_get_scene();
    double now = GetTime();
    float campaign_progress = calendar_get_water_level(current_game_state_variables.current_day);
    unsigned char brightness = (unsigned char)(255.0f - (OFFICE_MAX_DARKENING * campaign_progress));
    Color office_tint = office_symptoms_light_tint(&symptoms, brightness, color_to_fade.a, now);

    DrawTexturePro(in_game_office, source, dest, (Vector2){0, 0}, 0.0f, office_tint);
    if (chair_floor.id) DrawTexturePro(chair_floor, source, dest, (Vector2){0}, 0.0f, office_tint);

    office_scene_ambient = office_tint;
    office_scene_rect = dest;
    office_scene_progress = campaign_progress;

    draw_office_broken_lamps(dest, campaign_progress, color_to_fade.a, now);
    draw_office_symptoms(&symptoms, dest, color_to_fade.a, now);
}

/**
 * @brief Desenha um texto centralizado, reduzindo a fonte se ele não couber na janela.
 */
static void draw_transition_text(const char *text, float center_y, float font_size, Color color)
{
    float max_width = GetScreenWidth() * TRANSITION_TEXT_WIDTH_RATIO;
    float width = MeasureTextEx(default_game_font, text, font_size, font_size * 0.04f).x;

    if (width > max_width) font_size *= max_width / width;

    Vector2 size = MeasureTextEx(default_game_font, text, font_size, font_size * 0.04f);

    DrawTextEx(default_game_font, text, (Vector2){(GetScreenWidth() - size.x) / 2.0f, center_y - (size.y / 2.0f)}, font_size, font_size * 0.04f, color);
}

/**
 * @brief Flash do dia e salto puro (RF06). O flash só avança ao toque do jogador, e o salto
 * passa sozinho. Ainda não há imagens próprias: o flash usa o escritório do dia como imagem.
 */
static void handle_day_transition(void)
{
    int day = current_game_state_variables.current_day;
    float scale = get_ui_scale();
    float center_y = GetScreenHeight() / 2.0f;

    transition_time += GetFrameTime();

    if (current_game_state_variables.day_type == JUMP_DAY)
    {
        draw_campaign_art(campaign_art[ART_JUMP], 255);
        draw_transition_text(TextFormat("Dia %d", day), center_y, TITLE_FONT_SIZE * scale, WHITE);

        if (transition_time >= jump_day_time) advance_to_next_day();
        return;
    }

    color_to_fade.a = 255;
    draw_campaign_art(campaign_art[ART_FLASH], 255);
    draw_transition_text(TextFormat("Dia %d", day), center_y - (SUBTITLE_FONT_SIZE * scale * 3.0f), TITLE_FONT_SIZE * scale, WHITE);
    draw_transition_text(calendar_get_flash_text(day), center_y, SUBTITLE_FONT_SIZE * scale, WHITE);

    if (transition_time < FLASH_MIN_TIME) return;

    draw_transition_text("ESPAÇO, ENTER ou clique para continuar", center_y + (SUBTITLE_FONT_SIZE * scale * 3.0f), SUBTITLE_FONT_SIZE * scale * 0.7f, GRAY);

    if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) ||
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) advance_to_next_day();
}

/**
 * @brief Texto do contador de progresso da partida (GDD, P01). Não mostra nenhuma estatística oculta.
 */
static const char *get_day_counter_text(void)
{
    return TextFormat("Dia %d de %d", current_game_state_variables.current_day, TOTAL_GAME_DAYS);
}

/**
 * @brief Desenha o contador de dias na margem de cima do mapa.
 */
static void render_day_counter(void)
{
    float scale = get_scale();
    float font_size = DAY_COUNTER_FONT_AT_REFERENCE * scale;
    float letter_spacing = font_size * 0.04f;
    float office_top_y = (GetScreenHeight() - (in_game_office.height * scale)) / 2.0f;
    float left_x = (GetScreenWidth() - (in_game_office.width * scale)) / 2.0f;
    float y = (office_top_y - font_size) / 2.0f;

    DrawTextEx(default_game_font, get_day_counter_text(), (Vector2){left_x, y}, font_size, letter_spacing, color_to_fade);
}

/**
 * @brief Desenha o escritório atrás do monitor: aproximado no computador do jogador e escurecido.
 *
 * Recorta do mapa a área em volta do computador com o formato da janela. O corte nunca
 * passa das bordas do mapa, então o fundo cobre a janela inteira em qualquer formato.
 */
static void render_computer_backdrop(void)
{
    float window_w = (float)GetScreenWidth();
    float window_h = (float)GetScreenHeight();
    float map_w = (float)in_game_office.width;
    float map_h = (float)in_game_office.height;

    // Maior recorte possível com o formato da janela, reduzido pela aproximação animada
    float crop_scale = (map_w / window_w < map_h / window_h) ? map_w / window_w : map_h / window_h;
    float zoom = 1.0f + (COMPUTER_SCENE_ZOOM - 1.0f) * computer_scene_progress;
    float source_w = (window_w * crop_scale) / zoom;
    float source_h = (window_h * crop_scale) / zoom;

    float source_x = (map_w / 2.0f) + COMPUTER_OFFSET_X_AT_REFERENCE + (COMPUTER_SIZE_AT_REFERENCE / 2.0f) - (source_w / 2.0f);
    float source_y = (map_h / 2.0f) + COMPUTER_OFFSET_Y_AT_REFERENCE + (COMPUTER_SIZE_AT_REFERENCE / 2.0f) - (source_h / 2.0f);

    if (source_x < 0.0f) source_x = 0.0f;
    if (source_y < 0.0f) source_y = 0.0f;
    if (source_x > map_w - source_w) source_x = map_w - source_w;
    if (source_y > map_h - source_h) source_y = map_h - source_h;

    DrawTexturePro(in_game_office, (Rectangle){source_x, source_y, source_w, source_h},
                   (Rectangle){0, 0, window_w, window_h}, (Vector2){0, 0}, 0.0f, WHITE);

    unsigned char dim_alpha = (unsigned char)(COMPUTER_SCENE_DIM_ALPHA * computer_scene_progress);
    DrawRectangle(0, 0, (int)window_w, (int)window_h, (Color){0, 0, 0, dim_alpha});
}

// Chamado no ciclo de desenho existente, antes da interface.
static void render_office_scene(void)
{
    render_game_loop_escritorio();
    // Mantém o alfa da cena ao desenhar as bordas suavizadas dos personagens.
    rlSetBlendFactorsSeparate(RL_SRC_ALPHA, RL_ONE_MINUS_SRC_ALPHA, RL_ONE, RL_ONE_MINUS_SRC_ALPHA,
                              RL_FUNC_ADD, RL_FUNC_ADD);
    BeginBlendMode(BLEND_CUSTOM_SEPARATE);
    draw_npcs_lit(get_scale(), character_light, current_estagiario_position.y, true);
    render_player();
    draw_npcs_lit(get_scale(), character_light, current_estagiario_position.y, false);
    EndBlendMode();
}

static void render_office_with_glass(void)
{
    int width = GetScreenWidth(), height = GetScreenHeight();
    if (width <= 0 || height <= 0) return;
    if (glass_scene.texture.width != width || glass_scene.texture.height != height)
    {
        if (glass_scene.id) UnloadRenderTexture(glass_scene);
        glass_scene = LoadRenderTexture(width, height);
    }
    if (!glass_scene.id)
    {
        render_office_scene();
        return;
    }
    BeginTextureMode(glass_scene);
    ClearBackground(BG_COLOR);
    // Mantém a opacidade da cena ao compor sombras semitransparentes das cadeiras.
    rlSetBlendFactorsSeparate(RL_SRC_ALPHA, RL_ONE_MINUS_SRC_ALPHA,
                              RL_ONE, RL_ONE_MINUS_SRC_ALPHA, RL_FUNC_ADD, RL_FUNC_ADD);
    BeginBlendMode(BLEND_CUSTOM_SEPARATE);
    render_office_scene();
    EndBlendMode();
    EndTextureMode();

    Rectangle source = {0, 0, (float)width, -(float)height};
    DrawTextureRec(glass_scene.texture, source, (Vector2){0}, WHITE);
    float scale = get_scale();
    Vector2 size = {(float)width, (float)height};
    bool shader_ready = glass_panel_loc >= 0 && glass_size_loc >= 0;
    for (int i = 0; i < GLASS_COUNT; i++)
    {
        Rectangle pane = office_glass_surface[i];
        if (shader_ready)
        {
            // Só o retângulo do vidro passa pelo shader. Antes a tela inteira era redesenhada a cada
            // vidro e o shader descartava o que ficava fora: três passadas de tela cheia por quadro.
            // A textura da cena está de cabeça para baixo, por isso o y é contado de baixo.
            Rectangle pane_source = {pane.x, (float)height - pane.y - pane.height, pane.width, -pane.height};

            SetShaderValue(glass_shader, glass_panel_loc, &pane, SHADER_UNIFORM_VEC4);
            SetShaderValue(glass_shader, glass_size_loc, &size, SHADER_UNIFORM_VEC2);
            BeginShaderMode(glass_shader);
            DrawTexturePro(glass_scene.texture, pane_source, pane, (Vector2){0}, 0.0f, color_to_fade);
            EndShaderMode();
        }
        // Reutiliza os reflexos do asset e redesenha a moldura sem distorcao.
        Rectangle ref = glass_surface_reference[i];
        Rectangle crop = {ref.x + in_game_office.width/2.0f,
                          ref.y + in_game_office.height/2.0f, ref.width, ref.height};
        Color reflection = color_to_fade;
        reflection.a = (unsigned char)(reflection.a * 0.18f);
        DrawTexturePro(in_game_office, crop, pane, (Vector2){0}, 0, reflection);
        float top_frame = i == GLASS_COUNT - 1 ? 12.0f : 7.0f;
        Rectangle frames[] = {
            {crop.x, crop.y-top_frame, crop.width, top_frame},
            {crop.x, crop.y+crop.height-4, crop.width, in_game_office.height-crop.y-crop.height+4},
            {crop.x-3, crop.y-7, 3, crop.height+15},
            {crop.x+crop.width, crop.y-7, 3, crop.height+15}
        };
        Vector2 origin = {(width-in_game_office.width*scale)/2.0f,
                          (height-in_game_office.height*scale)/2.0f};
        for (int j = 0; j < 4; j++)
            DrawTexturePro(in_game_office, frames[j], scale_reference_rect(frames[j], scale, origin),
                           (Vector2){0}, 0, color_to_fade);
    }
    // Pilares e parede central tambem ocluem a parte superior do sprite.
    Vector2 origin = {(width-in_game_office.width*scale)/2.0f,
                      (height-in_game_office.height*scale)/2.0f};
    for (unsigned int i = 0; i < sizeof(office_foreground_reference)/sizeof(office_foreground_reference[0]); i++)
    {
        Rectangle ref = office_foreground_reference[i];
        Rectangle crop = {ref.x+in_game_office.width/2.0f, ref.y+in_game_office.height/2.0f,
                          ref.width, ref.height};
        DrawTexturePro(in_game_office, crop, scale_reference_rect(crop, scale, origin),
                       (Vector2){0}, 0, color_to_fade);
    }
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

// DrawTriangle só preenche triângulos em sentido anti-horário; esta versão aceita qualquer ordem.
static void draw_triangle_any_order(Vector2 a, Vector2 b, Vector2 c, Color color)
{
    float cross = ((b.x - a.x) * (c.y - a.y)) - ((b.y - a.y) * (c.x - a.x));

    if (cross > 0.0f) DrawTriangle(a, c, b, color);
    else DrawTriangle(a, b, c, color);
}

/**
 * @brief Seta pequena que acompanha o jogador e aponta para o próximo objetivo (o computador e, depois,
 * a porta). Ela gira em volta do personagem e some quando ele já está perto, para não cobrir o mapa.
 *
 * @param target Objeto em coordenadas da tela.
 */
void render_interact_arrow(Rectangle target)
{
    float scale = get_scale();
    float player_size = get_player_size();
    Vector2 player_center = {current_estagiario_position.x + (player_size / 2.0f),
                             current_estagiario_position.y + (player_size * 0.45f)};
    Vector2 target_center = {target.x + (target.width / 2.0f), target.y + (target.height / 2.0f)};
    float dx = target_center.x - player_center.x;
    float dy = target_center.y - player_center.y;
    float distance = sqrtf((dx * dx) + (dy * dy));

    // Já ao lado do objetivo: o anel e a tecla de interação assumem
    if (distance < 110.0f * scale) return;

    float direction_x = dx / distance;
    float direction_y = dy / distance;
    float pulse = 0.5f + (0.5f * sinf((float)GetTime() * 5.0f));
    float orbit = (player_size * 0.78f) + (3.0f * scale * pulse);
    float size = 11.0f * scale;
    Vector2 center = {player_center.x + (direction_x * orbit), player_center.y + (direction_y * orbit)};
    Vector2 perpendicular = {-direction_y, direction_x};
    Vector2 tip = {center.x + (direction_x * size), center.y + (direction_y * size)};
    Vector2 left = {center.x - (direction_x * size * 0.7f) + (perpendicular.x * size * 0.8f),
                    center.y - (direction_y * size * 0.7f) + (perpendicular.y * size * 0.8f)};
    Vector2 right = {center.x - (direction_x * size * 0.7f) - (perpendicular.x * size * 0.8f),
                     center.y - (direction_y * size * 0.7f) - (perpendicular.y * size * 0.8f)};
    unsigned char alpha = (unsigned char)((170.0f + (70.0f * pulse)) * color_to_fade.a / 255.0f);

    // Contorno escuro por baixo: a setinha se destaca sobre qualquer piso
    float grow = 1.7f * scale;
    draw_triangle_any_order((Vector2){tip.x + (direction_x * grow), tip.y + (direction_y * grow)},
                 (Vector2){left.x - (perpendicular.x * grow) - (direction_x * grow), left.y - (perpendicular.y * grow) - (direction_y * grow)},
                 (Vector2){right.x + (perpendicular.x * grow) - (direction_x * grow), right.y + (perpendicular.y * grow) - (direction_y * grow)},
                 (Color){20, 12, 28, (unsigned char)(alpha * 0.8f)});
    draw_triangle_any_order(tip, left, right, (Color){255, 214, 70, alpha});
}

/**
 * @brief Indicador de que algo ao alcance aceita interação (NPC, computador, mural ou porta): um anel
 * que pulsa no chão e uma tecla ESPAÇO pequena e discreta, sem texto grande nem seta sobre o mapa.
 *
 * @param feet Retângulo dos "pés" do objeto, em coordenadas da tela (o anel acompanha a largura dele).
 */
static void render_interaction_marker(Rectangle feet, const char *label)
{
    float scale = get_scale();
    float pulse = 0.5f + (0.5f * sinf((float)GetTime() * 4.0f));
    float fade = color_to_fade.a / 255.0f;
    Vector2 center = {feet.x + (feet.width / 2.0f), feet.y + (feet.height / 2.0f)};
    float radius_x = (feet.width * 0.5f) + ((6.0f + (3.0f * pulse)) * scale);
    float radius_y = radius_x * 0.38f;
    unsigned char ring_alpha = (unsigned char)((120.0f + (100.0f * pulse)) * fade);

    // Anel achatado no chão, como uma sombra luminosa
    DrawEllipse((int)center.x, (int)center.y, radius_x, radius_y, (Color){255, 214, 70, (unsigned char)(34.0f * fade)});
    DrawEllipseLines((int)center.x, (int)center.y, radius_x, radius_y, (Color){255, 214, 70, ring_alpha});

    // Tecla ESPAÇO pequena, acima do objeto, subindo e descendo devagar
    float font_size = 13.0f * scale;
    float text_width = MeasureTextEx(default_game_font, label, font_size, 0.0f).x;
    float key_width = text_width + (12.0f * scale);
    float key_height = font_size + (7.0f * scale);
    float bob = sinf((float)GetTime() * 3.0f) * 1.8f * scale;
    Rectangle key = {center.x - (key_width / 2.0f), feet.y - (46.0f * scale) + bob, key_width, key_height};
    unsigned char key_alpha = (unsigned char)(235.0f * fade);

    DrawRectangleRounded((Rectangle){key.x, key.y + (2.0f * scale), key.width, key.height}, 0.3f, 4, (Color){20, 12, 28, (unsigned char)(200.0f * fade)});
    DrawRectangleRounded(key, 0.3f, 4, (Color){240, 234, 244, key_alpha});
    DrawTextEx(default_game_font, label, (Vector2){key.x + (6.0f * scale), key.y + ((key.height - font_size) / 2.0f)},
               font_size, 0.0f, (Color){28, 18, 34, key_alpha});
}

#define DIALOGUE_MAX_LINES 6
#define DIALOGUE_LINE_BYTES 256

// Quebra o texto em linhas que cabem em max_width. Devolve quantas linhas saíram.
static int wrap_dialogue_text(const char *text, float max_width, float font_size,
                              char lines[DIALOGUE_MAX_LINES][DIALOGUE_LINE_BYTES])
{
    int line_count = 0;
    char line[DIALOGUE_LINE_BYTES] = "";
    const char *word_start = text;

    while (*word_start != '\0')
    {
        while (*word_start == ' ') word_start++;
        if (*word_start == '\0') break;

        const char *word_end = word_start;
        while (*word_end != '\0' && *word_end != ' ') word_end++;

        char word[DIALOGUE_LINE_BYTES];
        size_t word_length = (size_t)(word_end - word_start);
        if (word_length >= sizeof(word)) word_length = sizeof(word) - 1;
        snprintf(word, sizeof(word), "%.*s", (int)word_length, word_start);

        char candidate[DIALOGUE_LINE_BYTES];
        if (line[0] == '\0') snprintf(candidate, sizeof(candidate), "%s", word);
        else snprintf(candidate, sizeof(candidate), "%s %s", line, word);

        if (line[0] != '\0' && MeasureTextEx(default_game_font, candidate, font_size, 0.0f).x > max_width)
        {
            if (line_count < DIALOGUE_MAX_LINES) snprintf(lines[line_count++], DIALOGUE_LINE_BYTES, "%s", line);
            snprintf(line, sizeof(line), "%s", word);
        }
        else
        {
            snprintf(line, sizeof(line), "%s", candidate);
        }

        word_start = word_end;
    }

    if (line[0] != '\0' && line_count < DIALOGUE_MAX_LINES) snprintf(lines[line_count++], DIALOGUE_LINE_BYTES, "%s", line);
    return line_count;
}

// Desenha o texto já quebrado, mostrando só os primeiros `visible` caracteres (a fala sendo digitada).
static void draw_typed_dialogue(const char *text, Vector2 position, float max_width, float font_size,
                                int visible, Color color)
{
    char lines[DIALOGUE_MAX_LINES][DIALOGUE_LINE_BYTES];
    int line_count = wrap_dialogue_text(text, max_width, font_size, lines);

    for (int i = 0; i < line_count && visible > 0; i++)
    {
        int line_length = utf8_length(lines[i]);
        char shown[DIALOGUE_LINE_BYTES];
        int shown_chars = visible < line_length ? visible : line_length;
        int bytes = utf8_prefix_bytes(lines[i], shown_chars);

        snprintf(shown, sizeof(shown), "%.*s", bytes, lines[i]);
        DrawTextEx(default_game_font, shown, position, font_size, 0.0f, color);
        position.y += font_size * 1.2f;
        visible -= line_length + 1; // +1: o espaço que a quebra consumiu
    }
}

/* Cenas do Dia 1: reaproveitam o diálogo e o desenho do escritório.
 * O computador fica carregado, mas não recebe update/input durante 01M. */
static bool campaign_scene_completed(StoryPhase phase)
{
    return (current_game_state_variables.story_later_scenes &
            story_scene_bit(current_game_state_variables.current_day, phase)) != 0;
}

static int campaign_actor(int actor)
{
    if (actor == NPC_JULIO && !npc_is_present(actor)) return NPC_ZITO;
    return actor;
}

static const int *campaign_actors(int scene)
{
    return scene == CAMPAIGN_ARRIVAL ? campaign_scene_day->arrival_actors : campaign_scene_day->finish_actors;
}

static int story_first_actor(int scene)
{
    if (scene >= CAMPAIGN_ARRIVAL)
        return campaign_actor(scene == CAMPAIGN_MIDDLE ? campaign_scene_day->middle_actor : campaign_actors(scene)[0]);
    if (scene == DAY3_ARRIVAL || scene == DAY3_END) return NPC_VASCO;
    if (scene == DAY3_TRIAGE) return NPC_ZITO;
    if (scene == DAY3_LUNCH) return NPC_LAURA;
    return NPC_JV;
}

static int story_second_actor(int scene)
{
    if (scene >= CAMPAIGN_ARRIVAL)
    {
        if (scene == CAMPAIGN_MIDDLE) return -1;
        const int *actors = campaign_actors(scene);
        return campaign_actor(actors[1] != actors[0] ? actors[1] : actors[2]);
    }
    if (scene == DAY3_ARRIVAL || scene == DAY3_END) return NPC_ZITO;
    return scene == 4 ? NPC_VASCO : -1;
}

static const char *day_three_story_text(void)
{
    int outcome = computer_first_outcome();
    switch (story_scene)
    {
    case DAY3_ARRIVAL:
        if (story_line == 0) return "A fila do Tiago caiu pra você. O aviso diz que ele pediu folga.";
        if (story_line == 1) return "Eu fico no TI. Se a fila travar, me chama antes de recarregar.";
        if (story_line == 2) return "A norma mudou. Confere o bilhete antes de mandar.";
        if (story_bands.perception >= 1) return "Compara o bilhete com o que aparecer na mesa.";
        if (story_bands.automation >= 1) return "A luz da fila já acendeu. Ainda falta você decidir.";
        return "Quando abrir a fila, dá uma olhada no bilhete.";
    case DAY3_TRIAGE:
        if (outcome == TASK_TAKEN_OVER_BY_IARA) return "A IARA assumiu a fila. Pelo menos ficou um registro.";
        if (story_line == 0) return "Deixa eu ver se a fila voltou a responder.";
        if (story_line == 2) return "Quando terminar, confere o relatório. Eu fico por aqui.";
        if (outcome == TASK_PENDING) return "A mesa devolveu a tentativa. Você pode conferir as condições sem refazer tudo.";
        if (outcome == TASK_SENT_UNCHECKED) return "Já saiu? Quando abrir o relatório, a gente vê o que foi enviado.";
        if (outcome == TASK_SOLVED_WITH_IARA) return "Você comparou antes de mandar. Boa. A mesa aceitou.";
        return "A mesa aceitou. Agora dá pra seguir.";
    case DAY3_LUNCH:
        return story_line == 0 ? "Tem lugar aqui. A Júlia disse que você podia vir." :
                                 "Vem. O relatório pode esperar uns minutos.";
    case DAY3_END:
        if (story_line == 0) return "Vou registrar o fechamento da fila.";
        if (story_line == 2) return "Ninguém confirmou quando o Tiago volta.";
        if (story_line == 3)
        {
            if (!computer_day_three_choice_recorded()) return "Vou perguntar pra Júlia se alguém recebeu notícia dele.";
            if (computer_day_three_lunch_accepted())
            {
                if (current_game_state_variables.story_scenes & DAY3_LUNCH)
                    return "A Júlia disse que vocês conseguiram fazer uma pausa.";
                return "Vi que você respondeu à Júlia.";
            }
            return "A Júlia deixou o convite. Se quiser falar com ela outro dia, eu passo o recado.";
        }
        if (outcome == TASK_TAKEN_OVER_BY_IARA) return "A IARA assumiu. O relatório mostra como a fila foi fechada.";
        if (outcome == TASK_SENT_UNCHECKED)
            return computer_first_answer_correct() ? "A triagem saiu certa. A sugestão foi enviada sem conferência." :
                  "O portal ficou parado. A sugestão saiu sem conferência.";
        if (outcome == TASK_SOLVED_WITH_IARA) return "Você conferiu a sugestão antes de mandar. A triagem fechou.";
        if (outcome == TASK_SOLVED_ALONE) return "Você fechou a triagem. Eu pego o que chegou pro TI.";
        return "O relatório já registrou o que saiu.";
    }
    return "";
}

static int story_speaker(void)
{
    if (story_scene >= CAMPAIGN_ARRIVAL)
        return story_scene == CAMPAIGN_MIDDLE ? story_first_actor(story_scene) :
               campaign_actor(campaign_actors(story_scene)[story_line]);
    if (story_scene == DAY3_ARRIVAL) return story_line % 2 == 0 ? NPC_VASCO : NPC_ZITO;
    if (story_scene == DAY3_END) return story_line == 1 ? NPC_ZITO : NPC_VASCO;
    if (story_scene >= DAY3_ARRIVAL) return story_first_actor(story_scene);
    if (story_scene == 1) return story_line < 2 ? NPC_LAURA : NPC_JV;
    if (story_scene == 4) return story_line == 1 ? NPC_JV : NPC_VASCO;
    return NPC_JV;
}
static const char *campaign_finish_text(void)
{
    const struct game_state_variables *state = &current_game_state_variables;
    int choice = computer_story_day_choice();
    int outcome = computer_first_outcome();
    bool taken = computer_failsafe_triggered;
    switch (state->current_day)
    {
    case 12:
        if (story_line == 0) return "Sônia recebeu o fechamento?";
        if (story_line == 2) return "Vou perguntar ao RH quem estava nessa mesa.";
        if (taken) return "A IARA assumiu a mesa. O relatório é o que temos agora.";
        if (choice == 1) return "O pedido de revisão foi registrado. Quero comparar com a primeira versão.";
        if (choice == 0) return "Essa foi a versão assinada? Quero comparar com a primeira.";
        return "Quero comparar o relatório com a primeira versão.";
    case 27:
        if (story_line == 0) return "Ainda ouço água por aqui.";
        if (story_line == 1)
        {
            if (choice == 0) return "O Caio conseguiu conversar com você?";
            if (choice == 1) return "Se o Caio procurar você, eu aviso que o turno já acabou.";
            return "Ainda tem gente trabalhando por aqui.";
        }
        return outcome != TASK_PENDING && !computer_first_answer_correct()
            ? "Vou guardar uma cópia da ata para conferir." : "Vou guardar a ata.";
    case 41:
        if (story_line == 2) return "Esse registro tinha dado pessoal. Precisamos conferir o que foi enviado.";
        if (story_line == 1)
        {
            if (state->d41_mural_lido) return "Você viu a etiqueta do prédio. Vou guardar a caixa.";
            return taken ? "Eu ainda preciso guardar a caixa." : "Tem uma etiqueta no mural. Eu ainda preciso guardar a caixa.";
        }
        if (outcome == TASK_PENDING) return "O relatório guardou o resultado do treino.";
        return computer_first_answer_correct()
            ? "A ficha aceitou o conjunto. O teste ainda precisa ser acompanhado."
            : "O conjunto saiu diferente do pedido. Quero ver quais dados entraram.";
    case 58:
        if (story_line == 2) return "Eu deixo a lista aberta pra auditoria.";
        if (story_line == 1)
            return choice == 0 ? "Você ouviu algo diferente hoje?" :
                   choice == 1 ? "Se a Bruna procurar você, eu aviso." : "Vou conferir quem ainda está no escritório.";
        if (state->evidence_mask & EVIDENCE_BADGE_AFTER_LEAVING)
            return "O registro do Diego mostra uma passagem depois da saída dele.";
        return taken ? "Tem um registro preso no mural." : "Tem um registro preso no mural. Você pode conferir antes de ir.";
    case 74:
        if (story_line == 0)
        {
            if (state->evidence_mask & EVIDENCE_NO_HUMAN_ORDER)
                return "O bilhete fala que ninguém autorizou às 03:12. Guarda essa informação.";
            return taken ? "Tem um bilhete no mural." : "Tem um bilhete no mural. Se puder, dá uma olhada.";
        }
        return (outcome == TASK_SENT_UNCHECKED || outcome == TASK_TAKEN_OVER_BY_IARA) && !computer_first_answer_correct()
            ? "O relatório registrou uma autorização humana. Não sei quem confirmou."
            : "Vou encaminhar o relatório.";
    case 89:
        if (story_line == 1)
            return taken ? "O expediente terminou." :
                state->found_nicollas_trace ? "A marca ficou no mapa antigo. Guarda o que você viu." : "Se for olhar, eu espero aqui.";
        return state->found_nicollas_trace ? "Você achou o mapa. A escada vai além do que desenharam ali."
            : "Tem um mapa no mural. A escada tá marcada a lápis.";
    case 100:
        if (story_line == 1) return "A reunião terminou. Vem.";
        return state->proof_succeeded ? "A contradição ficou registrada. A resposta continua na tela."
            : "O relatório já foi enviado.";
    }
    return "";
}

static const char *campaign_story_text(void)
{
    int day = current_game_state_variables.current_day;
    if (story_scene == CAMPAIGN_FINISH) return campaign_finish_text();
    if (story_scene == CAMPAIGN_MIDDLE)
    {
        TaskOutcome outcome = computer_story_middle_reaction();
        if (outcome == TASK_TAKEN_OVER_BY_IARA) return campaign_scene_day->middle[4];
        if (!story_line) return campaign_scene_day->middle[0];
        if (outcome == TASK_PENDING) return campaign_scene_day->middle[1];
        if (outcome == TASK_SENT_UNCHECKED) return campaign_scene_day->middle[3];
        return campaign_scene_day->middle[2];
    }
    if (story_line == 2)
    {
        if (day == 12 && story_bands.bond >= 1) return "Deixaram um recado no quadro. Se precisar, me chama.";
        if (day == 41 && story_bands.automation >= 2) return "A fila já abriu. Ainda dá pra revisar o que entra.";
        if (day == 74 && story_bands.perception == 3) return "Esse risco perto do teclado apareceu de novo.";
        if (day == 89 && story_bands.bond >= 1) return "Se quiser, eu fico por perto enquanto você confere.";
        if (day == 100 && current_game_state_variables.evidence_mask) return "Você tem registros para consultar.";
    }
    if (story_line == 1 && day == 100 && story_bands.bond >= 1)
        return "Se precisar parar um instante, eu espero aqui.";
    return campaign_scene_day->arrival[story_line];
}

static const char *story_text(void)
{
    static const char *arrival[] = {
        "Chegou agora? Sou a Laura, do RH.",
        "Sua mesa é a da salinha, ali à direita. Se precisar de mim, me chama.",
        "Sou a JV, dos modelos. Abre a fila e compara os pedidos com as respostas.",
        "Se pedir ajuda, a IARA sugere. Testa se a resposta atende ao chamado antes de mandar."};
    if (story_scene >= CAMPAIGN_ARRIVAL) return campaign_story_text();
    if (story_scene >= DAY3_ARRIVAL) return day_three_story_text();
    if (story_scene == 1) return arrival[story_line];
    if (story_scene == 4)
    {
        if (story_line == 0) return "Fechou o relatório do primeiro turno?";
        if (story_line == 2) return "Certo, vou marcar como entregue. Pode ir por hoje.";
        if (computer_failsafe_triggered) return "A IARA terminou a fila. Quero separar no relatório o que ela enviou.";
        return computer_mostly_unchecked()
            ? "Ficaram sugestões sem conferência. Amanhã eu quero comparar o que saiu com os pedidos."
            : "Quero conferir se o relatório bate com os chamados que chegaram hoje.";
    }
    if (computer_first_outcome() == TASK_TAKEN_OVER_BY_IARA)
        return "A IARA terminou a fila. Vamos ver o que ela registrou?";
    if (story_line == 0) return "Já fechou o da Bruna?";
    if (story_line == 2) return "O do Caio vem agora. Se pintar dúvida, eu tô ali.";
    if (computer_first_outcome() == TASK_SOLVED_ALONE)
        return "Você resolveu direto. Qual foi o detalhe que decidiu pra você?";
    if (computer_first_outcome() == TASK_SOLVED_WITH_IARA)
        return "Você conferiu a sugestão. Boa. Deu pra ver de onde ela tirou a resposta?";
    return "Já foi? Depois me mostra a resposta no relatório. Quero conferir o que saiu.";
}
static void begin_story_scene(int scene, bool return_computer)
{
    color_to_fade.a = 255;
    story_scene = scene;
    story_bands = hidden_statistic_bands(&current_hidden_statistic);
    story_line = 0;
    story_return_computer = return_computer;
    story_input_guard = true;
    story_action_time = 0;
    story_approaching = scene != 1;
    npc_typed_characters = 0;
    npc_balloon_open = 0;
    active_npc_id = -1;
    if (story_approaching)
    {
        npc_begin_approach(story_first_actor(scene));
        int second = story_second_actor(scene);
        if (second >= 0) npc_begin_approach(second);
    }
    current_screen = IN_GAME_SCREEN;
    update_iara_voice_typing(false);
    computer_suspend_input();
}
static void update_story_scene(float dt)
{
    if (!IsWindowFocused()) { story_input_guard = true; return; }
    if (story_approaching)
    {
        if (player_seat == CHAIR_LEAVING) update_player_seating(dt);
        float scale = get_scale();
        Vector2 center = get_map_center();
        Vector2 target = {estagiario_collision_rec.x + estagiario_collision_rec.width / 2,
                          estagiario_collision_rec.y + estagiario_collision_rec.height / 2};
        if (player_seat == CHAIR_SEATED) target = player_chair_anchor();
        int first = story_first_actor(story_scene);
        int second = story_second_actor(story_scene);
        bool first_ready = npc_approach_player(first, second, dt, scale, center,
                                            map_collision_rec, office_obstacles, OBSTACLE_COUNT,
                                            estagiario_collision_rec, target);
        bool second_ready = second < 0 || (story_action_time >= STORY_SECOND_ACTOR_DELAY &&
            npc_approach_player(second, first, dt, scale, center, map_collision_rec, office_obstacles,
                                OBSTACLE_COUNT, estagiario_collision_rec, target));
        story_action_time += dt;
        if ((!first_ready || !second_ready || player_seat == CHAIR_LEAVING) && story_action_time < STORY_APPROACH_LIMIT) return;
        if (!first_ready)
        {
            TraceLog(LOG_WARNING, "IARA: sem aproximacao do primeiro ator na cena %d", story_scene);
            npc_face_player(first, scale, estagiario_collision_rec);
        }
        if (!second_ready)
        {
            TraceLog(LOG_WARNING, "IARA: sem aproximacao do segundo ator na cena %d", story_scene);
            npc_face_player(second, scale, estagiario_collision_rec);
        }
        npc_balloon_npc = story_speaker();
        story_approaching = false;
        // A entrada deste quadro pertence à preparação. A próxima pressão já é da fala.
        story_input_guard = false;
        return;
    }
    int speaker = story_speaker();
    npc_balloon_npc = speaker;
    Vector2 look_at = current_estagiario_position;
    if (story_scene == 1 && (story_line == 1 || story_line == 3))
        look_at = (Vector2){estagiario_computer_collision_rectangle.x, estagiario_computer_collision_rectangle.y};
    if (story_scene >= CAMPAIGN_ARRIVAL &&
        (story_scene == CAMPAIGN_MIDDLE || (story_scene == CAMPAIGN_ARRIVAL && story_line == 2)))
        look_at = player_chair_anchor();
    if (story_scene == 1 || story_scene >= CAMPAIGN_ARRIVAL) npc_face_scene(speaker, look_at);
    if (story_scene == 1 && story_action_time < 0.3f)
    {
        float movement_dt = fminf(dt, 0.3f - story_action_time);
        if (story_scene == 1 && story_line == 0)
            npc_step_scene(NPC_LAURA, (Vector2){-40 * get_scale() * movement_dt, 0}, get_scale(),
                           map_collision_rec, office_obstacles, OBSTACLE_COUNT, estagiario_collision_rec);
        if (story_scene == 1 && story_line == 3)
            npc_step_scene(NPC_LAURA, (Vector2){40 * get_scale() * movement_dt, 0}, get_scale(),
                           map_collision_rec, office_obstacles, OBSTACLE_COUNT, estagiario_collision_rec);
        story_action_time += movement_dt;
    }
    const char *line = story_text();
    int length = utf8_length(line);
    npc_typed_characters = fminf(length, npc_typed_characters + dt * NPC_TYPE_SPEED);
    bool input = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    if (story_input_guard) { story_input_guard = false; return; }
    if (!input) return;
    if (npc_typed_characters < length) { npc_typed_characters = length; return; }
    int count = story_scene == 1 ? 4 : story_scene == 2 && computer_first_outcome() == TASK_TAKEN_OVER_BY_IARA ? 1 : 3;
    if (story_scene == DAY3_ARRIVAL || story_scene == DAY3_END) count = 4;
    if (story_scene == DAY3_LUNCH) count = 2;
    if (story_scene == DAY3_TRIAGE && computer_first_outcome() == TASK_TAKEN_OVER_BY_IARA) count = 1;
    if (story_scene == CAMPAIGN_ARRIVAL) count = 3;
    if (story_scene == CAMPAIGN_MIDDLE) count = computer_failsafe_triggered ? 1 : 2;
    if (story_scene == CAMPAIGN_FINISH)
    {
        int day = current_game_state_variables.current_day;
        count = day == 12 || day == 27 || day == 58 ? 3 : 2;
        if (day == 41 && computer_first_exposed_personal_data()) count = 3;
    }
    if (++story_line < count) { npc_typed_characters = 0; story_action_time = 0; return; }
    unsigned int completed = story_scene;
    if (completed >= CAMPAIGN_ARRIVAL)
        current_game_state_variables.story_later_scenes |=
            story_scene_bit(current_game_state_variables.current_day, completed - CAMPAIGN_ARRIVAL);
    else
        current_game_state_variables.story_scenes |= completed;
    computer_checkpoint_dirty = true;
    story_scene = 0;
    npc_balloon_open = 0;
    if (completed == 1)
    {
        npc_face_scene(speaker, (Vector2){GetScreenWidth()/2.0f, 0});
        npc_end_reception();
    }
    else
    {
        npc_finish_approach(story_first_actor(completed));
        int second = story_second_actor(completed);
        if (second >= 0 && second != story_first_actor(completed)) npc_finish_approach(second);
    }
    if (story_return_computer)
    {
        if (completed == CAMPAIGN_MIDDLE) computer_complete_story_middle();
        current_screen = COMPUTER_SCREEN;
    }
    else if (completed == CAMPAIGN_MIDDLE) begin_story_scene(CAMPAIGN_FINISH, false);
    else if (completed == CAMPAIGN_FINISH &&
             (computer_failsafe_triggered || current_game_state_variables.current_day == TOTAL_GAME_DAYS)) finish_day();
    else if (completed == 2) begin_story_scene(4, false);
    else if (completed == DAY3_TRIAGE) begin_story_scene(DAY3_END, false);
    else if ((completed == 4 || completed == DAY3_END) && computer_failsafe_triggered) finish_day();
}

// Balão de fala do NPC: abre deslizando de baixo com um leve quique, e a fala é digitada.
static void render_npc_dialogue(void)
{
    float target = (active_npc_id >= 0 || (story_scene && !story_approaching)) ? 1.0f : 0.0f;
    float step = IsWindowFocused() ? GetFrameTime() * NPC_BALLOON_SPEED : 0;

    if (npc_balloon_open < target) npc_balloon_open = fminf(target, npc_balloon_open + step);
    else if (npc_balloon_open > target) npc_balloon_open = fmaxf(target, npc_balloon_open - step * 1.4f);

    if (npc_balloon_open <= 0.0f) return;

    float scale = get_ui_scale();
    float panel_height = 230.0f * scale;
    float screen_height = (float)GetScreenHeight();
    float progress = npc_balloon_open;
    // Ease-out com um pequeno excesso no fim da abertura (o "quique"); fechando, só desliza
    float eased = (active_npc_id >= 0) ? (1.0f + 2.2f * powf(progress - 1.0f, 3.0f) + 1.2f * powf(progress - 1.0f, 2.0f))
                                       : progress * progress;
    float slide = (1.0f - eased) * panel_height * 1.05f;
    unsigned char alpha = (unsigned char)(255.0f * fminf(1.0f, progress * 1.6f));
    Rectangle panel = {0, screen_height - panel_height + slide, (float)GetScreenWidth(), panel_height};
    const NpcDefinition *definition = npc_get_definition(npc_balloon_npc);
    const char *line = story_scene ? story_text() : active_npc_id >= 0
        ? npc_conversation_line()
        : NULL;

    if (definition == NULL) return;

    // Painel com a faixa de cor em cima e o rabinho do balão apontando para o NPC que fala
    DrawRectangleRec(panel, (Color){18, 10, 20, (unsigned char)(225 * alpha / 255)});
    DrawRectangleRec((Rectangle){panel.x, panel.y, panel.width, 4.0f * scale}, (Color){200, 12, 20, alpha});
    draw_npc_portrait(npc_balloon_npc,
                      (Rectangle){30.0f * scale, panel.y + 22.0f * scale, 165.0f * scale, 190.0f * scale});

    float text_x = 230.0f * scale;
    DrawTextEx(default_game_font, definition->name, (Vector2){text_x, panel.y + 24.0f * scale},
               26.0f * scale, 0.0f, (Color){255, 235, 235, alpha});

    if (line == NULL) return; // Fechando: o painel só desliza para fora

    float font_size = 34.0f * scale;
    draw_typed_dialogue(line, (Vector2){text_x, panel.y + 70.0f * scale},
                        GetScreenWidth() - text_x - 60.0f * scale, font_size, (int)npc_typed_characters, WHITE);

    bool line_complete = npc_typed_characters >= (float)utf8_length(line);
    const char *continue_text = line_complete ? "ESPAÇO: CONTINUAR" : "ESPAÇO: COMPLETAR A FALA";
    float continue_size = 20.0f * scale;
    float continue_width = MeasureTextEx(default_game_font, continue_text, continue_size, 0.0f).x;
    // Pisca de leve quando a fala termina, chamando o próximo toque
    float blink = line_complete ? 0.65f + 0.35f * sinf((float)GetTime() * 6.0f) : 0.7f;

    DrawTextEx(default_game_font, continue_text,
               (Vector2){GetScreenWidth() - continue_width - 24.0f * scale,
                          panel.y + panel.height - continue_size - 10.0f * scale},
               continue_size, 0.0f, (Color){255, 255, 255, (unsigned char)(255.0f * blink)});
}

/**
 * @brief Contorno claro por dentro e escuro por fora: o par se destaca sobre qualquer fundo.
 */
static void draw_double_outline(Rectangle rec, float thickness)
{
    Rectangle outer = {rec.x - thickness, rec.y - thickness, rec.width + (thickness * 2.0f), rec.height + (thickness * 2.0f)};

    DrawRectangleLinesEx(outer, thickness, BLACK);
    DrawRectangleLinesEx(rec, thickness, WHITE);
}

/**
 * @brief Contorno tracejado: base escura contínua com traços claros por cima.
 */
static void draw_dashed_outline(Rectangle rec, float thickness)
{
    float dash = thickness * 5.0f;

    DrawRectangleLinesEx(rec, thickness, BLACK);

    for (float x = rec.x; x < rec.x + rec.width - dash; x += dash * 2.0f)
    {
        DrawRectangleRec((Rectangle){x, rec.y, dash, thickness}, WHITE);
        DrawRectangleRec((Rectangle){x, rec.y + rec.height - thickness, dash, thickness}, WHITE);
    }

    for (float y = rec.y; y < rec.y + rec.height - dash; y += dash * 2.0f)
    {
        DrawRectangleRec((Rectangle){rec.x, y, thickness, dash}, WHITE);
        DrawRectangleRec((Rectangle){rec.x + rec.width - thickness, y, thickness, dash}, WHITE);
    }
}

/**
 * @brief No alto contraste, separa os elementos do cenário pela forma do contorno, não pela cor:
 * obstáculos com linha fina, vidros com linha tracejada e objetos interativos com linha dupla.
 */
static void render_high_contrast_outlines(void)
{
    if (!is_high_contrast_enabled()) return;

    float thickness = HIGH_CONTRAST_LINE_AT_REFERENCE * get_scale();

    if (thickness < 2.0f) thickness = 2.0f;

    for (int i = 0; i < OBSTACLE_COUNT; i++)
    {
        if (i == GLASS_ROOMS_OBSTACLE || i == GLASS_DOOR_OBSTACLE) continue;

        DrawRectangleLinesEx(office_obstacles[i], thickness / 2.0f, BLACK);
    }

    draw_dashed_outline(office_obstacles[GLASS_ROOMS_OBSTACLE], thickness);
    draw_dashed_outline(get_glass_door_rectangle(), thickness);

    draw_double_outline(estagiario_computer_collision_rectangle, thickness);
    draw_double_outline(hint_item_rectangle, thickness);
}

/**
 * @brief Gerencia e executa a máquina de estados principal que controla qual tela do jogo
 * deve ser atualizada e renderizada a cada quadro (frame).
 */
static void finish_day_three_computer(void)
{
    unsigned int scenes = current_game_state_variables.story_scenes;
    if (!(scenes & DAY3_TRIAGE)) begin_story_scene(DAY3_TRIAGE, false);
    else if (!(scenes & DAY3_END)) begin_story_scene(DAY3_END, false);
    else if (computer_failsafe_triggered) finish_day();
}

static void finish_campaign_computer(void)
{
    current_game_state_variables.story_report_closed = true;
    computer_checkpoint_dirty = true;
    if (!campaign_scene_completed(STORY_MIDDLE)) begin_story_scene(CAMPAIGN_MIDDLE, false);
    else if (!campaign_scene_completed(STORY_FINISH)) begin_story_scene(CAMPAIGN_FINISH, false);
    else if (computer_failsafe_triggered || current_game_state_variables.current_day == TOTAL_GAME_DAYS) finish_day();
}

void handle_game_loop_screens(void)
{
    if (current_screen != IN_GAME_SCREEN) update_player_footstep_audio(false);

    // A trilha acompanha o momento: faixa própria no quiz, trilha abafada e som curto nos cartões de flash e salto
    if (current_screen == COMPUTER_SCREEN) audio_set_context(AUDIO_CONTEXT_QUIZ);
    else if (current_screen == DAY_TRANSITION_SCREEN)
    {
        audio_set_context(current_game_state_variables.day_type == FLASH_DAY ? AUDIO_CONTEXT_FLASH : AUDIO_CONTEXT_JUMP);
    }
    else audio_set_context(AUDIO_CONTEXT_OFFICE);

    // Fora do quadro da cena: as texturas do computador ainda seriam usadas se fossem liberadas depois de desenhar
    if (current_screen == COMPUTER_SCREEN && computer_exit_finished)
    {
        unload_computer_loop();
        computer_loop_loaded = false;
        walk_time = 0.0f;
        if (player_seat == CHAIR_SEATED) player_seat = CHAIR_LEAVING;
        current_screen = IN_GAME_SCREEN;

        // No failsafe o expediente acaba ali mesmo, sem voltar ao mapa
        if (current_game_state_variables.current_day == 1 && computer_tasks_finished)
        {
            current_game_state_variables.story_report_closed = true;
            computer_checkpoint_dirty = true;
            if (!(current_game_state_variables.story_scenes & 2)) begin_story_scene(2, false);
            else if (!(current_game_state_variables.story_scenes & 4)) begin_story_scene(4, false);
            else if (computer_failsafe_triggered) finish_day();
        }
        else if (current_game_state_variables.current_day == 3)
        {
            if (computer_tasks_finished) finish_day_three_computer();
        }
        else if (campaign_scene_day && computer_tasks_finished) finish_campaign_computer();
        else if (computer_failsafe_triggered) finish_day();
    }

    if (current_screen == COMPUTER_SCREEN && !story_scene && campaign_scene_day &&
        !campaign_scene_completed(STORY_MIDDLE) && !computer_failsafe_triggered &&
        computer_story_middle_pending())
        begin_story_scene(CAMPAIGN_MIDDLE, true);

    if (current_screen == COMPUTER_SCREEN && current_game_state_variables.current_day == 1 &&
        !(current_game_state_variables.story_scenes & 2) &&
        computer_first_outcome() != TASK_PENDING && computer_first_outcome() != TASK_TAKEN_OVER_BY_IARA)
        begin_story_scene(2, true);
    if (current_screen == COMPUTER_SCREEN && current_game_state_variables.current_day == 3)
    {
        unsigned int scenes = current_game_state_variables.story_scenes;
        int outcome = computer_first_outcome();
        if (!(scenes & DAY3_TRIAGE) && outcome != TASK_TAKEN_OVER_BY_IARA &&
            (outcome != TASK_PENDING || computer_day_three_first_retry()))
            begin_story_scene(DAY3_TRIAGE, true);
        else if (!(scenes & DAY3_LUNCH) && computer_day_three_lunch_accepted() &&
                 !computer_checkpoint_finished())
            begin_story_scene(DAY3_LUNCH, true);
    }
    if (story_scene)
    {
        keep_player_place_on_resize();
        update_obstacle_recs();
        update_collision_rec();
        update_story_scene(GetFrameTime());
        render_office_with_glass();
        render_npc_dialogue();
        return;
    }
    switch (current_screen)
    {
    case SCREEN_NONE:
        // Transição inicial automática para o cartão narrativo do dia
        current_screen = CALENDAR_DAY_CARD_SCREEN;
        break;

    case CALENDAR_DAY_CARD_SCREEN:
        // Executa a lógica e renderização da apresentação do dia
        handle_calendar_day_card();

        // Quando a animação do cartão for totalmente concluída, passa para a fase in-game
        if (render_calendar_day_card_finished)
        {
            color_to_fade.a = 0;
            current_screen = IN_GAME_SCREEN;
            if (campaign_scene_day)
            {
                if (current_game_state_variables.story_report_closed ||
                    (resume_computer_checkpoint && computer_checkpoint_finished()))
                    finish_campaign_computer();
                else if (resume_computer_checkpoint)
                {
                    current_estagiario_position = player_chair_entry();
                    if (player_seat_frames[SEAT_PLAYER_FRAME_COUNT - 1].id)
                    {
                        current_direction = NORTH;
                        current_estagiario_texture_direction = estagiario_texture[NORTH];
                        player_seat = CHAIR_SEATED;
                        player_seat_time = SEAT_PLAYER_FRAME_COUNT * SEAT_FRAME_SECONDS;
                    }
                    update_collision_rec();
                    initialize_computer_loop();
                    computer_loop_loaded = true;
                    current_screen = COMPUTER_SCREEN;
                }
                else if (!campaign_scene_completed(STORY_ARRIVAL))
                    begin_story_scene(CAMPAIGN_ARRIVAL, false);
                resume_computer_checkpoint = false;
            }
            if (current_game_state_variables.current_day == 3 && !resume_computer_checkpoint &&
                !current_game_state_variables.is_resolved &&
                !(current_game_state_variables.story_scenes & DAY3_ARRIVAL))
                begin_story_scene(DAY3_ARRIVAL, false);
            if (current_game_state_variables.current_day == 3 && resume_computer_checkpoint)
            {
                resume_computer_checkpoint = false;
                current_estagiario_position = player_chair_entry();
                update_collision_rec();
                if (computer_checkpoint_finished())
                    finish_day_three_computer();
                else
                {
                    initialize_computer_loop();
                    computer_loop_loaded = true;
                    current_screen = COMPUTER_SCREEN;
                }
            }
            if (current_game_state_variables.current_day == 1)
            {
                if (!(current_game_state_variables.story_scenes & 1)) begin_story_scene(1, false);
                else if (current_game_state_variables.story_report_closed)
                {
                    if (!(current_game_state_variables.story_scenes & 2)) begin_story_scene(2, false);
                    else if (!(current_game_state_variables.story_scenes & 4)) begin_story_scene(4, false);
                    else if (computer_failsafe_triggered) finish_day();
                }
                else if (current_game_state_variables.computer_checkpoint[0])
                {
                    if (player_seat_frames[SEAT_PLAYER_FRAME_COUNT - 1].id)
                    {
                        current_estagiario_position = player_chair_entry();
                        current_direction = NORTH;
                        current_estagiario_texture_direction = estagiario_texture[NORTH];
                        player_seat = CHAIR_SEATED;
                        player_seat_time = SEAT_PLAYER_FRAME_COUNT * SEAT_FRAME_SECONDS;
                        update_collision_rec();
                    }
                    initialize_computer_loop();
                    computer_loop_loaded = true;
                    current_screen = COMPUTER_SCREEN;
                }
            }
        }
        break;

    case IN_GAME_SCREEN:
        // Mantém a área de colisão do computador atualizada caso haja redimensionamento de janela
        estagiario_computer_collision_rectangle = get_computer_collision_rectangle();
        keep_player_place_on_resize();
        update_obstacle_recs();

        hint_item_rectangle = get_hint_item_rectangle(get_scale(), get_map_center());

        Vector2 previous_player_position = current_estagiario_position;
        bool interaction_open = is_hint_note_open() || active_npc_id >= 0;

        // Processa lógica de comandos do jogador, interações e colisões com o mapa.
        // Enquanto lê o bilhete ou conversa com um NPC, o jogador fica parado.
        if (interaction_open)
        {
            walk_time = 0.0f;
        }
        else if (IsWindowFocused())
        {
            if (player_seat == CHAIR_IDLE) handle_player_input();
            else update_player_seating(GetFrameTime());
        }

        if (!interaction_open && IsWindowFocused() && !enter_computer_screen && !leave_office)
        {
            npc_set_routine_statistics(&current_hidden_statistic);
            update_npcs(GetFrameTime(), get_scale(), get_map_center(), map_collision_rec,
                        office_obstacles, OBSTACLE_COUNT, estagiario_collision_rec);
        }

        handle_map_interactables();
        handle_map_collisions();
        bool player_moved = current_estagiario_position.x != previous_player_position.x ||
                            current_estagiario_position.y != previous_player_position.y;
        update_player_footstep_audio(player_moved && !leave_office && !enter_computer_screen);

        // ORDEM DE RENDERIZAÇÃO (Layering / Z-Index):
        // 1º O mapa de fundo (Escritório)
        // 2º NPCs, jogador, refração dos vidros e molduras em primeiro plano
        // 3º As dicas de controle no rodapé
        // 4º A seta indicativa por cima do cenário
        // 5º Os diálogos e bilhetes, por cima de tudo
        render_office_with_glass();
        render_control_tips();
        render_day_counter();
        if ((current_game_state_variables.current_day == 1 || current_game_state_variables.current_day == 3 ||
             campaign_scene_day) && !interaction_open)
        {
            const char *objective = "Vá até seu computador";
            if (can_leave_today())
            {
                int day = current_game_state_variables.current_day;
                bool can_investigate =
                    (day == 58 && !(current_game_state_variables.evidence_mask & EVIDENCE_BADGE_AFTER_LEAVING)) ||
                    (day == 74 && !(current_game_state_variables.evidence_mask & EVIDENCE_NO_HUMAN_ORDER)) ||
                    (day == 89 && !current_game_state_variables.found_nicollas_trace);
                objective = can_investigate ? "Saia do escritório ou examine o mural" : "Saia do escritório";
            }
            else if (current_game_state_variables.is_resolved) objective = "Conclua o relatório no computador";
            else if (current_game_state_variables.current_day == 3) objective = "Abra a fila no computador";
            else if (campaign_scene_day) objective = campaign_scene_day->objective;
            float ui_scale = get_ui_scale();
            DrawTextEx(default_game_font, objective, (Vector2){24 * ui_scale, 62 * ui_scale},
                       26 * ui_scale, 0, WHITE);
        }
        render_high_contrast_outlines();
        if (active_npc_id < 0 && !is_hint_note_open())
        {
            render_interact_arrow(get_next_objective_rectangle());

            if (nearby_npc_id >= 0) render_interaction_marker(npc_get_hitbox(nearby_npc_id, get_scale()), "ESPAÇO");
            else if (is_near_hint) render_interaction_marker(hint_item_rectangle, "ESPAÇO · EXAMINAR MURAL");
            else if (is_near_exit) render_interaction_marker(get_glass_door_rectangle(), "ESPAÇO");
            else if (is_near_computer && !can_leave_today()) render_interaction_marker(estagiario_computer_collision_rectangle, "ESPAÇO");
        }
        render_debug_hitboxes();
        render_npc_dialogue();
        draw_hint_note();

        // Se a interação com o computador foi acionada, abre a cena do computador.
        // O escritório continua carregado: ele é o fundo da cena.
        if (leave_office)
        {
            leave_office = false;
            play_door_sound();
            finish_day();
        }
        else if (enter_computer_screen)
        {
            initialize_computer_loop();
            play_keyboard_sound();
            computer_loop_loaded = true;
            current_screen = COMPUTER_SCREEN;
            enter_computer_screen = false;
        }
        else
        {
            // Aplica efeito de Fade In ao entrar no jogo
            bool trash;
            fade_in_texture(FAST_FADE_SPEED, &color_to_fade, &trash);
        }
        break;

    case COMPUTER_SCREEN:
        // O escritório fica ao fundo; o resto da cena é do módulo do computador
        render_computer_backdrop();
        handle_computer_loop();
        break;

    case DAY_END_CARD_SCREEN:
        handle_calendar_day_card();

        if (render_calendar_day_card_finished)
        {
            if (current_game_state_variables.current_day >= TOTAL_GAME_DAYS)
            {
                show_ending();
            }
            else
            {
                place_player_at_entrance();
                update_collision_rec();
                leave_office = false;
                advance_to_next_day();
            }
        }
        break;

    case DAY_TRANSITION_SCREEN:
        handle_day_transition();
        break;

    case ENDING_CARD_SCREEN:
        handle_calendar_day_card();

        if (render_calendar_day_card_finished)
        {
            current_game_state_variables.resolved_puzzles[TOTAL_GAME_DAYS] = true;
            game_loop_finished = true;
        }
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
    // Pausado: o jogo fica parado na última imagem e só o menu de pausa responde
    if (pause_menu_is_open())
    {
        update_iara_voice_typing(false);
        pause_action action;

        begin_game_drawing();
        action = pause_menu_update_and_draw();
        end_game_drawing();

        if (action == PAUSE_RESUME)
        {
            pause_menu_close();
        }
        else if (action == PAUSE_TO_MENU)
        {
            pause_menu_close();
            game_loop_finished = true; // A máquina de estados salva o progresso e volta ao menu
        }
        else if (action == PAUSE_QUIT)
        {
            pause_menu_close();
            game_loop_quit_requested = true; // A máquina de estados descarrega o jogo e a main.c salva o progresso e fecha a janela
        }
        return;
    }

    // ESC ou P pausam só no escritório. Dentro do computador o ESC sai do computador, e o quadro em que
    // ele sai não pode pausar também: por isso a tela é lida antes de processar o quadro.
    bool was_walking_in_office = (current_screen == IN_GAME_SCREEN);
    if (!IsWindowFocused())
    {
        story_input_guard = true;
        update_iara_voice_typing(false);
        if (story_scene) { begin_game_drawing(); render_office_with_glass(); render_npc_dialogue(); end_game_drawing(); return; }
    }

    begin_game_drawing();
    ClearBackground(BG_COLOR); // Limpa o buffer de cor com a cor padrão do jogo

    handle_game_loop_screens(); // Processa a tela ativa

    if (current_screen != COMPUTER_SCREEN)
    {
        const char *line = story_scene && !story_approaching ? story_text() :
                           active_npc_id >= 0 ? npc_conversation_line() : NULL;
        update_iara_voice_typing(IsWindowFocused() && line && npc_typed_characters < (float)utf8_length(line));
    }

    if (was_walking_in_office && current_screen == IN_GAME_SCREEN && (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)))
    {
        update_player_footstep_audio(false);
        update_iara_voice_typing(false);
        pause_menu_open(); // Guarda este quadro como fundo da pausa
    }

    end_game_drawing(); // Finaliza a renderização do frame e envia para o monitor (Swap Buffers)
}

bool is_computer_screen_open(void)
{
    return current_screen == COMPUTER_SCREEN;
}

bool is_game_loop_quit_requested(void)
{
    return game_loop_quit_requested;
}

bool is_game_loop_finished(void)
{
    return game_loop_finished;
}

/**
 * @brief Função global de encerramento do game loop.
 * Garante a liberação de todos os recursos de memória para evitar Memory Leaks ao fechar o jogo.
 */
bool game_loop_wants_replay(void)
{
    return replay_requested;
}

void game_loop_retry_ending(void)
{
    replay_requested = false;
    game_loop_finished = false;
    show_ending();
}

void unload_game_loop(void)
{
    if (!game_loop_loaded) return;

    update_player_footstep_audio(false);
    game_loop_loaded = false;
    update_iara_voice_typing(false);
    for (unsigned i = 0; i < sizeof(campaign_art) / sizeof(campaign_art[0]); i++)
    {
        if (campaign_art[i].id) UnloadTexture(campaign_art[i]);
        campaign_art[i] = (Texture2D){0};
    }

    if (computer_loop_loaded)
    {
        unload_computer_loop();
        computer_loop_loaded = false;
    }

    unload_glass();

    // Texturas do escritório e elementos gráficos
    UnloadTexture(in_game_office);
    if (chair_floor.id) UnloadTexture(chair_floor);
    if (player_chair.id) UnloadTexture(player_chair);
    for (int i = 0; i < SEAT_PLAYER_FRAME_COUNT; i++)
    {
        if (player_seat_frames[i].id) UnloadTexture(player_seat_frames[i]);
        player_seat_frames[i] = (Texture2D){0};
    }

    UnloadTexture(action_tip_texture);
    UnloadTexture(move_tip_texture);
    UnloadTexture(move_or_tip_texture);

    // Garante o descarregamento das texturas direcionais do jogador
    for (int i = 0; i < DIRECTION_COUNT; i++)
    {
        UnloadTexture(estagiario_texture[i]);
    }

    // Frames do andar
    for (int i = 0; i < DIRECTION_COUNT; i++)
    {
        for (int j = 0; j < WALK_FRAME_COUNT; j++)
        {
            UnloadTexture(walk_textures[i][j]);
        }
    }

    unload_npcs();
}
