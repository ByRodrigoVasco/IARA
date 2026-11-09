#include <stdio.h>
#include <string.h>
#include <time.h>

#include "raylib.h"
#include "save_select.h"
#include "accessibility.h"
#include "screen_style.h"
#include "fonts.h"
#include "helper.h"
#include "macros.h"

typedef enum
{
    VIEW_LIST,      // A lista de saves
    VIEW_NAME,      // Digitando o nome de um save novo
    VIEW_DELETE     // Confirmando a exclusão de um save
} view_state;

static save_mode save_select_mode = SAVE_MODE_NORMAL;
static view_state save_view = VIEW_LIST;

static save_slot_info save_list[SAVE_SLOTS_MAX];
static char save_summaries[SAVE_SLOTS_MAX][64];
static char save_dates[SAVE_SLOTS_MAX][32];
static int save_count = 0;

// Linhas da lista: 0 é NOVO SAVE, de 1 a save_count são os saves, e a última é VOLTAR
static int save_selected_row = 0;
static int save_first_visible = 0;

static int save_select_result = SAVE_SELECT_NONE;

static char name_input[SAVE_NAME_MAX + 1];
static save_name_check save_name_status = SAVE_NAME_EMPTY;
static bool save_show_name_message = false;

static int save_delete_choice = 0;          // 0 cancelar (o padrão), 1 apagar
static char save_delete_name[SAVE_NAME_MAX + 1];

static char save_notice[96];
static double save_notice_until = 0.0;

// -----------------------------------------------------------------------------
// Lista
// -----------------------------------------------------------------------------

static bool names_equal(const char *a, const char *b)
{
    while (*a != '\0' && *b != '\0')
    {
        char x = (*a >= 'A' && *a <= 'Z') ? (char)(*a + 32) : *a;
        char y = (*b >= 'A' && *b <= 'Z') ? (char)(*b + 32) : *b;

        if (x != y) return false;
        a++;
        b++;
    }
    return *a == *b;
}

static void show_notice(const char *text)
{
    snprintf(save_notice, sizeof(save_notice), "%s", text);
    save_notice_until = GetTime() + 3.0;
}

static void reload_slots(void)
{
    save_count = save_slots_list(save_select_mode, save_list, SAVE_SLOTS_MAX);

    for (int i = 0; i < save_count; i++)
    {
        save_slot_summary(save_select_mode, save_list[i].name, save_summaries[i], sizeof(save_summaries[i]));
        save_dates[i][0] = '\0';
        if (save_list[i].modified > 0)
        {
            time_t moment = (time_t)save_list[i].modified;
            struct tm *local = localtime(&moment);

            if (local != NULL) strftime(save_dates[i], sizeof(save_dates[i]), "%d/%m/%Y %H:%M", local);
        }
    }
}

void initialize_save_select(save_mode mode)
{
    save_select_mode = mode;
    save_view = VIEW_LIST;
    save_select_result = SAVE_SELECT_NONE;
    save_selected_row = 0;
    save_first_visible = 0;
    name_input[0] = '\0';
    save_notice[0] = '\0';
    save_notice_until = 0.0;
    reload_slots();

    // Quem já tem um save entra com ele escolhido: é o caso mais comum
    if (save_count > 0) save_selected_row = 1;
}

static int row_total(void)
{
    return save_count + 2;
}

static void choose_slot(const char *name)
{
    active_save_set(save_select_mode, name);
    save_select_result = SAVE_SELECT_CHOSEN;
    SetMouseCursor(MOUSE_CURSOR_ARROW);
}

bool save_select_choose_existing(const char *name)
{
    if (!save_slot_exists(save_select_mode, name)) return false;
    for (int i = 0; i < save_count; i++)
    {
        // Usa o nome como está na pasta, para o caminho bater com maiúsculas e minúsculas
        if (names_equal(name, save_list[i].name))
        {
            choose_slot(save_list[i].name);
            return true;
        }
    }
    choose_slot(name);
    return true;
}

save_name_check save_select_create_named(const char *name)
{
    char trimmed[SAVE_NAME_MAX * 4];
    save_name_check check;

    snprintf(trimmed, sizeof(trimmed), "%s", name != NULL ? name : "");
    save_name_trim(trimmed);
    check = save_slot_check_name(save_select_mode, trimmed);
    if (check != SAVE_NAME_OK) return check;
    if (!save_slot_create(save_select_mode, trimmed)) return SAVE_NAME_INVALID;

    choose_slot(trimmed);
    return SAVE_NAME_OK;
}

void save_select_go_back(void)
{
    save_select_result = SAVE_SELECT_BACK;
}

bool save_select_delete_named(const char *name)
{
    bool deleted = save_slot_delete(save_select_mode, name);

    reload_slots();
    if (save_selected_row >= row_total()) save_selected_row = row_total() - 1;
    return deleted;
}

// -----------------------------------------------------------------------------
// Texto digitado
// -----------------------------------------------------------------------------

// Letras com acento viram a letra simples: os nomes viram pastas, que aqui só aceitam o alfabeto básico
static char simplify_character(int codepoint)
{
    if (codepoint >= 32 && codepoint < 127) return (char)codepoint;
    if ((codepoint >= 0xC0 && codepoint <= 0xC5)) return 'A';
    if (codepoint == 0xC7) return 'C';
    if (codepoint >= 0xC8 && codepoint <= 0xCB) return 'E';
    if (codepoint >= 0xCC && codepoint <= 0xCF) return 'I';
    if (codepoint == 0xD1) return 'N';
    if (codepoint >= 0xD2 && codepoint <= 0xD6) return 'O';
    if (codepoint >= 0xD9 && codepoint <= 0xDC) return 'U';
    if (codepoint >= 0xE0 && codepoint <= 0xE5) return 'a';
    if (codepoint == 0xE7) return 'c';
    if (codepoint >= 0xE8 && codepoint <= 0xEB) return 'e';
    if (codepoint >= 0xEC && codepoint <= 0xEF) return 'i';
    if (codepoint == 0xF1) return 'n';
    if (codepoint >= 0xF2 && codepoint <= 0xF6) return 'o';
    if (codepoint >= 0xF9 && codepoint <= 0xFC) return 'u';
    return 0;
}

static void refresh_name_status(void)
{
    save_name_status = save_slot_check_name(save_select_mode, name_input);
    // O aviso de nome repetido aparece na hora; os outros só depois de tentar confirmar
    if (save_name_status == SAVE_NAME_EXISTS) save_show_name_message = true;
}

static void open_name_input(void)
{
    save_view = VIEW_NAME;
    name_input[0] = '\0';
    save_show_name_message = false;
    save_name_status = SAVE_NAME_EMPTY;
    // Descarta as teclas que o jogador digitou antes de abrir o campo
    while (GetCharPressed() > 0) { }
}

static void try_create_from_input(void)
{
    save_name_check check = save_select_create_named(name_input);

    if (check != SAVE_NAME_OK)
    {
        save_name_status = check;
        save_show_name_message = true;
    }
}

static void handle_name_input(Rectangle create_button, Rectangle cancel_button)
{
    int codepoint = GetCharPressed();
    bool changed = false;

    while (codepoint > 0)
    {
        char character = simplify_character(codepoint);
        size_t length = strlen(name_input);

        if (character != 0 && length < SAVE_NAME_MAX)
        {
            // Só letras, números, espaço, - e _ ; o resto é ignorado com um aviso
            if ((character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') || (character >= '0' && character <= '9') || character == ' ' || character == '-' || character == '_')
            {
                name_input[length] = character;
                name_input[length + 1] = '\0';
                changed = true;
            }
            else
            {
                save_name_status = SAVE_NAME_INVALID;
                save_show_name_message = true;
            }
        }
        codepoint = GetCharPressed();
    }

    if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) && name_input[0] != '\0')
    {
        name_input[strlen(name_input) - 1] = '\0';
        changed = true;
    }
    if (changed) refresh_name_status();

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) try_create_from_input();
    if (IsKeyPressed(KEY_ESCAPE)) save_view = VIEW_LIST;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        if (style_mouse_over(create_button)) try_create_from_input();
        else if (style_mouse_over(cancel_button)) save_view = VIEW_LIST;
    }
}

// -----------------------------------------------------------------------------
// Quadro
// -----------------------------------------------------------------------------

static void draw_name_panel(Rectangle *create_button, Rectangle *cancel_button)
{
    float scale = get_ui_scale();
    float width = GetScreenWidth() * 0.62f;
    float height = 360.0f * scale;
    Rectangle panel = {(GetScreenWidth() - width) / 2.0f, (GetScreenHeight() - height) / 2.0f, width, height};
    float padding = 36.0f * scale;
    float title_size = style_font_size(0.62f);
    Rectangle box = {panel.x + padding, panel.y + padding + title_size + 16.0f * scale, panel.width - padding * 2.0f, 70.0f * scale};
    float text_size = style_font_size(0.6f);
    bool blink = ((int)(GetTime() * 2.0) % 2) == 0;
    float button_width = 220.0f * scale;
    float button_height = 62.0f * scale;
    Vector2 measure;

    style_panel(panel);
    DrawTextEx(default_game_font, "NOME DO NOVO SAVE", (Vector2){panel.x + padding, panel.y + padding}, title_size, title_size * 0.06f, STYLE_TEXT_COLOR);

    DrawRectangleRec(box, (Color){10, 6, 12, 255});
    DrawRectangleLinesEx(box, 2.0f * scale, save_show_name_message && save_name_status != SAVE_NAME_OK ? STYLE_WARNING_COLOR : (Color){120, 90, 140, 255});
    DrawTextEx(default_game_font, name_input, (Vector2){box.x + 16.0f * scale, box.y + (box.height - text_size) / 2.0f}, text_size, text_size * 0.06f, STYLE_TEXT_COLOR);
    measure = MeasureTextEx(default_game_font, name_input, text_size, text_size * 0.06f);
    if (blink) DrawRectangle((int)(box.x + 20.0f * scale + measure.x), (int)(box.y + box.height * 0.2f), (int)(4.0f * scale), (int)(box.height * 0.6f), STYLE_TEXT_COLOR);

    if (save_show_name_message && save_name_status != SAVE_NAME_OK)
    {
        DrawTextEx(default_game_font, save_name_check_message(save_name_status), (Vector2){box.x, box.y + box.height + 14.0f * scale}, DEFAULT_TEXT_FONT_SIZE * scale, 2.0f * scale, STYLE_WARNING_COLOR);
    }
    else
    {
        DrawTextEx(default_game_font, "Até 20 letras, números, espaço, - ou _. O nome não pode se repetir.", (Vector2){box.x, box.y + box.height + 14.0f * scale}, DEFAULT_TEXT_FONT_SIZE * 0.8f * scale, 2.0f * scale, STYLE_DIM_COLOR);
    }

    *create_button = (Rectangle){panel.x + panel.width - padding - button_width * 2.0f - 20.0f * scale, panel.y + panel.height - padding - button_height, button_width, button_height};
    *cancel_button = (Rectangle){panel.x + panel.width - padding - button_width, create_button->y, button_width, button_height};
    style_button(*create_button, "CRIAR", true, false);
    style_button(*cancel_button, "CANCELAR", false, false);
}

static void draw_delete_panel(Rectangle *cancel_button, Rectangle *delete_button)
{
    float scale = get_ui_scale();
    float width = GetScreenWidth() * 0.6f;
    float height = 340.0f * scale;
    Rectangle panel = {(GetScreenWidth() - width) / 2.0f, (GetScreenHeight() - height) / 2.0f, width, height};
    float padding = 36.0f * scale;
    float title_size = style_font_size(0.62f);
    float button_width = 240.0f * scale;
    float button_height = 62.0f * scale;
    char question[96];

    style_panel(panel);
    DrawTextEx(default_game_font, "APAGAR O SAVE?", (Vector2){panel.x + padding, panel.y + padding}, title_size, title_size * 0.06f, STYLE_WARNING_COLOR);
    snprintf(question, sizeof(question), "\"%s\"", save_delete_name);
    DrawTextEx(default_game_font, question, (Vector2){panel.x + padding, panel.y + padding + title_size + 14.0f * scale}, style_font_size(0.5f), 3.0f * scale, STYLE_TEXT_COLOR);
    style_wrapped_text("Todo o progresso desse save será perdido e não dá para desfazer.", panel.x + padding, panel.y + padding + title_size + 14.0f * scale + style_font_size(0.5f) + 18.0f * scale, panel.width - padding * 2.0f,
                       DEFAULT_TEXT_FONT_SIZE * scale, STYLE_DIM_COLOR);

    *cancel_button = (Rectangle){panel.x + panel.width - padding - button_width * 2.0f - 20.0f * scale, panel.y + panel.height - padding - button_height, button_width, button_height};
    *delete_button = (Rectangle){panel.x + panel.width - padding - button_width, cancel_button->y, button_width, button_height};
    style_button(*cancel_button, "CANCELAR", save_delete_choice == 0, false);
    style_button(*delete_button, "APAGAR", save_delete_choice == 1, true);
}

static void confirm_delete(void)
{
    if (save_delete_choice == 1)
    {
        if (save_select_delete_named(save_delete_name)) show_notice("Save apagado.");
        else show_notice("Não consegui apagar o save.");
    }
    save_view = VIEW_LIST;
}

static void confirm_row(int row)
{
    if (row == 0) open_name_input();
    else if (row <= save_count) choose_slot(save_list[row - 1].name);
    else
    {
        save_select_result = SAVE_SELECT_BACK;
        SetMouseCursor(MOUSE_CURSOR_ARROW);
    }
}

static void open_delete(int row)
{
    if (row < 1 || row > save_count) return;
    snprintf(save_delete_name, sizeof(save_delete_name), "%s", save_list[row - 1].name);
    save_delete_choice = 0;
    save_view = VIEW_DELETE;
}

int render_save_select(void)
{
    float scale = get_ui_scale();
    float screen_w = (float)GetScreenWidth();
    float screen_h = (float)GetScreenHeight();
    float column_width = screen_w * 0.74f;
    float column_x;
    float top = style_content_top();
    float name_size = style_font_size(0.6f);
    float detail_size = style_font_size(0.34f);
    float card_height = name_size + detail_size + 46.0f * scale;
    float card_gap = 14.0f * scale;
    Rectangle back_button;
    float bottom;
    int card_total = save_count + 1;            // NOVO SAVE e os saves; o VOLTAR é o botão de baixo
    int back_row = save_count + 1;
    int visible;
    view_state active_view = save_view;   // A tela em que o quadro começou: uma tecla que abre uma janela não vale também dentro dela
    Rectangle create_button = {0}, cancel_button = {0}, delete_button = {0};
    Rectangle row_areas[SAVE_SLOTS_MAX + 2];
    Rectangle remove_area = {0};
    int hovered = -1;
    bool hovered_remove = false;

    if (column_width > 1040.0f * scale) column_width = 1040.0f * scale;
    column_x = (screen_w - column_width) / 2.0f;
    back_button = (Rectangle){(screen_w - 260.0f * scale) / 2.0f, screen_h - 135.0f * scale, 260.0f * scale, 62.0f * scale};
    bottom = back_button.y - 56.0f * scale;
    visible = (int)((bottom - top + card_gap) / (card_height + card_gap));
    if (visible < 2) visible = 2;

    // A janela da lista segue a escolha; no VOLTAR ela fica onde estava
    if (save_selected_row < card_total)
    {
        if (save_selected_row < save_first_visible) save_first_visible = save_selected_row;
        if (save_selected_row >= save_first_visible + visible) save_first_visible = save_selected_row - visible + 1;
    }
    if (save_first_visible > card_total - visible) save_first_visible = card_total - visible;
    if (save_first_visible < 0) save_first_visible = 0;

    // -------- entrada
    if (save_view == VIEW_LIST)
    {
        float wheel = GetMouseWheelMove();

        if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S) || wheel < 0.0f) save_selected_row = (save_selected_row + 1) % row_total();
        if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W) || wheel > 0.0f) save_selected_row = (save_selected_row + row_total() - 1) % row_total();
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) confirm_row(save_selected_row);
        if (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_X)) open_delete(save_selected_row);
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) confirm_row(row_total() - 1);
    }
    else if (save_view == VIEW_DELETE)
    {
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) save_delete_choice = 0;
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) save_delete_choice = 1;
        if (IsKeyPressed(KEY_TAB)) save_delete_choice = 1 - save_delete_choice;
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) confirm_delete();
        if (IsKeyPressed(KEY_ESCAPE)) save_view = VIEW_LIST;
    }

    // -------- desenho
    begin_game_drawing();
    style_backdrop();
    style_title_centered(save_mode_title(save_select_mode), "ESCOLHA O SAVE");

    for (int row = save_first_visible; row < card_total && row < save_first_visible + visible; row++)
    {
        float y = top + (float)(row - save_first_visible) * (card_height + card_gap);
        bool selected = (row == save_selected_row && save_view == VIEW_LIST);
        Rectangle card = {column_x, y, column_width, card_height};
        float padding = 26.0f * scale;
        const char *label = row == 0 ? "+  NOVO SAVE" : save_list[row - 1].name;

        row_areas[row] = card;
        style_card_frame(card, selected, row == 0);
        DrawTextEx(default_game_font, label, (Vector2){card.x + padding, card.y + 12.0f * scale}, name_size, name_size * 0.06f, selected ? STYLE_TEXT_COLOR : (Color){200, 192, 206, 255});

        if (row == 0)
        {
            DrawTextEx(default_game_font, "Começar do zero com um nome novo", (Vector2){card.x + padding, card.y + 20.0f * scale + name_size}, detail_size, detail_size * 0.06f, STYLE_DIM_COLOR);
        }
        else
        {
            const char *date = save_dates[row - 1];
            Vector2 date_measure = MeasureTextEx(default_game_font, date, detail_size, detail_size * 0.06f);

            DrawTextEx(default_game_font, save_summaries[row - 1], (Vector2){card.x + padding, card.y + 20.0f * scale + name_size}, detail_size, detail_size * 0.06f, STYLE_DIM_COLOR);
            DrawTextEx(default_game_font, date, (Vector2){card.x + card.width - padding - date_measure.x, card.y + 16.0f * scale}, detail_size, detail_size * 0.06f, STYLE_DIM_COLOR);

            // O botão de apagar aparece no save escolhido, no canto de baixo à direita
            if (selected)
            {
                float size = DEFAULT_TEXT_FONT_SIZE * 0.8f * scale;
                const char *text = "DEL: APAGAR";
                Vector2 measure = MeasureTextEx(default_game_font, text, size, 2.0f * scale);

                remove_area = (Rectangle){card.x + card.width - padding - measure.x - 20.0f * scale, card.y + card.height - measure.y - 22.0f * scale, measure.x + 20.0f * scale, measure.y + 12.0f * scale};
                DrawRectangleLinesEx(remove_area, 2.0f * scale, style_mouse_over(remove_area) ? STYLE_WARNING_COLOR : (Color){170, 70, 70, 255});
                DrawTextEx(default_game_font, text, (Vector2){remove_area.x + 10.0f * scale, remove_area.y + 6.0f * scale}, size, 2.0f * scale, style_mouse_over(remove_area) ? STYLE_WARNING_COLOR : (Color){170, 70, 70, 255});
            }
        }
    }

    // Barra de rolagem, quando nem todos os cartões cabem
    if (card_total > visible)
    {
        float track_x = column_x + column_width + 16.0f * scale;
        float track_height = (float)visible * (card_height + card_gap) - card_gap;
        float thumb_height = track_height * (float)visible / (float)card_total;
        float thumb_y = top + (track_height - thumb_height) * (float)save_first_visible / (float)(card_total - visible);

        DrawRectangle((int)track_x, (int)top, (int)(6.0f * scale), (int)track_height, (Color){40, 28, 46, 255});
        DrawRectangle((int)track_x, (int)thumb_y, (int)(6.0f * scale), (int)thumb_height, (Color){120, 90, 140, 255});
    }

    style_button(back_button, "VOLTAR", save_selected_row == back_row && save_view == VIEW_LIST, false);

    if (save_view == VIEW_LIST && GetTime() < save_notice_until)
    {
        float size = DEFAULT_TEXT_FONT_SIZE * scale;
        Vector2 measure = MeasureTextEx(default_game_font, save_notice, size, 2.0f * scale);

        DrawTextEx(default_game_font, save_notice, (Vector2){(screen_w - measure.x) / 2.0f, back_button.y - 44.0f * scale}, size, 2.0f * scale, (Color){150, 230, 160, 255});
    }

    if (save_view == VIEW_LIST)
    {
        const char *hint = save_selected_row >= 1 && save_selected_row <= save_count ? "SETAS: escolher    ENTER: jogar    DEL: apagar    ESC: voltar" : "SETAS: escolher    ENTER: confirmar    ESC: voltar";

        style_hint(hint);
    }
    else if (save_view == VIEW_NAME)
    {
        draw_name_panel(&create_button, &cancel_button);
    }
    else
    {
        draw_delete_panel(&cancel_button, &delete_button);
    }

    end_game_drawing();

    // -------- mouse (depois do desenho, quando as áreas já foram calculadas)
    if (active_view == VIEW_LIST && save_view == VIEW_LIST)
    {
        for (int row = save_first_visible; row < card_total && row < save_first_visible + visible; row++)
        {
            if (style_mouse_over(row_areas[row])) hovered = row;
        }
        if (style_mouse_over(back_button)) hovered = back_row;
        hovered_remove = remove_area.width > 0.0f && style_mouse_over(remove_area);

        if (hovered >= 0 || hovered_remove) SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        else SetMouseCursor(MOUSE_CURSOR_ARROW);

        if (hovered >= 0 && (GetMouseDelta().x != 0.0f || GetMouseDelta().y != 0.0f)) save_selected_row = hovered;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            if (hovered_remove) open_delete(save_selected_row);
            else if (hovered >= 0)
            {
                save_selected_row = hovered;
                confirm_row(hovered);
            }
        }
    }
    else if (active_view == VIEW_NAME)
    {
        handle_name_input(create_button, cancel_button);
    }
    else if (active_view == VIEW_DELETE && save_view == VIEW_DELETE)
    {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            if (style_mouse_over(cancel_button)) { save_delete_choice = 0; confirm_delete(); }
            else if (style_mouse_over(delete_button)) { save_delete_choice = 1; confirm_delete(); }
        }
    }

    return save_select_result;
}
