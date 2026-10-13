#include <stdbool.h>
#include <stddef.h> // Define a constante NULL

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

typedef enum {
    STEP_STATEMENT,
    STEP_QUIZ
} StepType;

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

static int current_step_index = 0;

static DialogStep sequence[] = {
    {
        .type = STEP_STATEMENT,
        .lines = {
            "Bem-vindo(a) à CISSA, Estagiário.",
            "Antes de libertar as tuas tarefas preciso",
            "de te apresentar como trabalhamos por aqui.",
            "Isto leva menos de dois minutos!"
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

#define TOTAL_STEPS ((int)(sizeof(sequence) / sizeof(sequence[0])))

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

void render_current_step(void)
{
    // Se ultrapassou o último passo, marca como finalizado
    if (current_step_index >= TOTAL_STEPS)
    {
        computer_tasks_finished = true;
        return;
    }

    DialogStep *step = &sequence[current_step_index];

    // 1. Obtém as dimensões atuais da janela (Texture2D)
    float win_w = (float)window_texture.width;
    float win_h = (float)window_texture.height;
    float win_y = middle_screen_y - (win_h / 2.0f);

    // 2. Escala da fonte ajustada proporcionalmente
    float font_size = win_h * 0.038f;
    float line_spacing = win_h * 0.01f;

    // 3. Calcula a largura da maior linha de texto e altura acumulada
    float max_text_width = 0.0f;
    for (int i = 0; i < step->line_count; i++)
    {
        Vector2 line_size = MeasureTextEx(default_game_font, step->lines[i], font_size, DEFAULT_LETTER_SPACING);
        if (line_size.x > max_text_width)
        {
            max_text_width = line_size.x;
        }
    }

    float total_text_height = (step->line_count * font_size) + ((step->line_count - 1) * line_spacing);

    // 4. Margens internas (Paddings)
    float padding_x = win_w * 0.04f;
    float padding_y = win_h * 0.05f;

    // 5. Limites de largura da caixa (Trava Máxima em 85% da largura da janela)
    float max_box_width = win_w * 0.85f;
    float min_box_width = win_w * 0.60f;

    float text_box_width = max_text_width + (padding_x * 2.0f);

    if (text_box_width > max_box_width) text_box_width = max_box_width;
    if (text_box_width < min_box_width) text_box_width = min_box_width;

    float text_box_height = total_text_height + (padding_y * 2.0f);
    float text_box_margin_top = win_h * 0.15f; // Margem superior em relação à janela do PC

    Rectangle text_box = {
        middle_screen_x - (text_box_width / 2.0f),
        win_y + text_box_margin_top,
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
        Vector2 text_size = MeasureTextEx(default_game_font, step->lines[i], font_size, DEFAULT_LETTER_SPACING);
        Vector2 text_pos = {
            middle_screen_x - (text_size.x / 2.0f),
            current_y
        };

        DrawTextEx(default_game_font, step->lines[i], text_pos, font_size, DEFAULT_LETTER_SPACING, text_to_fade);
        current_y += font_size + line_spacing;
    }

    // 8. Renderiza os botões ancorados abaixo da caixa
    float buttons_start_y = text_box.y + text_box.height + (win_h * 0.025f);

    if (step->type == STEP_STATEMENT)
    {
        float button_width  = win_w * 0.30f;
        float button_height = win_h * 0.08f;

        Rectangle btn_rec = {
            middle_screen_x - (button_width / 2.0f),
            buttons_start_y,
            button_width,
            button_height
        };

        bool clicked = false;
        const char *label = (step->button_name != NULL) ? step->button_name : "Continuar";

        draw_button(btn_rec, label, &clicked);

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
            Vector2 text_size = MeasureTextEx(default_game_font, step->options[i], font_size, DEFAULT_LETTER_SPACING);
            if (text_size.x > max_option_text_width)
            {
                max_option_text_width = text_size.x;
            }
        }

        // 2. Define a largura do botão baseada no texto + margens (padding)
        float button_padding = win_w * 0.06f;
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

        float button_height  = win_h * 0.07f;
        float option_spacing = win_h * 0.015f;

        for (int i = 0; i < step->option_count; i++)
        {
            Rectangle btn_rec = {
                middle_screen_x - (button_width / 2.0f),
                buttons_start_y + (i * (button_height + option_spacing)),
                button_width,
                button_height
            };

            if (current_quiz_answered)
            {
                // Se já respondeu, renderizamos o botão sem capturar os cliques
                bool dummy_click = false;
                draw_button(btn_rec, step->options[i], &dummy_click);

                // Se esta for a opção escolhida, desenha o ícone ao lado dela
                if (i == current_quiz_selected)
                {
                    Texture2D icon = (i == step->correct_option) ? correct_icon_texture : wrong_icon_texture;

                    // Posiciona o ícone à direita do botão
                    float icon_x = btn_rec.x + btn_rec.width + (win_w * 0.015f);
                    float icon_y = btn_rec.y + (btn_rec.height / 2.0f) - (icon.height / 2.0f);

                    // Usa um tint que acompanha o fade global da janela
                    Color icon_tint = {255, 255, 255, color_to_fade.a};
                    DrawTexture(icon, (int)icon_x, (int)icon_y, icon_tint);
                }
            }
            else
            {
                // Se ainda não respondeu, renderiza os botões capturando o clique normal
                bool option_clicked = false;
                draw_button(btn_rec, step->options[i], &option_clicked);

                if (option_clicked)
                {
                    current_quiz_answered = true;
                    current_quiz_selected = i;
                }
            }
        }

        // Se o utilizador respondeu, desenhamos o botão "Continuar" abaixo de todas as opções
        if (current_quiz_answered)
        {
            float cont_button_width  = win_w * 0.30f;
            float cont_button_height = win_h * 0.08f;

            // Calcula o Y abaixo da última alternativa do quiz
            float cont_button_y = buttons_start_y + (step->option_count * (button_height + option_spacing)) + (win_h * 0.02f);

            Rectangle cont_btn_rec = {
                middle_screen_x - (cont_button_width / 2.0f),
                cont_button_y,
                cont_button_width,
                cont_button_height
            };

            bool cont_clicked = false;
            draw_button(cont_btn_rec, "Continuar", &cont_clicked);

            // Quando clica em continuar, avança de fase e reseta as variáveis do quiz
            if (cont_clicked)
            {
                current_step_index++;
                current_quiz_answered = false;
                current_quiz_selected = -1;
            }
        }
    }
}

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
            // Close game....
        }
    }
}

void unload_computer_loop(void)
{
    UnloadTexture(window_texture);
    UnloadTexture(correct_icon_texture);
    UnloadTexture(wrong_icon_texture);
}