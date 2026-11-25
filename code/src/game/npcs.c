#include <stddef.h>
#include <float.h>
#include <math.h>
#include <stdint.h>

#include "raylib.h"
#include "npcs.h"
#include "game_state.h"
#include "assets_fs.h"
#include "characters.h"
#include "calendar.h"
#include "collisions.h"
#include "fonts.h"
#include "macros.h"

#define NPC_SIZE_AT_REFERENCE 75.0f
#define NPC_HITBOX_OFFSET_X 10.0f
#define NPC_HITBOX_OFFSET_Y 48.0f
#define NPC_HITBOX_WIDTH 50.0f
#define NPC_HITBOX_HEIGHT 24.0f
#define NPC_FEET_X 37.5f
#define NPC_FEET_Y 64.0f
#define NPC_FEET_HALF_WIDTH 15.5f
#define NPC_FEET_HALF_HEIGHT 8.0f

static Rectangle walking_feet(Vector2 position, float scale);

// Cumprimentos: o primeiro é a boas-vindas do Dia 1, e o segundo o de todos os outros dias.
static const char *const npc_greetings[NPC_COUNT][2] = {
    [NPC_ANDERSON] = {"Oi, sou o Anderson, da análise de dados. Ainda tô conferindo os números da manhã.", "Oi de novo. Respira fundo, que o dia ainda não começou."},
    [NPC_JULIO] = {"Opa, sou o Julio, do desenvolvimento. Se a tela travar, me chama antes de reiniciar.", "E aí, Estagiário? Mais um dia no front."},
    [NPC_JV] = {"Oi de novo. Já conseguiu abrir a fila?", "Bom dia. Ainda bem que você chegou inteiro(a)."},
    [NPC_LAURA] = {"Bem-vindo(a)! Achou a salinha? Se a cadeira enroscar, eu dou um jeito.", "Bom dia! Já tomou água hoje?"},
    [NPC_NICOLLAS] = {"Sou o Nicollas. Cuido do arquivo lá embaixo. Se eu sumir, é por ali.", "Opa. Tudo tranquilo por aqui em cima?"},
    [NPC_VASCO] = {"Vasco. Eu organizo a fila do time. Se aparecerem dois urgentes, fala comigo.", "Bom dia. Vamos ao que importa."},
    [NPC_ZITO] = {"Oi, sou o Zito, do TI. Conseguiu entrar com o crachá?", "Bom dia. A rede está estável, por enquanto."}};

// A fala de cada NPC em cada um dos 9 dias jogáveis (1, 3, 12, 27, 41, 58, 74, 89 e 100).
static const char *const npc_day_lines[NPC_COUNT][NPC_PLAYABLE_DAY_COUNT][NPC_DAY_LINE_COUNT] = {
    [NPC_ANDERSON] = {
        {"Eu costumo abrir o registro antes de aprovar. Peraí, você já achou onde ele fica?", "Olha o que entrou no chamado e o que saiu. Às vezes sobra uma linha."},
        {"O Tiago pediu folga e deixou a fila inteira com você.", "Norma nova sem aviso é daquelas que eu releio duas vezes."},
        {"Fechamento do mês. A IARA encurtou as regras do Financeiro.", "Mais curto nem sempre quer dizer igual."},
        {"Ouvi a gravação da diretoria. Tem um trecho em que só se ouve água.", "Eu fico só com o que a ata consegue provar."},
        {"Vão treinar a IARA com o arquivo morto, e é você quem escolhe os dados.", "Dado ruim ensina coisa ruim, e ela não esquece."},
        {"A catraca me chamou pelo nome hoje. Eu nunca disse meu nome a ela.", "Por via das dúvidas, guardo o crachá no bolso."},
        {"A auditoria quer os logs, e o corredor já está molhado até a copa.", "Quando um cenário fecha fácil demais, eu desconfio do caminho."},
        {"A IARA colocou seu nome na lista. O meu não aparece.", "Leia a lista como se fosse uma prova, e não um convite."},
        {"Hoje a sala vai estar cheia. Eu fico no fundo, perto da porta.", "Se pedirem que aceite tudo, lembre: contradição também é prova."}},
    [NPC_JULIO] = {
        {"Se travar logo hoje, eu culpo meu deploy. Adianta o diagnóstico.", "Brincadeira. Me mostra a tela antes de rodar de novo."},
        {"O Suporte está vazio e a fila caiu no seu colo. Boa sorte!", "A norma mudou sem commit, sem aviso e sem autor."},
        {"Fechamento de mês é o único dia em que o café acaba antes do prazo.", "A IARA reescreveu as regras. Ninguém abriu pull request para isso."},
        {"Quem transcreve ata ouve coisa que não devia, né?", "Tirei o fone. Era só água, mas parecia alguém respirando."},
        {"Treinar IA com arquivo morto... bonito nome para um servidor úmido.", "Ele pinga desde ontem. Ninguém quer mexer."},
        {"O sistema de crachás caiu às 3h12. Eu estava de plantão.", "Subi o serviço duas vezes. Alguém desligava de novo, por dentro."},
        {"Os logs da IARA têm uma linha sem autor. Eu não mexi, juro.", "O corredor alaga e o relatório pede estabilidade."},
        {"O elevador não vai até onde termina a escada.", "Ela ainda sobe? A escada, quero dizer."},
        {"Último dia de experiência. Eu já passei por aquela porta.", "Se der pane, não reinicie nada. Apenas saia."}},
    [NPC_JV] = {
        {"Se a sugestão parecer estranha, guarda o chamado que eu testo daqui.", "Uma pergunta de cada vez. Assim a gente sabe qual teste mudou a resposta."},
        {"A norma mudou e a IARA já 'sabia'. Isso me incomoda.", "Leia a regra antes de confiar na sugestão."},
        {"Duas frases são equivalentes quando dizem exatamente o mesmo.", "Ela chama de simplificar. Eu chamo de reescrever sem avisar."},
        {"Em ata, só conta quem sustenta o que diz até o fim.", "Quando uma versão se contradiz, a outra pode ser a que sobra."},
        {"Se o treino erra, o modelo erra para sempre.", "O viés mora nos dados, não no código."},
        {"A lista de crachás tem nomes de gente que já saiu.", "A IARA diz que voltaram. Eu quero ver o registro."},
        {"Não deixe a IARA ser a única a ler os próprios logs.", "Quem se audita, sempre passa."},
        {"Todos são bem-vindos na lista. Devia soar bem, não é?", "Eu nunca confio em regra que não deixa ninguém de fora."},
        {"No último dia, a IARA fala mais bonito do que nunca.", "Resposta confiante ainda pode partir de uma premissa falsa."}},
    [NPC_LAURA] = {
        {"Seu crachá já tá ativo. Pra entrar na salinha, passa pelo corredor da direita.", "Se precisar de mim, fico pela mesa do RH."},
        {"O Tiago pediu folga e ninguém assumiu a fila. Coitado de você.", "Se ficar pesado, pare um minuto. Ninguém está contando."},
        {"Fechamento do mês: todo mundo fala baixo e ninguém sai da mesa.", "Tem uma mesa vazia perto da impressora. Ninguém me avisou do desligamento."},
        {"O ar-condicionado só pinga na sua mesa. Eu já abri chamado.", "A resposta foi 'não há vazamento'. Foi automática."},
        {"Metade das mesas está vazia, e ninguém fala disso.", "No RH há mais pasta de desligamento do que de contratação."},
        {"A catraca chamou você pelo nome? Comigo ela também fez isso.", "Eu conheço os nomes da lista. Eram do seu primeiro dia."},
        {"A chefia não veio. A sala dela está alagada até o rodapé.", "Ninguém do RH foi avisado. Eu fiquei com os formulários."},
        {"Seu nome está na lista de convidados do Dia 100. Eu vi ontem.", "Eu não deixaria passar sem ler com calma."},
        {"Eu ainda me lembro do seu primeiro dia, cheio de dúvidas.", "Seja o que for lá embaixo, você não está sozinho(a)."}},
    [NPC_NICOLLAS] = {
        {"A impressora engasga com papel grosso. Tem uma pilha fina do lado.", "Ontem achei uma folha molhada ali. Deve ter sido meu copo."},
        {"Hoje o Suporte inteiro está no seu colo, né?", "Os chamados ficam mais leves depois da terceira xícara."},
        {"Fecharam o mês e o porão ganhou mais uma caixa de arquivos.", "A etiqueta fala de uma assinatura que eu nunca vi."},
        {"A água no porão sobe um dedo por semana. Eu marco na parede.", "Hoje chegou quase no meu joelho. Deve ser chuva."},
        {"O arquivo morto está úmido. Cheira a rio.", "Pus uma caixa em cima da outra e a de baixo ficou mole."},
        {"Os crachás caíram, e a porta do porão destravou sozinha.", "Fechei duas vezes. Ela abriu duas vezes."},
        {"Preciso que você me conte o que ler nos logs.", "Pode ser que alguém tenha descido às 3h12."},
        {"Tem uma escada a lápis no mapa do prédio. Ela vai até o fundo.", "Eu desci. Voltei. Não volto de novo."},
        {"Hoje eu fico aqui em cima. Lá embaixo já tem gente demais.", "Se descer, confere a marca que deixei no mapa."}},
    [NPC_VASCO] = {
        {"Pro primeiro turno, resolve a fila que abriu no seu computador.", "Se tudo chegar marcado como urgente, começa pela regra da mesa."},
        {"A norma mudou? Então a prioridade também mudou.", "Separe o que está escrito do que apenas foi dito."},
        {"O Financeiro quer assinatura até as seis. Já li três versões.", "Conta que encurta demais costuma esconder uma linha."},
        {"A ata é o único documento em que o silêncio tem peso.", "Registre quem falou, e não quem foi citado."},
        {"Escolher os dados é decidir o que a IARA vai valorizar.", "Priorize o que não identifica ninguém."},
        {"Cada crachá da lista tem um prazo. Alguns já passaram.", "Prioridade é saber quem saiu e quem só foi esquecido."},
        {"Sem a chefia, as prioridades ficaram comigo, e eu não aguento tudo.", "Escolha o que fecha o raciocínio sem sobra."},
        {"A lista do Dia 100 saiu, nome por nome, e eu fiquei de fora.", "Quem nunca recebe convite costuma ser quem sobrevive."},
        {"Cheguei cedo para arrumar a sala. Ela já estava arrumada.", "Alguém me esperava na cabeceira. Saí antes de ver quem."}},
    [NPC_ZITO] = {
        {"Se a tela demorar, espera um segundo. Quero ver se é a rede ou a fila.", "Na dúvida, lê a condição do chamado em voz baixa. Comigo funciona."},
        {"A norma é uma conta com parênteses. O difícil é achar o parêntese certo.", "Quem erra a ordem manda gente demais para o TI. Eu sofro."},
        {"Duas frases diferentes podem dizer a mesma coisa. Eu uso isso no TI.", "Mas trocar P por Q no lugar errado muda tudo."},
        {"Em sala de verdade-ou-mentira, comece supondo que um deles é sincero.", "O resto cai em cadeia, como dominó."},
        {"Todo registro vira regra quando entra no treino.", "Um único dado indevido e a regra inteira cai."},
        {"Do 'se' sai o 'então', e do 'então' sai o 'portanto'.", "Se a consequência é falsa, quem a causou também é."},
        {"Dois requisitos, uma resposta. Quando um é certo, o resto se organiza.", "Comece pelo que não tem como ser diferente."},
        {"Desenhe os conjuntos. Dentro, fora: tudo vira círculo.", "Quando a figura incomoda, ela está dizendo a verdade."},
        {"A IARA quer que você aceite o registro dela. Confere o que pode provar.", "Procure o par: uma coisa e o seu oposto, na mesma resposta."}}};

// A dica do dia, dita por quem a tem (um só NPC por dia jogável).
static const char *const npc_day_hints[NPC_PLAYABLE_DAY_COUNT][NPC_HINT_LINE_COUNT] = {
    {"A mesa de triagem aceita uma única combinação. Teste uma de cada vez.", "O contador de erros diz quantas estão erradas, mas não quais."},
    {"Resolva primeiro o que está entre parênteses.", "Se um dos lados do ∨ é verdadeiro, o resto não importa."},
    {"Teste os valores que separam os dois lados de cada par.", "Experimente P falso e Q verdadeiro, e depois o contrário."},
    {"Suponha que a primeira pessoa diz a verdade e siga as consequências.", "Se alguma fala se contradiz, volte e suponha o contrário."},
    {"Um registro com CPF já quebra a exigência do treino inteiro.", "Cada bairro precisa de um aprovado e de um reprovado."},
    {"Encadeie as regras: de B sai C, e de C sai J.", "Se D levaria a T e T não vale, então D também não vale."},
    {"Comece pelo único valor que já está decidido.", "Os outros se encaixam a partir dele."},
    {"Desenhe: os convidados cabem dentro dos funcionários.", "Depois veja se algum funcionário pode ficar de fora."},
    {"Procure uma premissa e a sua negação.", "Um par assim já basta para mostrar o absurdo."}};

static const NpcDefinition npc_definitions[NPC_COUNT] = {
    [NPC_ANDERSON] = {"ANDERSON", "Anderson", {-258.0f, -263.0f}, {0}, NPC_STATIONARY, 0.0f, SOUTH, true},
    [NPC_JULIO] = {"JULIO", "Julio", {280.0f, -90.0f}, {0}, NPC_STATIONARY, 0.0f, SOUTHEAST, false},
    [NPC_JV] = {"JV", "JV", {310.0f, -120.0f}, {0}, NPC_STATIONARY, 0.0f, SOUTHWEST, false},
    [NPC_LAURA] = {"LAURA", "Laura", {-262.0f, 0.0f}, {0}, NPC_STATIONARY, 0.0f, SOUTH, true},
    [NPC_NICOLLAS] = {"NICOLLAS", "Nicollas", {76.0f, -7.0f}, {0}, NPC_STATIONARY, 0.0f, SOUTH, true},
    [NPC_VASCO] = {"VASCO", "Vasco", {178.0f, -7.0f}, {0}, NPC_STATIONARY, 0.0f, SOUTH, true},
    [NPC_ZITO] = {"ZITO", "Zito", {325.0f, -90.0f}, {0}, NPC_STATIONARY, 0.0f, SOUTHWEST, false}};

// Centros das cadeiras a sul, relativos ao centro de office.png.
static const Vector2 workstation[NPC_COUNT] = {
    [NPC_ANDERSON] = {-223, -201.5f}, [NPC_JULIO] = {208, -201.5f},
    [NPC_JV] = {109, -201.5f}, [NPC_LAURA] = {-230, 54.5f},
    [NPC_NICOLLAS] = {110, 54.5f}, [NPC_VASCO] = {213, 54.5f},
    [NPC_ZITO] = {-127, -201.5f}
};
static Texture2D empty_npc_chair;

#define NPC_POSE_DIRECTION_COUNT 3
static const int npc_pose_directions[NPC_POSE_DIRECTION_COUNT] = {SOUTH, SOUTHEAST, SOUTHWEST};

typedef enum {
    WAITING_TO_START, RETURNING, AT_DESK, GOING_OUT, AT_DESTINATION, APPROACHING, WITH_PLAYER
} NpcActivity;

// Pontos no piso comunicam as fileiras por meio dos corredores já usados na recepção.
static const Vector2 office_waypoints[] = {
    {-325, -270}, {-48, -270}, {309, -270}, {345, -270},
    {-325, -90}, {-325, 5}, {-325, 194}, {-325, 245},
    {-56, -220}, {-56, -90}, {-64, -48}, {31, -48}, {309, -48}, {345, -48},
    {-64, 33}, {31, 33}, {309, 33}, {345, 33},
    {-64, 184}, {43, 184}, {309, 184}, {345, 184},
    {-64, 245}, {43, 245}, {309, 245}
};

enum { OFFICE_ROUTE_POINTS = sizeof(office_waypoints) / sizeof(office_waypoints[0]) + 2 };

typedef struct {
    Vector2 points[OFFICE_ROUTE_POINTS];
    int count, next;
    float length;
} NpcRoute;

typedef struct {
    Vector2 position;
    int direction;
    int seat;                 // Índice em npc_definitions do lugar (posição, direção e se está sentado) que ocupa hoje
    bool staged;
    float blocked_time;
    int movement_sign;
    float walk_time;
    bool has_workstation;
    float seat_time;   // Negativo durante a aproximação; fixado no último quadro ao sentar.
    int route_step;
    NpcActivity activity;
    NpcRoute route;
    Vector2 destination, look_at;
    float due, retry_at, visit_deadline, dwell, seated_since;
    int group, partner, last_option, replans;
    bool has_sat, outing, first_work_done;
    Texture2D seat_frames[SEAT_NPC_FRAME_COUNT];
    Texture2D walk_frames[DIRECTION_COUNT][WALK_FRAME_COUNT];
    bool walk_loaded[DIRECTION_COUNT];
    Texture2D pose_textures[DIRECTION_COUNT];   // Só as direções de npc_pose_directions são carregadas
} NpcState;

static NpcState npc_states[NPC_COUNT];
static bool npcs_loaded = false;
static bool routines_active, reception_finished;
static int routine_day;
static float office_time, next_outing, next_first_sit;
static uint32_t routine_random;
static float desk_time_factor = 1, social_factor = 1, inspection_factor = 1;

void npc_set_routine_statistics(const struct hidden_statistic *statistics)
{
    HiddenStatisticBands bands = hidden_statistic_bands(statistics);
    static const float desk[] = {1, 1.10f, 1.20f, 1.35f};
    static const float automation_social[] = {1, 0.90f, 0.80f, 0.65f};
    static const float inspections[] = {1, 1.15f, 1.30f, 1.50f};
    static const float bond_social[] = {1, 1.25f, 1.50f};
    desk_time_factor = desk[bands.automation];
    social_factor = automation_social[bands.automation] * bond_social[bands.bond];
    inspection_factor = inspections[bands.perception];
}

void npc_seed_routines(unsigned int seed)
{
    routine_random = seed;
}

static float random_time(float start, float end)
{
    routine_random = routine_random * UINT32_C(1664525) + UINT32_C(1013904223);
    return start + (end - start) * (float)(routine_random >> 8) / 16777216.0f;
}

void resize_npcs(float old_scale, float new_scale, Vector2 old_center, Vector2 new_center)
{
    if (!npcs_loaded || old_scale <= 0.0f || new_scale <= 0.0f) return;

    for (int i = 0; i < NPC_COUNT; i++)
    {
        npc_states[i].position.x = new_center.x +
            (npc_states[i].position.x - old_center.x) * new_scale / old_scale;
        npc_states[i].position.y = new_center.y +
            (npc_states[i].position.y - old_center.y) * new_scale / old_scale;
    }
}

int npc_get_count(void)
{
    return NPC_COUNT;
}

const NpcDefinition *npc_get_definition(int npc_id)
{
    if (npc_id < 0 || npc_id >= NPC_COUNT) return NULL;
    return &npc_definitions[npc_id];
}

int npc_get_playable_day_index(int day)
{
    if (day < 1) return -1;

    int playable_day_index = 0;
    for (int current_day = 1; current_day <= day; current_day++)
    {
        if (calendar_get_day_type(current_day) != PLAYABLE_DAY) continue;
        if (current_day == day) return playable_day_index < NPC_PLAYABLE_DAY_COUNT ? playable_day_index : -1;
        playable_day_index++;
    }

    return -1;
}

static unsigned int story_absences;

void npc_set_story_absences(unsigned int absences)
{
    story_absences = absences;
}

bool npc_is_present(int npc_id)
{
    if (npc_id < 0 || npc_id >= NPC_COUNT) return false;
    if (npc_id == NPC_JULIO) return !(story_absences & STORY_ABSENT_JULIO);
    if (npc_id == NPC_ANDERSON) return !(story_absences & STORY_ABSENT_ANDERSON);
    return true;
}

int npc_get_daily_hint_owner(int day)
{
    int playable_day_index = npc_get_playable_day_index(day);
    if (playable_day_index < 0) return -1;
    int owner = playable_day_index % NPC_COUNT;
    if (day == 89 && !npc_is_present(owner)) return NPC_LAURA;
    if (day == 100 && !npc_is_present(owner)) return NPC_JV;
    return owner;
}

bool npc_has_hint(int npc_id, int day)
{
    return npc_is_present(npc_id) && npc_id == npc_get_daily_hint_owner(day);
}

const char *npc_get_greeting(int npc_id, int day)
{
    if (npc_id < 0 || npc_id >= NPC_COUNT || day < 1) return NULL;
    return npc_greetings[npc_id][day == 1 ? 0 : 1];
}

const char *npc_get_day_line(int npc_id, int day, int line_index)
{
    int playable_day_index = npc_get_playable_day_index(day);

    if (npc_id < 0 || npc_id >= NPC_COUNT || playable_day_index < 0) return NULL;
    if (line_index < 0 || line_index >= NPC_DAY_LINE_COUNT) return NULL;
    return npc_day_lines[npc_id][playable_day_index][line_index];
}

/* 0 = sem variante; 1..6 = A1, A2, P1, P2, V1, V2. */
static const unsigned char dialogue_variant_gate[NPC_PLAYABLE_DAY_COUNT][NPC_COUNT] = {
    {3, 1, 3, 1, 3, 1, 3}, {1, 1, 3, 5, 3, 1, 3},
    {4, 2, 4, 6, 4, 2, 4}, {4, 2, 4, 6, 4, 2, 4},
    {4, 2, 4, 6, 4, 2, 4}, {4, 2, 4, 6, 4, 5, 4},
    {4, 2, 4, 6, 4, 5, 4}, {4, 2, 4, 6, 4, 5, 4},
    {4, 2, 4, 6, 4, 5, 4}};

static const char *const statistic_day_lines[NPC_PLAYABLE_DAY_COUNT][NPC_COUNT] = {
    {"Deixa eu comparar essa linha com o registro também.", "A fila fechou. Vamos olhar o relatório antes do próximo deploy?",
     "Você consultou o registro? Quero comparar o que entrou com o que saiu.", "A fila ocupou seu primeiro turno inteiro? Quando puder, passa no RH.",
     "Você olhou o mural? Tem um risco na moldura que eu não tinha notado.", "A fila fechou. Antes da próxima, confere o relatório comigo.",
     "Boa. No registro, dá pra conferir uma coisa de cada vez."},
    {"A fila já abriu. Quero comparar a norma com a versão anterior.", "A fila já tá acesa? Essa atualização chegou antes do café.",
     "Quero testar a sugestão com a regra antiga e a nova. Você compara comigo?", "A Júlia disse que vocês conseguiram fazer uma pausa. Que bom.",
     "Tem um risco novo na moldura. Jurava que eu tinha limpado isso ontem.", "A fila já começou. Primeiro confira qual norma está valendo.",
     "Na regra nova, vamos localizar os parênteses primeiro."},
    {"Tem uma diferença aqui. Deixa eu comparar as duas linhas.", "A fila já abriu no fechamento. Tomara que não tenham pulado a revisão.",
     "Compara a condição nas duas versões. Uma palavra pode mudar o resultado.", "Se esse fechamento apertar, vem até o RH. A gente confere junto.",
     "A etiqueta da caixa fala de uma assinatura que eu nunca vi.", "A fila tá pronta, mas assinatura vem depois da conferência.",
     "Vamos trocar uma expressão de cada vez e conferir se o sentido continua."},
    {"Tem uma pausa na gravação. Não vou preencher o que não ouvi.", "A transcrição saiu depressa. Vou ouvir esse trecho antes de aceitar.",
     "Se duas falas não cabem juntas, separa o que cada pessoa afirmou.", "O chamado voltou sem solução. Quer olhar o vazamento comigo?",
     "A água chegou perto da marca que eu fiz no porão. Vou marcar de novo.", "A ata pode esperar um minuto. Confirme quem disse cada coisa.",
     "Anota a hipótese e segue as consequências. Se contradisser, volta um passo."},
    {"Quero conferir a lista antes de ela entrar na seleção.", "A fila já tá aberta? Vou conferir o serviço antes de mexer.",
     "Antes de treinar, quero ver quais registros ficaram de fora.", "Se encontrar um nome nessa lista, me chama. Posso procurar a pasta com você.",
     "Esse risco perto do arquivo é igual ao do mural. Você viu também?", "Deixa a fila aberta. Primeiro separa os registros que podem identificar alguém.",
     "Quero testar uma amostra. Você me ajuda a procurar dados pessoais?"},
    {"A catraca me chamou pelo nome. Quero conferir o horário registrado.", "Eu subi o serviço duas vezes. Quero conferir os horários.",
     "A IARA diz que voltaram. Preciso do registro antes de repetir isso.", "Estou aqui. Podemos conferir esses nomes juntos.",
     "Fechei a porta do porão duas vezes. Ela abriu duas vezes.", "Eu cuido da lista. Se você encontrar um nome conhecido, me avisa.",
     "Primeiro o horário; depois o crachá. Não dá pra concluir só pela tela."},
    {"Essa linha não indica autor. Vou deixar a autoria em aberto.", "O log voltou sem autor. Eu não vou reiniciar antes de guardar uma cópia.",
     "Esse registro não tem autor. Mostra a linha anterior pra mim.", "Vou separar os formulários. Se quiser, a gente confere os nomes junto.",
     "Anota o horário que aparecer. Quero comparar com a porta do porão.", "Antes de fechar a fila, confira se essa linha tem autor.",
     "Vamos conferir cada requisito contra o registro, um por vez."},
    {"Qual regra inclui seu nome? Quero ver a condição, não só a lista.", "A lista apareceu pronta. Eu queria saber quem pediu essa atualização.",
     "Procura alguém fora do conjunto. Se não houver, confere a regra de novo.", "Seu nome está na lista. Se quiser, eu fico aqui enquanto você lê.",
     "No mapa tem uma escada a lápis. Ela vai até o fundo.", "Meu nome não está na lista. Antes de você ir, me mostra o que ela diz.",
     "Marca quem atende à regra e confere se sobra alguém de fora."},
    {"Mostra as duas afirmações. Quero ver onde elas deixam de caber juntas.", "Se a tela travar, não reinicia. Sai de lá.",
     "Uma premissa por vez. Qual delas você consegue confrontar com o registro?", "Estou aqui antes de você entrar. Depois, me conta o que conseguir.",
     "Se descer, olha o mapa antes. A escada termina mais fundo do que parece.", "Antes de entrar, me diz se precisa de alguma coisa.",
     "Escolhe o par contraditório e confere se há registro sustentando uma das partes."}};

const char *npc_get_statistic_day_line(int npc_id, int day, const struct hidden_statistic *statistics)
{
    int day_index = npc_get_playable_day_index(day);
    if (!statistics || npc_id < 0 || npc_id >= NPC_COUNT || day_index < 0) return NULL;

    unsigned char gate = dialogue_variant_gate[day_index][npc_id];
    if (!gate) return NULL;
    HiddenStatisticBands bands = hidden_statistic_bands(statistics);
    int current = gate <= 2 ? bands.automation : gate <= 4 ? bands.perception : bands.bond;
    int required = gate <= 2 ? gate : gate <= 4 ? gate - 2 : gate - 4;
    if (current < required) return NULL;
    return statistic_day_lines[day_index][npc_id];
}

const char *npc_get_hint_line(int day, int line_index)
{
    int playable_day_index = npc_get_playable_day_index(day);

    if (playable_day_index < 0 || line_index < 0 || line_index >= NPC_HINT_LINE_COUNT) return NULL;
    return npc_day_hints[playable_day_index][line_index];
}

bool npc_is_moving(int npc_id)
{
    const NpcDefinition *definition = npc_get_definition(npc_id);
    return definition != NULL && definition->movement != NPC_STATIONARY;
}

bool npc_blocks_player(int npc_id)
{
    if (!npc_is_present(npc_id)) return false;
    if (npcs_loaded && routines_active) return npc_states[npc_id].seat_time < 0.0f;
    // Sem os NPCs carregados, cada um está no seu lugar de origem
    if (npcs_loaded && npc_states[npc_id].has_workstation)
        return npc_states[npc_id].seat_time < 0.0f;
    if (npcs_loaded && npc_states[npc_id].staged) return true;
    return !npc_definitions[npcs_loaded ? npc_states[npc_id].seat : npc_id].seated_pose;
}

int npc_get_dialogue_line_count(int npc_id, int day, bool first_today)
{
    if (!npc_is_present(npc_id) || npc_get_day_line(npc_id, day, 0) == NULL) return 0;

    int line_count = NPC_DAY_LINE_COUNT;
    if (first_today) line_count++;
    if (npc_has_hint(npc_id, day)) line_count += NPC_HINT_LINE_COUNT;
    return line_count;
}

const char *npc_get_dialogue_line(int npc_id, int day, bool first_today, int line_index)
{
    if (line_index < 0 || line_index >= npc_get_dialogue_line_count(npc_id, day, first_today)) return NULL;

    if (first_today)
    {
        if (line_index == 0) return npc_get_greeting(npc_id, day);
        line_index--;
    }

    if (line_index < NPC_DAY_LINE_COUNT) return npc_get_day_line(npc_id, day, line_index);
    return npc_get_hint_line(day, line_index - NPC_DAY_LINE_COUNT);
}

// Embaralhamento determinístico (uma semente por dia): o mesmo dia sempre rearruma igual,
// e ninguém repete o lugar do dia jogável anterior.
static unsigned int next_random(unsigned int *state)
{
    *state = (*state * 1664525u) + 1013904223u;
    return *state >> 8;
}

static void compute_seats_for_day(int day, int seats[NPC_COUNT])
{
    for (int npc = 0; npc < NPC_COUNT; npc++) seats[npc] = npc;

    int target = npc_get_playable_day_index(day);
    for (int step = 1; step <= target; step++)
    {
        int next[NPC_COUNT];
        unsigned int state = (unsigned int)(step + 1) * 2654435761u;
        bool found = false;

        for (int attempt = 0; attempt < 200 && !found; attempt++)
        {
            for (int npc = 0; npc < NPC_COUNT; npc++) next[npc] = npc;
            for (int npc = NPC_COUNT - 1; npc > 0; npc--)
            {
                int other = (int)(next_random(&state) % (unsigned int)(npc + 1));
                int swap = next[npc];
                next[npc] = next[other];
                next[other] = swap;
            }

            found = true;
            for (int npc = 0; npc < NPC_COUNT; npc++)
            {
                // next[npc] é o lugar que o NPC ocupará; seats guarda o do dia jogável anterior
                if (next[npc] == seats[npc]) found = false;
            }
        }

        // Sem sorteio válido, todos deslocam um lugar: ninguém fica onde estava
        if (!found)
        {
            for (int npc = 0; npc < NPC_COUNT; npc++) next[npc] = (seats[npc] + 1) % NPC_COUNT;
        }

        for (int npc = 0; npc < NPC_COUNT; npc++) seats[npc] = next[npc];
    }
}

int npc_get_seat_for_day(int npc_id, int day)
{
    int seats[NPC_COUNT];

    if (npc_id < 0 || npc_id >= NPC_COUNT) return -1;
    compute_seats_for_day(day, seats);
    return seats[npc_id];
}

void npc_arrange_for_day(int day, float scale, Vector2 map_center)
{
    int seats[NPC_COUNT];

    if (!npcs_loaded || scale <= 0.0f) return;

    routines_active = npc_get_playable_day_index(day) >= 0;
    routine_day = day;
    office_time = next_outing = next_first_sit = 0;
    reception_finished = day != 1;
    int order[NPC_COUNT];
    for (int i = 0; i < NPC_COUNT; i++) order[i] = i;
    for (int i = NPC_COUNT - 1; i > 0; i--)
    {
        int pick = (int)random_time(0, (float)(i + 1));
        int previous = order[i];
        order[i] = order[pick];
        order[pick] = previous;
    }
    float departure = random_time(0.5f, 1.1f);
    for (int i = 0; i < NPC_COUNT; i++)
    {
        if (!npc_is_present(order[i])) continue;
        npc_states[order[i]].due = departure;
        departure += random_time(0.8f, 1.15f);
    }

    compute_seats_for_day(day, seats);
    for (int npc = 0; npc < NPC_COUNT; npc++)
    {
        const NpcDefinition *seat = &npc_definitions[seats[npc]];

        npc_states[npc].staged = false;
        npc_states[npc].seat = seats[npc];
        npc_states[npc].position = (Vector2){map_center.x + seat->position.x * scale,
                                              map_center.y + seat->position.y * scale};
        npc_states[npc].direction = seat->facing;
        NpcState *state = &npc_states[npc];
        state->activity = WAITING_TO_START;
        state->group = state->partner = state->last_option = -1;
        state->route = (NpcRoute){0};
        state->retry_at = state->blocked_time = 0;
        state->has_sat = state->outing = state->first_work_done = false;
        state->visit_deadline = 0;
        state->has_workstation = state->seat_frames[SEAT_NPC_FRAME_COUNT - 1].id != 0;
        state->seat_time = -1.0f;
        state->route_step = -1;
        state->walk_time = 0.0f;
        if (state->has_workstation || routines_active)
        {
            const float pixel = NPC_SIZE_AT_REFERENCE * scale / CHARACTER_SPRITE_SIZE;
            Vector2 anchor = workstation[state->seat];
            state->position = (Vector2){map_center.x + anchor.x * scale - 16 * pixel,
                                        map_center.y + (anchor.y - 22) * scale - (31 + SEAT_ENTRY_OFFSET) * pixel};
            state->direction = SOUTH;
        }
    }
}

static Rectangle get_npc_hitbox_at(Vector2 position, float scale)
{
    Rectangle reference = {NPC_HITBOX_OFFSET_X, NPC_HITBOX_OFFSET_Y, NPC_HITBOX_WIDTH, NPC_HITBOX_HEIGHT};
    return scale_reference_rect(reference, scale, position);
}

Rectangle npc_get_hitbox(int npc_id, float scale)
{
    if (npc_id < 0 || npc_id >= NPC_COUNT) return (Rectangle){0};
    if (routines_active && npc_states[npc_id].seat_time < 0)
        return walking_feet(npc_states[npc_id].position, scale);
    return get_npc_hitbox_at(npc_states[npc_id].position, scale);
}

Vector2 npc_get_position(int npc_id)
{
    if (npc_id < 0 || npc_id >= NPC_COUNT) return (Vector2){0};
    return npc_states[npc_id].position;
}

Rectangle npc_get_interaction_zone(int npc_id, float scale)
{
    if (npc_id < 0 || npc_id >= NPC_COUNT) return (Rectangle){0};

    Vector2 position = npc_states[npc_id].position;
    float margin = 30.0f * scale;
    float size = NPC_SIZE_AT_REFERENCE * scale;
    return (Rectangle){position.x - margin, position.y - margin, size + margin * 2.0f, size + margin * 2.0f};
}

int npc_get_nearby(Rectangle player_hitbox, float scale)
{
    int nearest = -1;
    float nearest_distance = 0.0f;
    Vector2 player_center = {player_hitbox.x + player_hitbox.width / 2.0f,
                             player_hitbox.y + player_hitbox.height / 2.0f};

    for (int i = 0; i < NPC_COUNT; i++)
    {
        if (!npc_is_present(i)) continue;
        Rectangle zone = npc_get_interaction_zone(i, scale);
        if (!CheckCollisionRecs(player_hitbox, zone)) continue;

        float dx = player_center.x - (zone.x + zone.width / 2.0f);
        float dy = player_center.y - (zone.y + zone.height / 2.0f);
        float distance_squared = dx * dx + dy * dy;
        if (nearest < 0 || distance_squared < nearest_distance)
        {
            nearest = i;
            nearest_distance = distance_squared;
        }
    }
    return nearest;
}

bool initialize_npcs(float scale, Vector2 map_center)
{
    if (npcs_loaded) return true;

    static const char *const direction_names[DIRECTION_COUNT] = {
        "east", "south", "north", "west", "south-east",
        "north-west", "south-west", "north-east"};
    bool all_frames_loaded = true;
    empty_npc_chair = LoadTexture(OFFICE_CHAIR_SOUTH_PATH);

    for (int i = 0; i < NPC_COUNT; i++)
    {
        const NpcDefinition *definition = &npc_definitions[i];
        NpcState *state = &npc_states[i];
        state->seat = i;
        state->position = (Vector2){map_center.x + definition->position.x * scale,
                                    map_center.y + definition->position.y * scale};
        state->direction = definition->facing;
        state->blocked_time = 0.0f;
        state->movement_sign = 1;
        state->walk_time = 0.0f;
        for (int frame = 0; frame < SEAT_NPC_FRAME_COUNT; frame++)
        {
            state->seat_frames[frame] = LoadTexture(TextFormat(SEAT_NPC_FRAME_PATH, definition->sprite_folder, frame));
            if (!state->seat_frames[frame].id) break;
        }

        if (definition->movement == NPC_STATIONARY)
        {
            // Como o lugar muda a cada dia, o NPC carrega a pose de todas as direções que os lugares usam
            for (int pose = 0; pose < NPC_POSE_DIRECTION_COUNT; pose++)
            {
                int direction = npc_pose_directions[pose];
                const char *path = TextFormat("%s%s/walk/%s-0.png", PERSONAGENS_PATH,
                                              definition->sprite_folder, direction_names[direction]);

                if (!asset_exists(path))
                {
                    TraceLog(LOG_ERROR, "Pose do NPC %s nao encontrada: %s", definition->name, path);
                    all_frames_loaded = false;
                    continue;
                }

                state->pose_textures[direction] = LoadTexture(path);
                if (state->pose_textures[direction].id == 0) all_frames_loaded = false;
            }
            for (int direction = 0; direction < DIRECTION_COUNT; direction++)
                state->walk_loaded[direction] = load_walk_frames(definition->sprite_folder, direction, state->walk_frames[direction]);
            continue;
        }

        for (int direction = 0; direction < DIRECTION_COUNT; direction++)
        {
            state->walk_loaded[direction] = load_walk_frames(definition->sprite_folder, direction,
                                                              state->walk_frames[direction]);
            if (!state->walk_loaded[direction])
            {
                TraceLog(LOG_ERROR, "NPC %s sem animacao de movimento para direcao %d",
                         definition->name, direction);
                all_frames_loaded = false;
            }
        }
    }

    if (!all_frames_loaded)
    {
        npcs_loaded = true;
        unload_npcs();
        return false;
    }

    npcs_loaded = true;
    npc_seed_routines((unsigned int)GetRandomValue(0, 0x7ffffffe));
    return all_frames_loaded;
}

static bool position_is_in_movement_area(int npc_id, Vector2 position, float scale, Vector2 map_center)
{
    Rectangle area = scale_reference_rect(npc_definitions[npc_id].movement_area, scale, map_center);
    return position.x >= area.x && position.y >= area.y &&
           position.x <= area.x + area.width && position.y <= area.y + area.height;
}

static Rectangle walking_feet(Vector2 position, float scale)
{
    Rectangle reference = {NPC_FEET_X - NPC_FEET_HALF_WIDTH, NPC_FEET_Y - NPC_FEET_HALF_HEIGHT,
                           2 * NPC_FEET_HALF_WIDTH, 2 * NPC_FEET_HALF_HEIGHT};
    return scale_reference_rect(reference, scale, position);
}

typedef struct {
    float scale;
    Vector2 center;
    Rectangle bounds, player;
    const Rectangle *obstacles;
    int count;
} NpcOffice;

typedef enum {
    CONTINUE_WORKING, CHAT, CHECK_MONITOR, PRINTER, BOARD,
    SERVERS, SHELF, DOORWAY, MURAL, WATER, SOFA, BREAK_TOGETHER
} NpcAction;

#define ROUTINE_OPTION_COUNT 4
static const NpcAction routine_actions[NPC_COUNT][ROUTINE_OPTION_COUNT] = {
    [NPC_ANDERSON] = {PRINTER, WATER},
    [NPC_JULIO] = {CHECK_MONITOR},
    [NPC_JV] = {CHAT},
    [NPC_LAURA] = {SOFA, BREAK_TOGETHER},
    [NPC_NICOLLAS] = {SHELF, DOORWAY, WATER, MURAL},
    [NPC_VASCO] = {BOARD, CHAT},
    [NPC_ZITO] = {SERVERS}
};

typedef struct {
    float min, max, limit;
    Vector2 spots[2], look_at;
} NpcActionTiming;

static const NpcActionTiming action_timing[] = {
    [CHAT] = {.min = 4, .max = 7, .limit = 25},
    [CHECK_MONITOR] = {.min = 6, .max = 10, .limit = 40},
    [PRINTER] = {5, 9, 25, {{-365, -100}, {-365, -65}}, {-430, -88}},
    [BOARD] = {5, 9, 25, {{-95, -270}, {-36, -274}}, {-64, -350}},
    [SERVERS] = {9, 15, 40, {{343, -249}, {362, -200}}, {396, -267}},
    [SHELF] = {9, 15, 40, {{-399, 103}, {-399, 130}}, {-470, 100}},
    [DOORWAY] = {9, 15, 40, {{269, -278}, {286, -278}}, {270, -335}},
    [MURAL] = {9, 15, 40, {{-364, -203}, {-364, -174}}, {-415, -190}},
    [WATER] = {12, 20, 40, {{160, -284}, {224, -284}}, {197, -311}},
    [SOFA] = {12, 20, 40, {{-327, 272}, {-342, 234}}, {-388, 273}},
    [BREAK_TOGETHER] = {12, 20, 40, {{160, -284}, {224, -284}}, {0, 0}}
};

static Vector2 to_screen(const NpcOffice *office, Vector2 point)
{
    return (Vector2){office->center.x + point.x * office->scale,
                     office->center.y + point.y * office->scale};
}

static Vector2 to_map(const NpcOffice *office, Vector2 point)
{
    return (Vector2){(point.x - office->center.x) / office->scale,
                     (point.y - office->center.y) / office->scale};
}

static Vector2 feet_at(const NpcOffice *office, int id)
{
    Rectangle feet = walking_feet(npc_states[id].position, office->scale);
    return (Vector2){feet.x + feet.width / 2, feet.y + feet.height / 2};
}

static Vector2 desk_entry(int id)
{
    Vector2 entry = workstation[npc_states[id].seat];
    entry.y += NPC_FEET_Y - (31 + SEAT_ENTRY_OFFSET) * NPC_SIZE_AT_REFERENCE / CHARACTER_SPRITE_SIZE;
    return entry;
}

static bool segment_hits(Vector2 a, Vector2 b, Rectangle rect, float scale)
{
    if (rect.width <= 0 || rect.height <= 0) return false;
    rect.x -= NPC_FEET_HALF_WIDTH * scale;
    rect.y -= NPC_FEET_HALF_HEIGHT * scale;
    rect.width += 2 * NPC_FEET_HALF_WIDTH * scale;
    rect.height += 2 * NPC_FEET_HALF_HEIGHT * scale;
    Vector2 corners[] = {{rect.x, rect.y}, {rect.x + rect.width, rect.y},
                         {rect.x + rect.width, rect.y + rect.height}, {rect.x, rect.y + rect.height}};
    if (CheckCollisionPointRec(a, rect) || CheckCollisionPointRec(b, rect)) return true;
    Vector2 intersection;
    for (int edge = 0; edge < 4; edge++)
        if (CheckCollisionLines(a, b, corners[edge], corners[(edge + 1) % 4], &intersection)) return true;
    return false;
}

static bool clear_segment(const NpcOffice *office, int id, Vector2 a, Vector2 b, bool actors)
{
    float sx = NPC_FEET_HALF_WIDTH * office->scale, sy = NPC_FEET_HALF_HEIGHT * office->scale;
    Rectangle bounds = {office->bounds.x + sx, office->bounds.y + sy,
                        office->bounds.width - 2 * sx, office->bounds.height - 2 * sy};
    if (!CheckCollisionPointRec(b, bounds) || segment_hits(a, b, office->player, office->scale)) return false;
    for (int i = 0; i < office->count; i++)
        if (segment_hits(a, b, office->obstacles[i], office->scale)) return false;
    for (int i = 0; actors && i < NPC_COUNT; i++)
        if (i != id && npc_blocks_player(i) && segment_hits(a, b, npc_get_hitbox(i, office->scale), office->scale))
            return false;
    return true;
}

static NpcRoute office_route(const NpcOffice *office, int id, Vector2 from, Vector2 to, bool actors)
{
    NpcRoute route = {0};
    if (clear_segment(office, id, from, to, actors))
    {
        route.points[0] = to_map(office, to);
        route.count = 1;
        route.length = hypotf(to.x - from.x, to.y - from.y) / office->scale;
        return route;
    }
    Vector2 points[OFFICE_ROUTE_POINTS] = {from, to};
    float distance[OFFICE_ROUTE_POINTS];
    int previous[OFFICE_ROUTE_POINTS];
    bool visited[OFFICE_ROUTE_POINTS] = {0};
    for (int i = 0; i < OFFICE_ROUTE_POINTS; i++)
    {
        if (i >= 2) points[i] = to_screen(office, office_waypoints[i - 2]);
        distance[i] = FLT_MAX;
        previous[i] = -1;
    }
    distance[0] = 0;
    while (!visited[1])
    {
        int nearest = -1;
        for (int i = 0; i < OFFICE_ROUTE_POINTS; i++)
            if (!visited[i] && distance[i] < FLT_MAX && (nearest < 0 || distance[i] < distance[nearest]))
                nearest = i;
        if (nearest < 0) return route;
        visited[nearest] = true;
        Vector2 a = points[nearest];
        for (int i = 1; i < OFFICE_ROUTE_POINTS; i++)
        {
            if (visited[i]) continue;
            Vector2 b = points[i];
            float via = distance[nearest] + hypotf(a.x - b.x, a.y - b.y);
            if (via >= distance[i] || !clear_segment(office, id, a, b, actors)) continue;
            distance[i] = via;
            previous[i] = nearest;
        }
    }
    for (int i = 1; i != 0; i = previous[i]) route.count++;
    for (int i = 1, j = route.count; i != 0; i = previous[i]) route.points[--j] = to_map(office, points[i]);
    route.length = distance[1] / office->scale;
    return route;
}

static void face_point(int id, Vector2 from, Vector2 to)
{
    NpcState *state = &npc_states[id];
    float dx = to.x - from.x, dy = to.y - from.y;
    state->walk_time = 0;
    if (state->seat_time >= 0)
        state->direction = fabsf(dx) < 20 ? SOUTH : dx > 0 ? SOUTHEAST : SOUTHWEST;
    else if (fabsf(dx) > fabsf(dy) * 1.8f) state->direction = dx > 0 ? EAST : WEST;
    else if (fabsf(dy) > fabsf(dx) * 1.8f) state->direction = dy > 0 ? SOUTH : NORTH;
    else state->direction = dy > 0 ? (dx > 0 ? SOUTHEAST : SOUTHWEST) : (dx > 0 ? NORTHEAST : NORTHWEST);
}

static Vector2 departure_point(const NpcOffice *office, int id)
{
    return npc_states[id].seat_time >= 0 ? to_screen(office, desk_entry(id)) : feet_at(office, id);
}

static bool follow_route(const NpcOffice *office, int id, float dt, float speed)
{
    NpcState *state = &npc_states[id];
    NpcRoute *route = &state->route;
    if (route->count == 0) return false;
    if (state->seat_time >= 0)
    {
        Vector2 entry = to_screen(office, desk_entry(id));
        if (!clear_segment(office, id, entry, entry, true)) { state->blocked_time += dt; return false; }
        state->seat_time = state->has_workstation
            ? state->seat_time - dt
            : -1;
        if (state->seat_time >= 0) return false;
        state->position = (Vector2){entry.x - NPC_FEET_X * office->scale, entry.y - NPC_FEET_Y * office->scale};
    }
    if (route->next == route->count) return true;
    Vector2 from = feet_at(office, id), goal = to_screen(office, route->points[route->next]);
    float distance = hypotf(goal.x - from.x, goal.y - from.y);
    float step = speed * office->scale * fminf(dt, 0.05f);
    float portion = distance > step ? step / distance : 1;
    Vector2 next = {from.x + (goal.x - from.x) * portion, from.y + (goal.y - from.y) * portion};
    if (!clear_segment(office, id, from, next, true))
    {
        state->walk_time = 0;
        state->blocked_time += dt;
        return false;
    }
    state->blocked_time = 0;
    if (distance > 0.01f)
    {
        float walk_time = state->walk_time;
        face_point(id, from, goal);
        state->walk_time = walk_time + dt;
    }
    state->position.x += next.x - from.x;
    state->position.y += next.y - from.y;
    if (portion == 1) route->next++;
    return route->next == route->count;
}

#define ROUTINE_SPEED 135.0f
#define SCENE_SPEED 155.0f
#define ROUTE_BLOCKED_SECONDS 0.6f
#define NPC_SEAT_SECONDS (SEAT_NPC_FRAME_COUNT * SEAT_FRAME_SECONDS)

static void send_to_desk(int id)
{
    NpcState *state = &npc_states[id];
    state->activity = RETURNING;
    state->route.count = 0;
    state->retry_at = state->blocked_time = state->walk_time = 0;
    state->replans = 0;
}

static void end_outing(int leader)
{
    for (int i = 0; i < NPC_COUNT; i++)
    {
        NpcState *state = &npc_states[i];
        if (state->group != leader) continue;
        state->group = state->partner = -1;
        if (state->outing) send_to_desk(i);
    }
}

static bool same_spot(Vector2 a, Vector2 b)
{
    return fabsf(a.x - b.x) < 34 && fabsf(a.y - b.y) < 19;
}

static bool prepare_visit_route(const NpcOffice *office, int id, Vector2 spot, NpcRoute *route, float *return_time)
{
    for (int i = 0; i < NPC_COUNT; i++)
        if (i != id && (npc_states[i].activity == GOING_OUT || npc_states[i].activity == AT_DESTINATION) &&
            same_spot(npc_states[i].destination, spot)) return false;
    Vector2 goal = to_screen(office, spot);
    *route = office_route(office, id, departure_point(office, id), goal, true);
    NpcRoute back = office_route(office, id, goal, to_screen(office, desk_entry(id)), false);
    *return_time = back.length / ROUTINE_SPEED + NPC_SEAT_SECONDS;
    return route->count && back.count;
}

static void start_visit(int id, int leader, NpcRoute route, Vector2 destination, Vector2 look_at)
{
    NpcState *state = &npc_states[id];
    state->route = route;
    state->activity = GOING_OUT;
    state->destination = destination;
    state->look_at = look_at;
    state->group = leader;
    state->outing = true;
    state->blocked_time = 0;
    state->replans = 0;
}

typedef struct {
    NpcRoute route, companion;
    Vector2 spot, look_at, partner_spot;
    int partner;
    float max_dwell, return_time;
} NpcVisitPlan;

static bool prepare_visit(const NpcOffice *office, int id, NpcAction action, int vacancies, NpcVisitPlan *plan)
{
    bool paired_walk = action == BREAK_TOGETHER;
    bool social = action == CHAT || action == CHECK_MONITOR || paired_walk;
    bool at_desk = action == CHAT || action == CHECK_MONITOR;
    if (vacancies < (paired_walk ? 2 : 1)) return false;
    if (social)
    {
        for (int i = 0; i < NPC_COUNT; i++)
            if (npc_states[i].partner >= 0) return false;
    }
    const NpcActionTiming *timing = &action_timing[action];
    int valid = 0;
    for (int candidate = 0; candidate < (social ? NPC_COUNT : 1); candidate++)
    {
        int partner = social ? candidate : -1;
        if (partner == id || (id == NPC_JV && social && partner != NPC_ANDERSON) ||
            (action == CHECK_MONITOR && partner != NPC_JV)) continue;
        if (social)
        {
            if (!npc_is_present(partner)) continue;
            const NpcState *host = &npc_states[partner];
            if (host->activity != AT_DESK || host->group >= 0) continue;
            if (paired_walk && (!host->first_work_done || office_time - host->seated_since < 30)) continue;
        }
        for (int mark = 0; mark < 2; mark++)
        {
            NpcVisitPlan option = {.spot = timing->spots[mark], .look_at = timing->look_at, .partner = partner};
            if (at_desk)
            {
                int desk = social ? partner : id;
                Vector2 entry = desk_entry(desk);
                option.spot = (Vector2){entry.x + (mark == 0 ? -44 : 44), entry.y - 8};
                option.look_at = workstation[npc_states[desk].seat];
            }
            float back;
            if (!prepare_visit_route(office, id, option.spot, &option.route, &back)) continue;
            float arrival = option.route.length / ROUTINE_SPEED + NPC_SEAT_SECONDS;
            if (paired_walk)
            {
                option.partner_spot = timing->spots[1 - mark];
                option.look_at = option.partner_spot;
                float companion_back;
                if (!prepare_visit_route(office, partner, option.partner_spot, &option.companion, &companion_back)) continue;
                arrival = fmaxf(arrival, option.companion.length / ROUTINE_SPEED + NPC_SEAT_SECONDS);
                back = fmaxf(back, companion_back);
            }
            option.return_time = back;
            option.max_dwell = fminf(timing->max, timing->limit - arrival - back);
            if (option.max_dwell < timing->min) continue;
            if (random_time(0, (float)++valid) < 1) *plan = option;
        }
    }
    return valid > 0;
}

static void choose_routine(const NpcOffice *office, int id, int vacancies)
{
    NpcState *state = &npc_states[id];
    NpcVisitPlan plans[ROUTINE_OPTION_COUNT];
    float weights[ROUTINE_OPTION_COUNT] = {0};
    int eligible = 0;
    for (int i = 0; i < ROUTINE_OPTION_COUNT; i++)
    {
        NpcAction action = routine_actions[id][i];
        if (action == MURAL && (routine_day != 3 || inspection_factor <= 1)) continue;
        if (action == CONTINUE_WORKING || !prepare_visit(office, id, action, vacancies, &plans[i])) continue;
        weights[i] = action == CHAT || action == BREAK_TOGETHER ? social_factor
                   : action >= PRINTER && action <= MURAL ? inspection_factor : 1;
        eligible++;
    }
    float total = 2;
    for (int i = 0; i < ROUTINE_OPTION_COUNT; i++)
    {
        if (eligible > 1 && (int)routine_actions[id][i] == state->last_option) weights[i] *= 0.5f;
        total += weights[i];
    }
    float draw = random_time(0, total) - 2;
    for (int i = 0; i < ROUTINE_OPTION_COUNT && draw >= 0; i++)
    {
        draw -= weights[i];
        if (draw >= 0) continue;
        NpcAction action = routine_actions[id][i];
        const NpcActionTiming *timing = &action_timing[action];
        const NpcVisitPlan *plan = &plans[i];
        start_visit(id, id, plan->route, plan->spot, plan->look_at);
        state->partner = plan->partner;
        state->dwell = random_time(timing->min, plan->max_dwell);
        state->due = 0;
        state->visit_deadline = office_time + timing->limit - plan->return_time;
        state->last_option = action;
        if (action == BREAK_TOGETHER)
            start_visit(plan->partner, id, plan->companion, plan->partner_spot, plan->spot);
        else if (plan->partner >= 0)
            npc_states[plan->partner].group = id;
        next_outing = office_time + 3;
        return;
    }
    state->due = office_time + random_time(15, 25) * desk_time_factor;
}

static void update_return(const NpcOffice *office, int id, float dt)
{
    NpcState *state = &npc_states[id];
    if (state->seat_time < 0)
    {
        if (office_time < state->retry_at) return;
        if (!state->route.count)
            state->route = office_route(office, id, feet_at(office, id), to_screen(office, desk_entry(id)), true);
        if (!follow_route(office, id, dt, ROUTINE_SPEED))
        {
            if (!state->route.count || state->blocked_time >= ROUTE_BLOCKED_SECONDS)
            {
                bool reroute_now = state->route.count && state->replans++ == 0;
                state->route.count = 0;
                state->blocked_time = 0;
                if (!reroute_now) state->retry_at = office_time + random_time(2, 4);
            }
            return;
        }
        state->walk_time = 0;
        if (!state->has_sat && office_time < next_first_sit) return;
        if (!state->has_sat) next_first_sit = office_time + 1;
        state->direction = SOUTH;
        state->seat_time = state->has_workstation ? 0 : NPC_SEAT_SECONDS;
    }
    state->seat_time = fminf(state->seat_time + dt, NPC_SEAT_SECONDS);
    if (state->seat_time < NPC_SEAT_SECONDS) return;
    Vector2 feet = to_screen(office, workstation[state->seat]);
    float pixel = NPC_SIZE_AT_REFERENCE * office->scale / CHARACTER_SPRITE_SIZE;
    state->position = (Vector2){feet.x - 16 * pixel, feet.y - 31 * pixel};
    state->activity = AT_DESK;
    state->outing = false;
    state->has_sat = true;
    state->seated_since = office_time;
    state->due = office_time + (state->first_work_done ? random_time(35, 65) : random_time(25, 45)) * desk_time_factor;
}

static void update_office_routines(const NpcOffice *office, float dt)
{
    if (!reception_finished) return;
    office_time += dt;
    for (int i = 0; i < NPC_COUNT; i++)
    {
        NpcState *state = &npc_states[i];
        if (!npc_is_present(i)) continue;
        if (state->activity == WAITING_TO_START && office_time >= state->due)
            send_to_desk(i);
        if (state->outing && state->group == i && office_time >= state->visit_deadline) end_outing(i);
        if (state->activity == RETURNING) update_return(office, i, dt);
        if (state->activity != GOING_OUT) continue;
        if (follow_route(office, i, dt, ROUTINE_SPEED))
        {
            state->activity = AT_DESTINATION;
            face_point(i, feet_at(office, i), to_screen(office, state->look_at));
        }
        else if (state->blocked_time >= ROUTE_BLOCKED_SECONDS)
        {
            if (state->replans++ == 0)
                state->route = office_route(office, i, departure_point(office, i), to_screen(office, state->destination), true);
            else state->route.count = 0;
            state->blocked_time = 0;
            if (!state->route.count) end_outing(state->group);
        }
    }

    int vacancies = 2;
    float actor_weights[NPC_COUNT] = {0}, total = 0;
    for (int i = 0; i < NPC_COUNT; i++)
    {
        NpcState *state = &npc_states[i];
        if (!npc_is_present(i)) continue;
        vacancies -= state->outing;
        if (state->activity == AT_DESK && office_time >= state->due)
        {
            state->first_work_done = true;
            if (state->group < 0)
            {
                actor_weights[i] = 1 + fminf((office_time - state->due) / 20, 1);
                total += actor_weights[i];
            }
        }
        if (state->activity != AT_DESTINATION || state->group != i) continue;
        int partner = state->partner;
        if (partner >= 0 && npc_states[partner].activity == GOING_OUT) continue;
        if (partner >= 0 && npc_states[partner].activity == AT_DESK)
            face_point(partner, feet_at(office, partner), feet_at(office, i));
        if (state->due == 0) state->due = office_time + state->dwell;
        if (office_time >= state->due) end_outing(i);
    }
    if (office_time < next_outing || total <= 0) return;
    float draw = random_time(0, total);
    for (int id = 0; id < NPC_COUNT; id++)
    {
        draw -= actor_weights[id];
        if (draw < 0)
        {
            choose_routine(office, id, vacancies);
            return;
        }
    }
}

void npc_begin_approach(int id)
{
    if (!npcs_loaded || id < 0 || id >= NPC_COUNT) return;
    NpcState *state = &npc_states[id];
    if (state->group >= 0) end_outing(state->group);
    state->activity = APPROACHING;
    state->route.count = 0;
    state->walk_time = state->blocked_time = 0;
    state->replans = 0;
}

bool npc_approach_player(int id, int other, float dt, float scale, Vector2 center, Rectangle bounds,
                         const Rectangle *obstacles, int count, Rectangle player, Vector2 player_feet)
{
    if (!npcs_loaded || id < 0 || id >= NPC_COUNT || scale <= 0) return false;
    NpcOffice office = {scale, center, bounds, player, obstacles, count};
    NpcState *state = &npc_states[id];
    if (state->activity == WITH_PLAYER) return true;
    Vector2 from = feet_at(&office, id);
    float distance = hypotf(from.x - player_feet.x, from.y - player_feet.y) / scale;
    if (!state->route.count && state->replans == 0 && state->seat_time < 0 && distance >= 55 && distance <= 90 &&
        clear_segment(&office, id, from, from, true))
    {
        state->activity = WITH_PLAYER;
        state->destination = to_map(&office, from);
        face_point(id, from, player_feet);
        return true;
    }

    if (!state->route.count && state->replans > 1) return false;
    if (!state->route.count && state->blocked_time <= 0)
    {
        const Vector2 offsets[] = {{67, -12}, {61, 26}, {54, 48}, {0, 69}, {-54, 48}, {-61, 26}, {-67, -12}};
        for (size_t i = 0; i < sizeof(offsets) / sizeof(offsets[0]); i++)
        {
            float side = id == NPC_VASCO ? -1 : 1;
            Vector2 spot = {player_feet.x + offsets[i].x * side * scale, player_feet.y + offsets[i].y * scale};
            Vector2 mark = to_map(&office, spot);
            if (other >= 0 && other < NPC_COUNT &&
                (npc_states[other].route.count || npc_states[other].activity == WITH_PLAYER) &&
                same_spot(mark, npc_states[other].destination)) continue;
            NpcRoute route = office_route(&office, id, departure_point(&office, id), spot, true);
            if (!route.count || (state->route.count && route.length + 25 >= state->route.length)) continue;
            state->route = route;
            state->destination = mark;
        }
        if (!state->route.count) state->blocked_time = dt;
    }
    else if (!state->route.count) state->blocked_time += dt;
    if (follow_route(&office, id, dt, SCENE_SPEED))
    {
        state->activity = WITH_PLAYER;
        face_point(id, feet_at(&office, id), player_feet);
        return true;
    }
    if (state->blocked_time >= ROUTE_BLOCKED_SECONDS)
    {
        state->route.count = 0;
        state->replans++;
        state->blocked_time = 0;
    }
    return false;
}

void npc_finish_approach(int id)
{
    if (npcs_loaded && id >= 0 && id < NPC_COUNT) send_to_desk(id);
}

void npc_face_player(int id, float scale, Rectangle player)
{
    if (!npcs_loaded || id < 0 || id >= NPC_COUNT) return;
    NpcOffice office = {.scale = scale};
    Vector2 target = {player.x + player.width / 2, player.y + player.height / 2};
    face_point(id, feet_at(&office, id), target);
}

static void update_npc_seating(int id, float dt, float scale, Vector2 center,
                               const Rectangle *obstacles, int count, Rectangle player)
{
    NpcState *state = &npc_states[id];
    if (state->staged) return;
    const float pixel = NPC_SIZE_AT_REFERENCE * scale / CHARACTER_SPRITE_SIZE;
    Vector2 anchor = workstation[state->seat];
    Vector2 top_left = {center.x + anchor.x * scale - 16 * pixel,
                        center.y + anchor.y * scale - 31 * pixel};
    if (state->seat_time >= 0.0f)
    {
        state->seat_time = fminf(state->seat_time + dt, SEAT_NPC_FRAME_COUNT * SEAT_FRAME_SECONDS);
        if (state->seat_time >= (SEAT_NPC_FRAME_COUNT - 1) * SEAT_FRAME_SECONDS) state->position = top_left;
        return;
    }

    // Corredores entre as marcas da recepção e as mesas do Dia 1.
    static const Vector2 routes[NPC_COUNT][4] = {
        [NPC_LAURA] = {{-325, 245}, {-325, -9}, {-230, -9}},
        [NPC_JV] = {{303, 211}, {303, -271}, {109, -271}},
        [NPC_VASCO] = {{137, -270}, {344, -270}, {344, -14}, {213, -14}}
    };
    Vector2 goal = {top_left.x, top_left.y - SEAT_ENTRY_OFFSET * pixel};
    if (state->route_step >= 0)
    {
        Vector2 point = routes[id][state->route_step];
        goal = (Vector2){center.x + point.x * scale - 16 * pixel,
                         center.y + point.y * scale - 31 * pixel};
    }
    float dx = goal.x - state->position.x, dy = goal.y - state->position.y;
    float distance = hypotf(dx, dy);
    float step = 135.0f * scale * fminf(dt, 0.05f);
    float portion = distance > step ? step / distance : 1.0f;
    Vector2 next = {state->position.x + dx * portion, state->position.y + dy * portion};
    Rectangle feet = walking_feet(next, scale);
    bool blocked = collides_with_any(feet, obstacles, count) || CheckCollisionRecs(feet, player);
    for (int other = 0; other < NPC_COUNT && !blocked; other++)
    {
        if (other != id && npc_blocks_player(other))
            blocked = CheckCollisionRecs(feet, walking_feet(npc_states[other].position, scale));
    }
    if (blocked)
    {
        state->walk_time = 0.0f;
        return;
    }

    state->position = next;
    state->walk_time += dt;
    state->direction = fabsf(dx) > fabsf(dy) ? (dx < 0 ? WEST : EAST) : (dy < 0 ? NORTH : SOUTH);
    if (portion < 1.0f) return;
    if (state->route_step < 0)
    {
        state->direction = SOUTH;
        state->seat_time = 0.0f;
    }
    else if (++state->route_step >= (id == NPC_VASCO ? 4 : 3)) state->route_step = -1;
}

void update_npcs(float delta_time, float scale, Vector2 map_center, Rectangle map_bounds,
                 const Rectangle *obstacles, int obstacle_count, Rectangle player_hitbox)
{
    if (!npcs_loaded || scale <= 0.0f) return;

    if (routines_active)
    {
        NpcOffice office = {scale, map_center, map_bounds, player_hitbox, obstacles, obstacle_count};
        update_office_routines(&office, delta_time);
        return;
    }

    for (int i = 0; i < NPC_COUNT; i++)
    {
        NpcState *state = &npc_states[i];
        if (!npc_is_present(i)) continue;
        const NpcDefinition *definition = &npc_definitions[i];
        if (state->has_workstation)
        {
            update_npc_seating(i, delta_time, scale, map_center, obstacles, obstacle_count, player_hitbox);
            continue;
        }
        if (definition->movement == NPC_STATIONARY) continue;

        Rectangle area = scale_reference_rect(definition->movement_area, scale, map_center);
        float speed = definition->movement_speed * scale * delta_time * state->movement_sign;
        bool horizontal = definition->movement == NPC_MOVE_HORIZONTAL;
        float axis_position = horizontal ? state->position.x : state->position.y;
        float axis_min = horizontal ? area.x : area.y;
        float axis_max = axis_min + (horizontal ? area.width : area.height);
        float axis_next = axis_position + speed;
        bool reached_end = (state->movement_sign > 0 && axis_next >= axis_max) ||
                           (state->movement_sign < 0 && axis_next <= axis_min);

        if (reached_end)
        {
            axis_next = state->movement_sign > 0 ? axis_max : axis_min;
            state->movement_sign *= -1;
        }

        Vector2 candidate = state->position;
        if (horizontal) candidate.x = axis_next;
        else candidate.y = axis_next;

        Rectangle feet = get_npc_hitbox_at(candidate, scale);
        if (!position_is_in_movement_area(i, candidate, scale, map_center) ||
            collides_with_any(get_npc_hitbox_at(candidate, scale), obstacles, obstacle_count) ||
            feet.x < map_bounds.x || feet.y < map_bounds.y ||
            feet.x + feet.width > map_bounds.x + map_bounds.width ||
            feet.y + feet.height > map_bounds.y + map_bounds.height ||
            CheckCollisionRecs(get_npc_hitbox_at(candidate, scale), player_hitbox))
        {
            state->movement_sign *= -1;
            state->blocked_time = 0.0f;
            state->walk_time = 0.0f;
            continue;
        }

        bool blocked_by_npc = false;
        Rectangle candidate_hitbox = get_npc_hitbox_at(candidate, scale);
        for (int other = 0; other < NPC_COUNT; other++)
        {
            if (other != i && npc_blocks_player(other) &&
                CheckCollisionRecs(candidate_hitbox, npc_get_hitbox(other, scale)))
            {
                blocked_by_npc = true;
                break;
            }
        }

        if (blocked_by_npc)
        {
            state->blocked_time += delta_time;
            if (blocked_by_npc)
            {
                state->walk_time = 0.0f;
                continue;
            }
        }
        else
        {
            state->blocked_time = 0.0f;
        }

        state->position = candidate;
        state->walk_time += delta_time;
        state->direction = horizontal
            ? (state->movement_sign > 0 ? EAST : WEST)
            : (state->movement_sign > 0 ? SOUTH : NORTH);
    }
}

static void draw_npc(int npc_id, float scale, Color tint)
{
    if (!npc_is_present(npc_id)) return;
    const NpcState *state = &npc_states[npc_id];
    float size = NPC_SIZE_AT_REFERENCE * scale;
    Texture2D texture;
    Rectangle source;
    Rectangle destination;

    if (state->has_workstation || routines_active)
    {
        float pixel = size / CHARACTER_SPRITE_SIZE;
        Vector2 mark = workstation[state->seat];
        Vector2 anchor = {GetScreenWidth() / 2.0f + mark.x * scale, GetScreenHeight() / 2.0f + mark.y * scale};
        if (state->has_workstation && state->seat_time >= 0.0f)
        {
            int frame = (int)(state->seat_time / SEAT_FRAME_SECONDS);
            if (frame >= SEAT_NPC_FRAME_COUNT) frame = SEAT_NPC_FRAME_COUNT - 1;
            texture = state->seat_frames[frame];
            source = (Rectangle){0, 0, texture.width, SEAT_DESK_EDGE_Y};
            destination = scale_reference_rect((Rectangle){-SEAT_ORIGIN_X, -SEAT_ORIGIN_Y, source.width, source.height}, pixel, anchor);
            DrawTexturePro(texture, source, destination, (Vector2){0}, 0.0f, tint);
            return;
        }
        if (empty_npc_chair.id)
        {
            source = (Rectangle){0, 0, empty_npc_chair.width, CHAIR_ORIGIN_Y - (SEAT_ORIGIN_Y - SEAT_DESK_EDGE_Y)};
            destination = scale_reference_rect((Rectangle){-CHAIR_ORIGIN_X, -CHAIR_ORIGIN_Y, source.width, source.height}, pixel, anchor);
            DrawTexturePro(empty_npc_chair, source, destination, (Vector2){0}, 0.0f, tint);
        }
    }

    if (routines_active && state->seat_time >= 0 && !state->has_workstation)
    {
        texture = state->pose_textures[SOUTH];
        source = (Rectangle){0, 0, texture.width, texture.height * 0.75f};
        destination = (Rectangle){state->position.x, state->position.y, size, size * 0.75f};
    }
    else if (!routines_active && !npc_is_moving(npc_id) && !state->staged && !state->has_workstation)
    {
        texture = state->pose_textures[state->direction];
        if (texture.id == 0) return;

        // Sentado, a mesa cobre as pernas: a Laura (lugar de baixo, à esquerda) fica mais baixa que os demais
        float visible = npc_definitions[state->seat].seated_pose ? (state->seat == NPC_LAURA ? 0.625f : 0.75f) : 1.0f;

        source = (Rectangle){0, 0, (float)texture.width, texture.height * visible};
        destination = (Rectangle){state->position.x, state->position.y, size,
                                  size * source.height / texture.height};
    }
    else
    {
        if (!state->walk_loaded[state->direction])
        {
            texture = state->pose_textures[SOUTH];
            if (!texture.id) return;
            DrawTexturePro(texture, (Rectangle){0, 0, texture.width, texture.height},
                           (Rectangle){state->position.x, state->position.y, size, size}, (Vector2){0}, 0, tint);
            return;
        }
        int frame = state->walk_time > 0.0f ? get_walk_frame(state->walk_time) : 0;
        texture = state->walk_frames[state->direction][frame];
        source = (Rectangle){0, 0, (float)texture.width, (float)texture.height};
        destination = (Rectangle){state->position.x, state->position.y, size, size};
    }

    DrawTexturePro(texture, source, destination, (Vector2){0}, 0.0f, tint);
}

void draw_npcs(float scale, Color tint, float player_y, bool behind_player)
{
    if (!npcs_loaded) return;

    for (int i = 0; i < NPC_COUNT; i++)
    {
        bool npc_is_behind = npc_states[i].position.y < player_y;
        if (npc_is_behind == behind_player) draw_npc(i, scale, tint);
    }
}

void draw_npcs_lit(float scale, NpcLightFn light, float player_y, bool behind_player)
{
    if (!npcs_loaded) return;

    float size = NPC_SIZE_AT_REFERENCE * scale;

    for (int i = 0; i < NPC_COUNT; i++)
    {
        bool npc_is_behind = npc_states[i].position.y < player_y;
        if (npc_is_behind != behind_player) continue;

        Vector2 foot = {npc_states[i].position.x + (size * 0.5f), npc_states[i].position.y + (size * 0.9f)};
        draw_npc(i, scale, light(foot));
    }
}

void draw_npc_portrait(int npc_id, Rectangle destination)
{
    if (!npcs_loaded || npc_id < 0 || npc_id >= NPC_COUNT) return;

    Texture2D texture = npc_is_moving(npc_id)
        ? npc_states[npc_id].walk_frames[SOUTH][0]
        : npc_states[npc_id].pose_textures[SOUTH];
    Rectangle source = {0, 0, (float)texture.width, (float)texture.height};
    DrawTexturePro(texture, source, destination, (Vector2){0}, 0.0f, WHITE);
}

void unload_npcs(void)
{
    if (!npcs_loaded) return;

    for (int i = 0; i < NPC_COUNT; i++)
    {
        for (int direction = 0; direction < DIRECTION_COUNT; direction++)
        {
            for (int frame = 0; frame < WALK_FRAME_COUNT; frame++)
            {
                if (npc_states[i].walk_frames[direction][frame].id > 0)
                    UnloadTexture(npc_states[i].walk_frames[direction][frame]);
            }
            if (npc_states[i].pose_textures[direction].id > 0)
                UnloadTexture(npc_states[i].pose_textures[direction]);
        }
        for (int frame = 0; frame < SEAT_NPC_FRAME_COUNT; frame++)
            if (npc_states[i].seat_frames[frame].id) UnloadTexture(npc_states[i].seat_frames[frame]);
        npc_states[i] = (NpcState){0};
    }
    if (empty_npc_chair.id) UnloadTexture(empty_npc_chair);

    npcs_loaded = false;
}


void npc_stage_day_one(float scale, Vector2 center)
{
    const int ids[] = {NPC_LAURA, NPC_JV, NPC_VASCO};
    const Vector2 marks[] = {{-140, 190}, {300, 145}, {100, -275}};
    for (int i = 0; i < 3; i++)
    {
        npc_states[ids[i]].staged = true;
        npc_states[ids[i]].route_step = 0;
        npc_states[ids[i]].walk_time = 0;
        npc_states[ids[i]].position = (Vector2){center.x + marks[i].x * scale, center.y + marks[i].y * scale};
        npc_states[ids[i]].direction = SOUTH;
    }
}
void npc_end_reception(void)
{
    reception_finished = true;
    for (int i = 0; i < NPC_COUNT; i++)
        npc_states[i].staged = false;
}

void npc_face_scene(int npc_id, Vector2 target)
{
    if (!npcs_loaded || npc_id < 0 || npc_id >= NPC_COUNT) return;
    npc_states[npc_id].walk_time = 0;
    float dx = target.x - npc_states[npc_id].position.x;
    float dy = target.y - npc_states[npc_id].position.y;
    int direction = fabsf(dx) < 25 ? (dy < 0 ? NORTH : SOUTH) : fabsf(dy) < 25 ?
        (dx > 0 ? EAST : WEST) : dy < 0 ? (dx > 0 ? NORTHEAST : NORTHWEST) : (dx > 0 ? SOUTHEAST : SOUTHWEST);
    if (!npc_states[npc_id].staged) direction = fabsf(dx) < 25 ? SOUTH : dx > 0 ? SOUTHEAST : SOUTHWEST;
    npc_states[npc_id].direction = direction;
}
void npc_step_scene(int npc_id, Vector2 delta, float scale, Rectangle bounds,
                    const Rectangle *obstacles, int count, Rectangle player)
{
    if (!npcs_loaded || npc_id < 0 || npc_id >= NPC_COUNT) return;
    if (npc_states[npc_id].has_workstation && !npc_states[npc_id].staged) return;
    Vector2 candidate = {npc_states[npc_id].position.x + delta.x, npc_states[npc_id].position.y + delta.y};
    Rectangle feet = get_npc_hitbox_at(candidate, scale);
    if (feet.x < bounds.x || feet.y < bounds.y || feet.x + feet.width > bounds.x + bounds.width ||
        feet.y + feet.height > bounds.y + bounds.height || collides_with_any(feet, obstacles, count) ||
        CheckCollisionRecs(feet, player)) return;
    for (int i = 0; i < NPC_COUNT; i++)
        if (i != npc_id && npc_is_present(i) && CheckCollisionRecs(feet, npc_get_hitbox(i, scale))) return;
    npc_states[npc_id].position = candidate;
    npc_states[npc_id].walk_time += GetFrameTime();
}
