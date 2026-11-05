#include <stdio.h>
#include <string.h>
#include "raylib.h"
#include "accessibility.h"
#include "survival.h"
#include "sv_game.h"
#include "sv_render.h"
#include "sv_meta.h"
#include "sv_audio.h"
#include "sv_assets.h"

typedef enum { SCREEN_HUB, SCREEN_SHOP, SCREEN_RUN, SCREEN_PAUSE, SCREEN_OVER } SvScreen;

static SvScreen screen = SCREEN_HUB;
static bool loaded = false;
static bool leave_to_menu = false;
static int hub_selected = 0;
static int shop_selected = 0;
static int levelup_selected = 0;
static int pause_selected = 0;
static int earned_coins = 0;
static float shop_message_timer = 0.0f;
static const char *shop_message = "";

static const char *upgrade_names[UPG_COUNT] = {
    "Plano de Saúde Premium", "Bônus de Desempenho", "Sapato Confortável", "Atalhos de Teclado", "Monitor Ultrawide",
    "Network Ativo", "Mentoria", "Crachá Reforçado", "Ctrl+Z"};
static const char *upgrade_descriptions[UPG_COUNT] = {
    "+10 de vida máxima por nível", "+5% de dano por nível", "+4% de velocidade por nível", "-4% de recarga por nível", "+5% de área por nível",
    "+15% de alcance de coleta por nível", "+5% de XP por nível", "-1 de dano em cada golpe por nível", "Ao morrer, volta uma vez com 50% da vida"};

#define HUB_OPTIONS 3
static const char *hub_labels[HUB_OPTIONS] = {"JOGAR", "PLANO DE CARREIRA", "VOLTAR"};

void initialize_survival(void)
{
    sv_meta_load(SV_SAVE_PATH);
    sv_load_sprites();
    sv_load_player();
    sv_audio_init();
    loaded = true;
    leave_to_menu = false;
    screen = SCREEN_HUB;
    hub_selected = 0;
    shop_selected = 0;
    sv_music_play(MUS_MENU);
    sv_ambient_play(-1);
}

void unload_survival(void)
{
    if (!loaded) return;

    // Quem sai no meio da partida leva o que juntou até aqui
    if (screen == SCREEN_RUN || screen == SCREEN_PAUSE) sv_game_finish();

    sv_audio_close();
    sv_unload_player();
    sv_unload_sprites();
    SetMouseCursor(MOUSE_CURSOR_ARROW);
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

static void start_run(void)
{
    sv_game_start((unsigned)(GetTime() * 1000.0) | 1u);
    screen = SCREEN_RUN;
    levelup_selected = 0;
    pause_selected = 0;
}

static void draw_backdrop(void)
{
    float ui = sv_ui();

    ClearBackground((Color){8, 4, 12, 255});
    sv_draw(SPR_rm_fundo, 0, (Vector2){GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f}, GetScreenHeight() / 438.0f * 1.05f, 0.0f, false, (Color){70, 40, 60, 255});
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){6, 2, 10, 150});
    (void)ui;
}

static void render_hub(void)
{
    float ui = sv_ui();
    float w = (float)GetScreenWidth();
    char text[128];
    bool clicked;
    int hovered;

    if (pressed_down()) { hub_selected = (hub_selected + 1) % HUB_OPTIONS; sv_sfx(SND_escolha); }
    if (pressed_up()) { hub_selected = (hub_selected + HUB_OPTIONS - 1) % HUB_OPTIONS; sv_sfx(SND_escolha); }

    hovered = mouse_pick(HUB_OPTIONS, 560.0f, 90.0f, 620.0f, &clicked);
    if (hovered >= 0 && hovered != hub_selected) { hub_selected = hovered; sv_sfx(SND_escolha); }

    if (pressed_confirm() || clicked)
    {
        sv_sfx(SND_confirma);
        if (hub_selected == 0) start_run();
        else if (hub_selected == 1) { screen = SCREEN_SHOP; shop_selected = 0; sv_music_play(MUS_LOJA); }
        else leave_to_menu = true;
    }

    if (IsKeyPressed(KEY_ESCAPE)) leave_to_menu = true;

    begin_game_drawing();
    draw_backdrop();

    sv_text_centered("SOBREVIVÊNCIA", w / 2.0f, 90.0f * ui, 120.0f * ui, (Color){230, 40, 50, 255});
    sv_text_centered("Dez minutos de expediente. Sobreviva às cinco áreas e desligue a IARA.", w / 2.0f, 235.0f * ui, 30.0f * ui, (Color){220, 205, 215, 255});

    snprintf(text, sizeof(text), "BANCO DE HORAS  %d", sv_meta.coins);
    sv_text_centered(text, w / 2.0f, 310.0f * ui, 44.0f * ui, (Color){255, 214, 80, 255});
    snprintf(text, sizeof(text), "Partidas %d   Vitórias %d   Melhor tempo %d:%02d   Abates %d", sv_meta.runs, sv_meta.wins, sv_meta.best_seconds / 60, sv_meta.best_seconds % 60, sv_meta.kills);
    sv_text_centered(text, w / 2.0f, 370.0f * ui, 26.0f * ui, (Color){180, 170, 185, 255});

    sv_text_centered("Mover: WASD ou setas. Os poderes atacam sozinhos. ESC pausa.", w / 2.0f, 440.0f * ui, 26.0f * ui, (Color){180, 170, 185, 255});

    for (int i = 0; i < HUB_OPTIONS; i++)
    {
        Rectangle row = menu_row(i, 560.0f, 90.0f, 620.0f);
        bool selected = i == hub_selected;

        if (selected) DrawRectangleRec(row, (Color){70, 14, 26, 220});
        sv_text_centered(hub_labels[i], w / 2.0f, row.y + 8.0f * ui, 52.0f * ui, selected ? (Color){255, 70, 80, 255} : (Color){225, 215, 225, 255});
    }

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
            sv_meta_save(SV_SAVE_PATH);
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
    else
    {
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P))
        {
            screen = SCREEN_PAUSE;
            pause_selected = 0;
            sv_sfx(SND_confirma);
        }
        else
        {
            sv_game_update(dt, read_movement());
        }
    }

    sv_audio_update(dt);

    if (sv_run.phase == SV_DEAD || sv_run.phase == SV_WON)
    {
        earned_coins = sv_game_finish();
        screen = SCREEN_OVER;
        return;
    }

    begin_game_drawing();
    sv_render_game();
    if (sv_run.phase == SV_LEVELUP) sv_render_levelup(levelup_selected);
    end_game_drawing();
}

static void render_pause(void)
{
    float ui = sv_ui();
    float w = (float)GetScreenWidth(), h = (float)GetScreenHeight();
    static const char *labels[2] = {"CONTINUAR", "DESISTIR"};

    if (pressed_down() || pressed_up()) { pause_selected = 1 - pause_selected; sv_sfx(SND_escolha); }
    if (IsKeyPressed(KEY_ESCAPE)) { screen = SCREEN_RUN; sv_sfx(SND_confirma); }

    if (pressed_confirm())
    {
        sv_sfx(SND_confirma);
        if (pause_selected == 0) screen = SCREEN_RUN;
        else
        {
            // Desistir vale como derrota: o Banco de Horas coletado fica
            sv_run.phase = SV_DEAD;
            earned_coins = sv_game_finish();
            screen = SCREEN_OVER;
            sv_music_play(MUS_GAMEOVER);
        }
    }

    sv_audio_update(GetFrameTime());

    begin_game_drawing();
    sv_render_game();
    DrawRectangle(0, 0, (int)w, (int)h, (Color){4, 0, 8, 190});
    sv_text_centered("PAUSADO", w / 2.0f, h * 0.25f, 100.0f * ui, (Color){230, 40, 50, 255});
    for (int i = 0; i < 2; i++)
    {
        sv_text_centered(labels[i], w / 2.0f, h * 0.45f + (float)i * 90.0f * ui, 56.0f * ui, i == pause_selected ? (Color){255, 70, 80, 255} : (Color){225, 215, 225, 255});
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

    sv_text_centered(won ? "A IARA FOI DESLIGADA" : "VOCÊ FOI DEMITIDO", w / 2.0f, 110.0f * ui, 110.0f * ui, won ? (Color){120, 230, 160, 255} : (Color){230, 40, 50, 255});

    snprintf(text, sizeof(text), "Tempo %d:%02d   Nível %d   Abates %d   Chefes %d", (int)sv_run.time / 60, (int)sv_run.time % 60, sv_run.player.level, sv_run.kills, sv_run.bosses_defeated);
    sv_text_centered(text, w / 2.0f, 290.0f * ui, 34.0f * ui, (Color){220, 205, 215, 255});

    snprintf(text, sizeof(text), "+%d BANCO DE HORAS", earned_coins);
    sv_text_centered(text, w / 2.0f, 380.0f * ui, 64.0f * ui, (Color){255, 214, 80, 255});

    snprintf(text, sizeof(text), "Saldo %d", sv_meta.coins);
    sv_text_centered(text, w / 2.0f, 470.0f * ui, 36.0f * ui, (Color){255, 214, 80, 255});

    sv_text_centered("Enter para voltar", w / 2.0f, 600.0f * ui, 34.0f * ui, (Color){190, 180, 195, 255});

    end_game_drawing();
}

bool render_survival(void)
{
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

    return leave_to_menu;
}
