#include <stdbool.h>
#include <stddef.h> // Define a constante NULL
#include <math.h>   // sinf() das ondas do papel de parede

#include "raylib.h"
#include "computer_loop.h"
#include "game_state.h"
#include "macros.h"
#include "button.h"
#include "fonts.h"

#define MAX_LINES 4
#define MAX_LINE_LEN 64
#define MAX_OPTIONS 4
#define MAX_OPTION_LEN 256

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

bool computer_tasks_finished = false;
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

// Tipo de passo do diálogo do computador:
// STEP_STATEMENT = apenas texto com botão; STEP_QUIZ = pergunta com opções.
typedef enum {
    STEP_STATEMENT,
    STEP_QUIZ
} StepType;

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
    char *button_name;  // Nome do botão (usado apenas em STEP_STATEMENT)
} DialogStep;

// Índice do passo atual em dialog_sequence.
static int current_step_index = 0;

// Roteiro completo exibido no computador, na ordem em que aparece para o jogador.
static DialogStep dialog_sequence[] = {
    {
        .type = STEP_STATEMENT,
        .lines = {
            "Bem-vindo(a) à CISSA, Estagiário.",
            "Antes de liberar suas tarefas, preciso",
            "apresentar como trabalhamos por aqui.",
            "Isso leva menos de dois minutos!"
        },
        .line_count = 4,
        .button_name = "Continuar"
    },
    {
        .type = STEP_QUIZ,
        .lines = {
            "O que é Inteligência Artificial, no sentido",
            "usado no dia a dia?"
        },
        .line_count = 2,
        .options = {
            "A) Um sistema treinado a partir de dados",
            "B) Um robô físico que imita humanos",
            "C) Um banco de dados gigante"
        },
        .option_count = 3,
        .correct_option = 0 // Resposta A
    },
    {
        .type = STEP_QUIZ,
        .lines = {
            "O que significa 'treinar' um modelo?",
        },
        .line_count = 1,
        .options = {
            "A) Instalar num computador mais rápido",
            "B) Ajustar com exemplos até aprender padrões",
            "C) Testar o programa apenas uma vez"
        },
        .option_count = 3,
        .correct_option = 1 // Resposta B
    },
    {
        .type = STEP_QUIZ,
        .lines = {
            "O que é viés num sistema de IA?",
        },
        .line_count = 1,
        .options = {
            "A) Um erro de digitação no código",
            "B) Tratamento desigual devido aos dados de treino",
            "C) Uma falha de hardware"
        },
        .option_count = 3,
        .correct_option = 1
    },
    {
        .type = STEP_QUIZ,
        .lines = {
            "Por que motivos dados pessoais processados",
            "por IA levantam questões de privacidade?"
        },
        .line_count = 2,
        .options = {
            "A) Uso de formas não imaginadas ou autorizadas",
            "B) Porque deixam o sistema mais lento",
            "C) Relevante apenas para empresas de tecnologia"
        },
        .option_count = 3,
        .correct_option = 0
    },
    {
        // Condicional simples e modus ponens (GDD 6.3): a regra vem no próprio chamado.
        .type = STEP_QUIZ,
        .lines = {
            "Seu primeiro chamado chegou. Norma da CISSA:",
            "Se é urgente (P), vai para o TI (Q): P -> Q.",
            "Este chamado está marcado como urgente.",
            "O que você faz com ele?"
        },
        .line_count = 4,
        .options = {
            "A) Encaminho para o TI",
            "B) Nada: a norma não diz o que fazer",
            "C) Arquivo: só o TI decide o que é urgente"
        },
        .option_count = 3,
        .correct_option = 0
    },
    {
        .type = STEP_STATEMENT,
        .lines = {
            "Você finalizou o quiz do dia inicial.",
            "Até o próximo dia, Estagiário! Boa sorte!",
        },
        .line_count = 2,
        .button_name = "Concluir"
    },
};

// Quantidade total de passos do roteiro.
#define TOTAL_STEPS ((int)(sizeof(dialog_sequence) / sizeof(dialog_sequence[0])))

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
            float width = MeasureTextEx(default_game_font, dialog_sequence[step].lines[line], reference_size, reference_spacing).x;
            if (width > longest) longest = width;
        }

        for (int option = 0; option < dialog_sequence[step].option_count; option++)
        {
            float width = MeasureTextEx(default_game_font, dialog_sequence[step].options[option], reference_size, reference_spacing).x;
            if (width > longest) longest = width;
        }
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

// Prende o mouse à tela do monitor enquanto o computador está aberto. Com o cursor
// capturado, o ponteiro do sistema some e o jogo desenha o dele, então um movimento
// rápido não consegue escapar nem clicar fora da janela. Ao perder o foco (ALT+TAB),
// o sistema devolve o mouse e o captura de novo quando o jogo volta.
static void update_computer_mouse(void)
{
    if (!computer_mouse_capture_enabled) return;

    if (computer_tasks_finished)
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
    Vector2 size = MeasureTextEx(default_game_font, text, font_size, letter_spacing);

    DrawTextEx(default_game_font, text, (Vector2){center.x - (size.x / 2.0f), center.y - (size.y / 2.0f)},
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

// Barra de tarefas: botão CISSA, programa aberto e o dia atual no lugar do relógio.
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

    const char *day = TextFormat("Dia %d", current_game_state_variables.current_day);
    float day_spacing = font_size * QUIZ_LETTER_SPACING_RATIO;
    Vector2 day_size = MeasureTextEx(default_game_font, day, font_size, day_spacing);

    DrawTextEx(default_game_font, day, (Vector2){taskbar_rect.x + taskbar_rect.width - day_size.x - (padding * 2.0f), center_y - (day_size.y / 2.0f)},
               font_size, day_spacing, TASKBAR_TEXT_COLOR);
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

    DrawTextEx(default_game_font, "IARA - Integração",
               (Vector2){iara_window_rect.x + (iara_window_rect.width * WINDOW_ART_DOTS_WIDTH_RATIO), center_y - (font_size / 2.0f)},
               font_size, letter_spacing, WHITE);

    // Progresso: quantas perguntas já apareceram e quantas existem no roteiro
    int total_questions = 0;
    int current_question = 0;

    for (int i = 0; i < TOTAL_STEPS; i++)
    {
        if (dialog_sequence[i].type != STEP_QUIZ) continue;

        total_questions++;
        if (i <= current_step_index) current_question = total_questions;
    }

    if (current_step_index < TOTAL_STEPS && dialog_sequence[current_step_index].type == STEP_QUIZ)
    {
        const char *progress = TextFormat("Pergunta %d de %d", current_question, total_questions);
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

// Desenha o passo atual: caixa de texto, botões de opção e botão de continuar.
void render_current_step(void)
{
    // Se ultrapassou o último passo, marca como finalizado
    if (current_step_index >= TOTAL_STEPS)
    {
        computer_tasks_finished = true;
        return;
    }

    update_computer_layout();

    DialogStep *step = &dialog_sequence[current_step_index];

    // 1. Obtém as dimensões atuais da área útil da janela da IARA
    float win_w = quiz_area_rect.width;
    float win_h = quiz_area_rect.height;
    float win_y = quiz_area_rect.y;
    float center_x = quiz_area_rect.x + (win_w / 2.0f);

    // 2. Escala da fonte ajustada proporcionalmente
    float font_size = quiz_font_size;
    float letter_spacing = font_size * QUIZ_LETTER_SPACING_RATIO;
    float line_spacing = win_h * QUIZ_LINE_SPACING_RATIO;

    // 3. Calcula a largura da maior linha de texto e altura acumulada
    float max_text_width = 0.0f;
    for (int i = 0; i < step->line_count; i++)
    {
        Vector2 line_size = MeasureTextEx(default_game_font, step->lines[i], font_size, letter_spacing);
        if (line_size.x > max_text_width)
        {
            max_text_width = line_size.x;
        }
    }

    float total_text_height = (step->line_count * font_size) + ((step->line_count - 1) * line_spacing);

    // 4. Margens internas (Paddings)
    float padding_x = win_w * QUIZ_PADDING_X_RATIO;
    float padding_y = win_h * QUIZ_PADDING_Y_RATIO;

    // 5. Limites de largura da caixa (Trava Máxima em 85% da largura da janela)
    float max_box_width = win_w * QUIZ_MAX_WIDTH_RATIO;
    float min_box_width = win_w * QUIZ_MIN_WIDTH_RATIO;

    float text_box_width = max_text_width + (padding_x * 2.0f);

    if (text_box_width > max_box_width) text_box_width = max_box_width;
    if (text_box_width < min_box_width) text_box_width = min_box_width;

    float text_box_height = total_text_height + (padding_y * 2.0f);
    float text_box_margin_top = win_h * QUIZ_BOX_TOP_MARGIN_RATIO; // Margem superior em relação à área útil

    Rectangle text_box = {
        center_x - (text_box_width / 2.0f),
        win_y + text_box_margin_top,
        text_box_width,
        text_box_height
    };

    // 6. Desenha fundo e borda da caixa
    DrawRectangleRec(text_box, QUESTION_BOX_COLOR);
    DrawRectangleLinesEx(text_box, 2.0f, BLACK);

    // 7. Renderiza as linhas do texto
    float current_y = text_box.y + padding_y;

    for (int i = 0; i < step->line_count; i++)
    {
        Vector2 text_size = MeasureTextEx(default_game_font, step->lines[i], font_size, letter_spacing);
        Vector2 text_pos = {
            center_x - (text_size.x / 2.0f),
            current_y
        };

        DrawTextEx(default_game_font, step->lines[i], text_pos, font_size, letter_spacing, QUESTION_TEXT_COLOR);
        current_y += font_size + line_spacing;
    }

    // 8. Renderiza os botões ancorados abaixo da caixa
    float buttons_start_y = text_box.y + text_box.height + (win_h * QUIZ_BUTTONS_TOP_GAP_RATIO);

    if (step->type == STEP_STATEMENT)
    {
        float button_width  = win_w * ACTION_BUTTON_WIDTH_RATIO;
        float button_height = win_h * ACTION_BUTTON_HEIGHT_RATIO;

        Rectangle button_bounds = {
            center_x - (button_width / 2.0f),
            buttons_start_y,
            button_width,
            button_height
        };

        bool clicked = false;
        const char *label = (step->button_name != NULL) ? step->button_name : "Continuar";

        draw_button(button_bounds, label, font_size, BUTTON_FILL_COLOR, true, &clicked);

        if (clicked)
        {
            current_step_index++;
        }
    }
    else if (step->type == STEP_QUIZ)
    {
        // 1. Encontra a largura da maior opção de resposta dinamicamente
        float max_option_text_width = 0.0f;
        for (int i = 0; i < step->option_count; i++)
        {
            Vector2 text_size = MeasureTextEx(default_game_font, step->options[i], font_size, letter_spacing);
            if (text_size.x > max_option_text_width)
            {
                max_option_text_width = text_size.x;
            }
        }

        // 2. Define a largura do botão baseada no texto + margens (padding)
        float button_padding = win_w * OPTION_BUTTON_PADDING_RATIO;
        float button_width = max_option_text_width + button_padding;

        // Garante que o botão não fique menor que a caixa da pergunta
        if (button_width < text_box_width)
        {
            button_width = text_box_width;
        }

        // NOVA REGRA: Garante que o botão não ultrapasse 85% da janela
        if (button_width > max_box_width)
        {
            button_width = max_box_width;
        }

        float button_height  = win_h * OPTION_BUTTON_HEIGHT_RATIO;
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

                    // Posiciona o ícone à direita do botão
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
                // Se ainda não respondeu, renderiza os botões capturando o clique normal
                bool option_clicked = false;
                draw_button(button_bounds, step->options[i], font_size, BUTTON_FILL_COLOR, true, &option_clicked);

                if (option_clicked)
                {
                    current_quiz_answered = true;
                    current_quiz_selected = i;
                }
            }
        }

        // Se o usuário respondeu, desenhamos o botão "Continuar" abaixo de todas as opções
        if (current_quiz_answered)
        {
            float continue_button_width  = win_w * ACTION_BUTTON_WIDTH_RATIO;
            float continue_button_height = win_h * ACTION_BUTTON_HEIGHT_RATIO;

            // Calcula o Y abaixo da última alternativa do quiz
            float continue_button_y = buttons_start_y + (step->option_count * (button_height + option_spacing)) + (win_h * CONTINUE_BUTTON_GAP_RATIO);

            Rectangle continue_button_bounds = {
                center_x - (continue_button_width / 2.0f),
                continue_button_y,
                continue_button_width,
                continue_button_height
            };

            bool continue_clicked = false;
            draw_button(continue_button_bounds, "Continuar", font_size, BUTTON_FILL_COLOR, true, &continue_clicked);

            // Quando clica em continuar, avança de fase e reseta as variáveis do quiz
            if (continue_clicked)
            {
                current_step_index++;
                current_quiz_answered = false;
                current_quiz_selected = -1;
            }
        }
    }
}

// Controla a tela do computador: o monitor sobe, a tela liga, os passos rodam e, no fim, a tela desliga.
void handle_computer_loop(void)
{
    computer_scene_progress += GetFrameTime() / SCENE_ENTER_TIME;
    if (computer_scene_progress > 1.0f) computer_scene_progress = 1.0f;

    update_computer_layout();
    update_computer_mouse();

    render_computer_loop();

    if (!computer_tasks_finished)
    {
        render_current_step();
    }

    draw_scanlines();

    // A tela liga depois que o monitor chega ao lugar e desliga quando as tarefas acabam
    if (computer_tasks_finished)
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

    if (computer_mouse_captured)
    {
        draw_computer_cursor();
    }

    // TODO: quando a tela terminar de desligar, a saída é tratada pelo game loop.
}

// Libera da memória as texturas usadas pela tela do computador.
void unload_computer_loop(void)
{
    UnloadTexture(window_texture);
    UnloadTexture(correct_icon_texture);
    UnloadTexture(wrong_icon_texture);
}
