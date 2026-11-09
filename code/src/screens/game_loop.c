#include <time.h>
#include <math.h> // Necessário para operações matemáticas avançadas, como a função sin() usada na animação do indicador
#include <stdio.h>

#include "raylib.h"
#include "game_loop.h"
#include "game_state.h"
#include "macros.h"
#include "fonts.h"
#include "animations.h"
#include "characters.h"
#include "npcs.h"
#include "collisions.h"
#include "computer_loop.h"
#include "helper.h"
#include "hints.h"
#include "accessibility.h"
#include "calendar.h"
#include "audio.h"
#include "pause_menu.h"

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
#define COMPUTER_OFFSET_Y_AT_REFERENCE 93.0f   // Deslocamento Y do computador a partir do centro da tela
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

// Sintoma visual do avanço da campanha: o escritório escurece e a água sobe (GDD 6.1).
#define OFFICE_MAX_DARKENING 90.0f
#define WATER_COLOR_RGB 20, 80, 110
#define WATER_ALPHA 85

// ============================================================================
// DIMENSÕES E OFFSETS DA HITBOX (CAIXA DE COLISÃO DO JOGADOR)
// A colisão ocorre só nos pés do personagem, para ele poder passar atrás dos móveis (a cabeça fica por cima)
// ============================================================================
#define HITBOX_OFFSET_X_AT_REFERENCE 10.0f  // Distância do topo-esquerdo do sprite até o início da hitbox (X)
#define HITBOX_OFFSET_Y_AT_REFERENCE 48.0f  // Distância do topo-esquerdo do sprite até o início da hitbox (Y)
#define HITBOX_WIDTH_AT_REFERENCE    50.0f  // Largura efetiva da área de colisão do jogador
#define HITBOX_HEIGHT_AT_REFERENCE   24.0f  // Altura da área de colisão do jogador (só os pés)
#define PLAYER_SPAWN_OFFSET_X_AT_REFERENCE -37.0f  // Deslocamento X do spawn a partir do centro da tela
#define PLAYER_SPAWN_OFFSET_Y_AT_REFERENCE 208.0f  // Deslocamento Y do spawn (logo acima da porta de entrada)

// Zona em frente à porta de vidro, por onde o jogador encerra o dia
static const Rectangle EXIT_ZONE_REFERENCE = {-80.0f, 282.0f, 156.0f, 30.0f};

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
static int active_npc_id = -1;
static int active_npc_dialogue_line = 0;
static bool hint_bubble_unlocked = false;

// Texturas gerais de interface e mapas
static Color color_to_fade = {255, 255, 255, 0};          // Cor base de renderização (RGBA) usada para aplicar efeito de opacidade/fade

// Textos da tela de transição
static const char *current_card_title;
static const char *current_card_subtitle_line_1;
static const char *current_card_subtitle_line_2;
static char card_title_text[32];                // Título do cartão com o número do dia

// Passagem entre os dias
static float transition_time = 0.0f;            // Segundos no dia de flash ou de salto atual
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
    [ENDING_RUPTURE] = {"Final D: Ruptura", "A prova fecha, e o trato de Ricardo Marés não se sustenta.", "Quem ainda está desperto abre o porão com você. O rio volta a correr."}};

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
 * @brief Prepara o cartão narrativo com os textos indicados, para aparecer do início.
 */
static void start_day_card(const char *title, const char *line_1, const char *line_2)
{
    current_card_title = title;
    current_card_subtitle_line_1 = line_1;
    current_card_subtitle_line_2 = line_2;
    current_calendar_day_state = NONE;
    render_calendar_day_card_finished = false;
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
static Rectangle get_next_objective_rectangle(void)
{
    return current_game_state_variables.is_resolved ? get_glass_door_rectangle() : estagiario_computer_collision_rectangle;
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
    current_game_state_variables.resolved_puzzles[day] = true;
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
    walk_time = 0.0f;
}

/**
 * @brief Abre um dia jogável: chamados do dia, bilhete do mural e cartão de chegada.
 */
static void open_playable_day(void)
{
    int day = current_game_state_variables.current_day;

    hint_bubble_unlocked = false;

    // Quem perdeu o bilhete do Dia 1 não fica sem a colinha nos puzzles seguintes (RF07)
    if (day > 1) current_game_state_variables.has_cheat_sheet = true;

    reset_computer_progress();
    set_hint_note_lines(calendar_get_note_lines(day), CALENDAR_NOTE_LINE_COUNT);
    color_to_fade.a = 0;
    snprintf(card_title_text, sizeof(card_title_text), "Dia %d", day);
    start_day_card(card_title_text, calendar_get_arrival_line(day, 0), calendar_get_arrival_line(day, 1));
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
        open_playable_day();
        return;
    }

    if (current_game_state_variables.day_type == JUMP_DAY)
    {
        jump_day_time = JUMP_BLOCK_TIME / calendar_get_jump_block_length(day);
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
    const EndingCard *card = &ending_cards[get_game_ending()];

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
    if (day == 58) current_game_state_variables.evidence_mask |= EVIDENCE_BADGE_AFTER_LEAVING;
    if (day == 74) current_game_state_variables.evidence_mask |= EVIDENCE_NO_HUMAN_ORDER;
    if (day == 89) current_game_state_variables.found_mariana = true;

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
    game_loop_quit_requested = false;
    leave_office = false;
    enter_computer_screen = false;
    nearby_npc_id = -1;
    active_npc_id = -1;
    active_npc_dialogue_line = 0;
    hint_bubble_unlocked = false;
    walk_time = 0.0f;
    color_to_fade.a = 0;

    unload_glass();
    glass_shader = LoadShaderFromMemory(NULL, glass_fragment_shader);
    glass_panel_loc = GetShaderLocation(glass_shader, "panel");
    glass_size_loc = GetShaderLocation(glass_shader, "size");

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
void render_calendar_day_card(void)
{
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
 * @brief Verifica interações do jogador com objetos do cenário (computador).
 * 
 * Checa a sobreposição entre a caixa de colisão do jogador e cada objeto. Com a tecla ESPAÇO,
 * o computador abre a tela do quiz e o mural abre o bilhete com a dica; com o bilhete aberto,
 * ESPAÇO apenas o fecha. Examinar o bilhete não reduz a pontuação (US04). Depois de concluir
 * a tarefa, o computador deixa de abrir e a porta de vidro encerra o dia.
 */
void handle_map_interactables(void)
{
    // Checa colisão AABB (Axis-Aligned Bounding Box) entre o jogador e o computador
    is_near_computer = CheckCollisionRecs(estagiario_collision_rec, estagiario_computer_collision_rectangle);
    is_near_hint = calendar_has_investigation(current_game_state_variables.current_day) &&
                   is_hitbox_on_hint(estagiario_collision_rec, get_scale(), get_map_center());
    is_near_exit = current_game_state_variables.is_resolved && CheckCollisionRecs(estagiario_collision_rec, get_exit_zone());
    nearby_npc_id = npc_get_nearby(estagiario_collision_rec, get_scale());

    if (active_npc_id >= 0)
    {
        if (IsKeyPressed(KEY_SPACE))
        {
            active_npc_dialogue_line++;
            if (active_npc_dialogue_line >=
                npc_get_dialogue_line_count(active_npc_id, current_game_state_variables.current_day))
            {
                active_npc_id = -1;
                active_npc_dialogue_line = 0;
            }
        }
        return;
    }

    if (!IsKeyPressed(KEY_SPACE)) return;

    if (is_hint_note_open())
    {
        toggle_mural_note();
    }
    else if (nearby_npc_id >= 0)
    {
        active_npc_id = nearby_npc_id;
        active_npc_dialogue_line = 0;
    }
    else if (is_near_hint)
    {
        toggle_mural_note();
    }
    else if (is_near_exit)
    {
        leave_office = true;
    }
    else if (is_near_computer && !current_game_state_variables.is_resolved)
    {
        // Se estiver ao alcance e pressionar ESPAÇO, ativa a flag para abrir o computador
        enter_computer_screen = true;
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

/**
 * @brief Desenha o sprite do jogador na tela utilizando as dimensões e rotação adequadas.
 */
void render_player(void)
{
    float size = get_player_size();

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

    // Desenha a textura na tela com suporte a tint de opacidade (color_to_fade)
    DrawTexturePro(texture, source, dest, (Vector2){0, 0}, 0.0f, color_to_fade);
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

    // Sem nenhum número na tela: quanto mais tarde na campanha, mais escuro e mais alagado
    float water_level = calendar_get_water_level(current_game_state_variables.current_day);
    unsigned char brightness = (unsigned char)(255.0f - (OFFICE_MAX_DARKENING * water_level));
    Color office_tint = {brightness, brightness, brightness, color_to_fade.a};

    DrawTexturePro(in_game_office, source, dest, (Vector2){0, 0}, 0.0f, office_tint);

    if (water_level > 0.0f)
    {
        float water_height = dest.height * water_level;
        unsigned char water_alpha = (unsigned char)((WATER_ALPHA * color_to_fade.a) / 255);

        DrawRectangleRec((Rectangle){dest.x, dest.y + dest.height - water_height, dest.width, water_height}, (Color){WATER_COLOR_RGB, water_alpha});
    }
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
        draw_transition_text(TextFormat("Dia %d", day), center_y, TITLE_FONT_SIZE * scale, WHITE);

        if (transition_time >= jump_day_time) advance_to_next_day();
        return;
    }

    color_to_fade.a = 255;
    render_game_loop_escritorio();
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){0, 0, 0, FLASH_DIM_ALPHA});
    draw_transition_text(TextFormat("Dia %d", day), center_y - (SUBTITLE_FONT_SIZE * scale * 3.0f), TITLE_FONT_SIZE * scale, WHITE);
    draw_transition_text(calendar_get_flash_text(day), center_y, SUBTITLE_FONT_SIZE * scale, WHITE);

    if (transition_time < FLASH_MIN_TIME) return;

    draw_transition_text("ESPAÇO ou clique para continuar", center_y + (SUBTITLE_FONT_SIZE * scale * 3.0f), SUBTITLE_FONT_SIZE * scale * 0.7f, GRAY);

    if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) advance_to_next_day();
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
    draw_npcs(get_scale(), color_to_fade, current_game_state_variables.current_day,
              current_estagiario_position.y, true, hint_bubble_unlocked);
    render_player();
    draw_npcs(get_scale(), color_to_fade, current_game_state_variables.current_day,
              current_estagiario_position.y, false, hint_bubble_unlocked);
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
    render_office_scene();
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
            SetShaderValue(glass_shader, glass_panel_loc, &pane, SHADER_UNIFORM_VEC4);
            SetShaderValue(glass_shader, glass_size_loc, &size, SHADER_UNIFORM_VEC2);
            BeginShaderMode(glass_shader);
            DrawTextureRec(glass_scene.texture, source, (Vector2){0}, color_to_fade);
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

/**
 * @brief Desenha a seta amarela indicativa flutuando acima de um objeto interativo.
 * 
 * Utiliza a função matemática harmônica `sinf()` multiplicada pelo tempo do jogo (`GetTime()`)
 * para criar um efeito senoidal fluido de oscilação ("quicar"/bounce para cima e para baixo).
 *
 * @param target Objeto em coordenadas da tela.
 */
void render_interact_arrow(Rectangle target)
{
    float scale = get_scale();
    
    // Cálculo do desvio de flutuação vertical usando onda senoidal
    float bounce_offset = sinf(GetTime() * 6.0f) * (10.0f * scale);

    float arrow_width  = arrow_down_texture.width  * scale;
    float arrow_height = arrow_down_texture.height * scale;

    // Posição centralizada horizontalmente no topo da caixa de colisão do computador
    float arrow_x = target.x
                  + target.width / 2.0f
                  - arrow_width / 2.0f;
    
    // Posição vertical logo acima do computador + offset animado da onda senoidal
    float arrow_y = target.y
                  - arrow_height - (10.0f * scale) + bounce_offset;

    Rectangle source = {0, 0, (float)arrow_down_texture.width, (float)arrow_down_texture.height};
    Rectangle dest   = {arrow_x, arrow_y, arrow_width, arrow_height};

    DrawTexturePro(arrow_down_texture, source, dest, (Vector2){0, 0}, 0.0f, color_to_fade);
}

static void draw_npc_dialogue_line(const char *text, Vector2 position, float max_width,
                                   float font_size, float letter_spacing)
{
    char line[256] = "";
    const char *word_start = text;

    while (*word_start != '\0')
    {
        while (*word_start == ' ') word_start++;
        if (*word_start == '\0') break;

        const char *word_end = word_start;
        while (*word_end != '\0' && *word_end != ' ') word_end++;
        size_t word_length = (size_t)(word_end - word_start);
        char word[256];
        if (word_length >= sizeof(word)) word_length = sizeof(word) - 1;
        snprintf(word, sizeof(word), "%.*s", (int)word_length, word_start);

        char candidate[256];
        if (line[0] == '\0') snprintf(candidate, sizeof(candidate), "%s", word);
        else snprintf(candidate, sizeof(candidate), "%s %s", line, word);

        if (line[0] != '\0' &&
            MeasureTextEx(default_game_font, candidate, font_size, letter_spacing).x > max_width)
        {
            DrawTextEx(default_game_font, line, position, font_size, letter_spacing, WHITE);
            position.y += font_size * 1.15f;
            snprintf(line, sizeof(line), "%s", word);
        }
        else
        {
            snprintf(line, sizeof(line), "%s", candidate);
        }

        word_start = word_end;
    }

    if (line[0] != '\0') DrawTextEx(default_game_font, line, position, font_size, letter_spacing, WHITE);
}

static void render_npc_dialogue(void)
{
    if (active_npc_id < 0) return;

    float scale = get_ui_scale();
    float panel_height = 230.0f * scale;
    float screen_height = (float)GetScreenHeight();
    Rectangle panel = {0, screen_height - panel_height, (float)GetScreenWidth(), panel_height};
    const NpcDefinition *definition = npc_get_definition(active_npc_id);
    const char *line = npc_get_dialogue_line(active_npc_id, current_game_state_variables.current_day,
                                             active_npc_dialogue_line);
    if (definition == NULL || line == NULL) return;

    DrawRectangleRec(panel, (Color){18, 10, 20, 225});
    draw_npc_portrait(active_npc_id,
                      (Rectangle){30.0f * scale, panel.y + 22.0f * scale, 165.0f * scale, 190.0f * scale});

    float font_size = 34.0f * scale;
    float text_x = 230.0f * scale;
    DrawTextEx(default_game_font, definition->name, (Vector2){text_x, panel.y + 24.0f * scale},
               26.0f * scale, 0.0f, RAYWHITE);
    draw_npc_dialogue_line(line, (Vector2){text_x, panel.y + 70.0f * scale},
                           GetScreenWidth() - text_x - 60.0f * scale, font_size, 0.0f);

    const char *continue_text = "ESPAÇO: CONTINUAR";
    float continue_size = 20.0f * scale;
    float continue_width = MeasureTextEx(default_game_font, continue_text, continue_size, 0.0f).x;
    DrawTextEx(default_game_font, continue_text,
               (Vector2){GetScreenWidth() - continue_width - 24.0f * scale,
                          panel.y + panel.height - continue_size - 10.0f * scale},
               continue_size, 0.0f, RAYWHITE);
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
void handle_game_loop_screens(void)
{
    if (current_screen != IN_GAME_SCREEN) update_player_footstep_audio(false);

    // Fora do quadro da cena: as texturas do computador ainda seriam usadas se fossem liberadas depois de desenhar
    if (current_screen == COMPUTER_SCREEN && computer_exit_finished)
    {
        unload_computer_loop();
        computer_loop_loaded = false;
        walk_time = 0.0f;
        current_screen = IN_GAME_SCREEN;

        // No failsafe o expediente acaba ali mesmo, sem voltar ao mapa
        if (computer_failsafe_triggered) finish_day();
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
        // Enquanto lê o bilhete, o jogador fica parado.
        if (is_hint_note_open())
        {
            walk_time = 0.0f;
        }
        else
        {
            handle_player_input();
        }

        if (!interaction_open)
        {
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
        render_high_contrast_outlines();
        render_interact_arrow(get_next_objective_rectangle());
        if (is_near_hint) render_interact_arrow(hint_item_rectangle);
        if (nearby_npc_id >= 0 && active_npc_id < 0)
        {
            render_interact_arrow(npc_get_hitbox(nearby_npc_id, get_scale()));
        }
        render_debug_hitboxes();
        render_npc_dialogue();
        draw_hint_note();

        // Se a interação com o computador foi acionada, abre a cena do computador.
        // O escritório continua carregado: ele é o fundo da cena.
        if (leave_office)
        {
            leave_office = false;
            finish_day();
        }
        else if (enter_computer_screen)
        {
            initialize_computer_loop();
            computer_loop_loaded = true;
            hint_bubble_unlocked = true;
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

    begin_game_drawing();
    ClearBackground(BG_COLOR); // Limpa o buffer de cor com a cor padrão do jogo

    handle_game_loop_screens(); // Processa a tela ativa

    if (was_walking_in_office && current_screen == IN_GAME_SCREEN && (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)))
    {
        update_player_footstep_audio(false);
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
void unload_game_loop(void)
{
    if (!game_loop_loaded) return;

    update_player_footstep_audio(false);
    game_loop_loaded = false;

    if (computer_loop_loaded)
    {
        unload_computer_loop();
        computer_loop_loaded = false;
    }

    unload_glass();

    // Texturas do escritório e elementos gráficos
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
