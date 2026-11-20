#include <stddef.h>

#include "calendar.h"
#include "game_state.h"
#include "macros.h"

// A água aparece a partir do Ato III (GDD 2.6).
#define WATER_FIRST_DAY 46

typedef struct {
    int day;
    const char *arrival[2];
    const char *day_end;
    const char *note[CALENDAR_NOTE_LINE_COUNT]; // Vazio nos dias sem investigação
} PlayableDay;

typedef struct {
    int day;
    const char *text;
} FlashDay;

// Os 9 dias jogáveis (GDD 2.6). O GDD só define o conceito de cada um: estes textos são proposta.
static const PlayableDay playable_days[] = {
    {1,
     {"Sua mesa fica na salinha ao lado do corredor, um pouco afastada do resto.",
      "O computador já está ligado. Alguém espera perto da entrada."},
     "Amanhã tem mais.",
     {NULL}},
    {3,
     {"O Suporte está sem gente hoje. A fila veio inteira para você.",
      "A norma de triagem mudou, e ninguém avisou."},
     "No fim do turno, ainda não há notícia de Tiago.",
     {NULL}},
    {12,
     {"Fechamento do mês. O Financeiro pede sua assinatura na conciliação.",
      "A IARA já reescreveu as regras. Ficou mais curto."},
     "Sobrou uma mesa vazia perto da impressora.",
     {"Recibo esquecido no mural:",
      "\"Rescisão sem carta assinada. Pago assim mesmo.\"",
      "O nome foi riscado com caneta azul.",
      "Alguém escreveu embaixo: conte as cartas."}},
    {27,
     {"Pediram que você transcreva a ata da diretoria.",
      "A gravação tem um trecho em que só se ouve água."},
     "O ar-condicionado pinga na sua mesa. Só na sua.",
     {NULL}},
    {41,
     {"O arquivo morto vai treinar a próxima versão da IARA.",
      "Você escolhe o que ela vai aprender. Por enquanto."},
     "As telas continuam ligadas depois do expediente.",
     {"Etiqueta na caixa do arquivo morto:",
      "\"Prédio erguido em 1994 sobre igarapé canalizado.\"",
      "Por baixo, a lápis: poço de inspeção, porão.",
      "A caixa está úmida."}},
    {58,
     {"O sistema de crachás caiu de madrugada.",
      "Você reconhece os nomes da lista. Todos do seu primeiro dia."},
     "Você passa pela catraca. Ela te chama pelo nome.",
     {"Registro de catraca preso no mural:",
      "\"Diego. Saída definitiva em março.\"",
      "\"Diego. Entrada ontem, 03:12, acesso ao porão.\"",
      "Demitido não passa crachá."}},
    {74,
     {"A auditoria deixou os logs da IARA para você ler.",
      "O chão do corredor está molhado, e a chefia parou de cobrar."},
     "A chefia não veio hoje. A sala dela está alagada até o rodapé.",
     {"Bilhete dobrado, com letra apressada:",
      "\"O acesso das 03:12 não tem ninguém autorizando.\"",
      "\"Eu olhei. Não tem nome nenhum.\"",
      "\"Se você leu, não responda quando ela chamar.\""}},
    {89,
     {"A IARA quer a sala pronta para o Dia 100.",
      "A lista de convidados tem o seu nome."},
     "A água ficou parada perto da escada.",
     {"Mapa do prédio, com uma escada a lápis até O Fundo.",
      "A marca está assinada: Nicollas.",
      "A escada desce mais do que consta na planta.",
      "O papel está úmido."}},
    {100,
     {"Último dia do período de experiência.",
      "A IARA deixou as afirmações na tela. Ela espera sua resposta."},
     "",
     {NULL}}};

// Flash do dia: imagem estática + frase curta. Os demais dias são salto puro. Textos propostos.
static const FlashDay flash_days[] = {
    {2, "Ninguém pergunta como foi seu primeiro dia."},
    {7, "O café da copa acaba. Ninguém repõe."},
    {16, "A foto do time é trocada. Tem menos gente nela."},
    {22, "A IARA aprende o seu nome. Você não contou."},
    {33, "Três mesas amanhecem limpas, sem nenhuma despedida."},
    {38, "Um e-mail geral: \"Parabéns, batemos a meta de novo.\""},
    {47, "Tem uma poça embaixo da impressora."},
    {52, "Alguém cantarola no banheiro. Não tem ninguém lá."},
    {63, "Você passa um dia inteiro sem falar com uma pessoa."},
    {68, "A IARA responde antes de você terminar a pergunta."},
    {79, "Um bilhete sob o teclado: \"Ainda tem gente acordada.\""},
    {84, "A luz da diretoria fica acesa. A sala está vazia."},
    {93, "O elevador não para mais no seu andar."},
    {97, "Você ouve a água mesmo com os ouvidos tapados."}};

#define PLAYABLE_DAY_COUNT ((int)(sizeof(playable_days) / sizeof(playable_days[0])))
#define FLASH_DAY_COUNT ((int)(sizeof(flash_days) / sizeof(flash_days[0])))

static const PlayableDay *find_playable_day(int day)
{
    for (int i = 0; i < PLAYABLE_DAY_COUNT; i++)
    {
        if (playable_days[i].day == day) return &playable_days[i];
    }

    return NULL;
}

int calendar_get_day_type(int day)
{
    if (find_playable_day(day) != NULL) return PLAYABLE_DAY;

    return (calendar_get_flash_text(day) != NULL) ? FLASH_DAY : JUMP_DAY;
}

int calendar_get_jump_block_length(int day)
{
    int length = 0;

    while (day + length <= TOTAL_GAME_DAYS && calendar_get_day_type(day + length) == JUMP_DAY)
    {
        length++;
    }

    return length;
}

const char *calendar_get_flash_text(int day)
{
    for (int i = 0; i < FLASH_DAY_COUNT; i++)
    {
        if (flash_days[i].day == day) return flash_days[i].text;
    }

    return NULL;
}

const char *calendar_get_arrival_line(int day, int line)
{
    const PlayableDay *playable = find_playable_day(day);

    return (playable != NULL) ? playable->arrival[line] : "";
}

const char *calendar_get_day_end_line(int day)
{
    const PlayableDay *playable = find_playable_day(day);

    return (playable != NULL) ? playable->day_end : "";
}

bool calendar_has_investigation(int day)
{
    const PlayableDay *playable = find_playable_day(day);

    // O Dia 1 tem o bilhete com a colinha
    return playable != NULL && (day == 1 || playable->note[0] != NULL);
}

const char *const *calendar_get_note_lines(int day)
{
    const PlayableDay *playable = find_playable_day(day);

    return (playable != NULL && playable->note[0] != NULL) ? playable->note : NULL;
}

float calendar_get_water_level(int day)
{
    if (day < WATER_FIRST_DAY) return 0.0f;

    return (float)(day - WATER_FIRST_DAY + 1) / (float)(TOTAL_GAME_DAYS - WATER_FIRST_DAY + 1);
}
