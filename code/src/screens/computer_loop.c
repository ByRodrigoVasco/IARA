#include <stdbool.h>
#include <stddef.h> // Define a constante NULL
#include <string.h>

#include "raylib.h"
#include "computer_loop.h"
#include "macros.h"
#include "animations.h"
#include "button.h"
#include "fonts.h"

#define MAX_LINES 4
#define MAX_LINE_LEN 64
#define MAX_OPTIONS 4
#define MAX_OPTION_LEN 256

#define COMPUTER_WINDOW_MARGIN_TOP 30
#define COMPUTER_WINDOW_MARGIN_HORIZONTAL 40

// Proporções do layout do quiz, relativas ao tamanho da janela do computador.
// Largura: win_w (largura da janela). Altura: win_h (altura da janela).
#define QUIZ_FONT_SIZE_RATIO 0.038f          // tamanho da fonte (win_h)
#define QUIZ_LINE_SPACING_RATIO 0.01f        // espaço entre linhas (win_h)
#define QUIZ_OPTION_FONT_SIZE 20.0f
#define QUIZ_PADDING_X_RATIO 0.04f           // margem interna horizontal da caixa (win_w)
#define QUIZ_PADDING_Y_RATIO 0.05f           // margem interna vertical da caixa (win_h)
#define QUIZ_MAX_WIDTH_RATIO 0.85f           // largura máxima da caixa e dos botões (win_w)
#define QUIZ_MIN_WIDTH_RATIO 0.60f           // largura mínima da caixa (win_w)
#define QUIZ_BOX_TOP_MARGIN_RATIO 0.15f      // distância da caixa até o topo da janela (win_h)
#define QUIZ_BUTTONS_TOP_GAP_RATIO 0.025f    // espaço entre a caixa e os botões (win_h)
#define ACTION_BUTTON_WIDTH_RATIO 0.30f      // largura dos botões Continuar/Concluir (win_w)
#define ACTION_BUTTON_HEIGHT_RATIO 0.08f     // altura dos botões Continuar/Concluir (win_h)
#define OPTION_BUTTON_PADDING_RATIO 0.06f    // folga lateral do texto das opções (win_w)
#define OPTION_BUTTON_HEIGHT_RATIO 0.07f     // altura dos botões de opção (win_h)
#define OPTION_SPACING_RATIO 0.015f          // espaço entre opções (win_h)
#define ICON_GAP_RATIO 0.015f                // espaço entre opção e ícone de acerto/erro (win_w)
#define CONTINUE_BUTTON_GAP_RATIO 0.02f      // espaço entre a última opção e Continuar (win_h)

bool computer_tasks_finished = false;

static bool initial_fade_in_finished = false;
static bool text_fade_in_finished = false;

static Color color_to_fade = {255, 255, 255, 0};
static Color text_to_fade = {0, 0, 0, 0};

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

static float measure_max_dialog_line(const DialogStep *step, float font_size, float letter_spacing)
{
    float max_width = 0.0f;
    for (int i = 0; i < step->line_count; i++)
    {
        float width = MeasureTextEx(default_game_font, step->lines[i], font_size, letter_spacing).x;
        if (width > max_width) max_width = width;
    }
    return max_width;
}

static float fit_dialog_font_size(const DialogStep *step, float font_size, float max_width, float letter_spacing)
{
    for (int adjustment = 0; adjustment < 8; adjustment++)
    {
        float text_width = measure_max_dialog_line(step, font_size, letter_spacing);
        if (text_width <= max_width) break;
        font_size *= max_width / text_width * 0.99f;
    }
    return font_size;
}

static bool wrap_option_text(const char *text, char *wrapped_text, size_t buffer_size,
                             float max_width, int *line_count, float *max_line_width)
{
    if (text == NULL || wrapped_text == NULL || buffer_size == 0 || line_count == NULL || max_line_width == NULL)
    {
        return false;
    }

    char line[MAX_OPTION_LEN] = "";
    size_t line_length = 0;
    size_t output_length = 0;
    const char *cursor = text;
    *line_count = 0;
    *max_line_width = 0.0f;
    wrapped_text[0] = '\0';

    while (*cursor != '\0')
    {
        while (*cursor == ' ') cursor++;
        if (*cursor == '\0') break;

        const char *word_start = cursor;
        while (*cursor != '\0' && *cursor != ' ') cursor++;
        size_t word_length = (size_t)(cursor - word_start);
        size_t candidate_length = line_length + (line_length > 0 ? 1 : 0) + word_length;
        if (candidate_length >= sizeof(line))
        {
            return false;
        }

        char candidate[MAX_OPTION_LEN];
        size_t candidate_index = 0;
        if (line_length > 0)
        {
            memcpy(candidate, line, line_length);
            candidate_index = line_length;
            candidate[candidate_index++] = ' ';
        }
        memcpy(candidate + candidate_index, word_start, word_length);
        candidate[candidate_length] = '\0';

        float candidate_width = MeasureTextEx(default_game_font, candidate,
                                              QUIZ_OPTION_FONT_SIZE, DEFAULT_LETTER_SPACING).x;
        if (line_length > 0 && candidate_width > max_width)
        {
            float line_width = MeasureTextEx(default_game_font, line,
                                             QUIZ_OPTION_FONT_SIZE, DEFAULT_LETTER_SPACING).x;
            if (line_width > *max_line_width) *max_line_width = line_width;
            if (output_length + line_length + 2 > buffer_size) return false;
            memcpy(wrapped_text + output_length, line, line_length);
            output_length += line_length;
            wrapped_text[output_length++] = '\n';
            (*line_count)++;
            line_length = 0;
            candidate_length = word_length;
            memcpy(candidate, word_start, word_length);
            candidate[candidate_length] = '\0';
            candidate_width = MeasureTextEx(default_game_font, candidate,
                                            QUIZ_OPTION_FONT_SIZE, DEFAULT_LETTER_SPACING).x;
        }

        if (candidate_width > max_width) return false;
        memcpy(line, candidate, candidate_length + 1);
        line_length = candidate_length;
    }

    if (line_length > 0)
    {
        float line_width = MeasureTextEx(default_game_font, line,
                                         QUIZ_OPTION_FONT_SIZE, DEFAULT_LETTER_SPACING).x;
        if (line_width > *max_line_width) *max_line_width = line_width;
        if (output_length + line_length + 1 > buffer_size) return false;
        memcpy(wrapped_text + output_length, line, line_length);
        output_length += line_length;
        (*line_count)++;
    }

    wrapped_text[output_length] = '\0';
    return *line_count > 0;
}

// Carrega e redimensiona as texturas da janela e dos ícones, conforme a altura da tela.
void initialize_computer_loop(void)
{
    // Obtém o fator de escala baseado em 1080p
    float scale = (float)GetScreenHeight() / 1080.0f;
    float rescale_factor = 0.5f * scale;

    Image window_img = LoadImage(COMPUTER_WINDOW_PNG_PATH);
    ImageResizeNN(&window_img, (int)(window_img.width * rescale_factor), (int)(window_img.height * rescale_factor));

    Image correct_icon_img = LoadImage(COMPUTER_CORRECT_ICON);
    ImageResizeNN(&correct_icon_img, (int)(correct_icon_img.width * rescale_factor), (int)(correct_icon_img.height * rescale_factor));

    Image wrong_icon_img = LoadImage(COMPUTER_WRONG_ICON);
    ImageResizeNN(&wrong_icon_img, (int)(wrong_icon_img.width * rescale_factor), (int)(wrong_icon_img.height * rescale_factor));

    window_texture = LoadTextureFromImage(window_img);
    correct_icon_texture = LoadTextureFromImage(correct_icon_img);
    wrong_icon_texture = LoadTextureFromImage(wrong_icon_img);

    SetTextureFilter(window_texture, TEXTURE_FILTER_POINT);
    SetTextureFilter(correct_icon_texture, TEXTURE_FILTER_POINT);
    SetTextureFilter(wrong_icon_texture, TEXTURE_FILTER_POINT);

    UnloadImage(window_img);
    UnloadImage(correct_icon_img);
    UnloadImage(wrong_icon_img);
}

// Desenha o fundo e a janela do computador, aplicando o fade in.
void render_computer_loop(void)
{
    if (!initial_fade_in_finished)
    {
        fade_in_texture(MEDIUM_FADE_SPEED, &color_to_fade, &initial_fade_in_finished);
    }

    if (!text_fade_in_finished)
    {
        fade_in_texture(MEDIUM_FADE_SPEED, &text_to_fade, &text_fade_in_finished);
    }

    // 1. Calcula o fator de fade (de 0.0f a 1.0f) com base no alpha atual
    float fade_factor = color_to_fade.a / 255.0f;

    // 2. Escurece a cor do fundo proporcionalmente até ao preto
    Color dynamic_bg = {
        (unsigned char)(COMPUTER_BG_COLOR.r * fade_factor),
        (unsigned char)(COMPUTER_BG_COLOR.g * fade_factor),
        (unsigned char)(COMPUTER_BG_COLOR.b * fade_factor),
        255
    };

    ClearBackground(dynamic_bg);

    DrawTexture(window_texture, middle_screen_x - window_texture.width / 2, middle_screen_y - window_texture.height / 2, color_to_fade);
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

    DialogStep *step = &dialog_sequence[current_step_index];

    // 1. Obtém as dimensões atuais da janela (Texture2D)
    float win_w = (float)window_texture.width;
    float win_h = (float)window_texture.height;
    float screen_center_x = GetScreenWidth() / 2.0f;
    float screen_center_y = GetScreenHeight() / 2.0f;
    float win_y = screen_center_y - (win_h / 2.0f);

    // 2. Ajusta a fonte ao espaço disponível dentro da caixa.
    float padding_x = win_w * QUIZ_PADDING_X_RATIO;
    float padding_y = win_h * QUIZ_PADDING_Y_RATIO;
    float max_box_width = win_w * QUIZ_MAX_WIDTH_RATIO;
    float min_box_width = win_w * QUIZ_MIN_WIDTH_RATIO;
    float text_box_margin_top = win_h * QUIZ_BOX_TOP_MARGIN_RATIO;
    float available_text_width = max_box_width - (padding_x * 2.0f);
    float font_size = win_h * QUIZ_FONT_SIZE_RATIO;
    float letter_spacing = DEFAULT_LETTER_SPACING;
    font_size = fit_dialog_font_size(step, font_size, available_text_width, letter_spacing);

    float line_spacing = win_h * QUIZ_LINE_SPACING_RATIO;
    float text_box_top = win_y + text_box_margin_top;
    float text_box_width = measure_max_dialog_line(step, font_size, letter_spacing) + (padding_x * 2.0f);

    if (text_box_width > max_box_width) text_box_width = max_box_width;
    if (text_box_width < min_box_width) text_box_width = min_box_width;

    float total_text_height = (step->line_count * font_size) + ((step->line_count - 1) * line_spacing);
    float text_box_height = total_text_height + (padding_y * 2.0f);

    Rectangle text_box = {
        screen_center_x - (text_box_width / 2.0f),
        text_box_top,
        text_box_width,
        text_box_height
    };

    // 6. Desenha fundo e borda da caixa
    Color box_bg = (Color){ 20, 25, 35, 200 };
    Color box_border = (Color){ 60, 75, 100, 255 };

    DrawRectangleRounded(text_box, 0.08f, 6, box_bg);
    DrawRectangleRoundedLines(text_box, 0.08f, 6, box_border);

    // 7. Renderiza as linhas do texto
    float current_y = text_box.y + padding_y;

    for (int i = 0; i < step->line_count; i++)
    {
        Vector2 text_size = MeasureTextEx(default_game_font, step->lines[i], font_size, letter_spacing);
        Vector2 text_pos = {
            screen_center_x - (text_size.x / 2.0f),
            current_y
        };

        DrawTextEx(default_game_font, step->lines[i], text_pos, font_size, letter_spacing, text_to_fade);
        current_y += font_size + line_spacing;
    }

    // 8. Renderiza os botões ancorados abaixo da caixa
    float buttons_start_y = text_box.y + text_box.height + (win_h * QUIZ_BUTTONS_TOP_GAP_RATIO);

    if (step->type == STEP_STATEMENT)
    {
        float button_width  = win_w * ACTION_BUTTON_WIDTH_RATIO;
        float button_height = win_h * ACTION_BUTTON_HEIGHT_RATIO;

        Rectangle button_bounds = {
            screen_center_x - (button_width / 2.0f),
            buttons_start_y,
            button_width,
            button_height
        };

        bool clicked = false;
        const char *label = (step->button_name != NULL) ? step->button_name : "Continuar";

        draw_button(button_bounds, label, &clicked);

        if (clicked)
        {
            current_step_index++;
        }
    }
    else if (step->type == STEP_QUIZ)
    {
        char wrapped_options[MAX_OPTIONS][MAX_OPTION_LEN];
        float option_button_heights[MAX_OPTIONS];
        float button_padding = win_w * OPTION_BUTTON_PADDING_RATIO;
        float option_text_max_width = max_box_width - button_padding;
        float max_option_text_width = 0.0f;
        float base_button_height = win_h * OPTION_BUTTON_HEIGHT_RATIO;

        for (int i = 0; i < step->option_count; i++)
        {
            int option_line_count;
            float option_line_max_width;
            if (!wrap_option_text(step->options[i], wrapped_options[i], sizeof(wrapped_options[i]),
                                  option_text_max_width, &option_line_count, &option_line_max_width))
            {
                TraceLog(LOG_ERROR, "QUIZ: nao foi possivel ajustar a alternativa %d ao botao", i + 1);
                return;
            }

            if (option_line_max_width > max_option_text_width)
            {
                max_option_text_width = option_line_max_width;
            }

            float option_text_height = MeasureTextEx(default_game_font, wrapped_options[i],
                                                     QUIZ_OPTION_FONT_SIZE, DEFAULT_LETTER_SPACING).y;
            float wrapped_line_height = option_line_count * QUIZ_OPTION_FONT_SIZE;
            if (option_text_height < wrapped_line_height) option_text_height = wrapped_line_height;
            float button_vertical_padding = base_button_height > QUIZ_OPTION_FONT_SIZE
                ? base_button_height - QUIZ_OPTION_FONT_SIZE
                : 0.0f;
            option_button_heights[i] = option_text_height + button_vertical_padding;
            if (option_button_heights[i] < base_button_height) option_button_heights[i] = base_button_height;
        }

        // 2. Define uma largura comum para os botões com base nas linhas já quebradas.
        float button_width = max_option_text_width + button_padding;
        if (button_width < text_box_width) button_width = text_box_width;
        if (button_width > max_box_width) button_width = max_box_width;

        float option_spacing = win_h * OPTION_SPACING_RATIO;
        float option_y = buttons_start_y;

        for (int i = 0; i < step->option_count; i++)
        {
            Rectangle button_bounds = {
                screen_center_x - (button_width / 2.0f),
                option_y,
                button_width,
                option_button_heights[i]
            };
            option_y += option_button_heights[i] + option_spacing;

            if (current_quiz_answered)
            {
                // Se já respondeu, renderizamos o botão sem capturar os cliques
                bool ignored_click = false;
                draw_button(button_bounds, wrapped_options[i], &ignored_click);

                // Se esta for a opção escolhida, desenha o ícone ao lado dela
                if (i == current_quiz_selected)
                {
                    Texture2D icon = (i == step->correct_option) ? correct_icon_texture : wrong_icon_texture;

                    // Posiciona o ícone à direita do botão
                    float icon_x = button_bounds.x + button_bounds.width + (win_w * ICON_GAP_RATIO);
                    float icon_y = button_bounds.y + (button_bounds.height / 2.0f) - (icon.height / 2.0f);

                    // Usa um tint que acompanha o fade global da janela
                    Color icon_tint = {255, 255, 255, color_to_fade.a};
                    DrawTexture(icon, (int)icon_x, (int)icon_y, icon_tint);
                }
            }
            else
            {
                // Se ainda não respondeu, renderiza os botões capturando o clique normal
                bool option_clicked = false;
                draw_button(button_bounds, wrapped_options[i], &option_clicked);

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

            float continue_button_y = option_y - option_spacing + (win_h * CONTINUE_BUTTON_GAP_RATIO);

            Rectangle continue_button_bounds = {
                screen_center_x - (continue_button_width / 2.0f),
                continue_button_y,
                continue_button_width,
                continue_button_height
            };

            bool continue_clicked = false;
            draw_button(continue_button_bounds, "Continuar", &continue_clicked);

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

// Controla a tela do computador: mostra os passos até o fim e depois aplica o fade out.
void handle_computer_loop(void)
{
    render_computer_loop();

    if (!computer_tasks_finished)
    {
        render_current_step();
    }
    else
    {
        static bool window_fade_out_done = false;
        static bool text_fade_out_done = false;

        fade_out_texture(MEDIUM_FADE_SPEED, &color_to_fade, &window_fade_out_done);
        fade_out_texture(MEDIUM_FADE_SPEED, &text_to_fade, &text_fade_out_done);

        if (window_fade_out_done && text_fade_out_done)
        {
            // TODO: fim do fade out; a saída da tela é tratada pelo game loop.
        }
    }
}

// Libera da memória as texturas usadas pela tela do computador.
void unload_computer_loop(void)
{
    UnloadTexture(window_texture);
    UnloadTexture(correct_icon_texture);
    UnloadTexture(wrong_icon_texture);
}