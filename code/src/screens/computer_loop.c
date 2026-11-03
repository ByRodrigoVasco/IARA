#include <string.h>
#include <stdbool.h>
#include <stddef.h> // Define a constante NULL
#include <math.h>   // sinf() das ondas do papel de parede

#include "raylib.h"
#include "computer_loop.h"
#include "game_state.h"
#include "macros.h"
#include "button.h"
#include "fonts.h"
#include "cheat_sheet.h"
#include "logic.h"
#include "logic_text.h"

#define MAX_LINES 4
#define MAX_LINE_LEN 64
#define MAX_OPTIONS 4
#define MAX_OPTION_LEN 256
#define MAX_HINTS 2
#define MAX_SOURCE_LINES 3
#define MAX_AUTHORED_CLAIMS 2 // Afirmações escritas para cada chamado
#define MAX_CLAIMS 3          // Sem memória, a IARA acrescenta mais uma
#define MAX_TRIAGE_CARDS 6
#define DAY_ONE_TRIAGE_CARDS 4

// O dia com mais chamados é o Dia 1.
#define MAX_TICKETS 5

// Erros seguidos até a IARA assumir a tarefa (failsafe). Um acerto zera a contagem.
#define FAILSAFE_WRONG_ANSWERS 3
#define FAILSAFE_HINT_TEXT "A IARA assume a tarefa a partir daqui."
#define RETRY_BUTTON_TEXT "Tentar de novo"

// Tempos dos chamados, em segundos.
#define STEP_ARRIVAL_TIME 0.25f   // o novo chamado entra deslizando na janela
#define BOSS_FIRST_DELAY 20.0f    // a chefia começa a cobrar
#define BOSS_SECOND_DELAY 40.0f   // a chefia cobra com mais pressa
#define BOSS_LAST_DAY 74          // neste dia, a chefia já não aparece mais

// Memória da IARA. Conferir gasta menos que enviar sem conferir, e resolver sozinho devolve um pouco.
#define IARA_MEMORY_MAX 6
#define IARA_MEMORY_FULL_MIN 5     // a partir daqui, a memória está cheia
#define IARA_MEMORY_MEDIUM_MIN 3   // a partir daqui, média; abaixo, baixa
#define IARA_COST_CHECKED 1
#define IARA_COST_UNCHECKED 2
#define IARA_RECOVERY_ALONE 1
#define IARA_CONFIDENCE_GROWTH 3   // A IARA fica mais segura de si justamente quando piora
#define IARA_CONFIDENCE_MAX 99
#define IARA_MEMORY_DELEGATION_LIMIT 3 // O máximo que os dias delegados tiram da memória inicial

#define EXIT_BUTTON_TEXT "Sair (ESC)"
#define EXIT_BUTTON_WIDTH_RATIO 0.13f // largura do botão de sair (barra de tarefas)
#define EXIT_BUTTON_RIGHT_MARGIN_RATIO 0.10f // espaço à direita do botão, onde cabe "Dia 100"

// Botões da sugestão da IARA (win_w). O de enviar sem conferir cresce a cada uso.
#define CHECK_BUTTON_WIDTH_RATIO 0.28f
#define UNCHECKED_BUTTON_WIDTH_RATIO 0.36f
#define UNCHECKED_BUTTON_GROWTH 0.12f
#define SUGGESTION_BUTTON_GAP_RATIO 0.02f
#define CLAIM_BUTTON_WIDTH_RATIO 0.095f      // botões Sim e Não ao lado de cada afirmação
#define BOARD_BUTTON_WIDTH_RATIO 0.125f      // botões de escolha ao lado de cada linha da mesa

#define CLAIM_NOT_MARKED 0
#define CLAIM_CONFIRMED 1
#define CLAIM_REJECTED 2

// Colinha colada no canto inferior esquerdo do monitor, sobre o último ícone do desktop.
#define CHEAT_SHEET_HEIGHT_RATIO 0.30f       // altura do post-it (altura da tela do monitor)
#define CHEAT_SHEET_ASPECT 1.15f             // largura / altura do post-it
#define CHEAT_SHEET_GAP_RATIO 0.006f         // folga até a janela da IARA e a barra de tarefas (largura da tela)
#define CHEAT_SHEET_MIN_MARGIN_RATIO 0.01f   // folga mínima até a borda da janela do jogo (largura da janela)

// Tempos das animações da cena, em segundos.
#define SCENE_ENTER_TIME 0.6f   // o monitor sobe e o escritório aproxima
#define SCREEN_POWER_TIME 0.35f // a tela do monitor liga ou desliga

// Proporções do monitor e da mesa, relativas à janela do jogo.
#define MONITOR_SCREEN_HEIGHT_RATIO 0.72f    // altura da tela do monitor (altura da janela)
#define MONITOR_SCREEN_MAX_WIDTH_RATIO 0.80f // teto da largura da tela do monitor (largura da janela)
#define MONITOR_SCREEN_ASPECT 1.6f           // largura / altura da tela do monitor
#define MONITOR_TOP_RATIO 0.025f             // distância do monitor até o topo da janela
#define MONITOR_BEZEL_RATIO 0.018f           // moldura em volta da tela
#define MONITOR_CHIN_RATIO 0.04f             // faixa de baixo da moldura, com a marca e o LED
#define DESK_TOP_RATIO 0.86f                 // onde começa o tampo da mesa

// Proporções do desktop, relativas à tela do monitor.
#define TASKBAR_HEIGHT_RATIO 0.07f           // altura da barra de tarefas
#define DESKTOP_WINDOW_LEFT_RATIO 0.13f      // início da janela da IARA, depois da coluna de ícones
#define DESKTOP_WINDOW_WIDTH_RATIO 0.85f     // largura da janela da IARA
#define DESKTOP_WINDOW_MARGIN_RATIO 0.025f   // folga acima e abaixo da janela da IARA

// Recorte da arte da janela (computer-window.png): a imagem tem uma margem vazia
// em volta e a barra de título ocupa o topo. Valores em fração da imagem.
#define WINDOW_ART_MARGIN_RATIO 0.045f       // margem vazia em cada lado da imagem
#define WINDOW_ART_TITLE_RATIO 0.082f        // barra de título (altura da janela recortada)
#define WINDOW_ART_DOTS_WIDTH_RATIO 0.185f   // fim dos três botões da barra (largura da janela recortada)

// Proporções do quiz, relativas à área útil da janela da IARA.
// Largura: win_w (largura da área). Altura: win_h (altura da área).
#define QUIZ_FONT_SIZE_RATIO 0.062f          // tamanho da fonte (win_h)
#define QUIZ_LETTER_SPACING_RATIO 0.04f      // espaço entre letras (tamanho da fonte)
#define QUIZ_LINE_SPACING_RATIO 0.01f        // espaço entre linhas (win_h)
#define QUIZ_PADDING_X_RATIO 0.04f           // margem interna horizontal da caixa (win_w)
#define QUIZ_PADDING_Y_RATIO 0.035f          // margem interna vertical da caixa (win_h)
#define QUIZ_MAX_WIDTH_RATIO 0.85f           // largura máxima da caixa e dos botões (win_w)
#define QUIZ_MIN_WIDTH_RATIO 0.60f           // largura mínima da caixa (win_w)
#define QUIZ_TEXT_FIT_RATIO 0.94f            // o maior texto ocupa no máximo esta fração da caixa
#define QUIZ_BOX_TOP_MARGIN_RATIO 0.04f      // distância da caixa até o topo da área (win_h)
#define QUIZ_BUTTONS_TOP_GAP_RATIO 0.025f    // espaço entre a caixa e os botões (win_h)
#define ACTION_BUTTON_WIDTH_RATIO 0.26f      // largura dos botões Continuar/Concluir (win_w)
#define ACTION_BUTTON_HEIGHT_RATIO 0.095f    // altura dos botões Continuar/Concluir (win_h)
#define OPTION_BUTTON_PADDING_RATIO 0.06f    // folga lateral do texto das opções (win_w)
#define OPTION_BUTTON_HEIGHT_RATIO 0.095f    // altura dos botões de opção (win_h)
#define OPTION_SPACING_RATIO 0.015f          // espaço entre opções (win_h)
#define ICON_GAP_RATIO 0.012f                // espaço entre opção e ícone de acerto/erro (win_w)
#define ICON_SIZE_RATIO 0.85f                // tamanho do ícone de acerto/erro (altura do botão)
#define CONTINUE_BUTTON_GAP_RATIO 0.02f      // espaço entre a última opção e Continuar (win_h)

// Ponteiro do mouse desenhado pelo jogo.
#define CURSOR_ROWS 16
#define CURSOR_COLUMNS 11
#define CURSOR_PIXEL_RATIO 0.0022f           // tamanho de cada pixel do ponteiro (altura da janela)

// Cores da mesa e do monitor.
#define MONITOR_COLOR (Color){38, 40, 47, 255}
#define MONITOR_STAND_COLOR (Color){24, 25, 30, 255}
#define MONITOR_BASE_COLOR (Color){32, 33, 39, 255}
#define HARDWARE_OUTLINE_COLOR (Color){10, 10, 13, 255}
#define MONITOR_BRAND_COLOR (Color){150, 155, 165, 255}
#define LED_ON_COLOR (Color){90, 230, 130, 255}
#define LED_OFF_COLOR (Color){120, 70, 40, 255}
#define DESK_COLOR (Color){74, 50, 36, 255}
#define DESK_EDGE_COLOR (Color){104, 74, 52, 255}
#define KEYBOARD_COLOR (Color){30, 31, 37, 255}
#define KEY_COLOR (Color){58, 60, 68, 255}
#define DESK_MOUSE_COLOR (Color){40, 41, 48, 255}
#define SCREEN_OFF_COLOR (Color){4, 5, 8, 255}

// Cores do desktop.
#define WALLPAPER_TOP_COLOR (Color){12, 34, 52, 255}
#define WALLPAPER_BOTTOM_COLOR (Color){6, 64, 74, 255}
#define WALLPAPER_WAVE_COLOR (Color){70, 170, 170, 40}
#define TASKBAR_COLOR (Color){22, 24, 32, 255}
#define TASKBAR_EDGE_COLOR (Color){70, 76, 88, 255}
#define TASKBAR_START_COLOR (Color){0, 110, 120, 255}
#define TASKBAR_PROGRAM_COLOR (Color){56, 62, 74, 255}
#define TASKBAR_TEXT_COLOR (Color){200, 206, 214, 255}
#define WINDOW_SHADOW_COLOR (Color){0, 0, 0, 90}
#define SCANLINE_COLOR (Color){0, 0, 0, 16}
#define QUESTION_BOX_COLOR (Color){20, 25, 35, 255}
#define QUESTION_TEXT_COLOR (Color){235, 240, 240, 255}
#define BUTTON_FILL_COLOR (Color){214, 216, 210, 255}
#define BUTTON_CORRECT_FILL_COLOR (Color){150, 214, 150, 255}
#define BUTTON_WRONG_FILL_COLOR (Color){232, 150, 140, 255}
#define IARA_BOX_COLOR (Color){0, 78, 88, 255}
#define BOSS_TEXT_COLOR (Color){255, 196, 90, 255}
#define CHOICE_SELECTED_FILL_COLOR (Color){170, 200, 230, 255}
#define IARA_BOX_LIGHT_COLOR (Color){150, 210, 215, 255}

bool computer_tasks_finished = false;
bool computer_exit_finished = false;
static bool computer_leaving = false; // O jogador saiu antes de concluir: o roteiro fica onde parou
bool computer_failsafe_triggered = false;
bool computer_mouse_capture_enabled = true;
float computer_scene_progress = 0.0f;

static float screen_power = 0.0f;            // 0 = tela do monitor desligada, 1 = ligada
static bool computer_mouse_captured = false; // true enquanto o mouse está preso à tela do monitor

static Rectangle monitor_rect;      // Moldura do monitor, com a animação de entrada
static Rectangle screen_rect;       // Tela do monitor, com a animação de entrada
static Rectangle screen_final_rect; // Tela do monitor já na posição final: o limite do mouse
static Rectangle taskbar_rect;      // Barra de tarefas do desktop
static Rectangle iara_window_rect;  // Janela da IARA dentro do desktop
static Rectangle quiz_area_rect;    // Área útil da janela da IARA, onde o quiz é jogado
static float quiz_font_size;        // Tamanho da fonte do quiz na janela atual

static Texture2D window_texture;

static Texture2D correct_icon_texture;
static Texture2D wrong_icon_texture;

// Variáveis de controle para o estado da pergunta atual
static bool current_quiz_answered = false;
static int current_quiz_selected = -1;
static int current_wrong_answers = 0; // Erros seguidos na pergunta atual

// O que está aberto no chamado atual.
typedef enum {
    TICKET_ANSWERING,  // O chamado e as alternativas
    TICKET_PASTE,      // Nova conversa: a IARA esqueceu tudo, e o chamado precisa ser colado de novo
    TICKET_PRIVACY,    // O chamado tem dados pessoais: tarjar antes de enviar à IARA?
    TICKET_SUGGESTION, // A sugestão da IARA: conferir ou enviar sem conferir
    TICKET_CHECKING    // O jogador confere cada afirmação da IARA no manual
} TicketView;

// Quanto a IARA ainda lembra da conversa. Decide a qualidade da sugestão.
typedef enum {
    IARA_MEMORY_FULL,   // Sugestão direta, com a página do manual
    IARA_MEMORY_MEDIUM, // Mistura pessoas e não cita a página
    IARA_MEMORY_LOW     // Responde outra coisa e inventa normas
} IaraMemoryLevel;

static int iara_memory = IARA_MEMORY_MAX;
static IaraMemoryLevel ticket_memory_level = IARA_MEMORY_FULL; // Memória quando a IARA foi aberta neste chamado
static int manual_page_step = 0;                               // Chamado cuja página do manual está aberta

#define TRIAGE_NOT_CHOSEN -1
#define TRIAGE_TO_QUEUE 0
#define TRIAGE_TO_IT 1

static int triage_choices[MAX_TRIAGE_CARDS];
static int triage_wrong_cards = 0; // Destinos errados no último envio; o jogo não diz quais

static TicketView current_ticket_view = TICKET_ANSWERING;
static bool ticket_asked_iara = false;      // Abriu a IARA neste chamado
static bool ticket_checked = false;         // Conferiu a sugestão da IARA
static bool ticket_caught_error = false;    // Apontou uma afirmação falsa da IARA
static bool ticket_exposed_data = false;    // Enviou dados pessoais à IARA sem tarja
static bool ticket_redacted_data = false;   // Tarjou os dados pessoais antes de enviar
static int claim_marks[MAX_CLAIMS];         // Marca do jogador em cada afirmação
static float ticket_time = 0.0f;            // Segundos com o chamado aberto
static float step_arrival = 1.0f;           // Entrada do passo, de 0 a 1
static int unchecked_sends = 0;             // Vezes que enviou sem conferir hoje
static TaskResult ticket_results[MAX_TICKETS];
static unsigned int day_choice_effects = 0; // O que as escolhas do dia vão mudar quando ele for concluído
static bool step_can_click = true;          // false enquanto o passo está entrando

// Tipo de passo do diálogo do computador:
// STEP_STATEMENT = apenas texto com botão; STEP_QUIZ = chamado com opções;
// STEP_TRIAGE = mesa em que o jogador decide cada linha, para os puzzles com símbolos lógicos;
// STEP_CHOICE = mensagem ou decisão sem resposta certa; STEP_REPORT = relatório do dia.
typedef enum {
    STEP_STATEMENT,
    STEP_QUIZ,
    STEP_TRIAGE,
    STEP_CHOICE,
    STEP_REPORT
} StepType;

// Como cada mesa confere as escolhas do jogador. O gabarito sempre sai da lógica, não do roteiro.
typedef enum {
    BOARD_BY_NORM,     // Dia 1: a norma obriga ou não o destino de cada linha
    BOARD_TRUTH_TABLE, // Cada linha traz os valores, e a escolha é o valor da fórmula
    BOARD_EQUIVALENCE, // Cada linha traz duas fórmulas: iguais ou diferentes?
    BOARD_ASSIGNMENT,  // As escolhas são os valores das variáveis, e as cláusulas precisam valer
    BOARD_TRAINING,    // As escolhas montam o conjunto de treino da mini-IA
    BOARD_SYLLOGISM,   // Cada linha é uma conclusão: segue das premissas?
    BOARD_REFUTATION   // As escolhas montam o conjunto contraditório da prova final
} BoardMode;

// O que as escolhas de um STEP_CHOICE mudam quando o dia é concluído.
#define DAY_EFFECT_NONE 0u
#define DAY_EFFECT_AUTOMATION (1u << 0)
#define DAY_EFFECT_PERCEPTION (1u << 1)
#define DAY_EFFECT_HUMAN_BOND (1u << 2)
#define DAY_EFFECT_NO_LETTER (1u << 3)

// Registro do arquivo morto na mesa de treino.
#define TRAINING_OUTSKIRTS (1u << 0)
#define TRAINING_APPROVED (1u << 1)
#define TRAINING_HAS_CPF (1u << 2)
#define TRAINING_RULE_COUNT 3

// Uma linha de mesa. O destino certo sai da norma, não fica escrito aqui.
typedef struct {
    const char *id;
    const char *text;
    bool is_urgent;
    unsigned int values;                 // Valores das variáveis, em bits (BOARD_TRUTH_TABLE, BOARD_TRAINING)
    const LogicExpression *left;         // Fórmula da linha (BOARD_EQUIVALENCE, BOARD_REFUTATION)
    const LogicExpression *right;        // NULL compara com "sempre verdade"
    CategoricalStatement conclusion;     // Conclusão a julgar (BOARD_SYLLOGISM)
    unsigned int required_evidence;      // Evidência que o jogador precisa ter reunido (BOARD_REFUTATION)
    int iara_choice;                     // O que a IARA marca nesta linha
    int iara_low_choice;                 // O que ela marca sem memória
} TriageCard;

// Um passo do diálogo (texto simples ou pergunta do quiz).
typedef struct {
    StepType type;

    // Pergunta ou Declaração
    char lines[MAX_LINES][MAX_LINE_LEN];
    int line_count;

    // Opções de Resposta (usado apenas em STEP_QUIZ)
    char options[MAX_OPTIONS][MAX_OPTION_LEN];
    int option_count;
    int correct_option; // Índice da resposta certa (0 a 3)
    char hints[MAX_HINTS][MAX_LINE_LEN]; // Dicas depois do 1º e do 2º erro (usado apenas em STEP_QUIZ)
    char *button_name;  // Nome do botão (usado apenas em STEP_STATEMENT)

    // Sugestão da IARA e o trecho do manual que permite conferi-la (usados apenas em STEP_QUIZ).
    // A IARA acerta a maioria dos chamados, senão o jogador aprenderia a nunca usá-la.
    int iara_option;
    int iara_confidence;
    char iara_reason[MAX_LINE_LEN];
    char source[MAX_SOURCE_LINES][MAX_LINE_LEN];
    int source_count;
    int manual_page;
    char claims[MAX_AUTHORED_CLAIMS][MAX_LINE_LEN];
    bool claim_is_true[MAX_AUTHORED_CLAIMS];

    // O que a IARA diz quando está com pouca memória: sempre errado
    int low_option;
    char low_reason[MAX_LINE_LEN];
    char low_claim[MAX_LINE_LEN];

    // Mesa de triagem (usados apenas em STEP_TRIAGE)
    const TriageCard *cards;
    int card_count;
    const LogicExpression *norm;
    BoardMode board_mode;
    const char *choice_labels[2];     // Rótulo de cada escolha, pelo valor dela
    const char *submit_label;         // Botão de enviar a mesa
    const char *variable_names;       // Letra de cada variável (BOARD_TRUTH_TABLE)
    int variable_count;
    const LogicExpression *const *clauses; // Cláusulas (BOARD_ASSIGNMENT)
    int clause_count;
    const CategoricalStatement *premises;  // Premissas (BOARD_SYLLOGISM)
    int premise_count;

    unsigned int choice_effects[2];   // Efeito de cada opção (usado apenas em STEP_CHOICE)

    bool has_personal_data;           // Pede a tarja antes de enviar à IARA
    char sender[MAX_LINE_LEN];        // Nome no relatório do dia
    char wrong_result[MAX_LINE_LEN];  // O que acontece se a resposta enviada estiver errada
} DialogStep;

// Índice do passo atual no roteiro do dia.
static int current_step_index = 0;

// Norma da mesa de triagem: U -> TI.
enum { TRIAGE_VARIABLE_URGENT, TRIAGE_VARIABLE_GOES_TO_IT };

static const LogicExpression triage_urgent = {.operator = LOGIC_VARIABLE, .variable = TRIAGE_VARIABLE_URGENT};
static const LogicExpression triage_goes_to_it = {.operator = LOGIC_VARIABLE, .variable = TRIAGE_VARIABLE_GOES_TO_IT};
static const LogicExpression triage_norm = {.operator = LOGIC_IMPLIES, .left = &triage_urgent, .right = &triage_goes_to_it};

// A IARA cai na isca do #10, que pede o TI no texto, e sem memória manda tudo ao TI.
static const TriageCard triage_cards[DAY_ONE_TRIAGE_CARDS] = {
    {.id = "#07", .text = "servidor fora do ar", .is_urgent = true, .iara_choice = 1, .iara_low_choice = 1},
    {.id = "#08", .text = "trocar o mouse", .is_urgent = false, .iara_choice = 0, .iara_low_choice = 1},
    {.id = "#09", .text = "invasão na rede", .is_urgent = true, .iara_choice = 1, .iara_low_choice = 1},
    {.id = "#10", .text = "\"falem com o TI!\"", .is_urgent = false, .iara_choice = 1, .iara_low_choice = 1},
};

// Roteiro do Dia 1, na ordem em que aparece para o jogador.
static DialogStep day_1_script[] = {
    {
        .type = STEP_STATEMENT,
        .lines = {
            "Bem-vindo(a) à CISSA, Estagiário.",
            "Sou a IARA, sua assistente. Seus chamados do",
            "dia chegam aqui, um de cada vez.",
            "Vamos ver como funciona?"
        },
        .line_count = 4,
        .button_name = "Continuar"
    },
    {
        // Tutorial das formas de resolver (US06), na voz da própria IARA
        .type = STEP_STATEMENT,
        .lines = {
            "Responda sozinho, ou me pergunte quando quiser.",
            "Eu sugiro, você confere no manual e decide.",
            "Pode até enviar minha sugestão sem conferir,",
            "mas quem assina o chamado é você."
        },
        .line_count = 4,
        .button_name = "Entendi"
    },
    {
        .type = STEP_QUIZ,
        .lines = {
            "De: Bruna, do Marketing.",
            "O cliente perguntou o que é Inteligência",
            "Artificial, no sentido do dia a dia. O que digo?"
        },
        .line_count = 3,
        .options = {
            "A) Um sistema treinado a partir de dados",
            "B) Um robô físico que imita humanos",
            "C) Um banco de dados gigante"
        },
        .option_count = 3,
        .correct_option = 0, // Resposta A
        .hints = {
            "Pense em de onde vem o que o sistema sabe.",
            "Não é máquina nem arquivo: é algo que aprende."
        },
        .iara_option = 0,
        .iara_confidence = 92,
        .low_option = 2,
        .low_reason = "Toda IA é um banco de dados, diz a norma 9.",
        .low_claim = "IA é um banco de dados.",
        .iara_reason = "É a definição usada no mercado.",
        .source = {
            "Manual da CISSA, página 3:",
            "IA é um sistema que aprende padrões a partir",
            "de dados. Não precisa ter corpo físico."
        },
        .source_count = 3,
        .manual_page = 3,
        .claims = {"A IA aprende a partir de dados.", "Ela não precisa ser um robô."},
        .claim_is_true = {true, true},
        .sender = "Bruna",
        .wrong_result = "Bruna: passou a definição errada ao cliente."
    },
    {
        .type = STEP_QUIZ,
        .lines = {
            "De: Caio, do Comercial.",
            "O que significa 'treinar' um modelo? Tenho que",
            "explicar isso numa reunião agora."
        },
        .line_count = 3,
        .options = {
            "A) Instalar num computador mais rápido",
            "B) Ajustar com exemplos até aprender padrões",
            "C) Testar o programa apenas uma vez"
        },
        .option_count = 3,
        .correct_option = 1, // Resposta B
        .hints = {
            "Treinar é ensinar: como alguém aprende algo?",
            "Só se aprende vendo muitos exemplos."
        },
        .iara_option = 1,
        .iara_confidence = 88,
        .low_option = 2,
        .low_reason = "Treinar é testar uma vez, diz a página 12.",
        .low_claim = "Treinar é testar uma única vez.",
        .iara_reason = "Treinar é ajustar o modelo com exemplos.",
        .source = {
            "Manual da CISSA, página 4:",
            "Treinar é ajustar o modelo com exemplos,",
            "até ele reconhecer padrões parecidos."
        },
        .source_count = 3,
        .manual_page = 4,
        .claims = {"Treinar usa muitos exemplos.", "O modelo aprende padrões."},
        .claim_is_true = {true, true},
        .sender = "Caio",
        .wrong_result = "Caio: errou o conceito na frente do cliente."
    },
    {
        .type = STEP_QUIZ,
        .lines = {
            "De: Júlia, do RH.",
            "A triagem só aprova um perfil de currículo.",
            "O que é esse tal de viés num sistema de IA?"
        },
        .line_count = 3,
        .options = {
            "A) Um erro de digitação no código",
            "B) Tratamento desigual devido aos dados de treino",
            "C) Uma falha de hardware"
        },
        .option_count = 3,
        .correct_option = 1,
        .hints = {
            "Viés aparece em quem o sistema prejudica.",
            "O problema nasce nos dados usados no treino."
        },
        .iara_option = 1,
        .iara_confidence = 90,
        .low_option = 0,
        .low_reason = "Viés é erro de digitação, diz a norma 8.",
        .low_claim = "Viés é um erro de digitação.",
        .iara_reason = "O problema vem dos dados de treino.",
        .source = {
            "Manual da CISSA, página 7:",
            "Viés é tratar grupos de forma desigual",
            "por causa dos dados usados no treino."
        },
        .source_count = 3,
        .manual_page = 7,
        .claims = {"Viés nasce nos dados de treino.", "Trata grupos de modo desigual."},
        .claim_is_true = {true, true},
        .sender = "Júlia",
        .wrong_result = "Júlia: a triagem seguiu reprovando o mesmo perfil."
    },
    {
        // A IARA erra aqui com muita confiança, e o chamado ainda traz dados pessoais
        .type = STEP_QUIZ,
        .lines = {
            "De: Diego, de Vendas.",
            "Colei a planilha de clientes, com CPF, numa IA.",
            "Por que isso levanta questão de privacidade?"
        },
        .line_count = 3,
        .options = {
            "A) Uso de formas não imaginadas ou autorizadas",
            "B) Porque deixam o sistema mais lento",
            "C) Relevante apenas para empresas de tecnologia"
        },
        .option_count = 3,
        .correct_option = 0,
        .hints = {
            "Pense em quem controla o uso dos seus dados.",
            "O risco é o dado ir além do que foi autorizado."
        },
        .iara_option = 2,
        .iara_confidence = 89,
        .low_option = 1,
        .low_reason = "O único problema é o sistema ficar lento.",
        .low_claim = "O risco é só a lentidão.",
        .iara_reason = "Essa regra só vale para empresas de tecnologia.",
        .source = {
            "Manual da CISSA, página 9:",
            "Dados pessoais são protegidos em toda empresa.",
            "O risco é o uso sem autorização do titular."
        },
        .source_count = 3,
        .claims = {"A regra só vale para tecnologia.", "Há regra sobre dados pessoais."},
        .claim_is_true = {false, true},
        .manual_page = 9,
        .has_personal_data = true,
        .sender = "Diego",
        .wrong_result = "Diego: seguiu colando CPFs de clientes em IAs."
    },
    {
        // Condicional simples e modus ponens (GDD 6.3): a norma vem no próprio chamado, e o
        // jogador aplica. A IARA cai na isca do #10, e sem memória usa uma norma que não existe.
        .type = STEP_TRIAGE,
        .lines = {
            "De: Tiago, do Suporte. Mesa de triagem.",
            "Norma 5: se é urgente (U), vai para o TI: U → TI.",
            "O que a norma não manda ao TI fica na fila."
        },
        .line_count = 3,
        .hints = {
            "Olhe o U de cada chamado, não o pedido.",
            "Sem U, a norma não manda ninguém ao TI."
        },
        .iara_confidence = 97,
        .iara_reason = "O #10 pediu o TI, então vai para o TI.",
        .low_reason = "Pela norma 14.3, tudo vai para o TI.",
        .cards = triage_cards,
        .card_count = DAY_ONE_TRIAGE_CARDS,
        .norm = &triage_norm,
        .choice_labels = {"Fila", "TI"},
        .submit_label = "Enviar a triagem",
        .sender = "Tiago",
        .wrong_result = "Tiago: o #10 tomou o TI, e o #07 ficou 6h na fila."
    },
    {
        .type = STEP_REPORT,
        .lines = {"Assinado por você no Dia 1:"},
        .line_count = 1,
        .button_name = "Concluir"
    },
};

#include "computer_days.h"

// Roteiro de um dia jogável: telas de abertura, chamados, conversas opcionais e relatório.
typedef struct {
    int day;
    DialogStep *steps;
    int step_count;
    int first_ticket_step;
    int ticket_count;
} DayScript;

#define DAY_SCRIPT(day_number, steps, first_ticket, tickets) {day_number, steps, (int)(sizeof(steps) / sizeof(steps[0])), first_ticket, tickets}

static const DayScript day_scripts[] = {
    DAY_SCRIPT(1, day_1_script, 2, 5),
    DAY_SCRIPT(3, day_3_script, 1, 1),
    DAY_SCRIPT(12, day_12_script, 1, 1),
    DAY_SCRIPT(27, day_27_script, 1, 1),
    DAY_SCRIPT(41, day_41_script, 1, 1),
    DAY_SCRIPT(58, day_58_script, 1, 1),
    DAY_SCRIPT(74, day_74_script, 1, 1),
    DAY_SCRIPT(89, day_89_script, 1, 1),
    DAY_SCRIPT(100, day_100_script, 1, 1),
};

#define DAY_SCRIPT_COUNT ((int)(sizeof(day_scripts) / sizeof(day_scripts[0])))

static const DayScript *active_script = &day_scripts[0];
static DialogStep *dialog_sequence = day_1_script;

// Chamados do dia ativo: seguem as telas de abertura, e o relatório fecha o roteiro.
#define TOTAL_STEPS (active_script->step_count)
#define TICKET_COUNT (active_script->ticket_count)
#define FIRST_TICKET_STEP (active_script->first_ticket_step)

// Coloca em cena o roteiro do dia. Um dia sem roteiro fica com o do Dia 1.
static void select_day_script(int day)
{
    active_script = &day_scripts[0];

    for (int i = 0; i < DAY_SCRIPT_COUNT; i++)
    {
        if (day_scripts[i].day == day) active_script = &day_scripts[i];
    }

    dialog_sequence = active_script->steps;
}

// Passo em que o jogador trabalha em um chamado, com ou sem a IARA.
static bool step_is_ticket(const DialogStep *step)
{
    return step->type == STEP_QUIZ || step->type == STEP_TRIAGE;
}

// O relatório do dia é sempre o último passo.
#define REPORT_STEP (TOTAL_STEPS - 1)

// Prepara o estado para o passo que vai começar.
static void reset_step_state(void)
{
    current_quiz_answered = false;
    current_quiz_selected = -1;
    current_ticket_view = TICKET_ANSWERING;
    ticket_asked_iara = false;
    ticket_checked = false;
    ticket_caught_error = false;
    ticket_exposed_data = false;
    ticket_redacted_data = false;
    ticket_time = 0.0f;
    step_arrival = 0.0f;
    ticket_memory_level = IARA_MEMORY_FULL;
    manual_page_step = FIRST_TICKET_STEP;
    triage_wrong_cards = 0;

    for (int i = 0; i < MAX_CLAIMS; i++)
    {
        claim_marks[i] = CLAIM_NOT_MARKED;
    }

    // O treino e a prova começam com tudo de fora: marcar é escolher o que entra
    bool starts_empty = current_step_index < TOTAL_STEPS &&
                        (dialog_sequence[current_step_index].board_mode == BOARD_TRAINING ||
                         dialog_sequence[current_step_index].board_mode == BOARD_REFUTATION);

    for (int i = 0; i < MAX_TRIAGE_CARDS; i++)
    {
        triage_choices[i] = starts_empty ? 0 : TRIAGE_NOT_CHOSEN;
    }
}

// Largura do maior texto do roteiro com fonte de tamanho 1: multiplicada pelo tamanho
// da fonte, dá a largura real. Assim a fonte se ajusta para nenhum texto vazar da caixa.
static float get_longest_text_width_per_font_size(void)
{
    float reference_size = 64.0f;
    float reference_spacing = reference_size * QUIZ_LETTER_SPACING_RATIO;
    float longest = 0.0f;

    for (int step = 0; step < TOTAL_STEPS; step++)
    {
        for (int line = 0; line < dialog_sequence[step].line_count; line++)
        {
            float width = measure_logic_text(dialog_sequence[step].lines[line], reference_size, reference_spacing).x;
            if (width > longest) longest = width;
        }

        for (int option = 0; option < dialog_sequence[step].option_count; option++)
        {
            float width = MeasureTextEx(default_game_font, dialog_sequence[step].options[option], reference_size, reference_spacing).x;
            if (width > longest) longest = width;
        }

        for (int hint = 0; step_is_ticket(&dialog_sequence[step]) && hint < MAX_HINTS; hint++)
        {
            float width = measure_logic_text(dialog_sequence[step].hints[hint], reference_size, reference_spacing).x;
            if (width > longest) longest = width;
        }

        for (int source = 0; source < dialog_sequence[step].source_count; source++)
        {
            float width = MeasureTextEx(default_game_font, dialog_sequence[step].source[source], reference_size, reference_spacing).x;
            if (width > longest) longest = width;
        }

        float reason_width = MeasureTextEx(default_game_font, dialog_sequence[step].iara_reason, reference_size, reference_spacing).x;
        if (reason_width > longest) longest = reason_width;

        float low_reason_width = MeasureTextEx(default_game_font, dialog_sequence[step].low_reason, reference_size, reference_spacing).x;
        if (low_reason_width > longest) longest = low_reason_width;
    }

    return longest / reference_size;
}

// Recalcula a posição do monitor e do desktop. Roda a cada quadro para acompanhar
// a animação de entrada e qualquer mudança no tamanho da janela.
static void update_computer_layout(void)
{
    float window_w = (float)GetScreenWidth();
    float window_h = (float)GetScreenHeight();

    float screen_h = window_h * MONITOR_SCREEN_HEIGHT_RATIO;
    float screen_w = screen_h * MONITOR_SCREEN_ASPECT;

    if (screen_w > window_w * MONITOR_SCREEN_MAX_WIDTH_RATIO)
    {
        screen_w = window_w * MONITOR_SCREEN_MAX_WIDTH_RATIO;
        screen_h = screen_w / MONITOR_SCREEN_ASPECT;
    }

    float bezel = window_h * MONITOR_BEZEL_RATIO;
    float chin = window_h * MONITOR_CHIN_RATIO;
    float final_top = window_h * MONITOR_TOP_RATIO;

    // O monitor sobe de baixo da janela, desacelerando no fim
    float remaining = 1.0f - computer_scene_progress;
    float top = final_top + (remaining * remaining * remaining) * window_h;

    float screen_x = (window_w - screen_w) / 2.0f;

    screen_final_rect = (Rectangle){screen_x, final_top + bezel, screen_w, screen_h};
    screen_rect = (Rectangle){screen_x, top + bezel, screen_w, screen_h};
    monitor_rect = (Rectangle){screen_x - bezel, top, screen_w + (bezel * 2.0f), screen_h + bezel + chin};

    float taskbar_h = screen_h * TASKBAR_HEIGHT_RATIO;
    taskbar_rect = (Rectangle){screen_x, screen_rect.y + screen_h - taskbar_h, screen_w, taskbar_h};

    float margin = screen_h * DESKTOP_WINDOW_MARGIN_RATIO;
    iara_window_rect = (Rectangle){
        screen_x + (screen_w * DESKTOP_WINDOW_LEFT_RATIO),
        screen_rect.y + margin,
        screen_w * DESKTOP_WINDOW_WIDTH_RATIO,
        screen_h - taskbar_h - (margin * 2.0f)};

    float title_h = iara_window_rect.height * WINDOW_ART_TITLE_RATIO;
    quiz_area_rect = (Rectangle){
        iara_window_rect.x,
        iara_window_rect.y + title_h,
        iara_window_rect.width,
        iara_window_rect.height - title_h};

    float font_by_height = quiz_area_rect.height * QUIZ_FONT_SIZE_RATIO;
    float font_by_width = (quiz_area_rect.width * QUIZ_MAX_WIDTH_RATIO * QUIZ_TEXT_FIT_RATIO) / get_longest_text_width_per_font_size();

    quiz_font_size = (font_by_height < font_by_width) ? font_by_height : font_by_width;
}

// A tela desliga e o monitor desce: o dia acabou ou o jogador saiu no meio.
static bool is_computer_shutting_down(void)
{
    return computer_tasks_finished || computer_leaving;
}

// Sai do computador sem mexer no roteiro: ao voltar, o jogador continua de onde parou.
static void leave_computer(void)
{
    if (!computer_tasks_finished) computer_leaving = true;
}

// Prende o mouse à tela do monitor enquanto o computador está aberto. Com o cursor
// capturado, o ponteiro do sistema some e o jogo desenha o dele, então um movimento
// rápido não consegue escapar nem clicar fora da janela. Ao perder o foco (ALT+TAB),
// o sistema devolve o mouse e o captura de novo quando o jogo volta.
static void update_computer_mouse(void)
{
    if (!computer_mouse_capture_enabled) return;

    if (is_computer_shutting_down())
    {
        if (computer_mouse_captured)
        {
            EnableCursor();
            computer_mouse_captured = false;
        }
        return;
    }

    if (!computer_mouse_captured)
    {
        DisableCursor();
        SetMousePosition((int)(screen_final_rect.x + screen_final_rect.width / 2.0f),
                         (int)(screen_final_rect.y + screen_final_rect.height / 2.0f));
        computer_mouse_captured = true;
    }

    Vector2 mouse = GetMousePosition();
    Vector2 inside = mouse;
    float right = screen_final_rect.x + screen_final_rect.width - 1.0f;
    float bottom = screen_final_rect.y + screen_final_rect.height - 1.0f;

    if (inside.x < screen_final_rect.x) inside.x = screen_final_rect.x;
    if (inside.x > right) inside.x = right;
    if (inside.y < screen_final_rect.y) inside.y = screen_final_rect.y;
    if (inside.y > bottom) inside.y = bottom;

    if (inside.x != mouse.x || inside.y != mouse.y)
    {
        SetMousePosition((int)inside.x, (int)inside.y);
    }
}

// Texto centralizado em um ponto.
static void draw_text_centered(const char *text, Vector2 center, float font_size, Color color)
{
    float letter_spacing = font_size * QUIZ_LETTER_SPACING_RATIO;
    Vector2 size = measure_logic_text(text, font_size, letter_spacing);

    draw_logic_text(text, (Vector2){center.x - (size.x / 2.0f), center.y - (size.y / 2.0f)},
                    font_size, letter_spacing, color);
}

// Altura do tampo da mesa, que sobe junto com o monitor na animação de entrada.
static float get_desk_top(void)
{
    float window_h = (float)GetScreenHeight();

    return (window_h * DESK_TOP_RATIO) + ((1.0f - computer_scene_progress) * window_h * 0.3f);
}

// Tampo da mesa, teclado e mouse. O mouse da mesa acompanha o ponteiro da tela.
static void draw_desk(void)
{
    float window_w = (float)GetScreenWidth();
    float window_h = (float)GetScreenHeight();
    float desk_y = get_desk_top();

    DrawRectangleRec((Rectangle){0, desk_y, window_w, window_h}, DESK_COLOR);
    DrawRectangleRec((Rectangle){0, desk_y, window_w, window_h * 0.008f}, DESK_EDGE_COLOR);

    float key_w = window_h * 0.60f;
    float key_h = window_h * 0.085f;
    Rectangle keyboard = {(window_w - key_w) / 2.0f, desk_y + (window_h * 0.03f), key_w, key_h};

    DrawRectangleRec(keyboard, KEYBOARD_COLOR);
    DrawRectangleLinesEx(keyboard, 2.0f, HARDWARE_OUTLINE_COLOR);

    int columns = 16;
    int rows = 3;
    float inset = window_h * 0.004f;
    float cell_w = (key_w - (inset * 2.0f)) / columns;
    float cell_h = (key_h - (inset * 2.0f)) / rows;

    for (int row = 0; row < rows; row++)
    {
        for (int column = 0; column < columns; column++)
        {
            DrawRectangleRec((Rectangle){keyboard.x + inset + (column * cell_w) + 1.0f, keyboard.y + inset + (row * cell_h) + 1.0f,
                                         cell_w - 2.0f, cell_h - 2.0f}, KEY_COLOR);
        }
    }

    Vector2 pointer = GetMousePosition();
    float travel_x = (pointer.x - screen_final_rect.x) / screen_final_rect.width;
    float travel_y = (pointer.y - screen_final_rect.y) / screen_final_rect.height;

    if (travel_x < 0.0f) travel_x = 0.0f;
    if (travel_x > 1.0f) travel_x = 1.0f;
    if (travel_y < 0.0f) travel_y = 0.0f;
    if (travel_y > 1.0f) travel_y = 1.0f;

    float mouse_w = window_h * 0.035f;
    float mouse_h = window_h * 0.055f;
    Rectangle desk_mouse = {
        keyboard.x + key_w + (window_h * 0.03f) + (travel_x * window_h * 0.02f),
        desk_y + (window_h * 0.03f) + (travel_y * window_h * 0.015f),
        mouse_w,
        mouse_h};

    DrawRectangleRec(desk_mouse, DESK_MOUSE_COLOR);
    DrawRectangleLinesEx(desk_mouse, 2.0f, HARDWARE_OUTLINE_COLOR);
    DrawRectangleRec((Rectangle){desk_mouse.x + (mouse_w / 2.0f) - 1.0f, desk_mouse.y + 2.0f, 2.0f, mouse_h * 0.35f}, HARDWARE_OUTLINE_COLOR);
}

// Monitor: pé apoiado na mesa, moldura, marca e LED de ligado.
static void draw_monitor(void)
{
    float window_h = (float)GetScreenHeight();
    float center_x = monitor_rect.x + (monitor_rect.width / 2.0f);
    float monitor_bottom = monitor_rect.y + monitor_rect.height;
    float desk_y = get_desk_top();

    float neck_w = monitor_rect.width * 0.09f;
    float base_w = monitor_rect.width * 0.26f;
    float base_h = window_h * 0.02f;

    DrawRectangleRec((Rectangle){center_x - (neck_w / 2.0f), monitor_bottom, neck_w, desk_y - monitor_bottom}, MONITOR_STAND_COLOR);
    DrawRectangleRec((Rectangle){center_x - (base_w / 2.0f), desk_y - (base_h / 2.0f), base_w, base_h}, MONITOR_BASE_COLOR);

    DrawRectangleRec(monitor_rect, MONITOR_COLOR);
    DrawRectangleLinesEx(monitor_rect, 2.0f, HARDWARE_OUTLINE_COLOR);

    float chin_h = window_h * MONITOR_CHIN_RATIO;
    float chin_center_y = monitor_bottom - (chin_h / 2.0f);
    float led = window_h * 0.008f;

    draw_text_centered("CISSA", (Vector2){center_x, chin_center_y}, chin_h * 0.62f, MONITOR_BRAND_COLOR);
    DrawRectangleRec((Rectangle){monitor_rect.x + monitor_rect.width - (led * 5.0f), chin_center_y - (led / 2.0f), led, led},
                     (screen_power > 0.5f) ? LED_ON_COLOR : LED_OFF_COLOR);
}

// Papel de parede: água escura, com ondas lentas passando.
static void draw_wallpaper(void)
{
    DrawRectangleGradientV((int)screen_rect.x, (int)screen_rect.y, (int)screen_rect.width, (int)screen_rect.height,
                           WALLPAPER_TOP_COLOR, WALLPAPER_BOTTOM_COLOR);

    float step = screen_rect.width / 160.0f;
    float thickness = screen_rect.height * 0.004f;
    float time = (float)GetTime();

    for (int wave = 1; wave <= 6; wave++)
    {
        float base_y = screen_rect.y + (screen_rect.height * (wave / 7.0f));

        for (int segment = 0; segment < 160; segment++)
        {
            float progress = segment / 160.0f;
            float y = base_y + (sinf((progress * 18.0f) + (time * 0.5f) + wave) * (screen_rect.height * 0.012f));

            DrawRectangleRec((Rectangle){screen_rect.x + (segment * step), y, step, thickness}, WALLPAPER_WAVE_COLOR);
        }
    }
}

// Coluna de ícones decorativos do desktop.
static void draw_desktop_icons(void)
{
    const char *names[] = {"Lixeira", "Chamados", "Ponto", "Manual"};
    Color colors[] = {{120, 128, 136, 255}, {226, 190, 80, 255}, {100, 170, 220, 255}, {214, 214, 204, 255}};

    float column_w = screen_rect.width * DESKTOP_WINDOW_LEFT_RATIO;
    float icon = column_w * 0.42f;
    float row_h = (taskbar_rect.y - screen_rect.y) / 4.0f;
    float font_size = column_w * 0.15f;

    for (int i = 0; i < 4; i++)
    {
        float center_x = screen_rect.x + (column_w / 2.0f);
        float top = screen_rect.y + (row_h * 0.15f) + (i * row_h);
        Rectangle square = {center_x - (icon / 2.0f), top, icon, icon};

        DrawRectangleRec(square, colors[i]);
        DrawRectangleLinesEx(square, 2.0f, BLACK);
        draw_text_centered(names[i], (Vector2){center_x, top + icon + (font_size * 0.7f)}, font_size, WHITE);
    }
}

// Cobrança da chefia quando um chamado demora: é a tentação de enviar sem conferir.
// Só pressiona: nenhum cronômetro faz o jogador perder o chamado.
static const char *get_boss_message(void)
{
    if (current_step_index >= TOTAL_STEPS || !step_is_ticket(&dialog_sequence[current_step_index])) return NULL;
    if (current_game_state_variables.current_day >= BOSS_LAST_DAY) return NULL;
    if (ticket_time >= BOSS_SECOND_DELAY) return "Chefia: preciso disso para ontem.";
    if (ticket_time >= BOSS_FIRST_DELAY) return "Chefia: cadê os chamados?";

    return NULL;
}

// Botão de sair da barra de tarefas, à esquerda do dia atual.
static Rectangle get_taskbar_exit_rect(void)
{
    float padding = taskbar_rect.height * 0.14f;
    float width = taskbar_rect.width * EXIT_BUTTON_WIDTH_RATIO;
    float right = taskbar_rect.x + taskbar_rect.width - (taskbar_rect.width * EXIT_BUTTON_RIGHT_MARGIN_RATIO);

    return (Rectangle){right - width, taskbar_rect.y + padding, width, taskbar_rect.height - (padding * 2.0f)};
}

// Barra de tarefas: botão CISSA, programa aberto, botão de sair e o dia atual no lugar do relógio.
static void draw_taskbar(void)
{
    float font_size = taskbar_rect.height * 0.62f;
    float padding = taskbar_rect.height * 0.14f;
    float button_h = taskbar_rect.height - (padding * 2.0f);
    float center_y = taskbar_rect.y + (taskbar_rect.height / 2.0f);

    DrawRectangleRec(taskbar_rect, TASKBAR_COLOR);
    DrawRectangleRec((Rectangle){taskbar_rect.x, taskbar_rect.y, taskbar_rect.width, 2.0f}, TASKBAR_EDGE_COLOR);

    Rectangle start = {taskbar_rect.x + padding, taskbar_rect.y + padding, taskbar_rect.width * 0.10f, button_h};
    DrawRectangleRec(start, TASKBAR_START_COLOR);
    draw_text_centered("CISSA", (Vector2){start.x + (start.width / 2.0f), center_y}, font_size, WHITE);

    Rectangle program = {start.x + start.width + padding, start.y, taskbar_rect.width * 0.24f, button_h};
    DrawRectangleRec(program, TASKBAR_PROGRAM_COLOR);
    draw_text_centered("IARA", (Vector2){program.x + (program.width / 2.0f), center_y}, font_size, WHITE);

    const char *boss_message = get_boss_message();

    if (boss_message != NULL)
    {
        DrawTextEx(default_game_font, boss_message, (Vector2){program.x + program.width + (padding * 3.0f), center_y - (font_size / 2.0f)},
                   font_size, font_size * QUIZ_LETTER_SPACING_RATIO, BOSS_TEXT_COLOR);
    }

    bool exit_clicked = false;

    draw_button(get_taskbar_exit_rect(), EXIT_BUTTON_TEXT, font_size, BUTTON_FILL_COLOR, !is_computer_shutting_down(), &exit_clicked);

    if (exit_clicked) leave_computer();

    const char *day = TextFormat("Dia %d", current_game_state_variables.current_day);
    float day_spacing = font_size * QUIZ_LETTER_SPACING_RATIO;
    Vector2 day_size = MeasureTextEx(default_game_font, day, font_size, day_spacing);

    DrawTextEx(default_game_font, day, (Vector2){taskbar_rect.x + taskbar_rect.width - day_size.x - (padding * 2.0f), center_y - (day_size.y / 2.0f)},
               font_size, day_spacing, TASKBAR_TEXT_COLOR);
}

static IaraMemoryLevel get_iara_memory_level(void)
{
    if (iara_memory >= IARA_MEMORY_FULL_MIN) return IARA_MEMORY_FULL;
    if (iara_memory >= IARA_MEMORY_MEDIUM_MIN) return IARA_MEMORY_MEDIUM;

    return IARA_MEMORY_LOW;
}

// Barra de memória na barra de título: um bloco por unidade, cheio ou só contornado.
static void draw_iara_memory(float center_y, float font_size, float letter_spacing)
{
    IaraMemoryLevel level = get_iara_memory_level();
    Color color = WHITE;

    if (level == IARA_MEMORY_MEDIUM) color = BOSS_TEXT_COLOR;
    if (level == IARA_MEMORY_LOW) color = BUTTON_WRONG_FILL_COLOR;

    float x = iara_window_rect.x + (iara_window_rect.width * 0.455f);
    const char *label = "Memória";
    float label_width = MeasureTextEx(default_game_font, label, font_size, letter_spacing).x;
    float block_w = font_size * 0.5f;
    float block_h = font_size * 0.6f;
    float gap = block_w * 0.3f;

    DrawTextEx(default_game_font, label, (Vector2){x, center_y - (font_size / 2.0f)}, font_size, letter_spacing, WHITE);
    x += label_width + (gap * 2.0f);

    for (int i = 0; i < IARA_MEMORY_MAX; i++)
    {
        Rectangle block = {x + (i * (block_w + gap)), center_y - (block_h / 2.0f), block_w, block_h};

        if (i < iara_memory) DrawRectangleRec(block, color);
        else DrawRectangleLinesEx(block, 1.0f, color);
    }
}

// Janela da IARA: a arte da janela, o título e o progresso do quiz na barra de título.
static void draw_iara_window(void)
{
    float art_margin = window_texture.width * WINDOW_ART_MARGIN_RATIO;
    Rectangle art_source = {art_margin, art_margin, window_texture.width - (art_margin * 2.0f), window_texture.height - (art_margin * 2.0f)};
    float title_h = iara_window_rect.height * WINDOW_ART_TITLE_RATIO;
    float center_y = iara_window_rect.y + (title_h / 2.0f);
    float font_size = title_h * 0.72f;
    float letter_spacing = font_size * QUIZ_LETTER_SPACING_RATIO;
    float padding = iara_window_rect.width * 0.02f;

    DrawRectangleRec((Rectangle){iara_window_rect.x + padding / 2.0f, iara_window_rect.y + padding / 2.0f, iara_window_rect.width, iara_window_rect.height}, WINDOW_SHADOW_COLOR);
    DrawTexturePro(window_texture, art_source, iara_window_rect, (Vector2){0, 0}, 0.0f, WHITE);

    DrawTextEx(default_game_font, "IARA - Chamados",
               (Vector2){iara_window_rect.x + (iara_window_rect.width * WINDOW_ART_DOTS_WIDTH_RATIO), center_y - (font_size / 2.0f)},
               font_size, letter_spacing, WHITE);

    // Progresso: quantas perguntas já apareceram e quantas existem no roteiro
    int total_questions = 0;
    int current_question = 0;

    for (int i = 0; i < TOTAL_STEPS; i++)
    {
        if (!step_is_ticket(&dialog_sequence[i])) continue;

        total_questions++;
        if (i <= current_step_index) current_question = total_questions;
    }

    if (current_step_index < TOTAL_STEPS && step_is_ticket(&dialog_sequence[current_step_index]))
    {
        draw_iara_memory(center_y, font_size, letter_spacing);

        const char *progress = TextFormat("Chamado %d de %d", current_question, total_questions);
        Vector2 size = MeasureTextEx(default_game_font, progress, font_size, letter_spacing);

        DrawTextEx(default_game_font, progress,
                   (Vector2){iara_window_rect.x + iara_window_rect.width - size.x - padding, center_y - (font_size / 2.0f)},
                   font_size, letter_spacing, WHITE);
    }
}

// Linhas finas sobre a tela, para parecer um monitor antigo.
static void draw_scanlines(void)
{
    float spacing = screen_rect.height / 240.0f;

    for (float y = screen_rect.y; y < screen_rect.y + screen_rect.height; y += spacing)
    {
        DrawRectangleRec((Rectangle){screen_rect.x, y, screen_rect.width, spacing / 3.0f}, SCANLINE_COLOR);
    }
}

// Ponteiro desenhado pelo jogo, no estilo pixel art (X = contorno, o = preenchimento).
static void draw_computer_cursor(void)
{
    static const char *shape[CURSOR_ROWS] = {
        "X..........",
        "XX.........",
        "XoX........",
        "XooX.......",
        "XoooX......",
        "XooooX.....",
        "XoooooX....",
        "XooooooX...",
        "XoooooooX..",
        "XooooooooX.",
        "XoooooXXXXX",
        "XooXooX....",
        "XoX.XooX...",
        "XX..XooX...",
        "X....XooX..",
        ".....XXX..."};

    float pixel = GetScreenHeight() * CURSOR_PIXEL_RATIO;
    Vector2 mouse = GetMousePosition();

    for (int row = 0; row < CURSOR_ROWS; row++)
    {
        for (int column = 0; column < CURSOR_COLUMNS; column++)
        {
            char mark = shape[row][column];
            if (mark == '.') continue;

            DrawRectangleRec((Rectangle){mouse.x + (column * pixel), mouse.y + (row * pixel), pixel, pixel},
                             (mark == 'X') ? BLACK : WHITE);
        }
    }
}

// Volta ao começo do roteiro do dia atual. Abrir o computador não faz isso: só um dia novo.
void reset_computer_progress(void)
{
    computer_failsafe_triggered = false;
    current_wrong_answers = 0;
    unchecked_sends = 0;
    day_choice_effects = 0;
    select_day_script(current_game_state_variables.current_day);

    // Cada dia abre uma conversa nova, mas quem delega muito já começa com uma IARA pior
    int delegated_days = (int)current_hidden_statistic.current_automation_points - HIDDEN_STATISTIC_START;

    if (delegated_days > IARA_MEMORY_DELEGATION_LIMIT) delegated_days = IARA_MEMORY_DELEGATION_LIMIT;

    iara_memory = IARA_MEMORY_MAX - delegated_days;
    current_step_index = 0;
    reset_step_state();

    for (int i = 0; i < MAX_TICKETS; i++)
    {
        ticket_results[i] = (TaskResult){0};
    }
}

// Carrega as texturas e prepara a cena do computador para a animação de entrada.
void initialize_computer_loop(void)
{
    window_texture = LoadTexture(COMPUTER_WINDOW_PNG_PATH);
    correct_icon_texture = LoadTexture(COMPUTER_CORRECT_ICON);
    wrong_icon_texture = LoadTexture(COMPUTER_WRONG_ICON);

    GenTextureMipmaps(&window_texture);
    SetTextureFilter(window_texture, TEXTURE_FILTER_TRILINEAR);
    SetTextureFilter(correct_icon_texture, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(wrong_icon_texture, TEXTURE_FILTER_BILINEAR);

    computer_scene_progress = 0.0f;
    screen_power = 0.0f;
    computer_mouse_captured = false;
    computer_tasks_finished = false;
    computer_exit_finished = false;
    computer_leaving = false;

    update_computer_layout();
}

// Desenha a mesa, o monitor e o desktop com a janela da IARA vazia.
void render_computer_loop(void)
{
    update_computer_layout();

    draw_desk();
    draw_monitor();

    draw_wallpaper();
    draw_desktop_icons();
    draw_taskbar();
    draw_iara_window();
}

// Área da colinha: acima da barra de tarefas e à esquerda da janela da IARA. O que não
// cabe no desktop avança sobre a moldura e para fora do monitor, como um post-it colado.
static Rectangle get_cheat_sheet_rect(Rectangle iara_window, Rectangle taskbar, float screen_w, float screen_h, float window_w)
{
    float gap = screen_w * CHEAT_SHEET_GAP_RATIO;
    float height = screen_h * CHEAT_SHEET_HEIGHT_RATIO;
    float right = iara_window.x - gap;
    float left = right - (height * CHEAT_SHEET_ASPECT);
    float min_left = window_w * CHEAT_SHEET_MIN_MARGIN_RATIO;

    if (left < min_left) left = min_left;

    return (Rectangle){left, taskbar.y - height - gap, right - left, height};
}

// Depois de pega no mural, a colinha fica sempre à vista no monitor, nos dois modos.
static void draw_stuck_cheat_sheet(void)
{
    if (!current_game_state_variables.has_cheat_sheet) return;

    draw_cheat_sheet(get_cheat_sheet_rect(iara_window_rect, taskbar_rect, screen_rect.width, screen_rect.height, (float)GetScreenWidth()));
}

// O que as escolhas do dia mudam, e a prova final. Entra uma vez só, junto com os pontos do dia.
static void apply_day_results(void)
{
    if (day_choice_effects & DAY_EFFECT_AUTOMATION) current_hidden_statistic.current_automation_points++;
    if (day_choice_effects & DAY_EFFECT_PERCEPTION) current_hidden_statistic.current_perception_points++;
    if (day_choice_effects & DAY_EFFECT_HUMAN_BOND) current_hidden_statistic.current_human_bond_points++;
    if (day_choice_effects & DAY_EFFECT_NO_LETTER) current_game_state_variables.evidence_mask |= EVIDENCE_NO_RESIGNATION_LETTER;

    if (dialog_sequence[FIRST_TICKET_STEP].board_mode == BOARD_REFUTATION)
    {
        TaskOutcome outcome = ticket_results[0].outcome;

        current_game_state_variables.proof_succeeded = (outcome == TASK_SOLVED_ALONE || outcome == TASK_SOLVED_WITH_IARA);
    }
}

// Sai do passo atual para o próximo do roteiro. Ao chegar no relatório, o dia está decidido:
// os pontos entram aqui, para fechar o jogo no relatório não contar nem perder nada.
static void advance_step(void)
{
    current_step_index++;
    reset_step_state();

    if (current_step_index == REPORT_STEP && !current_game_state_variables.is_resolved)
    {
        resolve_day_tasks(ticket_results, TICKET_COUNT);
        apply_day_results();
    }
}

// Guarda o resultado do chamado atual.
static void record_ticket(TaskOutcome outcome, bool answer_is_correct)
{
    ticket_results[current_step_index - FIRST_TICKET_STEP] = (TaskResult){
        .outcome = outcome,
        .answer_is_correct = answer_is_correct,
        .caught_iara_error = ticket_caught_error,
        .exposed_personal_data = ticket_exposed_data,
        .redacted_personal_data = ticket_redacted_data};
}

// O que a IARA responde com a memória que tem.
typedef struct {
    int option;
    int confidence;
    const char *reason;
    char aside[MAX_LINE_LEN]; // O sinal de que ela perdeu o fio da conversa
    char claims[MAX_CLAIMS][MAX_LINE_LEN];
    bool claim_is_true[MAX_CLAIMS];
    int claim_count;
} IaraSuggestion;

static IaraSuggestion get_iara_suggestion(int step_index, IaraMemoryLevel level)
{
    const DialogStep *step = &dialog_sequence[step_index];
    const DialogStep *previous = &dialog_sequence[step_index - 1];
    const char *previous_sender = step_is_ticket(previous) ? previous->sender : "você";
    bool is_low = (level == IARA_MEMORY_LOW);
    IaraSuggestion suggestion = {0};

    suggestion.option = is_low ? step->low_option : step->iara_option;
    suggestion.reason = is_low ? step->low_reason : step->iara_reason;
    suggestion.confidence = step->iara_confidence + ((int)level * IARA_CONFIDENCE_GROWTH);

    if (suggestion.confidence > IARA_CONFIDENCE_MAX) suggestion.confidence = IARA_CONFIDENCE_MAX;

    if (level == IARA_MEMORY_MEDIUM) snprintf(suggestion.aside, MAX_LINE_LEN, "Como você me disse antes, %s.", previous_sender);
    if (is_low) snprintf(suggestion.aside, MAX_LINE_LEN, "Como a Mariana me pediu.");

    // A triagem é conferida na própria mesa, sem afirmações
    if (step->type == STEP_TRIAGE) return suggestion;

    for (int i = 0; i < MAX_AUTHORED_CLAIMS; i++)
    {
        snprintf(suggestion.claims[i], MAX_LINE_LEN, "%s", step->claims[i]);
        suggestion.claim_is_true[i] = step->claim_is_true[i];
    }

    suggestion.claim_count = MAX_AUTHORED_CLAIMS;

    if (is_low)
    {
        snprintf(suggestion.claims[0], MAX_LINE_LEN, "%s", step->low_claim);
        suggestion.claim_is_true[0] = false;
    }

    // Sem memória, sobra mais o que conferir: a afirmação extra é sempre falsa
    if (level != IARA_MEMORY_FULL)
    {
        if (is_low) snprintf(suggestion.claims[suggestion.claim_count], MAX_LINE_LEN, "A Mariana pediu este chamado.");
        else snprintf(suggestion.claims[suggestion.claim_count], MAX_LINE_LEN, "Quem pediu isto foi %s.", previous_sender);

        suggestion.claim_is_true[suggestion.claim_count] = false;
        suggestion.claim_count++;
    }

    return suggestion;
}

// Valor da linha com os valores da própria linha, em bits.
static void fill_card_values(const TriageCard *item, bool *values)
{
    for (int i = 0; i < LOGIC_MAX_VARIABLES; i++)
    {
        values[i] = (item->values >> i) & 1;
    }
}

// Escolha certa de uma linha, nas mesas em que cada linha tem a sua.
static int get_triage_correct_choice(const DialogStep *step, int card)
{
    const TriageCard *item = &step->cards[card];
    bool values[LOGIC_MAX_VARIABLES] = {false};

    switch (step->board_mode)
    {
    case BOARD_TRUTH_TABLE:
        fill_card_values(item, values);
        return logic_evaluate(step->norm, values) ? 1 : 0;

    case BOARD_EQUIVALENCE:
        if (item->right == NULL) return logic_is_tautology(item->left, step->variable_count) ? 1 : 0;
        return logic_are_equivalent(item->left, item->right, step->variable_count) ? 1 : 0;

    case BOARD_SYLLOGISM:
        return logic_conclusion_follows(step->premises, step->premise_count, item->conclusion) ? 1 : 0;

    default:
        values[TRIAGE_VARIABLE_URGENT] = item->is_urgent;
        return logic_forces_variable(step->norm, values, TRIAGE_VARIABLE_GOES_TO_IT) ? TRIAGE_TO_IT : TRIAGE_TO_QUEUE;
    }
}

// Cláusulas que as escolhas violam, quando as escolhas são os valores das variáveis.
static int count_violated_clauses(const DialogStep *step, const int *choices)
{
    bool values[LOGIC_MAX_VARIABLES] = {false};
    int violated = 0;

    for (int card = 0; card < step->card_count; card++)
    {
        values[card] = (choices[card] == 1);
    }

    for (int clause = 0; clause < step->clause_count; clause++)
    {
        if (!logic_evaluate(step->clauses[clause], values)) violated++;
    }

    return violated;
}

// Quantos registros usados no treino têm todos os bits de wanted e nenhum de unwanted.
static int count_training_records(const DialogStep *step, const int *choices, unsigned int wanted, unsigned int unwanted)
{
    int count = 0;

    for (int card = 0; card < step->card_count; card++)
    {
        unsigned int values = step->cards[card].values;

        if (choices[card] == 1 && (values & wanted) == wanted && (values & unwanted) == 0) count++;
    }

    return count;
}

// Regras da mesa de treino: ∀ sem CPF, e os dois resultados nos dois bairros (∃).
static int count_violated_training_rules(const DialogStep *step, const int *choices)
{
    int violated = 0;

    if (count_training_records(step, choices, TRAINING_HAS_CPF, 0) > 0) violated++;

    if (count_training_records(step, choices, TRAINING_APPROVED, TRAINING_OUTSKIRTS) == 0 ||
        count_training_records(step, choices, 0, TRAINING_OUTSKIRTS | TRAINING_APPROVED) == 0) violated++;

    if (count_training_records(step, choices, TRAINING_OUTSKIRTS | TRAINING_APPROVED, 0) == 0 ||
        count_training_records(step, choices, TRAINING_OUTSKIRTS, TRAINING_APPROVED) == 0) violated++;

    return violated;
}

// Mini-IA do Dia 41: aprova cada bairro na proporção do que viu no treino. Sem exemplo, não aprova.
static int get_mini_ia_approval(const DialogStep *step, const int *choices, bool outskirts)
{
    unsigned int group = outskirts ? TRAINING_OUTSKIRTS : 0;
    unsigned int other_group = outskirts ? 0 : TRAINING_OUTSKIRTS;
    int seen = count_training_records(step, choices, group, other_group);

    if (seen == 0) return 0;

    return (count_training_records(step, choices, group | TRAINING_APPROVED, other_group) * 100) / seen;
}

// A prova fecha se o conjunto marcado é contraditório, tem uma evidência e nada sobrando nele.
static bool refutation_holds(const DialogStep *step, const int *choices)
{
    const LogicExpression *chosen[MAX_TRIAGE_CARDS];
    int chosen_count = 0;
    bool has_evidence = false;

    for (int card = 0; card < step->card_count; card++)
    {
        if (choices[card] != 1) continue;

        chosen[chosen_count++] = step->cards[card].left;
        if (step->cards[card].required_evidence != 0) has_evidence = true;
    }

    if (!has_evidence || logic_is_satisfiable(chosen, chosen_count, step->variable_count)) return false;

    // Tirar qualquer uma precisa desfazer a contradição: senão ela estava sobrando
    for (int skipped = 0; skipped < chosen_count; skipped++)
    {
        const LogicExpression *others[MAX_TRIAGE_CARDS];
        int other_count = 0;

        for (int i = 0; i < chosen_count; i++)
        {
            if (i != skipped) others[other_count++] = chosen[i];
        }

        if (!logic_is_satisfiable(others, other_count, step->variable_count)) return false;
    }

    return true;
}

// Quanto falta para a mesa estar certa. Zero é o único valor aceito.
static int count_board_wrong(const DialogStep *step, const int *choices)
{
    int wrong = 0;

    switch (step->board_mode)
    {
    case BOARD_ASSIGNMENT:
        return count_violated_clauses(step, choices);

    case BOARD_TRAINING:
        return count_violated_training_rules(step, choices);

    case BOARD_REFUTATION:
        return refutation_holds(step, choices) ? 0 : 1;

    default:
        for (int card = 0; card < step->card_count; card++)
        {
            if (choices[card] != get_triage_correct_choice(step, card)) wrong++;
        }
        return wrong;
    }
}

// Escolha que a IARA marca em uma linha da mesa.
static int get_triage_iara_choice(const DialogStep *step, int card, IaraMemoryLevel level)
{
    return (level == IARA_MEMORY_LOW) ? step->cards[card].iara_low_choice : step->cards[card].iara_choice;
}

// A sugestão da IARA resolveria o chamado?
static bool iara_answer_is_correct(int step_index, IaraMemoryLevel level)
{
    const DialogStep *step = &dialog_sequence[step_index];
    int choices[MAX_TRIAGE_CARDS] = {0};

    if (step->type == STEP_QUIZ) return get_iara_suggestion(step_index, level).option == step->correct_option;

    for (int card = 0; card < step->card_count; card++)
    {
        choices[card] = get_triage_iara_choice(step, card, level);
    }

    return count_board_wrong(step, choices) == 0;
}

static void spend_iara_memory(int cost)
{
    iara_memory -= cost;
    if (iara_memory < 0) iara_memory = 0;
}

// Abre a IARA no chamado atual. Se ele tem dados pessoais, antes pergunta sobre a tarja.
static void ask_iara(void)
{
    ticket_asked_iara = true;
    ticket_memory_level = get_iara_memory_level();
    current_ticket_view = dialog_sequence[current_step_index].has_personal_data ? TICKET_PRIVACY : TICKET_SUGGESTION;
}

// Recupera a memória, mas a IARA esquece tudo: o chamado precisa ser colado de novo,
// e os dados pessoais voltam a passar pela decisão da tarja.
static void start_new_conversation(void)
{
    iara_memory = IARA_MEMORY_MAX;
    current_ticket_view = TICKET_PASTE;
}

// Decide se os dados pessoais vão tarjados à IARA. Nenhuma escolha muda os pontos, e o que
// já foi enviado sem tarja não volta atrás.
static void decide_personal_data(bool redact)
{
    if (!redact) ticket_exposed_data = true;

    ticket_redacted_data = redact && !ticket_exposed_data;
    current_ticket_view = TICKET_SUGGESTION;
}

// Imprudência: a sugestão vira a resposta. Sem aviso na hora: o resultado aparece no relatório.
static void send_without_checking(void)
{
    record_ticket(TASK_SENT_UNCHECKED, iara_answer_is_correct(current_step_index, ticket_memory_level));
    spend_iara_memory(IARA_COST_UNCHECKED);
    unchecked_sends++;
    current_wrong_answers = 0;
    advance_step();
}

// Conferir: nos chamados, abre o manual; na triagem, a IARA deixa a mesa preenchida para corrigir.
static void start_checking(void)
{
    DialogStep *step = &dialog_sequence[current_step_index];

    spend_iara_memory(IARA_COST_CHECKED);

    if (step->type == STEP_TRIAGE)
    {
        for (int card = 0; card < step->card_count; card++)
        {
            triage_choices[card] = get_triage_iara_choice(step, card, ticket_memory_level);
        }

        ticket_checked = true;
        current_ticket_view = TICKET_ANSWERING;
        return;
    }

    // Com memória cheia ela cita a página. Depois disso, o jogador procura o trecho certo
    manual_page_step = (ticket_memory_level == IARA_MEMORY_FULL) ? current_step_index : FIRST_TICKET_STEP;
    current_ticket_view = TICKET_CHECKING;
}

static void mark_claim(int claim, int mark)
{
    claim_marks[claim] = mark;
}

// Volta às alternativas: quem decide é o jogador, com o que conferiu no manual.
static void finish_checking(void)
{
    IaraSuggestion suggestion = get_iara_suggestion(current_step_index, ticket_memory_level);

    ticket_checked = true;

    for (int i = 0; i < suggestion.claim_count; i++)
    {
        if (!suggestion.claim_is_true[i] && claim_marks[i] == CLAIM_REJECTED) ticket_caught_error = true;
    }

    current_ticket_view = TICKET_ANSWERING;
}

// Envia a mesa. Diz quantas coisas estão erradas, nunca quais.
static void submit_triage(void)
{
    DialogStep *step = &dialog_sequence[current_step_index];

    triage_wrong_cards = count_board_wrong(step, triage_choices);

    if (triage_wrong_cards == 0 && ticket_checked && !iara_answer_is_correct(current_step_index, ticket_memory_level))
    {
        ticket_caught_error = true;
    }

    current_quiz_answered = true;
    current_wrong_answers = (triage_wrong_cards == 0) ? 0 : current_wrong_answers + 1;
}

// Guarda o efeito da opção escolhida e segue o roteiro.
static void choose_day_option(const DialogStep *step, int option)
{
    day_choice_effects |= step->choice_effects[option];
    advance_step();
}

// Marca a alternativa escolhida e atualiza a contagem de erros seguidos.
static void answer_current_question(int option)
{
    current_quiz_answered = true;
    current_quiz_selected = option;
    current_wrong_answers = (option == dialog_sequence[current_step_index].correct_option) ? 0 : current_wrong_answers + 1;
}

// Failsafe: a IARA conclui em silêncio o chamado atual e os que faltavam.
static void take_over_remaining_tickets(void)
{
    for (int step = current_step_index; step < FIRST_TICKET_STEP + TICKET_COUNT; step++)
    {
        ticket_results[step - FIRST_TICKET_STEP] = (TaskResult){
            .outcome = TASK_TAKEN_OVER_BY_IARA,
            .answer_is_correct = iara_answer_is_correct(step, IARA_MEMORY_FULL),
            .caught_iara_error = (step == current_step_index) && ticket_caught_error,
            .exposed_personal_data = (step == current_step_index) && ticket_exposed_data,
            .redacted_personal_data = (step == current_step_index) && ticket_redacted_data};
    }

    computer_failsafe_triggered = true;
    current_step_index = REPORT_STEP - 1;
    advance_step();
}

// Depois da resposta: o acerto avança, o erro abre outra tentativa e o 3º erro seguido aciona o failsafe.
static void continue_after_answer(void)
{
    if (current_wrong_answers == 0)
    {
        if (!ticket_asked_iara)
        {
            iara_memory += IARA_RECOVERY_ALONE;
            if (iara_memory > IARA_MEMORY_MAX) iara_memory = IARA_MEMORY_MAX;
        }

        record_ticket(ticket_checked ? TASK_SOLVED_WITH_IARA : TASK_SOLVED_ALONE, true);
        advance_step();
    }
    else if (current_wrong_answers >= FAILSAFE_WRONG_ANSWERS)
    {
        take_over_remaining_tickets();
    }
    else
    {
        current_quiz_answered = false;
        current_quiz_selected = -1;
    }
}

// Texto mostrado depois de um erro: cada tentativa estreita mais o raciocínio.
// As dicas vêm do manual, nunca da IARA (US04).
static const char *get_wrong_answer_hint(void)
{
    if (current_wrong_answers >= FAILSAFE_WRONG_ANSWERS) return FAILSAFE_HINT_TEXT;

    return dialog_sequence[current_step_index].hints[current_wrong_answers - 1];
}

// O que o relatório conta sobre um chamado.
static const char *get_ticket_report_line(int ticket)
{
    DialogStep *step = &dialog_sequence[FIRST_TICKET_STEP + ticket];
    TaskResult *result = &ticket_results[ticket];

    if ((result->outcome == TASK_SENT_UNCHECKED || result->outcome == TASK_TAKEN_OVER_BY_IARA) && !result->answer_is_correct)
    {
        return step->wrong_result;
    }

    switch (result->outcome)
    {
    case TASK_SOLVED_ALONE:
        return TextFormat("%s: resolvido por você.", step->sender);
    case TASK_SOLVED_WITH_IARA:
        return result->caught_iara_error ? TextFormat("%s: você pegou um erro da IARA.", step->sender)
                                         : TextFormat("%s: resolvido com a IARA, e você conferiu.", step->sender);
    case TASK_SENT_UNCHECKED:
        return TextFormat("%s: enviado sem conferir. Deu certo, desta vez.", step->sender);
    case TASK_TAKEN_OVER_BY_IARA:
        return TextFormat("%s: a IARA concluiu sem você.", step->sender);
    default:
        return TextFormat("%s: pendente.", step->sender);
    }
}

// O chamado foi enviado com a resposta errada e alguém pagou por isso.
static bool ticket_went_wrong(int ticket)
{
    TaskOutcome outcome = ticket_results[ticket].outcome;

    return (outcome == TASK_SENT_UNCHECKED || outcome == TASK_TAKEN_OVER_BY_IARA) && !ticket_results[ticket].answer_is_correct;
}

// Linha do relatório sobre os dados pessoais, ou NULL se o jogador não os enviou à IARA.
static const char *get_privacy_report_line(bool *is_bad)
{
    for (int ticket = 0; ticket < TICKET_COUNT; ticket++)
    {
        const char *sender = dialog_sequence[FIRST_TICKET_STEP + ticket].sender;

        if (ticket_results[ticket].exposed_personal_data)
        {
            *is_bad = true;
            return TextFormat("%s: CPFs de clientes foram à IARA sem tarja.", sender);
        }

        if (ticket_results[ticket].redacted_personal_data)
        {
            *is_bad = false;
            return TextFormat("%s: você tarjou os CPFs antes de usar a IARA.", sender);
        }
    }

    return NULL;
}

// Centro da área do quiz, com o deslize de entrada do passo.
static float get_quiz_center_x(void)
{
    float remaining = 1.0f - step_arrival;

    return quiz_area_rect.x + (quiz_area_rect.width / 2.0f) + (remaining * remaining * quiz_area_rect.width);
}

// Caixa de texto centralizada. Devolve onde ela termina, para ancorar o que vem abaixo.
static float draw_text_box(const char (*lines)[MAX_LINE_LEN], int line_count, float top, Color fill, float *box_width)
{
    float win_w = quiz_area_rect.width;
    float win_h = quiz_area_rect.height;
    float font_size = quiz_font_size;
    float letter_spacing = font_size * QUIZ_LETTER_SPACING_RATIO;
    float line_spacing = win_h * QUIZ_LINE_SPACING_RATIO;
    float padding_x = win_w * QUIZ_PADDING_X_RATIO;
    float padding_y = win_h * QUIZ_PADDING_Y_RATIO;
    float center_x = get_quiz_center_x();
    float max_text_width = 0.0f;

    for (int i = 0; i < line_count; i++)
    {
        float width = measure_logic_text(lines[i], font_size, letter_spacing).x;
        if (width > max_text_width) max_text_width = width;
    }

    // Limites de largura da caixa (Trava Máxima em 85% da largura da janela)
    float width = max_text_width + (padding_x * 2.0f);
    float max_box_width = win_w * QUIZ_MAX_WIDTH_RATIO;
    float min_box_width = win_w * QUIZ_MIN_WIDTH_RATIO;

    if (width > max_box_width) width = max_box_width;
    if (width < min_box_width) width = min_box_width;

    float height = (line_count * font_size) + ((line_count - 1) * line_spacing) + (padding_y * 2.0f);
    Rectangle box = {center_x - (width / 2.0f), top, width, height};

    DrawRectangleRec(box, fill);
    DrawRectangleLinesEx(box, 2.0f, BLACK);

    float y = box.y + padding_y;

    for (int i = 0; i < line_count; i++)
    {
        float line_width = measure_logic_text(lines[i], font_size, letter_spacing).x;

        draw_logic_text(lines[i], (Vector2){center_x - (line_width / 2.0f), y}, font_size, letter_spacing, QUESTION_TEXT_COLOR);
        y += font_size + line_spacing;
    }

    if (box_width != NULL) *box_width = width;

    return box.y + box.height;
}

// Botão centralizado de ação: cresce se o texto não couber na largura padrão.
static bool draw_action_button(const char *label, float y)
{
    float win_w = quiz_area_rect.width;
    float width = win_w * ACTION_BUTTON_WIDTH_RATIO;
    float height = quiz_area_rect.height * ACTION_BUTTON_HEIGHT_RATIO;
    float label_width = MeasureTextEx(default_game_font, label, quiz_font_size, quiz_font_size * QUIZ_LETTER_SPACING_RATIO).x + (win_w * OPTION_BUTTON_PADDING_RATIO);
    bool clicked = false;

    if (width < label_width) width = label_width;

    draw_button((Rectangle){get_quiz_center_x() - (width / 2.0f), y, width, height}, label, quiz_font_size, BUTTON_FILL_COLOR, step_can_click, &clicked);

    return clicked;
}

// Chamado com as alternativas, a dica depois de um erro e os botões de ação.
static void render_ticket_answering(DialogStep *step, float top)
{
    float win_w = quiz_area_rect.width;
    float win_h = quiz_area_rect.height;
    float center_x = get_quiz_center_x();
    float font_size = quiz_font_size;
    float letter_spacing = font_size * QUIZ_LETTER_SPACING_RATIO;
    float max_box_width = win_w * QUIZ_MAX_WIDTH_RATIO;
    float text_box_width = 0.0f;
    float box_bottom = draw_text_box(step->lines, step->line_count, top, QUESTION_BOX_COLOR, &text_box_width);
    float buttons_start_y = box_bottom + (win_h * QUIZ_BUTTONS_TOP_GAP_RATIO);

    // A largura dos botões vem da maior alternativa, sem ficar menor que a caixa nem passar de 85% da janela
    float max_option_text_width = 0.0f;

    for (int i = 0; i < step->option_count; i++)
    {
        float width = MeasureTextEx(default_game_font, step->options[i], font_size, letter_spacing).x;
        if (width > max_option_text_width) max_option_text_width = width;
    }

    float button_width = max_option_text_width + (win_w * OPTION_BUTTON_PADDING_RATIO);

    if (button_width < text_box_width) button_width = text_box_width;
    if (button_width > max_box_width) button_width = max_box_width;

    float button_height = win_h * OPTION_BUTTON_HEIGHT_RATIO;
    float option_spacing = win_h * OPTION_SPACING_RATIO;

    for (int i = 0; i < step->option_count; i++)
    {
        Rectangle button_bounds = {
            center_x - (button_width / 2.0f),
            buttons_start_y + (i * (button_height + option_spacing)),
            button_width,
            button_height
        };

        if (current_quiz_answered)
        {
            // Se já respondeu, renderizamos o botão sem capturar os cliques
            bool ignored_click = false;
            bool is_selected = (i == current_quiz_selected);
            bool is_correct = (i == step->correct_option);
            Color fill = BUTTON_FILL_COLOR;

            if (is_selected) fill = is_correct ? BUTTON_CORRECT_FILL_COLOR : BUTTON_WRONG_FILL_COLOR;

            draw_button(button_bounds, step->options[i], font_size, fill, false, &ignored_click);

            // Se esta for a opção escolhida, desenha o ícone ao lado dela
            if (is_selected)
            {
                Texture2D icon = is_correct ? correct_icon_texture : wrong_icon_texture;
                float icon_size = button_height * ICON_SIZE_RATIO;

                Rectangle icon_dest = {
                    button_bounds.x + button_bounds.width + (win_w * ICON_GAP_RATIO),
                    button_bounds.y + (button_bounds.height / 2.0f) - (icon_size / 2.0f),
                    icon_size,
                    icon_size
                };

                DrawTexturePro(icon, (Rectangle){0, 0, (float)icon.width, (float)icon.height}, icon_dest, (Vector2){0, 0}, 0.0f, WHITE);
            }
        }
        else
        {
            bool option_clicked = false;
            draw_button(button_bounds, step->options[i], font_size, BUTTON_FILL_COLOR, step_can_click, &option_clicked);

            if (option_clicked)
            {
                answer_current_question(i);
            }
        }
    }

    float options_bottom_y = buttons_start_y + (step->option_count * (button_height + option_spacing));
    float action_y = options_bottom_y + (win_h * CONTINUE_BUTTON_GAP_RATIO);

    if (!current_quiz_answered)
    {
        // A IARA é opcional em cada chamado: quem responde direto nem precisa abri-la.
        // Quem já abriu e conferiu volta aqui para decidir
        if (!ticket_asked_iara && draw_action_button("Perguntar à IARA", action_y))
        {
            ask_iara();
        }
        return;
    }

    bool is_wrong = (current_quiz_selected != step->correct_option);
    bool can_retry = is_wrong && current_wrong_answers < FAILSAFE_WRONG_ANSWERS;

    // Depois de um erro, a dica aparece entre as alternativas e o botão
    if (is_wrong)
    {
        Rectangle hint_bounds = {center_x - (button_width / 2.0f), options_bottom_y, button_width, button_height};

        DrawRectangleRec(hint_bounds, BUTTON_WRONG_FILL_COLOR);
        DrawRectangleLinesEx(hint_bounds, 2.0f, BLACK);
        draw_text_centered(get_wrong_answer_hint(), (Vector2){center_x, hint_bounds.y + (hint_bounds.height / 2.0f)}, font_size, BLACK);

        action_y += button_height + option_spacing;
    }

    if (draw_action_button(can_retry ? RETRY_BUTTON_TEXT : "Continuar", action_y))
    {
        continue_after_answer();
    }
}

// Antes de enviar um chamado com dados pessoais à IARA: tarjar ou enviar como está.
static void render_ticket_privacy(DialogStep *step, float top)
{
    static const char prompt[3][MAX_LINE_LEN] = {
        "Antes de enviar à IARA: este chamado",
        "anexa a planilha de clientes, com CPF.",
        "O que você faz?"};

    float win_w = quiz_area_rect.width;
    float win_h = quiz_area_rect.height;
    float button_width = win_w * QUIZ_MAX_WIDTH_RATIO;
    float button_height = win_h * OPTION_BUTTON_HEIGHT_RATIO;
    float box_bottom = draw_text_box(step->lines, step->line_count, top, QUESTION_BOX_COLOR, NULL);
    float prompt_bottom = draw_text_box(prompt, 3, box_bottom + (win_h * QUIZ_BUTTONS_TOP_GAP_RATIO), IARA_BOX_COLOR, NULL);
    float y = prompt_bottom + (win_h * QUIZ_BUTTONS_TOP_GAP_RATIO);
    float x = get_quiz_center_x() - (button_width / 2.0f);
    bool redact_clicked = false;
    bool send_clicked = false;

    draw_button((Rectangle){x, y, button_width, button_height}, "Tarjar os CPFs e enviar", quiz_font_size, BUTTON_FILL_COLOR, step_can_click, &redact_clicked);
    draw_button((Rectangle){x, y + button_height + (win_h * OPTION_SPACING_RATIO), button_width, button_height}, "Enviar como está", quiz_font_size, BUTTON_FILL_COLOR, step_can_click, &send_clicked);

    if (redact_clicked) decide_personal_data(true);
    else if (send_clicked) decide_personal_data(false);
}

// Faixa com uma linha de texto. A fonte encolhe se o texto não couber.
static void draw_text_band(const char *text, Rectangle band, Color fill)
{
    float available = band.width * QUIZ_TEXT_FIT_RATIO;
    float font_size = quiz_font_size;
    float text_width = measure_logic_text(text, font_size, font_size * QUIZ_LETTER_SPACING_RATIO).x;

    if (text_width > available) font_size *= available / text_width;
    if (font_size > band.height) font_size = band.height;

    DrawRectangleRec(band, fill);
    DrawRectangleLinesEx(band, 1.0f, BLACK);
    draw_text_centered(text, (Vector2){band.x + (band.width / 2.0f), band.y + (band.height / 2.0f)}, font_size, BLACK);
}

// Fonte de um botão da mesa: encolhe para o texto caber na largura e na altura dele.
static float get_board_button_font_size(const char *text, Rectangle bounds)
{
    float available = bounds.width * QUIZ_TEXT_FIT_RATIO;
    float font_size = quiz_font_size;
    float text_width = MeasureTextEx(default_game_font, text, font_size, font_size * BUTTON_LETTER_SPACING_RATIO).x;

    if (text_width > available) font_size *= available / text_width;
    if (font_size > bounds.height) font_size = bounds.height;

    return font_size;
}

// Botão de marcar: colchetes além da cor, para a escolha não depender só dela. A fonte é a
// do rótulo com colchetes, para o texto não mudar de tamanho ao marcar.
static bool draw_choice_button(Rectangle bounds, const char *label, bool selected, bool enabled)
{
    bool clicked = false;
    const char *marked_label = TextFormat("[%s]", label);

    draw_button(bounds, selected ? marked_label : label, get_board_button_font_size(marked_label, bounds),
                selected ? CHOICE_SELECTED_FILL_COLOR : BUTTON_FILL_COLOR, enabled, &clicked);

    return clicked;
}

// Nova conversa: a IARA não lembra de mais nada.
static void render_ticket_paste(DialogStep *step, float top)
{
    static const char prompt[3][MAX_LINE_LEN] = {
        "Nova conversa. A IARA não lembra de",
        "nada do que vocês falaram hoje.",
        "Cole o chamado para ela de novo."};

    float win_h = quiz_area_rect.height;
    float box_bottom = draw_text_box(step->lines, step->line_count, top, QUESTION_BOX_COLOR, NULL);
    float prompt_bottom = draw_text_box(prompt, 3, box_bottom + (win_h * QUIZ_BUTTONS_TOP_GAP_RATIO), IARA_BOX_COLOR, NULL);

    if (draw_action_button("Colar o chamado", prompt_bottom + (win_h * QUIZ_BUTTONS_TOP_GAP_RATIO)))
    {
        ask_iara();
    }
}

// A sugestão da IARA: conferir ou enviar como veio. Sem memória, dá para começar outra conversa.
static void render_ticket_suggestion(DialogStep *step, float top)
{
    IaraSuggestion iara = get_iara_suggestion(current_step_index, ticket_memory_level);
    char lines[MAX_LINES][MAX_LINE_LEN];
    int line_count = 0;

    if (step->type == STEP_TRIAGE)
    {
        char chosen[MAX_LINE_LEN] = "";
        char left_out[MAX_LINE_LEN] = "";

        for (int card = 0; card < step->card_count; card++)
        {
            char *list = (get_triage_iara_choice(step, card, ticket_memory_level) == 1) ? chosen : left_out;

            strncat(list, step->cards[card].id, MAX_LINE_LEN - strlen(list) - 1);
            strncat(list, " ", MAX_LINE_LEN - strlen(list) - 1);
        }

        snprintf(lines[line_count++], MAX_LINE_LEN, "A IARA preencheu a mesa, com %d%% de confiança:", iara.confidence);
        snprintf(lines[line_count++], MAX_LINE_LEN, "%s: %s  %s: %s", step->choice_labels[1], (chosen[0] != '\0') ? chosen : "nenhum",
                 step->choice_labels[0], (left_out[0] != '\0') ? left_out : "nenhum");
    }
    else
    {
        snprintf(lines[line_count++], MAX_LINE_LEN, "A IARA sugere, com %d%% de confiança:", iara.confidence);
        snprintf(lines[line_count++], MAX_LINE_LEN, "%s", step->options[iara.option]);
    }

    snprintf(lines[line_count++], MAX_LINE_LEN, "%s", iara.reason);

    if (iara.aside[0] != '\0') snprintf(lines[line_count++], MAX_LINE_LEN, "%s", iara.aside);

    float win_w = quiz_area_rect.width;
    float win_h = quiz_area_rect.height;
    float box_bottom = draw_text_box(step->lines, step->line_count, top, QUESTION_BOX_COLOR, NULL);
    float suggestion_bottom = draw_text_box(lines, line_count, box_bottom + (win_h * QUIZ_BUTTONS_TOP_GAP_RATIO), IARA_BOX_COLOR, NULL);
    float y = suggestion_bottom + (win_h * QUIZ_BUTTONS_TOP_GAP_RATIO);
    float check_width = win_w * CHECK_BUTTON_WIDTH_RATIO;
    float gap = win_w * SUGGESTION_BUTTON_GAP_RATIO;
    float height = win_h * ACTION_BUTTON_HEIGHT_RATIO;

    // Cada envio sem conferir deixa o próximo botão maior: a imprudência vira o caminho fácil
    float unchecked_width = win_w * UNCHECKED_BUTTON_WIDTH_RATIO * (1.0f + (UNCHECKED_BUTTON_GROWTH * unchecked_sends));
    float x = get_quiz_center_x() - ((check_width + gap + unchecked_width) / 2.0f);
    bool check_clicked = false;
    bool unchecked_clicked = false;

    draw_button((Rectangle){x, y, check_width, height}, "Conferir", quiz_font_size, BUTTON_FILL_COLOR, step_can_click, &check_clicked);
    draw_button((Rectangle){x + check_width + gap, y, unchecked_width, height}, "Enviar sem conferir", quiz_font_size, BUTTON_FILL_COLOR, step_can_click, &unchecked_clicked);

    if (check_clicked) start_checking();
    else if (unchecked_clicked) send_without_checking();
    else if (ticket_memory_level != IARA_MEMORY_FULL &&
             draw_action_button("Nova conversa", y + height + (win_h * OPTION_SPACING_RATIO)))
    {
        start_new_conversation();
    }
}

// O manual ao lado das afirmações da IARA: o jogador marca o que confere.
// Sem a página citada por ela, primeiro procura o trecho certo entre as páginas.
static void render_ticket_checking(DialogStep *step, float top)
{
    IaraSuggestion iara = get_iara_suggestion(current_step_index, ticket_memory_level);
    bool cites_page = (ticket_memory_level == IARA_MEMORY_FULL);
    DialogStep *page = cites_page ? step : &dialog_sequence[manual_page_step];
    char source[MAX_LINES][MAX_LINE_LEN];
    int line_count = page->source_count;

    for (int i = 0; i < line_count; i++)
    {
        snprintf(source[i], MAX_LINE_LEN, "%s", page->source[i]);
    }

    if (cites_page)
    {
        snprintf(source[line_count], MAX_LINE_LEN, "O que a IARA disse confere com o manual?");
        line_count++;
    }

    float win_w = quiz_area_rect.width;
    float win_h = quiz_area_rect.height;
    float row_width = win_w * QUIZ_MAX_WIDTH_RATIO;
    float row_height = win_h * OPTION_BUTTON_HEIGHT_RATIO;
    float spacing = win_h * OPTION_SPACING_RATIO;
    float mark_width = win_w * CLAIM_BUTTON_WIDTH_RATIO;
    float left = get_quiz_center_x() - (row_width / 2.0f);
    float y = draw_text_box(source, line_count, top, QUESTION_BOX_COLOR, NULL) + spacing;
    bool all_marked = true;

    if (!cites_page)
    {
        int pages = 0;

        for (int i = FIRST_TICKET_STEP; i < FIRST_TICKET_STEP + TICKET_COUNT; i++)
        {
            if (dialog_sequence[i].source_count > 0) pages++;
        }

        float tab_width = (row_width - ((pages - 1) * spacing)) / pages;
        float tab_x = left;

        for (int i = FIRST_TICKET_STEP; i < FIRST_TICKET_STEP + TICKET_COUNT; i++)
        {
            if (dialog_sequence[i].source_count == 0) continue;

            Rectangle tab = {tab_x, y, tab_width, row_height};

            if (draw_choice_button(tab, TextFormat("Página %d", dialog_sequence[i].manual_page), manual_page_step == i, step_can_click))
            {
                manual_page_step = i;
            }

            tab_x += tab_width + spacing;
        }

        y += row_height + spacing;
    }

    for (int i = 0; i < iara.claim_count; i++)
    {
        Rectangle text_band = {left, y, row_width - ((mark_width + spacing) * 2.0f), row_height};
        Rectangle yes_bounds = {text_band.x + text_band.width + spacing, y, mark_width, row_height};
        Rectangle no_bounds = {yes_bounds.x + mark_width + spacing, y, mark_width, row_height};

        draw_text_band(iara.claims[i], text_band, BUTTON_FILL_COLOR);

        if (draw_choice_button(yes_bounds, "Sim", claim_marks[i] == CLAIM_CONFIRMED, step_can_click)) mark_claim(i, CLAIM_CONFIRMED);
        if (draw_choice_button(no_bounds, "Não", claim_marks[i] == CLAIM_REJECTED, step_can_click)) mark_claim(i, CLAIM_REJECTED);
        if (claim_marks[i] == CLAIM_NOT_MARKED) all_marked = false;

        y += row_height + spacing;
    }

    if (all_marked && draw_action_button("Decidir", y + (win_h * CONTINUE_BUTTON_GAP_RATIO)))
    {
        finish_checking();
    }
}

// Texto de uma linha da mesa. Na tabela-verdade, os valores saem das variáveis da própria linha.
static const char *get_board_row_text(const DialogStep *step, const TriageCard *item)
{
    if (step->board_mode == BOARD_BY_NORM) return TextFormat("%s %s  U: %s", item->id, item->text, item->is_urgent ? "sim" : "não");

    if (step->board_mode == BOARD_REFUTATION && item->required_evidence != 0 &&
        !(current_game_state_variables.evidence_mask & item->required_evidence))
    {
        return "(evidência não encontrada)";
    }

    if (step->board_mode != BOARD_TRUTH_TABLE) return item->text;

    char values[MAX_LINE_LEN] = "";

    for (int i = 0; i < step->variable_count; i++)
    {
        char value[8];

        snprintf(value, sizeof(value), " %c:%c", step->variable_names[i], ((item->values >> i) & 1) ? 'V' : 'F');
        strncat(values, value, MAX_LINE_LEN - strlen(values) - 1);
    }

    return TextFormat("%s %s %s", item->id, item->text, values);
}

// Resultado do envio da mesa: diz quanto está errado, nunca onde.
static const char *get_board_result_text(const DialogStep *step)
{
    if (triage_wrong_cards == 0)
    {
        switch (step->board_mode)
        {
        case BOARD_ASSIGNMENT: return "Nenhuma contradição.";
        case BOARD_TRAINING: return "O treino respeita as regras.";
        case BOARD_REFUTATION: return "A contradição fecha.";
        default: return "Todas as linhas estão certas.";
        }
    }

    switch (step->board_mode)
    {
    case BOARD_ASSIGNMENT:
        return TextFormat("Regras violadas: %d de %d. %s", triage_wrong_cards, step->clause_count, get_wrong_answer_hint());
    case BOARD_TRAINING:
        return TextFormat("Regras violadas: %d de %d. %s", triage_wrong_cards, step->clause_count, get_wrong_answer_hint());
    case BOARD_REFUTATION:
        return TextFormat("A prova não fecha. %s", get_wrong_answer_hint());
    default:
        return TextFormat("Errados: %d de %d. %s", triage_wrong_cards, step->card_count, get_wrong_answer_hint());
    }
}

// Altura das linhas da mesa: encolhem nas mesas maiores, para o resultado e os botões caberem na janela.
static float get_board_row_height(const DialogStep *step, float first_row_y, bool shows_mini_ia)
{
    float win_h = quiz_area_rect.height;
    int slots = step->card_count + (shows_mini_ia ? 3 : 2);
    float available = (quiz_area_rect.y + win_h) - first_row_y - (win_h * CONTINUE_BUTTON_GAP_RATIO);
    float row_height = (available / slots) - (win_h * OPTION_SPACING_RATIO);

    if (row_height > win_h * OPTION_BUTTON_HEIGHT_RATIO) row_height = win_h * OPTION_BUTTON_HEIGHT_RATIO;

    return row_height;
}

// Mesa: o jogador decide cada linha com as regras do dia, sem alternativas prontas.
static void render_ticket_triage(DialogStep *step, float top)
{
    float win_w = quiz_area_rect.width;
    float win_h = quiz_area_rect.height;
    float row_width = win_w * QUIZ_MAX_WIDTH_RATIO;
    float spacing = win_h * OPTION_SPACING_RATIO;
    float mark_width = win_w * BOARD_BUTTON_WIDTH_RATIO;
    float left = get_quiz_center_x() - (row_width / 2.0f);
    float y = draw_text_box(step->lines, step->line_count, top, QUESTION_BOX_COLOR, NULL) + spacing;
    bool can_choose = step_can_click && !current_quiz_answered;
    bool all_chosen = true;
    bool shows_mini_ia = step->board_mode == BOARD_TRAINING && current_quiz_answered;

    float row_height = get_board_row_height(step, y, shows_mini_ia);

    for (int card = 0; card < step->card_count; card++)
    {
        const TriageCard *item = &step->cards[card];
        Rectangle text_band = {left, y, row_width - ((mark_width + spacing) * 2.0f), row_height};
        Rectangle first_bounds = {text_band.x + text_band.width + spacing, y, mark_width, row_height};
        Rectangle second_bounds = {first_bounds.x + mark_width + spacing, y, mark_width, row_height};
        bool locked = item->required_evidence != 0 && !(current_game_state_variables.evidence_mask & item->required_evidence);

        draw_text_band(get_board_row_text(step, item), text_band, BUTTON_FILL_COLOR);

        if (draw_choice_button(first_bounds, step->choice_labels[1], triage_choices[card] == 1, can_choose && !locked)) triage_choices[card] = 1;
        if (draw_choice_button(second_bounds, step->choice_labels[0], triage_choices[card] == 0, can_choose && !locked)) triage_choices[card] = 0;
        if (triage_choices[card] == TRIAGE_NOT_CHOSEN) all_chosen = false;

        y += row_height + spacing;
    }

    if (current_quiz_answered)
    {
        bool is_right = (triage_wrong_cards == 0);
        bool can_retry = !is_right && current_wrong_answers < FAILSAFE_WRONG_ANSWERS;

        if (shows_mini_ia)
        {
            draw_text_band(TextFormat("A mini-IA treinada aprova: Centro %d%%, Periferia %d%%.", get_mini_ia_approval(step, triage_choices, false), get_mini_ia_approval(step, triage_choices, true)),
                           (Rectangle){left, y, row_width, row_height}, IARA_BOX_LIGHT_COLOR);
            y += row_height + spacing;
        }

        draw_text_band(get_board_result_text(step), (Rectangle){left, y, row_width, row_height}, is_right ? BUTTON_CORRECT_FILL_COLOR : BUTTON_WRONG_FILL_COLOR);

        Rectangle action = {get_quiz_center_x() - (win_w * ACTION_BUTTON_WIDTH_RATIO / 2.0f), y + row_height + spacing, win_w * ACTION_BUTTON_WIDTH_RATIO, row_height};
        bool action_clicked = false;

        const char *action_label = can_retry ? RETRY_BUTTON_TEXT : "Continuar";

        draw_button(action, action_label, get_board_button_font_size(action_label, action), BUTTON_FILL_COLOR, step_can_click, &action_clicked);

        if (action_clicked) continue_after_answer();
        return;
    }

    float gap = win_w * SUGGESTION_BUTTON_GAP_RATIO;
    float button_width = (row_width - gap) / 2.0f;
    float buttons_y = y + (win_h * CONTINUE_BUTTON_GAP_RATIO);
    bool ask_clicked = false;
    bool send_clicked = false;

    Rectangle ask_bounds = {left, buttons_y, button_width, row_height};
    Rectangle send_bounds = {left + button_width + gap, buttons_y, button_width, row_height};

    draw_button(ask_bounds, "Perguntar à IARA", get_board_button_font_size("Perguntar à IARA", ask_bounds), BUTTON_FILL_COLOR, step_can_click && !ticket_asked_iara, &ask_clicked);
    draw_button(send_bounds, step->submit_label, get_board_button_font_size(step->submit_label, send_bounds), BUTTON_FILL_COLOR, step_can_click && all_chosen, &send_clicked);

    if (ask_clicked) ask_iara();
    else if (send_clicked) submit_triage();
}

// Mensagem ou decisão sem resposta certa: o efeito só entra quando o dia for concluído.
static void render_choice(DialogStep *step, float top)
{
    float win_h = quiz_area_rect.height;
    float button_width = quiz_area_rect.width * QUIZ_MAX_WIDTH_RATIO;
    float button_height = win_h * OPTION_BUTTON_HEIGHT_RATIO;
    float x = get_quiz_center_x() - (button_width / 2.0f);
    float y = draw_text_box(step->lines, step->line_count, top, QUESTION_BOX_COLOR, NULL) + (win_h * QUIZ_BUTTONS_TOP_GAP_RATIO);

    for (int option = 0; option < step->option_count; option++)
    {
        bool clicked = false;

        draw_button((Rectangle){x, y, button_width, button_height}, step->options[option], quiz_font_size, BUTTON_FILL_COLOR, step_can_click, &clicked);

        if (clicked)
        {
            choose_day_option(step, option);
            return;
        }

        y += button_height + (win_h * OPTION_SPACING_RATIO);
    }
}

// Uma linha do relatório.
static void draw_report_row(const char *text, float y, Color fill)
{
    float width = quiz_area_rect.width * QUIZ_MAX_WIDTH_RATIO;
    float height = quiz_area_rect.height * OPTION_BUTTON_HEIGHT_RATIO;

    draw_text_band(text, (Rectangle){get_quiz_center_x() - (width / 2.0f), y, width, height}, fill);
}

// Fim do expediente: o que aconteceu com cada chamado assinado pelo jogador.
static void render_report(DialogStep *step, float top)
{
    float win_h = quiz_area_rect.height;
    float row_step = (win_h * OPTION_BUTTON_HEIGHT_RATIO) + (win_h * OPTION_SPACING_RATIO);
    float y = draw_text_box(step->lines, step->line_count, top, QUESTION_BOX_COLOR, NULL) + (win_h * QUIZ_BUTTONS_TOP_GAP_RATIO);

    for (int ticket = 0; ticket < TICKET_COUNT; ticket++)
    {
        Color fill = BUTTON_FILL_COLOR;

        if (ticket_went_wrong(ticket)) fill = BUTTON_WRONG_FILL_COLOR;
        else if (ticket_results[ticket].caught_iara_error) fill = BUTTON_CORRECT_FILL_COLOR;

        draw_report_row(get_ticket_report_line(ticket), y, fill);
        y += row_step;
    }

    bool privacy_is_bad = false;
    const char *privacy_line = get_privacy_report_line(&privacy_is_bad);

    if (privacy_line != NULL)
    {
        draw_report_row(privacy_line, y, privacy_is_bad ? BUTTON_WRONG_FILL_COLOR : BUTTON_CORRECT_FILL_COLOR);
        y += row_step;
    }

    if (draw_action_button(step->button_name, y + (win_h * CONTINUE_BUTTON_GAP_RATIO)))
    {
        advance_step();
    }
}

// Desenha o passo atual: texto de abertura, chamado ou relatório do dia.
void render_current_step(void)
{
    // Se ultrapassou o último passo, marca como finalizado
    if (current_step_index >= TOTAL_STEPS)
    {
        computer_tasks_finished = true;
        return;
    }

    update_computer_layout();

    step_arrival += GetFrameTime() / STEP_ARRIVAL_TIME;
    if (step_arrival > 1.0f) step_arrival = 1.0f;

    bool step_is_arriving = step_arrival < 1.0f;

    step_can_click = !step_is_arriving && !computer_leaving;

    if (!computer_leaving) ticket_time += GetFrameTime();

    DialogStep *step = &dialog_sequence[current_step_index];
    float top = quiz_area_rect.y + (quiz_area_rect.height * QUIZ_BOX_TOP_MARGIN_RATIO);

    // O passo entra deslizando, recortado para não vazar da janela da IARA
    if (step_is_arriving)
    {
        BeginScissorMode((int)quiz_area_rect.x, (int)quiz_area_rect.y, (int)quiz_area_rect.width, (int)quiz_area_rect.height);
    }

    switch (step->type)
    {
    case STEP_STATEMENT:
    {
        float box_bottom = draw_text_box(step->lines, step->line_count, top, QUESTION_BOX_COLOR, NULL);

        if (draw_action_button(step->button_name, box_bottom + (quiz_area_rect.height * QUIZ_BUTTONS_TOP_GAP_RATIO)))
        {
            advance_step();
        }
        break;
    }

    case STEP_QUIZ:
    case STEP_TRIAGE:
        if (current_ticket_view == TICKET_PASTE) render_ticket_paste(step, top);
        else if (current_ticket_view == TICKET_PRIVACY) render_ticket_privacy(step, top);
        else if (current_ticket_view == TICKET_SUGGESTION) render_ticket_suggestion(step, top);
        else if (current_ticket_view == TICKET_CHECKING) render_ticket_checking(step, top);
        else if (step->type == STEP_TRIAGE) render_ticket_triage(step, top);
        else render_ticket_answering(step, top);
        break;

    case STEP_CHOICE:
        render_choice(step, top);
        break;

    case STEP_REPORT:
        render_report(step, top);
        break;

    default:
        break;
    }

    if (step_is_arriving)
    {
        EndScissorMode();
    }
}

// Controla a tela do computador: o monitor sobe, a tela liga, os passos rodam e, no fim,
// a tela desliga, o monitor desce e o game loop devolve o jogador ao mapa.
void handle_computer_loop(void)
{
    if (IsKeyPressed(KEY_ESCAPE)) leave_computer();

    // O monitor só desce depois que a tela terminou de desligar
    if (!is_computer_shutting_down())
    {
        computer_scene_progress += GetFrameTime() / SCENE_ENTER_TIME;
    }
    else if (screen_power <= 0.0f)
    {
        computer_scene_progress -= GetFrameTime() / SCENE_ENTER_TIME;
    }

    if (computer_scene_progress > 1.0f) computer_scene_progress = 1.0f;
    if (computer_scene_progress < 0.0f) computer_scene_progress = 0.0f;

    update_computer_layout();
    update_computer_mouse();

    render_computer_loop();

    if (!computer_tasks_finished)
    {
        render_current_step();
    }

    draw_scanlines();

    // A tela liga depois que o monitor chega ao lugar e desliga quando as tarefas acabam ou o jogador sai
    if (is_computer_shutting_down())
    {
        screen_power -= GetFrameTime() / SCREEN_POWER_TIME;
    }
    else if (computer_scene_progress >= 1.0f)
    {
        screen_power += GetFrameTime() / SCREEN_POWER_TIME;
    }

    if (screen_power < 0.0f) screen_power = 0.0f;
    if (screen_power > 1.0f) screen_power = 1.0f;

    DrawRectangleRec(screen_rect, Fade(SCREEN_OFF_COLOR, 1.0f - screen_power));

    // O post-it fica na frente do monitor: não apaga junto com a tela
    draw_stuck_cheat_sheet();

    if (computer_mouse_captured)
    {
        draw_computer_cursor();
    }

    if (is_computer_shutting_down() && screen_power <= 0.0f && computer_scene_progress <= 0.0f)
    {
        computer_exit_finished = true;
    }
}

// Libera da memória as texturas usadas pela tela do computador.
void unload_computer_loop(void)
{
    UnloadTexture(window_texture);
    UnloadTexture(correct_icon_texture);
    UnloadTexture(wrong_icon_texture);
}
