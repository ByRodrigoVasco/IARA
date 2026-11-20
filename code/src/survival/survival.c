#include <math.h>
#include <stdio.h>
#include <string.h>
#include "raylib.h"
#include "accessibility.h"
#include "survival.h"
#include "sv_game.h"
#include "sv_render.h"
#include "sv_light.h"
#include "sv_ranking.h"
#include "sv_meta.h"
#include "sv_audio.h"
#include "sv_assets.h"
#include "sv_debug.h"
#include "sv_duo.h"
#include "characters.h"
#include "macros.h"
#include "loading.h"
#include "save_slots.h"
#include "confirm_dialog.h"
#include "help.h"
#include "notice.h"
#include "ui_theme.h"

typedef enum { SCREEN_HUB, SCREEN_SHOP, SCREEN_CHARS, SCREEN_RUN, SCREEN_PAUSE, SCREEN_OVER } SvScreen;

static SvScreen screen = SCREEN_HUB;
static bool loaded = false;
static bool leave_to_menu = false;
static bool leave_to_saves = false;    // TROCAR JOGADOR: volta para a lista de saves do modo
static int hub_selected = 0;
static bool duo = false;               // O save em uso é de uma dupla (saves/dupla): as partidas são em dupla
static int char_cursor = 0;
static int char_editing = 0;           // Dupla: de quem é o personagem que está sendo escolhido (0 ou 1)
static Texture2D roster_icons[CH_COUNT];
static float char_message_timer = 0.0f;
static const char *char_message = "";
static int shop_selected = 0;
static int levelup_selected = 0;
static int pause_selected = 0;
static bool pause_help_open = false;   // O painel de ajuda está aberto por cima da pausa
static int earned_coins = 0;
static float shop_message_timer = 0.0f;
static const char *shop_message = "";

// Ranking: cada save é um jogador, e o nome do save é o nome no ranking
static bool run_ranked = false;        // A partida atual já foi registrada no ranking
static bool last_run_record = false;   // A última partida virou o recorde do jogador
static int last_rank_position = 0;     // Posição do jogador depois da última partida (1 = primeiro)
static long last_rank_order = 0;       // Para destacar a linha do jogador no ranking

static void finish_run(void);

static const char *upgrade_names[UPG_COUNT] = {
    "Plano de Saúde Premium", "Bônus de Desempenho", "Sapato Confortável", "Atalhos de Teclado", "Monitor Ultrawide",
    "Network Ativo", "Mentoria", "Crachá Reforçado", "Ctrl+Z"};
static const char *upgrade_descriptions[UPG_COUNT] = {
    "+10 de vida máxima por nível", "+5% de dano por nível", "+4% de velocidade por nível", "-4% de recarga por nível", "+5% de área por nível",
    "+15% de alcance de coleta por nível", "+5% de XP por nível", "-1 de dano em cada golpe por nível", "Ao morrer, volta uma vez com 50% da vida"};

#define HUB_OPTIONS 6
static const char *hub_labels[HUB_OPTIONS] = {"JOGAR", "PERSONAGEM", "PLANO DE CARREIRA", "COMO JOGAR", "TROCAR JOGADOR", "VOLTAR"};
static const char *hub_labels_duo[HUB_OPTIONS] = {"JOGAR EM DUPLA", "PERSONAGENS", "PLANO DE CARREIRA", "COMO JOGAR", "TROCAR DUPLA", "VOLTAR"};
static bool hub_help_open = false;   // A ajuda está aberta por cima do início

// O carregamento é dividido em trabalhos pequenos: a tela de carregamento roda um pedaço por quadro e mostra a porcentagem.
// Se o disco não deixou gravar, o jogador precisa saber: avisar na hora é melhor que perder o progresso em silêncio.
static void warn_if_save_failed(void)
{
    if (!sv_meta_last_save_ok()) notice_show("Não consegui salvar o progresso do Sobrevivência. Confira se há espaço livre no disco e se a pasta de saves permite gravar arquivos.", NOTICE_ERROR);
}

static float job_read_save(void)
{
    char path[512];

    // O ranking é um só para todos os saves do modo (a feira inteira joga no mesmo computador).
    // A dupla tem o ranking dela, num arquivo separado: as duplas não entram no ranking solo.
    duo = active_save_is_set() && active_save_mode() == SAVE_MODE_DUO;
    sv_ranking_set_path(duo ? SV_RANKING_DUO_PATH : NULL);
    sv_ranking_load();

    // Cada save tem o seu arquivo de progresso; sem save escolhido (testes), vale o arquivo antigo
    if (active_save_is_set() && save_mode_is_survival(active_save_mode()) && active_save_path(path, sizeof(path))) sv_meta_set_path(path);
    else sv_meta_set_path(NULL);

    sv_meta_load(sv_meta_path());
    if (sv_meta.character < 0 || sv_meta.character >= CH_COUNT || !sv_character_unlocked(sv_meta.character, sv_meta.runs, sv_meta.wins, sv_meta.best_seconds, sv_meta.kills)) sv_meta.character = CH_ESTAGIARIO;
    if (sv_meta.character2 < 0 || sv_meta.character2 >= CH_COUNT || !sv_character_unlocked(sv_meta.character2, sv_meta.runs, sv_meta.wins, sv_meta.best_seconds, sv_meta.kills)) sv_meta.character2 = CH_ESTAGIARIO;
    char_cursor = sv_meta.character;
    char_editing = 0;
    return 1.0f;
}

static float job_sprites(void)
{
    return sv_load_sprites_step();
}

static float job_characters(void)
{
    sv_load_player(sv_meta.character);
    if (duo)
    {
        // Dupla: o jogador 2 tem o seu conjunto de quadros
        sv_use_player_slot(1);
        sv_load_player(sv_meta.character2);
        sv_use_player_slot(0);
    }
    for (int i = 0; i < CH_COUNT; i++)
    {
        roster_icons[i] = LoadTexture(TextFormat("%s%s/walk/south-0.png", PERSONAGENS_PATH, sv_character(i)->folder));
        if (roster_icons[i].id) SetTextureFilter(roster_icons[i], TEXTURE_FILTER_POINT);
    }
    return 1.0f;
}

static float job_audio(void)
{
    return sv_audio_init_step();
}

static float job_ready(void)
{
    loaded = true;
    leave_to_menu = false;
    leave_to_saves = false;
    hub_help_open = false;
    screen = SCREEN_HUB;
    hub_selected = 0;
    shop_selected = 0;
    sv_music_play(MUS_MENU);
    sv_ambient_play(-1);
    return 1.0f;
}

int survival_loading_jobs(loading_job *jobs)
{
    jobs[0] = (loading_job){"Lendo o save", 0.3f, job_read_save};
    jobs[1] = (loading_job){"Imagens do mapa, dos inimigos e dos poderes", 5.0f, job_sprites};
    jobs[2] = (loading_job){"Personagens", 1.0f, job_characters};
    jobs[3] = (loading_job){"Sons e músicas", 3.5f, job_audio};
    jobs[4] = (loading_job){"Preparando o modo", 0.2f, job_ready};
    return 5;
}

void initialize_survival(void)
{
    loading_job jobs[LOADING_MAX_JOBS];
    int count = survival_loading_jobs(jobs);

    for (int i = 0; i < count; i++)
    {
        while (jobs[i].step() < 1.0f) { }
    }
}

void unload_survival(void)
{
    if (!loaded) return;

    // Quem sai no meio da partida leva o que juntou até aqui
    if (screen == SCREEN_RUN || screen == SCREEN_PAUSE) finish_run();

    for (int i = 0; i < CH_COUNT; i++)
    {
        if (roster_icons[i].id) UnloadTexture(roster_icons[i]);
        roster_icons[i] = (Texture2D){0};
    }
    sv_audio_close();
    sv_unload_all_players();
    sv_unload_sprites();
    sv_light_unload();
    SetMouseCursor(MOUSE_CURSOR_ARROW);
    sv_ranking_set_path(NULL);
    duo = false;
    loaded = false;
}

// -----------------------------------------------------------------------------
// Entrada
// -----------------------------------------------------------------------------

static bool pressed_up(void) { return IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W); }
static bool pressed_down(void) { return IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S); }
static bool pressed_left(void) { return IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A); }
static bool pressed_right(void) { return IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D); }
static bool pressed_confirm(void) { return IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_KP_ENTER); }

static Vector2 read_movement(void)
{
    Vector2 move = {0.0f, 0.0f};

    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) move.x -= 1.0f;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) move.x += 1.0f;
    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) move.y -= 1.0f;
    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) move.y += 1.0f;
    return move;
}

// Dupla: o analógico esquerdo do controle soma ao teclado (zona morta pequena para o boneco não andar sozinho)
static Vector2 add_gamepad(Vector2 move, int gamepad)
{
    if (IsGamepadAvailable(gamepad))
    {
        float x = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_X);
        float y = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_Y);

        if (fabsf(x) > 0.25f) move.x += x;
        if (fabsf(y) > 0.25f) move.y += y;
        if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) move.x -= 1.0f;
        if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) move.x += 1.0f;
        if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_UP)) move.y -= 1.0f;
        if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) move.y += 1.0f;
    }
    return move;
}

// Dupla: jogador 1 no WASD (ou no controle 1), jogador 2 nas setas (ou no controle 2)
static Vector2 read_movement_p1(void)
{
    Vector2 move = {0.0f, 0.0f};

    if (IsKeyDown(KEY_A)) move.x -= 1.0f;
    if (IsKeyDown(KEY_D)) move.x += 1.0f;
    if (IsKeyDown(KEY_W)) move.y -= 1.0f;
    if (IsKeyDown(KEY_S)) move.y += 1.0f;
    return add_gamepad(move, 0);
}

static Vector2 read_movement_p2(void)
{
    Vector2 move = {0.0f, 0.0f};

    if (IsKeyDown(KEY_LEFT)) move.x -= 1.0f;
    if (IsKeyDown(KEY_RIGHT)) move.x += 1.0f;
    if (IsKeyDown(KEY_UP)) move.y -= 1.0f;
    if (IsKeyDown(KEY_DOWN)) move.y += 1.0f;
    return add_gamepad(move, 1);
}

// Retângulo de uma linha de menu centrada
static Rectangle menu_row(int index, float first_y, float spacing, float width)
{
    float ui = sv_ui();

    return (Rectangle){(GetScreenWidth() - width * ui) / 2.0f, first_y * ui + (float)index * spacing * ui, width * ui, spacing * ui * 0.85f};
}

// Seleção por mouse: devolve a linha sob o cursor se ele se moveu, e se houve clique
static int mouse_pick(int count, float first_y, float spacing, float width, bool *clicked)
{
    Vector2 mouse = GetMousePosition();
    Vector2 delta = GetMouseDelta();
    int hovered = -1;

    *clicked = false;
    for (int i = 0; i < count; i++)
    {
        if (CheckCollisionPointRec(mouse, menu_row(i, first_y, spacing, width))) hovered = i;
    }

    if (hovered >= 0 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) *clicked = true;
    if (hovered >= 0 && delta.x == 0.0f && delta.y == 0.0f && !*clicked) return -1;
    return hovered;
}

// -----------------------------------------------------------------------------
// Início e Plano de Carreira
// -----------------------------------------------------------------------------

// Nome do jogador no ranking: o nome do save em uso
static const char *player_name(void)
{
    return active_save_is_set() ? active_save_name() : "Anônimo";
}

// Termina a partida: paga o Banco de Horas e registra o resultado no ranking (uma vez só)
static void finish_run(void)
{
    earned_coins = sv_game_finish();
    if (run_ranked || sv_run.cheated) return;   // Partida com cheat não entra no ranking
    run_ranked = true;
    last_rank_position = sv_ranking_add(player_name(), (int)sv_run.time, sv_run.kills, sv_run.player.level, sv_run.bosses_defeated, sv_run.player.character, sv_run.phase == SV_WON, &last_run_record);
    last_rank_order = last_rank_position > 0 ? sv_ranking_get(last_rank_position - 1)->order : 0;
}

static void start_run(void)
{
    run_ranked = false;
    last_run_record = false;
    last_rank_position = 0;
    last_rank_order = 0;
    sv_duo_mode = duo;
    sv_game_start((unsigned)(GetTime() * 1000.0) | 1u);
    sv_time_scale = 1;
    screen = SCREEN_RUN;
    levelup_selected = 0;
    pause_selected = 0;
}

static void draw_backdrop(void)
{
    float ui = sv_ui();

    ClearBackground((Color){8, 4, 12, 255});
    sv_draw(SPR_fl_4_6, 0, (Vector2){GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f}, GetScreenHeight() / 438.0f * 1.05f, 0.0f, false, (Color){70, 40, 60, 255});
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){6, 2, 10, 150});
    (void)ui;
}

// -----------------------------------------------------------------------------
// Ranking
// -----------------------------------------------------------------------------

// Tabela do ranking: posição, nome, tempo e abates. A partida que acabou de terminar aparece destacada.
static void draw_ranking(float x, float y, float width, int rows, float size)
{
    float ui = sv_ui();
    float line = size * 1.25f;
    float pad = 16.0f * ui;
    int count = sv_ranking_count();
    char text[96];

    DrawRectangleRec((Rectangle){x, y, width, pad * 2.0f + line * (float)(rows + 1) + size * 0.4f}, (Color){10, 4, 16, 215});
    DrawRectangleLinesEx((Rectangle){x, y, width, pad * 2.0f + line * (float)(rows + 1) + size * 0.4f}, 2.0f * ui, (Color){120, 30, 44, 255});

    sv_text("#", x + pad, y + pad, size, (Color){160, 150, 165, 255});
    sv_text("NOME", x + pad + size * 1.6f, y + pad, size, (Color){160, 150, 165, 255});
    sv_text("TEMPO", x + width - pad - size * 5.6f, y + pad, size, (Color){160, 150, 165, 255});
    sv_text("ABATES", x + width - pad - sv_text_width("ABATES", size), y + pad, size, (Color){160, 150, 165, 255});

    for (int i = 0; i < rows; i++)
    {
        const SvRankEntry *e = sv_ranking_get(i);
        float ry = y + pad + line * (float)(i + 1) + size * 0.4f;
        Color color = i == 0 ? (Color){255, 214, 80, 255} : (i < 3 ? (Color){235, 225, 235, 255} : (Color){200, 190, 205, 255});

        if (!e)
        {
            if (i == 0 && count == 0) sv_text("Ninguém jogou ainda. Seja o primeiro!", x + pad, ry, size * 0.85f, (Color){160, 150, 165, 255});
            break;
        }
        if (last_rank_order != 0 && e->order == last_rank_order)
        {
            DrawRectangleRec((Rectangle){x + 4.0f * ui, ry - size * 0.1f, width - 8.0f * ui, line}, (Color){90, 20, 34, 230});
            color = (Color){255, 120, 130, 255};
        }
        snprintf(text, sizeof(text), "%d", i + 1);
        sv_text(text, x + pad, ry, size, color);
        {
            // Nome comprido é cortado com ".." para não invadir a coluna do tempo
            char shown[SV_NAME_BYTES + 4];
            float room = (x + width - pad - size * 5.6f) - (x + pad + size * 1.6f) - size * 0.4f;
            int len;

            snprintf(shown, sizeof(shown), "%s", e->name);
            len = (int)strlen(shown);
            while (len > 0 && sv_text_width(shown, size) > room)
            {
                len--;
                while (len > 0 && ((unsigned char)shown[len] & 0xC0) == 0x80) len--;   // não corta no meio de um acento
                shown[len] = '\0';
                if (len > 0) snprintf(shown + len, sizeof(shown) - (size_t)len, "..");
                if (sv_text_width(shown, size) <= room) break;
                shown[len] = '\0';
            }
            sv_text(shown, x + pad + size * 1.6f, ry, size, color);
        }
        snprintf(text, sizeof(text), "%d:%02d", e->seconds / 60, e->seconds % 60);
        sv_text(text, x + width - pad - size * 5.6f, ry, size, e->won ? (Color){120, 230, 160, 255} : color);
        snprintf(text, sizeof(text), "%d", e->kills);
        sv_text(text, x + width - pad - sv_text_width(text, size), ry, size, color);
    }
}

static void render_hub(void)
{
    float ui = sv_ui();
    float w = (float)GetScreenWidth();
    char text[128];
    bool clicked = false;
    int hovered;

    // Com a ajuda aberta, só ela responde às teclas e ao mouse
    if (!hub_help_open)
    {
        if (pressed_down()) { hub_selected = (hub_selected + 1) % HUB_OPTIONS; sv_sfx(SND_escolha); }
        if (pressed_up()) { hub_selected = (hub_selected + HUB_OPTIONS - 1) % HUB_OPTIONS; sv_sfx(SND_escolha); }

        hovered = mouse_pick(HUB_OPTIONS, 540.0f, 72.0f, 620.0f, &clicked);
        if (hovered >= 0 && hovered != hub_selected) { hub_selected = hovered; sv_sfx(SND_escolha); }

        if (pressed_confirm() || clicked)
        {
            sv_sfx(SND_confirma);
            if (hub_selected == 0) start_run();
            else if (hub_selected == 1) { screen = SCREEN_CHARS; char_editing = 0; char_cursor = sv_meta.character; char_message_timer = 0.0f; }
            else if (hub_selected == 2) { screen = SCREEN_SHOP; shop_selected = 0; sv_music_play(MUS_LOJA); }
            else if (hub_selected == 3) { help_select_page(1); hub_help_open = true; }
            else if (hub_selected == 4) leave_to_saves = true;
            else leave_to_menu = true;
        }

        if (IsKeyPressed(KEY_ESCAPE)) leave_to_menu = true;
    }

    begin_game_drawing();
    draw_backdrop();

    if (duo)
    {
        sv_text_centered("SOBREVIVÊNCIA EM DUPLA", w / 2.0f, 100.0f * ui, 96.0f * ui, (Color){230, 40, 50, 255});
        sv_text_centered("Dez minutos de expediente, dois funcionários. Se um cair, o outro levanta.", w / 2.0f, 235.0f * ui, 30.0f * ui, (Color){220, 205, 215, 255});
    }
    else
    {
        sv_text_centered("SOBREVIVÊNCIA", w / 2.0f, 90.0f * ui, 120.0f * ui, (Color){230, 40, 50, 255});
        sv_text_centered("Dez minutos de expediente. Sobreviva às cinco áreas e desligue a IARA.", w / 2.0f, 235.0f * ui, 30.0f * ui, (Color){220, 205, 215, 255});
    }

    snprintf(text, sizeof(text), "%s   BANCO DE HORAS  %d", player_name(), sv_meta.coins);
    sv_text_centered(text, w / 2.0f, 310.0f * ui, 44.0f * ui, (Color){255, 214, 80, 255});
    snprintf(text, sizeof(text), "Partidas %d   Vitórias %d   Melhor tempo %d:%02d   Abates %d", sv_meta.runs, sv_meta.wins, sv_meta.best_seconds / 60, sv_meta.best_seconds % 60, sv_meta.kills);
    sv_text_centered(text, w / 2.0f, 370.0f * ui, 26.0f * ui, (Color){180, 170, 185, 255});

    if (duo) sv_text_centered("Jogador 1: WASD (ou controle 1).  Jogador 2: setas (ou controle 2).  Fique ao lado de quem caiu para levantar.", w / 2.0f, 440.0f * ui, 26.0f * ui, (Color){180, 170, 185, 255});
    else sv_text_centered("Mover: WASD ou setas. Os poderes atacam sozinhos. ESC pausa.", w / 2.0f, 440.0f * ui, 26.0f * ui, (Color){180, 170, 185, 255});

    for (int i = 0; i < HUB_OPTIONS; i++)
    {
        Rectangle row = menu_row(i, 540.0f, 72.0f, 620.0f);
        bool selected = i == hub_selected;

        if (selected) DrawRectangleRec(row, (Color){70, 14, 26, 220});
        sv_text_centered(duo ? hub_labels_duo[i] : hub_labels[i], w / 2.0f, row.y + 8.0f * ui, 52.0f * ui, selected ? (Color){255, 70, 80, 255} : (Color){225, 215, 225, 255});
    }

    sv_text_centered(duo ? "RANKING DAS DUPLAS" : "RANKING", w - 300.0f * ui, 470.0f * ui, (duo ? 40.0f : 52.0f) * ui, (Color){255, 214, 80, 255});
    draw_ranking(w - 560.0f * ui, 530.0f * ui, 520.0f * ui, 10, 30.0f * ui);

    if (hub_help_open)
    {
        if (help_update_and_draw_panel()) hub_help_open = false;
    }
    else
    {
        ui_draw_hint("SETAS: escolher    ENTER: confirmar    ESC: voltar ao menu");
    }

    end_game_drawing();
}

// -----------------------------------------------------------------------------
// Escolha do personagem
// -----------------------------------------------------------------------------

static bool character_unlocked(int id)
{
    // No modo debug todos os personagens estão liberados
    return sv_debug_mode || sv_character_unlocked(id, sv_meta.runs, sv_meta.wins, sv_meta.best_seconds, sv_meta.kills);
}

static Rectangle roster_cell(int index)
{
    float ui = sv_ui();
    float cell_w = 210.0f * ui;
    float total;

    // Com muitos personagens a fileira encolhe para caber na tela
    if (cell_w * CH_COUNT > (float)GetScreenWidth() - 20.0f * ui) cell_w = ((float)GetScreenWidth() - 20.0f * ui) / CH_COUNT;
    total = cell_w * CH_COUNT;

    return (Rectangle){(GetScreenWidth() - total) / 2.0f + cell_w * (float)index, 790.0f * ui, cell_w - 10.0f * ui, 190.0f * ui};
}

// Personagem salvo de quem está escolhendo (na dupla, o do jogador 1 ou o do jogador 2)
static int saved_character(void)
{
    return char_editing == 1 ? sv_meta.character2 : sv_meta.character;
}

// Carrega a prévia no conjunto de quadros de quem está escolhendo (no solo, sempre o 0)
static void load_preview(int id)
{
    sv_use_player_slot(char_editing);
    sv_load_player(id);
    sv_use_player_slot(0);
}

static void choose_character(int id)
{
    if (id == char_cursor) return;
    char_cursor = id;
    load_preview(id);
    sv_sfx(SND_escolha);
}

// Dupla: TAB passa a escolha para o outro jogador (quem saiu sem confirmar volta ao personagem salvo)
static void switch_character_editor(void)
{
    if (char_cursor != saved_character()) load_preview(saved_character());
    char_editing = 1 - char_editing;
    char_cursor = saved_character();
    char_message_timer = 0.0f;
    sv_sfx(SND_escolha);
}

static void render_characters(void)
{
    float ui = sv_ui();
    float w = (float)GetScreenWidth();
    const SvCharacter *c = sv_character(char_cursor);
    bool unlocked = character_unlocked(char_cursor);
    Vector2 mouse = GetMousePosition();
    bool confirm = pressed_confirm();
    int frame = get_walk_frame((float)GetTime());

    if (duo && IsKeyPressed(KEY_TAB)) switch_character_editor();
    if (pressed_right()) choose_character((char_cursor + 1) % CH_COUNT);
    if (pressed_left()) choose_character((char_cursor + CH_COUNT - 1) % CH_COUNT);

    for (int i = 0; i < CH_COUNT; i++)
    {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, roster_cell(i)))
        {
            choose_character(i);
            confirm = true;
        }
    }
    c = sv_character(char_cursor);
    unlocked = character_unlocked(char_cursor);

    if (confirm)
    {
        if (!unlocked)
        {
            char_message = "Personagem bloqueado.";
            sv_sfx(SND_semSaldo);
        }
        else
        {
            if (char_editing == 1) sv_meta.character2 = char_cursor;
            else sv_meta.character = char_cursor;
            sv_meta_save(sv_meta_path()); warn_if_save_failed();
            char_message = "Personagem escolhido.";
            sv_sfx(SND_confirma);
        }
        char_message_timer = 2.0f;
    }

    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE))
    {
        // Quem sai sem escolher volta ao personagem salvo
        if (char_cursor != saved_character()) load_preview(saved_character());
        char_editing = 0;
        screen = SCREEN_HUB;
        sv_sfx(SND_confirma);
    }

    if (char_message_timer > 0.0f) char_message_timer -= GetFrameTime();

    begin_game_drawing();
    draw_backdrop();

    if (duo)
    {
        // Dupla: de quem é a vez de escolher, na cor do marcador da partida
        static const Color colors[2] = {{255, 90, 100, 255}, {90, 170, 255, 255}};

        sv_text_centered(char_editing == 0 ? "PERSONAGEM DO JOGADOR 1" : "PERSONAGEM DO JOGADOR 2", w / 2.0f, 40.0f * ui, 64.0f * ui, colors[char_editing]);
        sv_text_centered("TAB troca de jogador", w / 2.0f, 112.0f * ui, 24.0f * ui, (Color){190, 180, 195, 255});
    }
    else sv_text_centered("PERSONAGEM", w / 2.0f, 40.0f * ui, 80.0f * ui, (Color){230, 40, 50, 255});

    // Retrato grande. Bloqueado aparece como silhueta.
    DrawRectangleRounded((Rectangle){w / 2.0f - 170.0f * ui, 150.0f * ui, 340.0f * ui, 300.0f * ui}, 0.08f, 8, (Color){18, 8, 22, 220});
    sv_use_player_slot(char_editing);
    sv_draw_player(SOUTH, unlocked ? frame : 0, (Vector2){w / 2.0f, 300.0f * ui}, 8.0f * ui, unlocked ? WHITE : (Color){12, 8, 18, 255});
    sv_use_player_slot(0);

    sv_text_centered(unlocked ? c->name : "???", w / 2.0f, 465.0f * ui, 64.0f * ui, WHITE);
    sv_text_centered(c->title, w / 2.0f, 535.0f * ui, 30.0f * ui, (Color){180, 170, 185, 255});

    sv_text_centered(c->ability_name, w / 2.0f, 600.0f * ui, 38.0f * ui, (Color){255, 214, 80, 255});
    sv_text_centered(c->ability_text, w / 2.0f, 650.0f * ui, 26.0f * ui, (Color){225, 215, 225, 255});

    if (unlocked) sv_text_centered(char_cursor == saved_character() ? "EM USO" : "ENTER para escolher", w / 2.0f, 715.0f * ui, 30.0f * ui, char_cursor == saved_character() ? (Color){120, 230, 140, 255} : (Color){190, 180, 195, 255});
    else
    {
        char text[160];

        snprintf(text, sizeof(text), "BLOQUEADO: %s", c->unlock_text);
        sv_text_centered(text, w / 2.0f, 715.0f * ui, 30.0f * ui, (Color){240, 90, 90, 255});
    }

    for (int i = 0; i < CH_COUNT; i++)
    {
        Rectangle cell = roster_cell(i);
        bool open = character_unlocked(i);
        bool selected = i == char_cursor;

        DrawRectangleRec(cell, selected ? (Color){70, 14, 26, 230} : (Color){18, 8, 22, 200});
        if (selected) DrawRectangleLinesEx(cell, 3.0f * ui, (Color){240, 50, 60, 255});
        if (duo)
        {
            // Dupla: contorno vermelho no personagem do jogador 1 e azul no do jogador 2
            if (i == sv_meta.character) DrawRectangleLinesEx(cell, 3.0f * ui, (Color){255, 90, 100, 255});
            if (i == sv_meta.character2) DrawRectangleLinesEx((Rectangle){cell.x + 5.0f * ui, cell.y + 5.0f * ui, cell.width - 10.0f * ui, cell.height - 10.0f * ui}, 3.0f * ui, (Color){90, 170, 255, 255});
        }
        else if (i == sv_meta.character) DrawRectangleLinesEx(cell, 3.0f * ui, (Color){120, 230, 140, 255});

        if (roster_icons[i].id)
        {
            float size = 96.0f * ui;

            DrawTexturePro(roster_icons[i], (Rectangle){0, 0, (float)roster_icons[i].width, (float)roster_icons[i].height}, (Rectangle){cell.x + (cell.width - size) / 2.0f, cell.y + 20.0f * ui, size, size}, (Vector2){0, 0}, 0.0f, open ? WHITE : (Color){12, 8, 18, 255});
        }
        sv_text_centered(open ? sv_character(i)->name : "Bloqueado", cell.x + cell.width / 2.0f, cell.y + 145.0f * ui, 28.0f * ui, open ? WHITE : (Color){120, 100, 125, 255});
    }

    sv_text_centered(duo ? "Setas ou A/D trocam. Enter escolhe. TAB troca de jogador. ESC volta." : "Setas ou A/D trocam. Enter escolhe. ESC volta.", w / 2.0f, GetScreenHeight() - 60.0f * ui, 26.0f * ui, (Color){190, 180, 195, 255});
    if (char_message_timer > 0.0f) sv_text_centered(char_message, w / 2.0f, GetScreenHeight() - 105.0f * ui, 30.0f * ui, (Color){255, 214, 80, 255});

    end_game_drawing();
}

static void render_shop(void)
{
    float ui = sv_ui();
    float w = (float)GetScreenWidth();
    char text[128];
    bool clicked;
    int hovered;
    float first_y = 190.0f, spacing = 74.0f;

    if (pressed_down()) { shop_selected = (shop_selected + 1) % UPG_COUNT; sv_sfx(SND_escolha); }
    if (pressed_up()) { shop_selected = (shop_selected + UPG_COUNT - 1) % UPG_COUNT; sv_sfx(SND_escolha); }

    hovered = mouse_pick(UPG_COUNT, first_y, spacing, 1300.0f, &clicked);
    if (hovered >= 0 && hovered != shop_selected) { shop_selected = hovered; sv_sfx(SND_escolha); }

    if (pressed_confirm() || clicked)
    {
        int cost = sv_upgrade_cost(shop_selected, sv_meta.upgrades[shop_selected]);

        if (cost < 0)
        {
            shop_message = "Já está no máximo.";
            sv_sfx(SND_semSaldo);
        }
        else if (sv_meta_buy(shop_selected))
        {
            shop_message = "Comprado.";
            sv_sfx(SND_compra);
            sv_meta_save(sv_meta_path()); warn_if_save_failed();
        }
        else
        {
            shop_message = "Banco de Horas insuficiente.";
            sv_sfx(SND_semSaldo);
        }
        shop_message_timer = 2.0f;
    }

    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE))
    {
        screen = SCREEN_HUB;
        sv_music_play(MUS_MENU);
        sv_sfx(SND_confirma);
    }

    if (shop_message_timer > 0.0f) shop_message_timer -= GetFrameTime();

    begin_game_drawing();
    draw_backdrop();

    sv_text_centered("PLANO DE CARREIRA", w / 2.0f, 40.0f * ui, 80.0f * ui, (Color){230, 40, 50, 255});
    snprintf(text, sizeof(text), "BANCO DE HORAS  %d", sv_meta.coins);
    sv_text_centered(text, w / 2.0f, 125.0f * ui, 40.0f * ui, (Color){255, 214, 80, 255});

    for (int i = 0; i < UPG_COUNT; i++)
    {
        Rectangle row = menu_row(i, first_y, spacing, 1300.0f);
        bool selected = i == shop_selected;
        int level = sv_meta.upgrades[i];
        int max = sv_upgrade_max(i);
        int cost = sv_upgrade_cost(i, level);

        DrawRectangleRec(row, selected ? (Color){70, 14, 26, 230} : (Color){18, 8, 22, 200});
        if (selected) DrawRectangleLinesEx(row, 3.0f * ui, (Color){240, 50, 60, 255});

        sv_text(upgrade_names[i], row.x + 20.0f * ui, row.y + 4.0f * ui, 32.0f * ui, WHITE);
        sv_text(upgrade_descriptions[i], row.x + 20.0f * ui, row.y + 34.0f * ui, 22.0f * ui, (Color){190, 180, 195, 255});

        // Pontos do nível
        for (int p = 0; p < max; p++)
        {
            Rectangle pip = {row.x + row.width - 420.0f * ui + (float)p * 18.0f * ui, row.y + 20.0f * ui, 12.0f * ui, 22.0f * ui};

            DrawRectangleRec(pip, p < level ? (Color){240, 50, 60, 255} : (Color){60, 40, 60, 255});
        }

        if (cost < 0) snprintf(text, sizeof(text), "MÁX");
        else snprintf(text, sizeof(text), "%d", cost);
        sv_text(text, row.x + row.width - 120.0f * ui, row.y + 12.0f * ui, 34.0f * ui, cost >= 0 && sv_meta.coins >= cost ? (Color){255, 214, 80, 255} : (Color){150, 120, 120, 255});
    }

    sv_text_centered("Setas e Enter para comprar. ESC volta.", w / 2.0f, GetScreenHeight() - 70.0f * ui, 26.0f * ui, (Color){190, 180, 195, 255});
    if (shop_message_timer > 0.0f) sv_text_centered(shop_message, w / 2.0f, GetScreenHeight() - 110.0f * ui, 30.0f * ui, (Color){255, 214, 80, 255});

    end_game_drawing();
}

// -----------------------------------------------------------------------------
// Partida
// -----------------------------------------------------------------------------

static void render_run(void)
{
    float dt = GetFrameTime();
    bool debug_pause = sv_debug_update();   // F2 abre o menu de cheats (só no modo debug) e a partida fica parada

    if (sv_run.phase == SV_LEVELUP)
    {
        bool clicked = false;

        if (pressed_left()) { levelup_selected = (levelup_selected + sv_run.choice_count - 1) % sv_run.choice_count; sv_sfx(SND_escolha); }
        if (pressed_right()) { levelup_selected = (levelup_selected + 1) % sv_run.choice_count; sv_sfx(SND_escolha); }

        for (int i = 0; i < sv_run.choice_count; i++)
        {
            if (IsKeyPressed(KEY_ONE + i)) { levelup_selected = i; clicked = true; }
            if (CheckCollisionPointRec(GetMousePosition(), sv_levelup_card(i)) && (GetMouseDelta().x != 0.0f || GetMouseDelta().y != 0.0f))
            {
                if (levelup_selected != i) sv_sfx(SND_escolha);
                levelup_selected = i;
            }
            if (CheckCollisionPointRec(GetMousePosition(), sv_levelup_card(i)) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                levelup_selected = i;
                clicked = true;
            }
        }

        if (pressed_confirm() || clicked)
        {
            sv_game_choose(levelup_selected);
            levelup_selected = 0;
        }
    }
    else if (!debug_pause)
    {
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P))
        {
            screen = SCREEN_PAUSE;
            pause_selected = 0;
            pause_help_open = false;
            sv_sfx(SND_confirma);
        }
        else
        {
            // Acelerar o tempo (TAB ou clique no botão) vale só até os 7:00
            if (sv_run.time >= SV_SPEEDUP_LIMIT) sv_time_scale = 1;
            else if (IsKeyPressed(KEY_TAB) || (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), sv_speed_button_rect())))
            {
                sv_time_scale = sv_time_scale % 3 + 1;
                sv_sfx(SND_escolha);
            }

            for (int step = 0; step < sv_time_scale && sv_run.phase == SV_PLAYING; step++)
            {
                if (duo) sv_game_update_duo(dt, read_movement_p1(), read_movement_p2());
                else sv_game_update(dt, read_movement());
            }
        }
    }

    sv_audio_update(dt);

    if (sv_run.phase == SV_DEAD || sv_run.phase == SV_WON)
    {
        finish_run(); warn_if_save_failed();
        screen = SCREEN_OVER;
        return;
    }

    begin_game_drawing();
    sv_render_game();
    if (sv_run.phase == SV_LEVELUP) sv_render_levelup(levelup_selected);
    sv_debug_draw_menu();
    end_game_drawing();
}

#define PAUSE_OPTIONS 3


// Desistir vale como derrota: o Banco de Horas coletado fica
static void abandon_run(void)
{
    sv_run.phase = SV_DEAD;
    finish_run(); warn_if_save_failed();
    screen = SCREEN_OVER;
    sv_music_play(MUS_GAMEOVER);
}

static void choose_pause_option(int option)
{
    sv_sfx(SND_confirma);
    if (option == 0)
    {
        screen = SCREEN_RUN;
    }
    else if (option == 1)
    {
        help_select_page(1);   // A página do Sobrevivência
        pause_help_open = true;
    }
    else
    {
        confirm_open("DESISTIR DA PARTIDA?", "Desistir conta como derrota. O Banco de Horas que você juntou até aqui fica com você.", "DESISTIR", "CONTINUAR", true);
    }
}

static void render_pause(void)
{
    float ui = sv_ui();
    float w = (float)GetScreenWidth(), h = (float)GetScreenHeight();
    static const char *labels[PAUSE_OPTIONS] = {"CONTINUAR", "COMO JOGAR", "DESISTIR"};
    bool dialog_blocks = confirm_is_open() || pause_help_open;   // Com uma janela aberta, só ela responde às teclas
    bool clicked = false;
    int hovered = -1;

    if (!dialog_blocks)
    {
        if (pressed_down()) { pause_selected = (pause_selected + 1) % PAUSE_OPTIONS; sv_sfx(SND_escolha); }
        if (pressed_up()) { pause_selected = (pause_selected + PAUSE_OPTIONS - 1) % PAUSE_OPTIONS; sv_sfx(SND_escolha); }

        hovered = mouse_pick(PAUSE_OPTIONS, h * 0.45f / ui, 90.0f, 620.0f, &clicked);
        if (hovered >= 0 && hovered != pause_selected) { pause_selected = hovered; sv_sfx(SND_escolha); }

        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) { screen = SCREEN_RUN; sv_sfx(SND_confirma); }
        else if (pressed_confirm() || clicked) choose_pause_option(pause_selected);
    }

    sv_audio_update(GetFrameTime());

    begin_game_drawing();
    sv_render_game();
    DrawRectangle(0, 0, (int)w, (int)h, (Color){4, 0, 8, 190});

    if (pause_help_open)
    {
        if (help_update_and_draw_panel()) pause_help_open = false;
    }
    else
    {
        sv_text_centered("PAUSADO", w / 2.0f, h * 0.25f, 100.0f * ui, (Color){230, 40, 50, 255});
        for (int i = 0; i < PAUSE_OPTIONS; i++)
        {
            sv_text_centered(labels[i], w / 2.0f, h * 0.45f + (float)i * 90.0f * ui, 56.0f * ui, i == pause_selected ? (Color){255, 70, 80, 255} : (Color){225, 215, 225, 255});
        }
        ui_draw_hint("SETAS: escolher    ENTER: confirmar    ESC ou P: continuar");

        if (confirm_is_open() && confirm_update_and_draw() == CONFIRM_YES) abandon_run();
    }
    end_game_drawing();
}

static void render_over(void)
{
    float ui = sv_ui();
    float w = (float)GetScreenWidth();
    bool won = sv_run.phase == SV_WON;
    char text[128];

    sv_audio_update(GetFrameTime());
    if (won && sv_music_current() != MUS_VITORIA) sv_music_play(MUS_VITORIA);

    if (pressed_confirm() || IsKeyPressed(KEY_ESCAPE))
    {
        screen = SCREEN_HUB;
        hub_selected = 0;
        sv_music_play(MUS_MENU);
        sv_ambient_play(-1);
        sv_sfx(SND_confirma);
    }

    begin_game_drawing();
    draw_backdrop();

    sv_text_centered(won ? "A IARA FOI DESLIGADA" : (duo ? "VOCÊS FORAM DEMITIDOS" : "VOCÊ FOI DEMITIDO"), w / 2.0f, 110.0f * ui, 110.0f * ui, won ? (Color){120, 230, 160, 255} : (Color){230, 40, 50, 255});

    snprintf(text, sizeof(text), "Tempo %d:%02d   Nível %d   Abates %d   Chefes %d", (int)sv_run.time / 60, (int)sv_run.time % 60, sv_run.player.level, sv_run.kills, sv_run.bosses_defeated);
    if (duo && sv_run.duo) snprintf(text, sizeof(text), "Tempo %d:%02d   Nível J1 %d · J2 %d   Abates %d   Chefes %d", (int)sv_run.time / 60, (int)sv_run.time % 60, sv_duo_player(0)->level, sv_duo_player(1)->level, sv_run.kills, sv_run.bosses_defeated);
    sv_text_centered(text, w / 2.0f, 290.0f * ui, 34.0f * ui, (Color){220, 205, 215, 255});

    snprintf(text, sizeof(text), "+%d BANCO DE HORAS", earned_coins);
    sv_text_centered(text, w / 2.0f, 380.0f * ui, 64.0f * ui, (Color){255, 214, 80, 255});

    snprintf(text, sizeof(text), "Saldo %d", sv_meta.coins);
    sv_text_centered(text, w / 2.0f, 470.0f * ui, 36.0f * ui, (Color){255, 214, 80, 255});

    if (sv_run.cheated) sv_text_centered("Cheats usados: esta partida não conta para o progresso nem para o ranking.", w / 2.0f, 530.0f * ui, 28.0f * ui, (Color){255, 140, 120, 255});
    else if (last_rank_position > 0)
    {
        if (last_run_record) snprintf(text, sizeof(text), "NOVO RECORDE! %s agora está em %dº de %d no ranking!", player_name(), last_rank_position, sv_ranking_count());
        else snprintf(text, sizeof(text), "%s, seu recorde continua em %dº de %d no ranking.", player_name(), last_rank_position, sv_ranking_count());
        sv_text_centered(text, w / 2.0f, 530.0f * ui, 40.0f * ui, last_rank_position <= 5 ? (Color){255, 214, 80, 255} : (Color){230, 220, 230, 255});
    }

    draw_ranking(w / 2.0f - 420.0f * ui, 600.0f * ui, 840.0f * ui, 5, 34.0f * ui);

    sv_text_centered("Enter para voltar", w / 2.0f, 1000.0f * ui, 34.0f * ui, (Color){190, 180, 195, 255});

    end_game_drawing();
}

bool render_survival(void)
{
    // F1 liga e desliga o modo debug em qualquer tela do modo
    if (IsKeyPressed(KEY_F1)) sv_debug_toggle();

    switch (screen)
    {
    case SCREEN_HUB:
        sv_audio_update(GetFrameTime());
        render_hub();
        break;
    case SCREEN_SHOP:
        sv_audio_update(GetFrameTime());
        render_shop();
        break;
    case SCREEN_CHARS:
        sv_audio_update(GetFrameTime());
        render_characters();
        break;
    case SCREEN_RUN:
        render_run();
        break;
    case SCREEN_PAUSE:
        render_pause();
        break;
    case SCREEN_OVER:
        render_over();
        break;
    }

    return leave_to_menu || leave_to_saves;
}

bool survival_wants_player_switch(void)
{
    return leave_to_saves;
}
