#include <stdio.h>

#include "raylib.h"
#include "help.h"
#include "ui_theme.h"
#include "fonts.h"
#include "helper.h"
#include "accessibility.h"
#include "macros.h"

#define HELP_MAX_LINES 10

typedef struct
{
    const char *title;
    const char *lines[HELP_MAX_LINES];   // Terminada por NULL
} help_page;

static const help_page help_pages[] = {
    {"CAMPANHA",
     {"Objetivo: você é a nova pessoa estagiária da empresa. Resolva a tarefa de cada dia no seu computador e descubra o que a IARA esconde.",
      "Andar: WASD ou as setas movem o personagem.",
      "Conversar e usar: chegue perto de um colega ou do computador e aperte ESPAÇO.",
      "No computador: escolha as respostas com o mouse ou com o teclado. ESC sai do computador.",
      "Dicas: alguns colegas dão dicas da tarefa do dia, e o mural guarda bilhetes.",
      "Fim do dia: com a tarefa resolvida, vá até a porta de vidro para encerrar o expediente.",
      "Pausa: ESC (ou P) pausa o jogo, e de lá você volta ao menu ou sai.",
      "Salvar: o progresso é salvo sozinho ao voltar ao menu e ao fechar o jogo.",
      "Tela: F11 alterna entre janela e tela cheia. Brilho, volume e contraste ficam em CONFIGURAÇÕES.",
      NULL}}};

static int help_current_page = 0;

int help_page_count(void)
{
    return (int)(sizeof(help_pages) / sizeof(help_pages[0]));
}

void initialize_help_screen(void)
{
    help_current_page = 0;
}

// Desenha o painel e lê as teclas. Devolve true quando o jogador pediu para fechar.
static bool update_and_draw_panel(void)
{
    float scale = get_ui_scale();
    float width = GetScreenWidth() * 0.82f;
    float height = GetScreenHeight() * 0.8f;
    float padding = 36.0f * scale;
    float y;
    int pages = help_page_count();
    Rectangle panel;
    Rectangle back_button;
    bool closed = false;
    char page_label[64];

    if (width > 1300.0f * scale) width = 1300.0f * scale;
    panel = (Rectangle){(GetScreenWidth() - width) / 2.0f, (GetScreenHeight() - height) / 2.0f - 10.0f * scale, width, height};
    back_button = (Rectangle){panel.x + panel.width - padding - 250.0f * scale, panel.y + panel.height - padding - 60.0f * scale, 250.0f * scale, 60.0f * scale};

    if (pages > 1)
    {
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) help_current_page = (help_current_page + 1) % pages;
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) help_current_page = (help_current_page + pages - 1) % pages;
    }
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_ENTER)) closed = true;
    if (ui_mouse_over(back_button))
    {
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) closed = true;
    }
    else
    {
        SetMouseCursor(MOUSE_CURSOR_ARROW);
    }

    ui_draw_panel(panel);
    y = panel.y + padding;
    DrawTextEx(default_game_font, "COMO JOGAR", (Vector2){panel.x + padding, y}, ui_font_size(2.0f), ui_font_size(2.0f) * 0.06f, UI_TEXT_COLOR);
    snprintf(page_label, sizeof(page_label), "%s", help_pages[help_current_page].title);
    {
        float size = ui_font_size(1.1f);
        Vector2 measure = MeasureTextEx(default_game_font, page_label, size, size * 0.06f);

        DrawTextEx(default_game_font, page_label, (Vector2){panel.x + panel.width - padding - measure.x, y + 8.0f * scale}, size, size * 0.06f, UI_WARNING_COLOR);
    }
    y += ui_font_size(2.0f) + 22.0f * scale;
    DrawRectangle((int)(panel.x + padding), (int)y, (int)(panel.width - padding * 2.0f), (int)(2.0f * scale), UI_BLOOD_COLOR);
    y += 20.0f * scale;

    for (int i = 0; help_pages[help_current_page].lines[i] != NULL; i++)
    {
        float used = ui_draw_wrapped(help_pages[help_current_page].lines[i], panel.x + padding + 22.0f * scale, y, panel.width - padding * 2.0f - 22.0f * scale, ui_font_size(1.0f), UI_TEXT_COLOR, false);

        DrawRectangle((int)(panel.x + padding), (int)(y + ui_font_size(1.0f) * 0.4f), (int)(8.0f * scale), (int)(8.0f * scale), UI_BLOOD_COLOR);
        y += used + 10.0f * scale;
    }

    ui_draw_button(back_button, "VOLTAR", true, false);
    if (pages > 1)
    {
        snprintf(page_label, sizeof(page_label), "Página %d de %d   (ESQUERDA / DIREITA)", help_current_page + 1, pages);
        DrawTextEx(default_game_font, page_label, (Vector2){panel.x + padding, back_button.y + 14.0f * scale}, ui_font_size(0.9f), ui_font_size(0.9f) * 0.06f, UI_DIM_COLOR);
    }

    return closed;
}

bool help_update_and_draw_panel(void)
{
    ui_draw_dim();
    return update_and_draw_panel();
}

bool render_help_screen(void)
{
    bool closed;

    begin_game_drawing();
    ui_draw_backdrop();
    closed = update_and_draw_panel();
    ui_draw_hint(help_page_count() > 1 ? "ESQUERDA / DIREITA: trocar de página    ESC ou ENTER: voltar" : "ESC ou ENTER: voltar");
    end_game_drawing();

    if (closed) SetMouseCursor(MOUSE_CURSOR_ARROW);
    return closed;
}
