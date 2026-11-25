/*
 * Testes das heurísticas de usabilidade (Nielsen): aviso de status, confirmação de ações
 * difíceis de desfazer, ajuda e pausa. Cada caso tem o código UX-nn.
 *
 * Compilar e rodar: tools/tests/run_tests.ps1
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "raylib.h"
#include "janela_de_teste.h"

#include "accessibility.h"
#include "confirm_dialog.h"
#include "fonts.h"
#include "help.h"
#include "notice.h"
#include "pause_menu.h"

/* A main.c define estas globais; aqui o teste assume esse papel. */
unsigned int screen_width = 0;
unsigned int screen_height = 0;
unsigned int middle_screen_x = 0;
unsigned int middle_screen_y = 0;

static int total_casos = 0;
static int casos_ok = 0;

static void checar(const char *id, const char *descricao, bool condicao)
{
    total_casos++;
    if (condicao) casos_ok++;
    printf("  [%s]  %-6s %s\n", condicao ? " OK " : "FALHA", id, descricao);
}

static void titulo(const char *texto)
{
    printf("\n  --- %s ---\n", texto);
}

static void testar_aviso(void)
{
    titulo("Visibilidade do status: avisos (UX-01 a UX-03)");

    notice_clear();
    checar("UX-01", "Sem aviso, nada aparece", !notice_is_active() && notice_text_now()[0] == '\0');

    notice_show("Progresso salvo.", NOTICE_SUCCESS);
    checar("UX-02", "O aviso aparece com o texto dado", notice_is_active() && strcmp(notice_text_now(), "Progresso salvo.") == 0);

    begin_game_drawing();
    ClearBackground(BLACK);
    end_game_drawing();   /* O aviso e desenhado por end_game_drawing: nao pode quebrar o quadro */
    notice_show("Nao consegui salvar o progresso.", NOTICE_ERROR);
    bool erro_ativo = notice_is_active();
    notice_clear();
    checar("UX-03", "O aviso de erro aparece e some ao ser limpo", erro_ativo && !notice_is_active());
}

static void testar_confirmacao(void)
{
    titulo("Prevencao de erros: confirmacao (UX-04 a UX-06)");

    confirm_close();
    checar("UX-04", "A confirmacao comeca fechada", !confirm_is_open());

    confirm_open("SAIR DO JOGO?", "O jogo fecha. O seu progresso fica salvo.", "SAIR", "CONTINUAR", true);
    checar("UX-05", "A confirmacao abre", confirm_is_open());

    begin_game_drawing();
    ClearBackground(BLACK);
    confirm_result primeira = confirm_update_and_draw();
    confirm_result segunda = confirm_update_and_draw();
    end_game_drawing();

    /* Sem tecla nem clique, a pergunta continua aberta: nada acontece sozinho */
    checar("UX-06", "Sem resposta do jogador, a confirmacao segue aberta", primeira == CONFIRM_PENDING && segunda == CONFIRM_PENDING && confirm_is_open());
    confirm_close();
}

static void testar_ajuda(void)
{
    titulo("Ajuda e documentacao (UX-07 e UX-08)");

    checar("UX-07", "A ajuda tem pelo menos uma pagina", help_page_count() >= 1);

    initialize_help_screen();
    bool fechou_sozinha = render_help_screen();
    checar("UX-08", "A tela de ajuda desenha e fica aberta ate o jogador voltar", !fechou_sozinha);
}

static void testar_pausa(void)
{
    titulo("Controle e liberdade: pausa (UX-09 a UX-11)");

    checar("UX-09", "O jogo comeca sem pausa", !pause_menu_is_open());

    begin_game_drawing();
    ClearBackground(DARKPURPLE);
    pause_menu_open();
    end_game_drawing();
    checar("UX-10", "A pausa abre guardando a imagem do jogo", pause_menu_is_open());

    begin_game_drawing();
    pause_action acao = pause_menu_update_and_draw();
    end_game_drawing();
    checar("UX-11", "Sem tecla, a pausa continua parada no mesmo lugar", acao == PAUSE_NONE && pause_menu_is_open());

    pause_menu_close();
    checar("UX-12", "Fechar a pausa devolve o jogo", !pause_menu_is_open());
}

int main(void)
{
    SetTraceLogLevel(LOG_WARNING);

    abrir_janela_de_teste(1280, 720, "IARA - testes de usabilidade");

    screen_width = GetScreenWidth();
    screen_height = GetScreenHeight();
    middle_screen_x = screen_width / 2;
    middle_screen_y = screen_height / 2;

    printf("\n========================================================================\n");
    printf(" IARA - TESTES DE USABILIDADE (heuristicas de Nielsen)\n");
    printf("========================================================================\n");

    initialize_fonts();

    testar_aviso();
    testar_confirmacao();
    testar_ajuda();
    testar_pausa();

    printf("\n------------------------------------------------------------------------\n");
    printf(" Usabilidade: %d de %d casos aprovados", casos_ok, total_casos);
    if (casos_ok != total_casos) printf(" | %d FALHA(S)", total_casos - casos_ok);
    printf("\n========================================================================\n\n");

    unload_fonts();
    CloseWindow();

    return (casos_ok == total_casos) ? 0 : 1;
}
