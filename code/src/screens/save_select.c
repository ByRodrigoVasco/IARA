#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "raylib.h"
#include "save_select.h"
#include "accessibility.h"
#include "screen_style.h"
#include "notice.h"
#include "fonts.h"
#include "helper.h"
#include "macros.h"
#include "sv_ranking.h"
#include "cloud.h"
#include "cloud_sync.h"

#define VISIBLE_MAX 64                 // Cartões desenhados de uma vez (a tela mostra bem menos)
#define REFRESH_SECONDS 10.0           // Nuvem: atualiza a lista para ver quem entrou ou saiu
#define PASSWORD_BYTES 96

typedef enum
{
    VIEW_LIST,      // A lista de saves
    VIEW_NAME,      // Digitando o nome de um save novo
    VIEW_DELETE,    // Confirmando a exclusão de um save
    VIEW_BUSY       // Nuvem: esperando o servidor criar, abrir ou apagar
} view_state;

typedef struct
{
    save_slot_info info;
    char summary[64];
    char date[32];
    bool in_use;     // Nuvem: aberto em outro notebook
} save_entry;

static save_mode save_select_mode = SAVE_MODE_NORMAL;
static view_state save_view = VIEW_LIST;

static save_entry *save_list = NULL;     // Cresce conforme a lista: na nuvem não há limite de saves
static int save_capacity = 0;
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

static char save_notice[128];
static double save_notice_until = 0.0;
static bool save_notice_is_error = false;

// Nuvem
static int list_ticket = 0;              // Pedido da lista em andamento
static bool list_loaded = false;         // Já veio uma lista desta nuvem
static bool list_offline = false;        // A última busca falhou
static double list_requested_at = 0.0;
static int busy_ticket = 0;
static enum { BUSY_CREATE, BUSY_OPEN, BUSY_DELETE } busy_kind;
static const char *busy_label = "";
static char busy_name[SAVE_NAME_MAX + 1];
static char save_password[PASSWORD_BYTES];
static const char *password_error = NULL;

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
    save_notice_is_error = false;
    save_notice_until = GetTime() + 3.0;
}

static void show_error(const char *text)
{
    show_notice(text);
    save_notice_is_error = true;
    save_notice_until = GetTime() + 6.0;
}

static bool reserve_entries(int count)
{
    save_entry *grown;
    int capacity = save_capacity > 0 ? save_capacity : SAVE_SLOTS_MAX;

    if (count <= save_capacity) return true;
    while (capacity < count) capacity *= 2;
    grown = realloc(save_list, sizeof(*grown) * (size_t)capacity);
    if (grown == NULL) return false;
    save_list = grown;
    save_capacity = capacity;
    return true;
}

static void format_date(long long modified, char *out, size_t size)
{
    time_t moment = (time_t)modified;
    struct tm *local = modified > 0 ? localtime(&moment) : NULL;

    out[0] = '\0';
    if (local != NULL) strftime(out, size, "%d/%m/%Y %H:%M", local);
}

static void reload_local_slots(void)
{
    save_slot_info slots[SAVE_SLOTS_MAX];
    int count = save_slots_list(save_select_mode, slots, SAVE_SLOTS_MAX);

    save_count = reserve_entries(count) ? count : 0;
    for (int i = 0; i < save_count; i++)
    {
        save_list[i].info = slots[i];
        save_list[i].in_use = false;
        save_slot_summary(save_select_mode, slots[i].name, save_list[i].summary, sizeof(save_list[i].summary));
        format_date(slots[i].modified, save_list[i].date, sizeof(save_list[i].date));
    }
}

static void request_cloud_list(void)
{
    if (list_ticket != 0) return;
    list_ticket = cloud_list_saves(save_mode_folder(save_select_mode));
    list_requested_at = GetTime();
}

static void reload_slots(void)
{
    if (cloud_enabled()) request_cloud_list();
    else reload_local_slots();
}

// Põe na tela a lista que veio da nuvem, mantendo escolhido o mesmo save
static void apply_cloud_list(const cloud_result *result)
{
    char chosen[SAVE_NAME_MAX + 1] = "";
    bool first = !list_loaded;
    bool on_back = !first && save_selected_row == save_count + 1;

    if (save_selected_row >= 1 && save_selected_row <= save_count) snprintf(chosen, sizeof(chosen), "%s", save_list[save_selected_row - 1].info.name);

    save_count = reserve_entries(result->save_count) ? result->save_count : 0;
    for (int i = 0; i < save_count; i++)
    {
        const cloud_save_info *from = &result->saves[i];
        save_entry *to = &save_list[i];

        snprintf(to->info.name, sizeof(to->info.name), "%s", from->name);
        to->info.modified = (long)from->updated;
        to->in_use = from->in_use;
        snprintf(to->summary, sizeof(to->summary), "%s", from->in_use ? "Em uso em outro notebook" : from->has_progress ? "Com progresso" : "Sem progresso");
        format_date(from->updated, to->date, sizeof(to->date));
    }
    list_loaded = true;
    list_offline = false;

    if (on_back) save_selected_row = save_count + 1;
    // Quem já tem um save entra com ele escolhido: é o caso mais comum
    else if (first) save_selected_row = save_count > 0 ? 1 : 0;
    for (int i = 0; chosen[0] != '\0' && i < save_count; i++)
    {
        if (names_equal(chosen, save_list[i].info.name)) save_selected_row = i + 1;
    }
    if (save_selected_row > save_count + 1) save_selected_row = save_count + 1;
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
    save_count = 0;
    list_ticket = 0;      // Uma resposta de antes, de outro modo, é ignorada
    list_loaded = false;
    list_offline = false;
    busy_ticket = 0;
    memset(save_password, 0, sizeof(save_password));
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
        if (names_equal(name, save_list[i].info.name))
        {
            choose_slot(save_list[i].info.name);
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

// No modo sobrevivência cada save é um jogador (o nome do save é o nome no ranking); na dupla, cada save é uma dupla
static bool is_player_mode(void)
{
    return save_mode_is_survival(save_select_mode);
}

// Texto conforme o modo: save do modo normal, jogador do Sobrevivência ou dupla
#define MODE_TEXT(normal, player, duo) (save_select_mode == SAVE_MODE_DUO ? (duo) : is_player_mode() ? (player) : (normal))

// Apaga a cópia deste computador e a linha do ranking local
static bool delete_local(const char *name)
{
    bool deleted = save_slot_delete(save_select_mode, name);

    // Jogador apagado sai do ranking junto com o progresso
    if (deleted && is_player_mode())
    {
        // A dupla tem o ranking dela, separado do solo
        sv_ranking_set_path(save_select_mode == SAVE_MODE_DUO ? SV_RANKING_DUO_PATH : NULL);
        sv_ranking_load();
        sv_ranking_remove(name);
        sv_ranking_set_path(NULL);
    }
    return deleted;
}

bool save_select_delete_named(const char *name)
{
    bool deleted = delete_local(name);

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

// Na nuvem o nome é único no evento: a conferência usa a lista que veio do servidor
static save_name_check check_new_name(void)
{
    save_name_check check;
    char trimmed[SAVE_NAME_MAX + 1];

    if (!cloud_enabled()) return save_slot_check_name(save_select_mode, name_input);
    check = save_name_check_rules(name_input);
    if (check != SAVE_NAME_OK) return check;
    snprintf(trimmed, sizeof(trimmed), "%s", name_input);
    save_name_trim(trimmed);
    for (int i = 0; i < save_count; i++)
    {
        if (names_equal(trimmed, save_list[i].info.name)) return SAVE_NAME_EXISTS;
    }
    return SAVE_NAME_OK;
}

static void refresh_name_status(void)
{
    save_name_status = check_new_name();
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

static void start_busy(int ticket, int kind, const char *name, const char *label)
{
    busy_ticket = ticket;
    busy_kind = kind;
    busy_label = label;
    snprintf(busy_name, sizeof(busy_name), "%s", name);
    save_view = VIEW_BUSY;
    SetMouseCursor(MOUSE_CURSOR_ARROW);
}

static void start_open(const char *name)
{
    int ticket = cloud_open_save(save_mode_folder(save_select_mode), name);

    if (ticket != 0) start_busy(ticket, BUSY_OPEN, name, "Abrindo o save...");
    else show_error("Esse nome de save não é aceito.");
}

static void try_create_from_input(void)
{
    save_name_check check;

    if (cloud_enabled())
    {
        char trimmed[SAVE_NAME_MAX + 1];

        check = check_new_name();
        if (check == SAVE_NAME_OK)
        {
            snprintf(trimmed, sizeof(trimmed), "%s", name_input);
            save_name_trim(trimmed);
            start_busy(cloud_create_save(save_mode_folder(save_select_mode), trimmed), BUSY_CREATE, trimmed, "Criando o save...");
        }
    }
    else check = save_select_create_named(name_input);

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
    DrawTextEx(default_game_font, MODE_TEXT("NOME DO NOVO SAVE", "NOME DO NOVO JOGADOR", "NOME DA NOVA DUPLA"), (Vector2){panel.x + padding, panel.y + padding}, title_size, title_size * 0.06f, STYLE_TEXT_COLOR);

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
        DrawTextEx(default_game_font, MODE_TEXT("Até 20 letras, números, espaço, - ou _. O nome não pode se repetir.", "É o nome que aparece no ranking. Até 20 letras, números, espaço, - ou _.", "É o nome da dupla no ranking (ex.: Ana e Beto). Até 20 letras, números, espaço, - ou _."), (Vector2){box.x, box.y + box.height + 14.0f * scale}, DEFAULT_TEXT_FONT_SIZE * 0.8f * scale, 2.0f * scale, STYLE_DIM_COLOR);
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
    float height = (cloud_enabled() ? 500.0f : 340.0f) * scale;
    Rectangle panel = {(GetScreenWidth() - width) / 2.0f, (GetScreenHeight() - height) / 2.0f, width, height};
    float padding = 36.0f * scale;
    float title_size = style_font_size(0.62f);
    float button_width = 240.0f * scale;
    float button_height = 62.0f * scale;
    char question[96];

    style_panel(panel);
    DrawTextEx(default_game_font, MODE_TEXT("APAGAR O SAVE?", "APAGAR O JOGADOR?", "APAGAR A DUPLA?"), (Vector2){panel.x + padding, panel.y + padding}, title_size, title_size * 0.06f, STYLE_WARNING_COLOR);
    snprintf(question, sizeof(question), "\"%s\"", save_delete_name);
    DrawTextEx(default_game_font, question, (Vector2){panel.x + padding, panel.y + padding + title_size + 14.0f * scale}, style_font_size(0.5f), 3.0f * scale, STYLE_TEXT_COLOR);
    style_wrapped_text(MODE_TEXT("Todo o progresso desse save será perdido e não dá para desfazer.", "O progresso e a posição desse jogador no ranking serão perdidos, e não dá para desfazer.", "O progresso e a posição dessa dupla no ranking serão perdidos, e não dá para desfazer."), panel.x + padding, panel.y + padding + title_size + 14.0f * scale + style_font_size(0.5f) + 18.0f * scale, panel.width - padding * 2.0f,
                       DEFAULT_TEXT_FONT_SIZE * scale, STYLE_DIM_COLOR);

    *cancel_button = (Rectangle){panel.x + panel.width - padding - button_width * 2.0f - 20.0f * scale, panel.y + panel.height - padding - button_height, button_width, button_height};
    if (cloud_enabled())
    {
        float size = DEFAULT_TEXT_FONT_SIZE * 0.8f * scale;
        Rectangle box = {panel.x + padding, cancel_button->y - 112.0f * scale, panel.width - padding * 2.0f, 60.0f * scale};
        char hidden[PASSWORD_BYTES];
        int letters = 0;

        // Um asterisco por letra (os bytes de continuação do UTF-8 não contam)
        for (const char *c = save_password; *c != '\0'; c++)
        {
            if (((unsigned char)*c & 0xC0) != 0x80) hidden[letters++] = '*';
        }
        hidden[letters] = '\0';

        DrawTextEx(default_game_font, "Senha da organização:", (Vector2){box.x, box.y - size - 8.0f * scale}, size, 2.0f * scale, STYLE_DIM_COLOR);
        DrawRectangleRec(box, (Color){10, 6, 12, 255});
        DrawRectangleLinesEx(box, 2.0f * scale, password_error != NULL ? STYLE_WARNING_COLOR : (Color){120, 90, 140, 255});
        DrawTextEx(default_game_font, hidden, (Vector2){box.x + 16.0f * scale, box.y + (box.height - size) / 2.0f}, size, 4.0f * scale, STYLE_TEXT_COLOR);
        if (password_error != NULL) DrawTextEx(default_game_font, password_error, (Vector2){box.x, box.y + box.height + 8.0f * scale}, size, 2.0f * scale, STYLE_WARNING_COLOR);
    }
    *delete_button = (Rectangle){panel.x + panel.width - padding - button_width, cancel_button->y, button_width, button_height};
    style_button(*cancel_button, "CANCELAR", save_delete_choice == 0, false);
    style_button(*delete_button, "APAGAR", save_delete_choice == 1, true);
}

static void close_delete(void)
{
    memset(save_password, 0, sizeof(save_password));
    password_error = NULL;
    save_view = VIEW_LIST;
}

static void confirm_delete(void)
{
    if (save_delete_choice != 1)
    {
        close_delete();
        return;
    }
    if (cloud_enabled())
    {
        if (save_password[0] == '\0') password_error = "Digite a senha para apagar.";
        else
        {
            start_busy(cloud_delete_save(save_mode_folder(save_select_mode), save_delete_name, save_password), BUSY_DELETE, save_delete_name, "Apagando...");
            memset(save_password, 0, sizeof(save_password));
            password_error = NULL;
        }
        return;
    }
    if (save_select_delete_named(save_delete_name)) show_notice(MODE_TEXT("Save apagado.", "Jogador apagado.", "Dupla apagada."));
    else show_error("Não consegui apagar o save.");
    save_view = VIEW_LIST;
}

static void confirm_row(int row)
{
    if (row == 0)
    {
        // Sem a lista da nuvem, a primeira linha tenta buscar de novo
        if (cloud_enabled() && (!list_loaded || list_offline)) request_cloud_list();
        else open_name_input();
    }
    else if (row <= save_count)
    {
        if (cloud_enabled()) start_open(save_list[row - 1].info.name);
        else choose_slot(save_list[row - 1].info.name);
    }
    else
    {
        save_select_result = SAVE_SELECT_BACK;
        SetMouseCursor(MOUSE_CURSOR_ARROW);
    }
}

static void open_delete(int row)
{
    if (row < 1 || row > save_count) return;
    snprintf(save_delete_name, sizeof(save_delete_name), "%s", save_list[row - 1].info.name);
    save_delete_choice = 0;
    memset(save_password, 0, sizeof(save_password));
    password_error = NULL;
    // A tecla X que abriu o painel não pode virar a primeira letra da senha
    while (GetCharPressed() > 0) { }
    save_view = VIEW_DELETE;
}

// Digitação da senha de apagar (nuvem)
static void handle_password_input(void)
{
    int codepoint = GetCharPressed();
    size_t length = strlen(save_password);

    while (codepoint > 0)
    {
        int bytes = 0;
        const char *text = CodepointToUTF8(codepoint, &bytes);

        if (codepoint >= 32 && length + (size_t)bytes < sizeof(save_password))
        {
            memcpy(save_password + length, text, (size_t)bytes);
            length += (size_t)bytes;
            save_password[length] = '\0';
            save_delete_choice = 1;
            password_error = NULL;
        }
        codepoint = GetCharPressed();
    }
    if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) && length > 0)
    {
        do length--;
        while (length > 0 && ((unsigned char)save_password[length] & 0xC0) == 0x80);
        save_password[length] = '\0';
        password_error = NULL;
    }
}

// -----------------------------------------------------------------------------
// Respostas da nuvem
// -----------------------------------------------------------------------------

static void finish_create(const cloud_result *result)
{
    if (result->status == CLOUD_OK) start_open(busy_name);
    else if (result->status == CLOUD_EXISTS || result->status == CLOUD_INVALID)
    {
        save_name_status = result->status == CLOUD_EXISTS ? SAVE_NAME_EXISTS : SAVE_NAME_INVALID;
        save_show_name_message = true;
        save_view = VIEW_NAME;
    }
    else
    {
        show_error("Sem conexão com a nuvem: o save não foi criado.");
        save_view = VIEW_LIST;
    }
}

static void finish_open(const cloud_result *result)
{
    save_view = VIEW_LIST;
    if (result->status == CLOUD_OK)
    {
        if (cloud_sync_write_local(save_select_mode, busy_name, result->content))
        {
            if (result->local_discarded) notice_show("Havia uma gravação antiga deste computador que não subiu. Vale a da nuvem.", NOTICE_ERROR);
            choose_slot(busy_name);
            return;
        }
        cloud_close_save();
        show_error("Não consegui gravar o save neste computador.");
    }
    else if (result->status == CLOUD_IN_USE) show_error("Esse save está em uso em outro notebook.");
    else if (result->status == CLOUD_NOT_FOUND) show_error("Esse save foi apagado.");
    else show_error("Sem conexão com a nuvem: o save não foi aberto.");
    request_cloud_list();
}

static void finish_delete(const cloud_result *result)
{
    if (result->status == CLOUD_WRONG_PASSWORD)
    {
        password_error = "Senha errada.";
        save_view = VIEW_DELETE;
        while (GetCharPressed() > 0) { }
        return;
    }
    save_view = VIEW_LIST;
    if (result->status == CLOUD_OK)
    {
        delete_local(busy_name);
        show_notice(MODE_TEXT("Save apagado.", "Jogador apagado.", "Dupla apagada."));
    }
    else if (result->status == CLOUD_IN_USE) show_error("Esse save está em uso em outro notebook e não foi apagado.");
    else if (result->status == CLOUD_NOT_FOUND) show_error("Esse save já tinha sido apagado.");
    else show_error("Sem conexão com a nuvem: nada foi apagado.");
    list_ticket = 0;      // Uma lista pedida antes de apagar ainda traria o save
    request_cloud_list();
}

static void update_cloud(void)
{
    cloud_result result;

    if (!cloud_enabled()) return;

    if (list_ticket != 0 && cloud_poll(list_ticket, &result))
    {
        list_ticket = 0;
        if (result.status == CLOUD_OK) apply_cloud_list(&result);
        else list_offline = true;
        cloud_result_free(&result);
    }
    if (save_view == VIEW_BUSY)
    {
        if (busy_ticket == 0)
        {
            show_error("Não consegui falar com a nuvem.");
            save_view = VIEW_LIST;
        }
        else if (cloud_poll(busy_ticket, &result))
        {
            busy_ticket = 0;
            if (busy_kind == BUSY_CREATE) finish_create(&result);
            else if (busy_kind == BUSY_OPEN) finish_open(&result);
            else finish_delete(&result);
            cloud_result_free(&result);
        }
    }
    else if (save_view == VIEW_LIST && GetTime() - list_requested_at > REFRESH_SECONDS) request_cloud_list();
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
    int card_total;
    int back_row;
    int visible;
    view_state active_view;   // A tela em que o quadro começou: uma tecla que abre uma janela não vale também dentro dela
    Rectangle create_button = {0}, cancel_button = {0}, delete_button = {0};
    Rectangle row_areas[VISIBLE_MAX] = {0};      // Pela posição na tela
    Rectangle remove_area = {0};
    int hovered = -1;
    bool hovered_remove = false;

    if (column_width > 1040.0f * scale) column_width = 1040.0f * scale;
    update_cloud();
    card_total = save_count + 1;            // NOVO SAVE e os saves; o VOLTAR é o botão de baixo
    back_row = save_count + 1;
    active_view = save_view;

    column_x = (screen_w - column_width) / 2.0f;
    back_button = (Rectangle){(screen_w - 260.0f * scale) / 2.0f, screen_h - 135.0f * scale, 260.0f * scale, 62.0f * scale};
    bottom = back_button.y - 56.0f * scale;
    visible = (int)((bottom - top + card_gap) / (card_height + card_gap));
    if (visible < 2) visible = 2;
    if (visible > VISIBLE_MAX) visible = VISIBLE_MAX;

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
        // Com senha, as letras e o espaço são dela: só as setas, TAB, ENTER e ESC comandam o painel
        bool typing = cloud_enabled();

        if (typing) handle_password_input();
        if (IsKeyPressed(KEY_LEFT) || (!typing && IsKeyPressed(KEY_A))) save_delete_choice = 0;
        if (IsKeyPressed(KEY_RIGHT) || (!typing && IsKeyPressed(KEY_D))) save_delete_choice = 1;
        if (IsKeyPressed(KEY_TAB)) save_delete_choice = 1 - save_delete_choice;
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || (!typing && IsKeyPressed(KEY_SPACE))) confirm_delete();
        if (IsKeyPressed(KEY_ESCAPE)) close_delete();
    }

    // -------- desenho
    begin_game_drawing();
    style_backdrop();
    style_title_centered(save_mode_title(save_select_mode), save_select_mode == SAVE_MODE_DUO ? "QUAL DUPLA VAI JOGAR?" : is_player_mode() ? "QUEM VAI JOGAR?" : "ESCOLHA O SAVE");

    for (int row = save_first_visible; row < card_total && row < save_first_visible + visible; row++)
    {
        float y = top + (float)(row - save_first_visible) * (card_height + card_gap);
        bool selected = (row == save_selected_row && save_view == VIEW_LIST);
        Rectangle card = {column_x, y, column_width, card_height};
        float padding = 26.0f * scale;
        const char *label = row == 0 ? (MODE_TEXT("+  NOVO SAVE", "+  NOVO JOGADOR", "+  NOVA DUPLA")) : save_list[row - 1].info.name;
        const char *first_detail = MODE_TEXT("Começar do zero com um nome novo", "Digite seu nome para entrar no ranking", "Digite o nome da dupla para entrar no ranking");

        if (row == 0 && cloud_enabled() && (!list_loaded || list_offline))
        {
            label = list_ticket != 0 ? "BUSCANDO OS SAVES..." : "TENTAR DE NOVO";
            first_detail = list_offline ? "Sem conexão com a nuvem. Confira a internet deste notebook." : "Os saves ficam na nuvem, para todos os notebooks.";
        }
        row_areas[row - save_first_visible] = card;
        style_card_frame(card, selected, row == 0);
        DrawTextEx(default_game_font, label, (Vector2){card.x + padding, card.y + 12.0f * scale}, name_size, name_size * 0.06f, selected ? STYLE_TEXT_COLOR : (Color){200, 192, 206, 255});

        if (row == 0)
        {
            DrawTextEx(default_game_font, first_detail, (Vector2){card.x + padding, card.y + 20.0f * scale + name_size}, detail_size, detail_size * 0.06f, STYLE_DIM_COLOR);
        }
        else
        {
            const char *date = save_list[row - 1].date;
            Vector2 date_measure = MeasureTextEx(default_game_font, date, detail_size, detail_size * 0.06f);

            DrawTextEx(default_game_font, save_list[row - 1].summary, (Vector2){card.x + padding, card.y + 20.0f * scale + name_size}, detail_size, detail_size * 0.06f, save_list[row - 1].in_use ? STYLE_WARNING_COLOR : STYLE_DIM_COLOR);
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

        DrawTextEx(default_game_font, save_notice, (Vector2){(screen_w - measure.x) / 2.0f, back_button.y - 44.0f * scale}, size, 2.0f * scale, save_notice_is_error ? STYLE_WARNING_COLOR : (Color){150, 230, 160, 255});
    }
    else if (save_view == VIEW_LIST && cloud_enabled() && (list_offline || cloud_pending_count() > 0))
    {
        float size = DEFAULT_TEXT_FONT_SIZE * 0.8f * scale;
        char status[128];
        Vector2 measure;

        if (list_offline) snprintf(status, sizeof(status), "Sem conexão com a nuvem.");
        else snprintf(status, sizeof(status), "Envios aguardando a nuvem: %d", cloud_pending_count());
        measure = MeasureTextEx(default_game_font, status, size, 2.0f * scale);
        DrawTextEx(default_game_font, status, (Vector2){(screen_w - measure.x) / 2.0f, back_button.y - 40.0f * scale}, size, 2.0f * scale, list_offline ? STYLE_WARNING_COLOR : STYLE_DIM_COLOR);
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
    else if (save_view == VIEW_DELETE)
    {
        draw_delete_panel(&cancel_button, &delete_button);
    }
    else
    {
        float width = screen_w * 0.5f;
        Rectangle panel = {(screen_w - width) / 2.0f, (screen_h - 180.0f * scale) / 2.0f, width, 180.0f * scale};
        float size = style_font_size(0.5f);
        Vector2 measure = MeasureTextEx(default_game_font, busy_label, size, size * 0.06f);

        style_panel(panel);
        DrawTextEx(default_game_font, busy_label, (Vector2){(screen_w - measure.x) / 2.0f, panel.y + (panel.height - measure.y) / 2.0f}, size, size * 0.06f, STYLE_TEXT_COLOR);
    }

    end_game_drawing();

    // -------- mouse (depois do desenho, quando as áreas já foram calculadas)
    if (active_view == VIEW_LIST && save_view == VIEW_LIST)
    {
        for (int row = save_first_visible; row < card_total && row < save_first_visible + visible; row++)
        {
            if (style_mouse_over(row_areas[row - save_first_visible])) hovered = row;
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
