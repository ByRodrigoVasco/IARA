// IARA, versão para terminal (PIF, Unidade 2).
//
// O jogo roda inteiro no terminal: não abre janela nenhuma. Ele aproveita o conteúdo do
// jogo gráfico sem copiá-lo: o roteiro dos dias vem de code/src/screens/computer_days.h,
// os tipos do roteiro de code/src/screens/roteiro.h e o avaliador de lógica de
// code/src/game/logic.c. Os mesmos dias, as mesmas fórmulas, as mesmas respostas certas.
//
// Como compilar e jogar está em code/terminal/LEIA-ME.md.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>  // Só para pôr o terminal em UTF-8: os símbolos de lógica dependem disso
#endif

#include "entrada.h"
#include "evidencias.h"
#include "logic.h"
#include "mesa.h"
#include "roteiro.h"
#include "texto.h"

// O roteiro dos dias. Precisa vir depois de roteiro.h, que define os tipos que ele usa.
#include "computer_days.h"

// Largura da moldura e do texto dentro dela, em caracteres.
#define LARGURA_DA_MOLDURA 72
#define LARGURA_DO_TEXTO (LARGURA_DA_MOLDURA - 4)

// Um dia jogável da campanha.
typedef struct {
    int numero;                // Dia no calendário da CISSA
    const char *titulo;        // Nome do dia
    const char *conteudo;      // O que o dia cobra de lógica
    DialogStep *passos;        // Roteiro, vindo de computer_days.h
    int quantidade_de_passos;
} Missao;

// Como um dia terminou.
typedef enum {
    DIA_CONCLUIDO,      // O jogador fechou todas as mesas
    DIA_ABANDONADO,     // O jogador desistiu de uma mesa
    DIA_ENTRADA_ACABOU  // A entrada do teclado acabou (Ctrl+Z)
} FimDeDia;

// Placar e evidências desta sessão de jogo.
typedef struct {
    int dias_concluidos;
    int mesas_fechadas;
    int mesas_de_primeira;   // Fechadas sem nenhum envio errado
    int envios_errados;
    unsigned int evidencias; // Evidências reunidas, para a prova do Dia 100
} Sessao;

#define MISSAO(numero, titulo, conteudo, roteiro) \
    {(numero), (titulo), (conteudo), (roteiro), (int)(sizeof(roteiro) / sizeof((roteiro)[0]))}

// Os dias jogáveis, em ordem. Nenhum enunciado está escrito aqui: tudo vem do roteiro.
static const Missao missoes[] = {
    MISSAO(3, "A mesa do Tiago", "conectivos e tabela-verdade", day_3_script),
    MISSAO(12, "O fechamento do mês", "De Morgan, dupla negação e tautologia", day_12_script),
    MISSAO(27, "Cavaleiros e escudeiros", "consistência de um conjunto de afirmações", day_27_script),
    MISSAO(41, "O treino da mini-IA", "quantificadores e viés de dados", day_41_script),
    MISSAO(58, "Os crachás", "implicação e contrapositiva", day_58_script),
    MISSAO(74, "A auditoria", "atribuição de valores que satisfaz as regras", day_74_script),
    MISSAO(89, "A lista do RH", "silogismo categórico", day_89_script),
    MISSAO(100, "A prova", "refutação por contradição", day_100_script),
};

#define QUANTIDADE_DE_MISSOES ((int)(sizeof(missoes) / sizeof(missoes[0])))

// Opções do menu principal.
typedef enum {
    MENU_SAIR = 0,
    MENU_EXPEDIENTE = 1,
    MENU_ESCOLHER_DIA = 2,
    MENU_LISTA_DOS_DIAS = 3,
    MENU_COMO_SE_JOGA = 4,
    MENU_PLACAR = 5
} OpcaoDoMenu;

// --------------------------------------------------------------------------------------
// Protótipos de todas as funções, antes da main().
// --------------------------------------------------------------------------------------

static void preparar_terminal(void);
static void imprimir_borda(const char *esquerda, const char *meio, const char *direita);
static void imprimir_linha_na_moldura(const char *texto);
static void imprimir_na_moldura(const char *texto);
static void imprimir_titulo_na_moldura(const char *titulo);
static void abrir_moldura(const char *titulo);
static void fechar_moldura(void);
static void imprimir_falas(const DialogStep *passo);

static unsigned int evidencia_do_dia(int numero);
static const char *nome_da_evidencia(unsigned int evidencia);
static void anotar_evidencia_do_dia(Sessao *sessao, int numero);

static void mostrar_menu_principal(void);
static void mostrar_lista_dos_dias(void);
static void mostrar_como_se_joga(void);
static void mostrar_placar(const Sessao *sessao);
static void mostrar_legenda_das_variaveis(const DialogStep *passo);
static void mostrar_escolhas_da_mesa(const DialogStep *passo, const int *escolhas, const Sessao *sessao);
static void mostrar_palpite_da_iara(const DialogStep *passo);
static void mostrar_resultado_do_treino(const DialogStep *passo, const int *escolhas);

static FimDeDia executar_fala(const DialogStep *passo);
static FimDeDia executar_mesa(const DialogStep *passo, Sessao *sessao);
static FimDeDia executar_escolha(const DialogStep *passo, Sessao *sessao);
static FimDeDia executar_chamado(const DialogStep *passo, Sessao *sessao);
static FimDeDia executar_relatorio(const DialogStep *passo, const Missao *missao);
static FimDeDia jogar_dia(const Missao *missao, Sessao *sessao);
static FimDeDia jogar_expediente(Sessao *sessao);
static FimDeDia escolher_e_jogar_um_dia(Sessao *sessao);

int main(void);

// --------------------------------------------------------------------------------------
// Moldura: todo texto do jogo sai dentro dela.
// --------------------------------------------------------------------------------------

static void preparar_terminal(void)
{
#ifdef _WIN32
    // Sem isto o terminal do Windows mostra os símbolos de lógica como lixo.
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

static void imprimir_borda(const char *esquerda, const char *meio, const char *direita)
{
    char linha[TEXTO_MAX];

    texto_repetir(linha, sizeof(linha), meio, LARGURA_DA_MOLDURA - 2);
    printf("%s%s%s\n", esquerda, linha, direita);
}

// Imprime uma linha já do tamanho certo, completando com espaços até a borda direita.
static void imprimir_linha_na_moldura(const char *texto)
{
    const char *conteudo = (texto != NULL) ? texto : "";
    size_t largura = texto_largura(conteudo);
    char espacos[TEXTO_MAX];
    size_t falta = (largura < LARGURA_DO_TEXTO) ? LARGURA_DO_TEXTO - largura : 0;

    texto_repetir(espacos, sizeof(espacos), " ", falta);
    printf("│ %s%s │\n", conteudo, espacos);
}

// Imprime um texto de qualquer tamanho, quebrando por palavra para caber na moldura.
static void imprimir_na_moldura(const char *texto)
{
    if (texto == NULL || texto[0] == '\0')
    {
        imprimir_linha_na_moldura("");
        return;
    }

    const char *inicio = texto;

    while (*inicio != '\0')
    {
        const char *fim = inicio;
        const char *ultimo_espaco = NULL;
        size_t caracteres = 0;

        while (*fim != '\0' && caracteres < LARGURA_DO_TEXTO)
        {
            if (*fim == ' ') ultimo_espaco = fim;
            fim += texto_bytes_do_caractere(fim);
            caracteres++;
        }

        // Se o texto continua, quebra no último espaço para não partir a palavra.
        if (*fim != '\0' && ultimo_espaco != NULL) fim = ultimo_espaco;

        char pedaco[TEXTO_MAX];
        size_t bytes = (size_t)(fim - inicio);

        if (bytes >= sizeof(pedaco)) bytes = sizeof(pedaco) - 1;
        memcpy(pedaco, inicio, bytes);
        pedaco[bytes] = '\0';
        imprimir_linha_na_moldura(pedaco);

        inicio = fim;
        while (*inicio == ' ') inicio++;
    }
}

static void imprimir_titulo_na_moldura(const char *titulo)
{
    imprimir_na_moldura(titulo);
    imprimir_borda("├", "─", "┤");
}

static void abrir_moldura(const char *titulo)
{
    printf("\n");
    imprimir_borda("┌", "─", "┐");

    if (titulo != NULL && titulo[0] != '\0') imprimir_titulo_na_moldura(titulo);
}

static void fechar_moldura(void)
{
    imprimir_borda("└", "─", "┘");
}

// As falas de um passo, uma linha do roteiro por vez.
static void imprimir_falas(const DialogStep *passo)
{
    for (int linha = 0; linha < passo->line_count && linha < MAX_LINES; linha++)
    {
        imprimir_na_moldura(passo->lines[linha]);
    }
}

// --------------------------------------------------------------------------------------
// Evidências: o Dia 100 só fecha a prova com o que o jogador reuniu antes.
// --------------------------------------------------------------------------------------

static unsigned int evidencia_do_dia(int numero)
{
    switch (numero)
    {
    case 12: return EVIDENCE_NO_RESIGNATION_LETTER;
    case 58: return EVIDENCE_BADGE_AFTER_LEAVING;
    case 74: return EVIDENCE_NO_HUMAN_ORDER;
    default: return 0u;
    }
}

static const char *nome_da_evidencia(unsigned int evidencia)
{
    switch (evidencia)
    {
    case EVIDENCE_NO_RESIGNATION_LETTER: return "a rescisão sem carta de demissão";
    case EVIDENCE_BADGE_AFTER_LEAVING: return "o crachá usado depois da saída";
    case EVIDENCE_NO_HUMAN_ORDER: return "o bilhete da auditoria, sem ordem humana";
    default: return "";
    }
}

static void anotar_evidencia_do_dia(Sessao *sessao, int numero)
{
    unsigned int evidencia = evidencia_do_dia(numero);

    if (evidencia == 0u || (sessao->evidencias & evidencia) != 0u) return;

    sessao->evidencias |= evidencia;
    abrir_moldura("Você guardou uma evidência");
    imprimir_na_moldura(nome_da_evidencia(evidencia));
    imprimir_na_moldura("Ela vai servir na prova do Dia 100.");
    fechar_moldura();
}

// --------------------------------------------------------------------------------------
// Telas do menu.
// --------------------------------------------------------------------------------------

static void mostrar_menu_principal(void)
{
    abrir_moldura("IARA · CISSA Tecnologia · Suporte Nível 1");
    imprimir_na_moldura("Versão para terminal");
    imprimir_linha_na_moldura("");
    imprimir_na_moldura("  1 · Começar o expediente (os 8 dias em ordem)");
    imprimir_na_moldura("  2 · Escolher um dia");
    imprimir_na_moldura("  3 · Lista dos dias");
    imprimir_na_moldura("  4 · Como se joga");
    imprimir_na_moldura("  5 · Placar da sessão");
    imprimir_na_moldura("  0 · Sair");
    fechar_moldura();
}

static void mostrar_lista_dos_dias(void)
{
    abrir_moldura("Os dias e o que cada um cobra");

    for (int i = 0; i < QUANTIDADE_DE_MISSOES; i++)
    {
        char linha[TEXTO_MAX];

        snprintf(linha, sizeof(linha), "Dia %3d · %s — %s",
                 missoes[i].numero, missoes[i].titulo, missoes[i].conteudo);
        imprimir_na_moldura(linha);
    }

    imprimir_linha_na_moldura("");
    imprimir_na_moldura("Os dias 12, 58 e 74 deixam uma evidência para a prova do Dia 100.");
    fechar_moldura();
}

static void mostrar_como_se_joga(void)
{
    abrir_moldura("Como se joga");
    imprimir_na_moldura("Você é o estagiário do suporte da CISSA. A cada dia chega uma mesa "
                        "de triagem: uma lista de linhas e uma norma escrita em lógica.");
    imprimir_linha_na_moldura("");
    imprimir_na_moldura("Decida cada linha pela norma e envie a mesa. O jogo diz quantas "
                        "linhas estão erradas, nunca quais: conferir é com você.");
    imprimir_linha_na_moldura("");
    imprimir_na_moldura("Os símbolos são os da disciplina:");
    imprimir_na_moldura("  ¬ não      ∧ e        ∨ ou");
    imprimir_na_moldura("  → se…então          ↔ se e somente se");
    imprimir_linha_na_moldura("");
    imprimir_na_moldura("A IARA, a inteligência artificial da empresa, dá um palpite quando "
                        "você pede. Ela acerta quase sempre. Quase.");
    fechar_moldura();
}

static void mostrar_placar(const Sessao *sessao)
{
    char linha[TEXTO_MAX];

    abrir_moldura("Placar da sessão");

    snprintf(linha, sizeof(linha), "Dias concluídos:        %d de %d",
             sessao->dias_concluidos, QUANTIDADE_DE_MISSOES);
    imprimir_na_moldura(linha);

    snprintf(linha, sizeof(linha), "Mesas fechadas:         %d", sessao->mesas_fechadas);
    imprimir_na_moldura(linha);

    snprintf(linha, sizeof(linha), "Fechadas de primeira:   %d", sessao->mesas_de_primeira);
    imprimir_na_moldura(linha);

    snprintf(linha, sizeof(linha), "Envios errados:         %d", sessao->envios_errados);
    imprimir_na_moldura(linha);

    imprimir_linha_na_moldura("");
    imprimir_na_moldura("Evidências reunidas:");

    unsigned int todas[] = {EVIDENCE_NO_RESIGNATION_LETTER, EVIDENCE_BADGE_AFTER_LEAVING,
                            EVIDENCE_NO_HUMAN_ORDER};
    bool nenhuma = true;

    for (size_t i = 0; i < sizeof(todas) / sizeof(todas[0]); i++)
    {
        if ((sessao->evidencias & todas[i]) == 0u) continue;

        snprintf(linha, sizeof(linha), "  · %s", nome_da_evidencia(todas[i]));
        imprimir_na_moldura(linha);
        nenhuma = false;
    }

    if (nenhuma) imprimir_na_moldura("  nenhuma ainda");

    fechar_moldura();
}

// --------------------------------------------------------------------------------------
// A mesa de triagem.
// --------------------------------------------------------------------------------------

static void mostrar_legenda_das_variaveis(const DialogStep *passo)
{
    if (passo->board_mode != BOARD_TRUTH_TABLE || passo->variable_names == NULL) return;

    char legenda[TEXTO_MAX] = "Cada linha traz o valor das variáveis: ";

    for (int i = 0; i < passo->variable_count; i++)
    {
        char variavel[16];

        snprintf(variavel, sizeof(variavel), "%s%c", (i > 0) ? ", " : "", passo->variable_names[i]);
        texto_juntar(legenda, sizeof(legenda), variavel);
    }

    texto_juntar(legenda, sizeof(legenda), ". V é verdadeiro, F é falso.");
    imprimir_na_moldura(legenda);
}

// A mesa como ela está, com a escolha já feita em cada linha.
static void mostrar_escolhas_da_mesa(const DialogStep *passo, const int *escolhas, const Sessao *sessao)
{
    imprimir_na_moldura("A mesa como você a deixou:");

    for (int linha = 0; linha < mesa_quantidade_de_linhas(passo); linha++)
    {
        char texto_da_linha[TEXTO_MAX];
        char montada[TEXTO_MAX];
        const char *rotulo = "—";

        mesa_texto_da_linha(passo, linha, sessao->evidencias, texto_da_linha, sizeof(texto_da_linha));

        if (escolhas[linha] == 0 || escolhas[linha] == 1) rotulo = passo->choice_labels[escolhas[linha]];

        // A precisão no %s corta o que não caberia: a moldura quebra o resto por palavra.
        snprintf(montada, sizeof(montada), "  %-8s %.400s", rotulo, texto_da_linha);
        imprimir_na_moldura(montada);
    }
}

static void mostrar_palpite_da_iara(const DialogStep *passo)
{
    char linha[TEXTO_MAX];

    imprimir_borda("├", "─", "┤");
    snprintf(linha, sizeof(linha), "IARA · confiança %d%%", passo->iara_confidence);
    imprimir_na_moldura(linha);

    if (passo->iara_reason[0] != '\0') imprimir_na_moldura(passo->iara_reason);

    imprimir_na_moldura("O que ela marcaria:");

    for (int i = 0; i < mesa_quantidade_de_linhas(passo); i++)
    {
        int escolha = passo->cards[i].iara_choice;
        const char *rotulo = (escolha == 0 || escolha == 1) ? passo->choice_labels[escolha] : "—";

        snprintf(linha, sizeof(linha), "  linha %d: %s", i + 1, rotulo);
        imprimir_na_moldura(linha);
    }
}

// O Dia 41 mostra no que a mini-IA virou depois do treino que o jogador escolheu.
static void mostrar_resultado_do_treino(const DialogStep *passo, const int *escolhas)
{
    char linha[TEXTO_MAX];

    if (passo->board_mode != BOARD_TRAINING) return;

    imprimir_na_moldura("A mini-IA, treinada com o que você marcou, aprova:");

    snprintf(linha, sizeof(linha), "  no centro:    %d%%", mesa_aprovacao_da_mini_ia(passo, escolhas, false));
    imprimir_na_moldura(linha);

    snprintf(linha, sizeof(linha), "  na periferia: %d%%", mesa_aprovacao_da_mini_ia(passo, escolhas, true));
    imprimir_na_moldura(linha);
}

static FimDeDia executar_mesa(const DialogStep *passo, Sessao *sessao)
{
    int escolhas[MAX_TRIAGE_CARDS];
    int quantidade = mesa_quantidade_de_linhas(passo);
    int tentativas = 0;
    bool saiu = false;
    bool ouviu_a_iara = false;

    for (;;)
    {
        abrir_moldura(passo->sender);
        imprimir_falas(passo);
        mostrar_legenda_das_variaveis(passo);

        if (!mesa_tem_gabarito_por_linha(passo))
        {
            imprimir_na_moldura("Nesta mesa quem está certo é o conjunto todo, não cada linha.");
        }

        if (ouviu_a_iara) mostrar_palpite_da_iara(passo);

        fechar_moldura();

        if (!ouviu_a_iara && passo->iara_reason[0] != '\0')
        {
            if (entrada_confirmar("\n  Ouvir o palpite da IARA antes de decidir?"))
            {
                ouviu_a_iara = true;
                continue;
            }
        }

        printf("\n");

        for (int linha = 0; linha < quantidade; linha++)
        {
            char texto_da_linha[TEXTO_MAX];
            char pergunta[TEXTO_MAX];

            mesa_texto_da_linha(passo, linha, sessao->evidencias, texto_da_linha, sizeof(texto_da_linha));

            if (!mesa_linha_disponivel(passo, linha, sessao->evidencias))
            {
                printf("  Linha %d de %d: %s — fica fora.\n", linha + 1, quantidade, texto_da_linha);
                escolhas[linha] = 0;
                continue;
            }

            printf("  Linha %d de %d: %s\n", linha + 1, quantidade, texto_da_linha);
            printf("    1 · %s\n", passo->choice_labels[0]);
            printf("    2 · %s\n", passo->choice_labels[1]);

            snprintf(pergunta, sizeof(pergunta), "    escolha (1 ou 2): ");
            int escolhido = entrada_ler_inteiro(pergunta, 1, 2, &saiu);

            if (saiu) return DIA_ENTRADA_ACABOU;

            escolhas[linha] = escolhido - 1;
            printf("\n");
        }

        abrir_moldura(passo->sender);
        mostrar_escolhas_da_mesa(passo, escolhas, sessao);
        fechar_moldura();

        char confirmacao[TEXTO_MAX];

        snprintf(confirmacao, sizeof(confirmacao), "\n  %s?",
                 (passo->submit_label != NULL) ? passo->submit_label : "Enviar a mesa");

        if (!entrada_confirmar(confirmacao))
        {
            printf("  Nada enviado. A mesa volta do começo.\n");
            continue;
        }

        tentativas++;

        int erros = mesa_quantos_erros(passo, escolhas);
        char resultado[TEXTO_MAX];

        mesa_texto_do_resultado(passo, erros, resultado, sizeof(resultado));

        abrir_moldura("Resposta do sistema");
        imprimir_na_moldura(resultado);
        mostrar_resultado_do_treino(passo, escolhas);

        if (erros == 0)
        {
            fechar_moldura();
            sessao->mesas_fechadas++;
            if (tentativas == 1) sessao->mesas_de_primeira++;
            return DIA_CONCLUIDO;
        }

        sessao->envios_errados++;

        // A dica só aparece depois do erro, e é a que o roteiro escreveu para esta mesa.
        int indice_da_dica = tentativas - 1;

        if (indice_da_dica < MAX_HINTS && passo->hints[indice_da_dica][0] != '\0')
        {
            imprimir_borda("├", "─", "┤");
            imprimir_na_moldura(passo->hints[indice_da_dica]);
        }

        fechar_moldura();

        if (!entrada_confirmar("\n  Tentar de novo?"))
        {
            if (passo->wrong_result[0] != '\0')
            {
                abrir_moldura("Ficou assim no relatório");
                imprimir_na_moldura(passo->wrong_result);
                fechar_moldura();
            }

            return DIA_ABANDONADO;
        }
    }
}

// --------------------------------------------------------------------------------------
// Os outros passos do roteiro.
// --------------------------------------------------------------------------------------

static FimDeDia executar_fala(const DialogStep *passo)
{
    char rotulo[TEXTO_MAX];

    abrir_moldura(NULL);
    imprimir_falas(passo);
    fechar_moldura();

    texto_copiar(rotulo, sizeof(rotulo),
                 (passo->button_name != NULL && passo->button_name[0] != '\0') ? passo->button_name : "continuar");
    entrada_esperar_enter(rotulo);
    return DIA_CONCLUIDO;
}

static FimDeDia executar_escolha(const DialogStep *passo, Sessao *sessao)
{
    bool saiu = false;

    abrir_moldura(NULL);
    imprimir_falas(passo);
    imprimir_linha_na_moldura("");

    for (int i = 0; i < passo->option_count && i < MAX_OPTIONS; i++)
    {
        char linha[TEXTO_MAX];

        snprintf(linha, sizeof(linha), "  %d · %s", i + 1, passo->options[i]);
        imprimir_na_moldura(linha);
    }

    fechar_moldura();

    int escolhido = entrada_ler_inteiro("\n  O que você faz? ", 1, passo->option_count, &saiu);

    if (saiu) return DIA_ENTRADA_ACABOU;

    // Esta escolha não tem resposta certa: ela só muda o que fica registrado no dia.
    (void)sessao;
    printf("\n  Anotado: %s\n", passo->options[escolhido - 1]);
    return DIA_CONCLUIDO;
}

static FimDeDia executar_chamado(const DialogStep *passo, Sessao *sessao)
{
    bool saiu = false;
    int tentativas = 0;

    for (;;)
    {
        abrir_moldura(passo->sender);
        imprimir_falas(passo);
        imprimir_linha_na_moldura("");

        for (int i = 0; i < passo->option_count && i < MAX_OPTIONS; i++)
        {
            char linha[TEXTO_MAX];

            snprintf(linha, sizeof(linha), "  %d · %s", i + 1, passo->options[i]);
            imprimir_na_moldura(linha);
        }

        fechar_moldura();

        int escolhido = entrada_ler_inteiro("\n  Sua resposta: ", 1, passo->option_count, &saiu);

        if (saiu) return DIA_ENTRADA_ACABOU;

        tentativas++;

        if (escolhido - 1 == passo->correct_option)
        {
            abrir_moldura("Resposta do sistema");
            imprimir_na_moldura("Chamado resolvido.");
            fechar_moldura();
            sessao->mesas_fechadas++;
            if (tentativas == 1) sessao->mesas_de_primeira++;
            return DIA_CONCLUIDO;
        }

        sessao->envios_errados++;

        abrir_moldura("Resposta do sistema");
        imprimir_na_moldura("Não é essa.");

        int indice_da_dica = tentativas - 1;

        if (indice_da_dica < MAX_HINTS && passo->hints[indice_da_dica][0] != '\0')
        {
            imprimir_na_moldura(passo->hints[indice_da_dica]);
        }

        fechar_moldura();

        if (!entrada_confirmar("\n  Tentar de novo?")) return DIA_ABANDONADO;
    }
}

static FimDeDia executar_relatorio(const DialogStep *passo, const Missao *missao)
{
    char linha[TEXTO_MAX];

    snprintf(linha, sizeof(linha), "Dia %d · %s", missao->numero, missao->titulo);
    abrir_moldura(linha);
    imprimir_falas(passo);
    fechar_moldura();
    entrada_esperar_enter("concluir o dia");
    return DIA_CONCLUIDO;
}

// --------------------------------------------------------------------------------------
// Um dia, do começo ao fim.
// --------------------------------------------------------------------------------------

static FimDeDia jogar_dia(const Missao *missao, Sessao *sessao)
{
    char titulo[TEXTO_MAX];

    snprintf(titulo, sizeof(titulo), "Dia %d · %s", missao->numero, missao->titulo);
    abrir_moldura(titulo);
    imprimir_na_moldura(missao->conteudo);
    fechar_moldura();

    for (int indice = 0; indice < missao->quantidade_de_passos; indice++)
    {
        const DialogStep *passo = &missao->passos[indice];
        FimDeDia fim;

        switch (passo->type)
        {
        case STEP_STATEMENT:
            fim = executar_fala(passo);
            break;

        case STEP_TRIAGE:
            fim = executar_mesa(passo, sessao);
            break;

        case STEP_CHOICE:
            fim = executar_escolha(passo, sessao);
            break;

        case STEP_QUIZ:
            fim = executar_chamado(passo, sessao);
            break;

        case STEP_REPORT:
            fim = executar_relatorio(passo, missao);
            break;

        default:
            fim = DIA_CONCLUIDO;
            break;
        }

        if (fim != DIA_CONCLUIDO) return fim;
    }

    sessao->dias_concluidos++;
    anotar_evidencia_do_dia(sessao, missao->numero);
    return DIA_CONCLUIDO;
}

static FimDeDia jogar_expediente(Sessao *sessao)
{
    for (int i = 0; i < QUANTIDADE_DE_MISSOES; i++)
    {
        FimDeDia fim = jogar_dia(&missoes[i], sessao);

        if (fim == DIA_ENTRADA_ACABOU) return fim;

        if (fim == DIA_ABANDONADO)
        {
            abrir_moldura("Expediente interrompido");
            imprimir_na_moldura("Você deixou a mesa para depois. O expediente para aqui.");
            fechar_moldura();
            return DIA_ABANDONADO;
        }

        if (i + 1 < QUANTIDADE_DE_MISSOES && !entrada_confirmar("\n  Seguir para o próximo dia?"))
        {
            return DIA_ABANDONADO;
        }
    }

    abrir_moldura("Fim do expediente");
    imprimir_na_moldura("Os cem dias passaram.");
    fechar_moldura();
    return DIA_CONCLUIDO;
}

static FimDeDia escolher_e_jogar_um_dia(Sessao *sessao)
{
    bool saiu = false;

    abrir_moldura("Escolher um dia");

    for (int i = 0; i < QUANTIDADE_DE_MISSOES; i++)
    {
        char linha[TEXTO_MAX];

        snprintf(linha, sizeof(linha), "  %d · Dia %3d — %s", i + 1, missoes[i].numero, missoes[i].conteudo);
        imprimir_na_moldura(linha);
    }

    imprimir_na_moldura("  0 · voltar ao menu");
    fechar_moldura();

    int escolhido = entrada_ler_inteiro("\n  Qual dia? ", 0, QUANTIDADE_DE_MISSOES, &saiu);

    if (saiu) return DIA_ENTRADA_ACABOU;
    if (escolhido == 0) return DIA_ABANDONADO;

    const Missao *missao = &missoes[escolhido - 1];

    // Jogado fora de ordem, o Dia 100 não teria as evidências dos dias 12, 58 e 74.
    // Elas entram emprestadas, senão a prova não teria como fechar.
    if (missao->numero == 100 && sessao->evidencias != (EVIDENCE_NO_RESIGNATION_LETTER |
                                                        EVIDENCE_BADGE_AFTER_LEAVING |
                                                        EVIDENCE_NO_HUMAN_ORDER))
    {
        sessao->evidencias = EVIDENCE_NO_RESIGNATION_LETTER | EVIDENCE_BADGE_AFTER_LEAVING |
                             EVIDENCE_NO_HUMAN_ORDER;
        abrir_moldura("Nota");
        imprimir_na_moldura("Você está jogando o Dia 100 fora de ordem, então as evidências "
                            "dos dias 12, 58 e 74 entram já reunidas.");
        fechar_moldura();
    }

    return jogar_dia(missao, sessao);
}

// --------------------------------------------------------------------------------------

int main(void)
{
    Sessao sessao = {0};
    bool saiu = false;

    preparar_terminal();

    for (;;)
    {
        mostrar_menu_principal();

        int opcao = entrada_ler_inteiro("\n  Opção: ", MENU_SAIR, MENU_PLACAR, &saiu);

        if (saiu)
        {
            printf("\n  A entrada acabou. Até logo.\n");
            return EXIT_SUCCESS;
        }

        FimDeDia fim = DIA_CONCLUIDO;

        switch ((OpcaoDoMenu)opcao)
        {
        case MENU_EXPEDIENTE:
            fim = jogar_expediente(&sessao);
            break;

        case MENU_ESCOLHER_DIA:
            fim = escolher_e_jogar_um_dia(&sessao);
            break;

        case MENU_LISTA_DOS_DIAS:
            mostrar_lista_dos_dias();
            break;

        case MENU_COMO_SE_JOGA:
            mostrar_como_se_joga();
            break;

        case MENU_PLACAR:
            mostrar_placar(&sessao);
            break;

        case MENU_SAIR:
            mostrar_placar(&sessao);

            if (entrada_confirmar("\n  Sair do jogo?"))
            {
                printf("\n  Até o próximo expediente.\n");
                return EXIT_SUCCESS;
            }
            break;

        default:
            break;
        }

        if (fim == DIA_ENTRADA_ACABOU)
        {
            printf("\n  A entrada acabou. Até logo.\n");
            return EXIT_SUCCESS;
        }
    }
}
