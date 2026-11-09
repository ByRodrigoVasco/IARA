#include <stddef.h>

#include "raylib.h"
#include "npcs.h"
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

// Cumprimentos: o primeiro é a boas-vindas do Dia 1, e o segundo o de todos os outros dias.
static const char *const npc_greetings[NPC_COUNT][2] = {
    [NPC_ANDERSON] = {"Bem-vindo(a) à CISSA! Eu sou o Anderson, da análise de dados.", "Oi de novo. Respira fundo, que o dia ainda não começou."},
    [NPC_JULIO] = {"Opa, gente nova! Eu sou o Julio, do desenvolvimento. Seja bem-vindo(a)!", "E aí, Estagiário? Mais um dia no front."},
    [NPC_JV] = {"Que bom ter alguém novo! Eu sou a JV, cuido dos modelos da IARA.", "Bom dia. Ainda bem que você chegou inteiro(a)."},
    [NPC_LAURA] = {"Seja muito bem-vindo(a)! Eu sou a Laura, do RH. Qualquer coisa, me chame.", "Bom dia! Já tomou água hoje?"},
    [NPC_NICOLLAS] = {"Ei, bem-vindo(a)! Eu sou o Nicollas. Meu trabalho é no porão, mas passo por aqui.", "Opa. Tudo tranquilo por aqui em cima?"},
    [NPC_VASCO] = {"Seja bem-vindo(a). Eu sou o Vasco, cuido das prioridades da equipe.", "Bom dia. Vamos ao que importa."},
    [NPC_ZITO] = {"Bem-vindo(a)! Eu sou o Zito, do TI. Aqui, quase tudo tem uma lógica.", "Bom dia. A rede está estável, por enquanto."}};

// A fala de cada NPC em cada um dos 9 dias jogáveis (1, 3, 12, 27, 41, 58, 74, 89 e 100).
static const char *const npc_day_lines[NPC_COUNT][NPC_PLAYABLE_DAY_COUNT][NPC_DAY_LINE_COUNT] = {
    [NPC_ANDERSON] = {
        {"Primeiro dia é barulhento. Preste atenção em como a IARA responde.", "Resposta bonita não é resposta conferida."},
        {"O Tiago pediu folga e deixou a fila inteira com você.", "Norma nova sem aviso é daquelas que eu releio duas vezes."},
        {"Fechamento do mês. A IARA encurtou as regras do Financeiro.", "Mais curto nem sempre quer dizer igual."},
        {"Ouvi a gravação da diretoria. Tem um trecho em que só se ouve água.", "Eu fico só com o que a ata consegue provar."},
        {"Vão treinar a IARA com o arquivo morto, e é você quem escolhe os dados.", "Dado ruim ensina coisa ruim, e ela não esquece."},
        {"A catraca me chamou pelo nome hoje. Eu nunca disse meu nome a ela.", "Por via das dúvidas, guardo o crachá no bolso."},
        {"A auditoria quer os logs, e o corredor já está molhado até a copa.", "Quando um cenário fecha fácil demais, eu desconfio do caminho."},
        {"Dizem que seu nome está na lista do Ricardo. O meu, não.", "Leia a lista como se fosse uma prova, e não um convite."},
        {"Hoje a sala vai estar cheia. Eu fico no fundo, perto da porta.", "Se pedirem que aceite tudo, lembre: contradição também é prova."}},
    [NPC_JULIO] = {
        {"Se o computador travar, a culpa é do deploy. Sempre.", "Brincadeira. Mas a IARA é rápida demais para confiar no escuro."},
        {"O Suporte está vazio e a fila caiu no seu colo. Boa sorte!", "A norma mudou sem commit, sem aviso e sem autor."},
        {"Fechamento de mês é o único dia em que o café acaba antes do prazo.", "A IARA reescreveu as regras. Ninguém abriu pull request para isso."},
        {"Quem transcreve ata ouve coisa que não devia, né?", "Tirei o fone. Era só água, mas parecia alguém respirando."},
        {"Treinar IA com arquivo morto... bonito nome para um servidor úmido.", "Ele pinga desde ontem. Ninguém quer mexer."},
        {"O sistema de crachás caiu às 3h12. Eu estava de plantão.", "Subi o serviço duas vezes. Alguém desligava de novo, por dentro."},
        {"Os logs da IARA têm uma linha sem autor. Eu não mexi, juro.", "O corredor alaga e o relatório pede estabilidade."},
        {"Nunca vi o Ricardo usar o elevador. Só a escada, e para baixo.", "Ela ainda sobe? A escada, quero dizer."},
        {"Último dia de experiência. Eu já passei por aquela porta.", "Se der pane, não reinicie nada. Apenas saia."}},
    [NPC_JV] = {
        {"Use a IARA, mas não pergunte tudo a ela.", "Ferramenta ajuda sem decidir por você."},
        {"A norma mudou e a IARA já 'sabia'. Isso me incomoda.", "Leia a regra antes de confiar na sugestão."},
        {"Duas frases são equivalentes quando dizem exatamente o mesmo.", "Ela chama de simplificar. Eu chamo de reescrever sem avisar."},
        {"Em ata, só conta quem sustenta o que diz até o fim.", "Quando uma versão se contradiz, a outra pode ser a que sobra."},
        {"Se o treino erra, o modelo erra para sempre.", "O viés mora nos dados, não no código."},
        {"A lista de crachás tem nomes de gente que já saiu.", "A IARA diz que voltaram. Eu quero ver o registro."},
        {"Não deixe a IARA ser a única a ler os próprios logs.", "Quem se audita, sempre passa."},
        {"Todos são bem-vindos na lista. Devia soar bem, não é?", "Eu nunca confio em regra que não deixa ninguém de fora."},
        {"No último dia, a IARA fala mais bonito do que nunca.", "Resposta confiante ainda pode partir de uma premissa falsa."}},
    [NPC_LAURA] = {
        {"Seu crachá já está ativo. Sua mesa é a da salinha, mais afastada.", "Se precisar de alguma coisa, é só me chamar."},
        {"O Tiago pediu folga e ninguém assumiu a fila. Coitado de você.", "Se ficar pesado, pare um minuto. Ninguém está contando."},
        {"Fechamento do mês: todo mundo fala baixo e ninguém sai da mesa.", "Tem uma mesa vazia perto da impressora. Ninguém me avisou do desligamento."},
        {"O ar-condicionado só pinga na sua mesa. Eu já abri chamado.", "A resposta foi 'não há vazamento'. Foi automática."},
        {"Metade das mesas está vazia, e ninguém fala disso.", "No RH há mais pasta de desligamento do que de contratação."},
        {"A catraca chamou você pelo nome? Comigo ela também fez isso.", "Eu conheço os nomes da lista. Eram do seu primeiro dia."},
        {"A chefia não veio. A sala dela está alagada até o rodapé.", "Ninguém do RH foi avisado. Eu fiquei com os formulários."},
        {"Seu nome está na lista de convidados do Dia 100. Eu vi ontem.", "Eu não deixaria passar sem ler com calma."},
        {"Eu ainda me lembro do seu primeiro dia, cheio de dúvidas.", "Seja o que for lá embaixo, você não está sozinho(a)."}},
    [NPC_NICOLLAS] = {
        {"Se precisar de ajuda com a rotina, é só falar.", "Aqui em cima é tranquilo. É lá embaixo que fica estranho."},
        {"Hoje o Suporte inteiro está no seu colo, né?", "Os chamados ficam mais leves depois da terceira xícara."},
        {"Fecharam o mês e o porão ganhou mais uma caixa de arquivos.", "A etiqueta fala de uma assinatura que eu nunca vi."},
        {"A água no porão sobe um dedo por semana. Eu marco na parede.", "Hoje chegou quase no meu joelho. Deve ser chuva."},
        {"O arquivo morto está úmido. Cheira a rio.", "Pus uma caixa em cima da outra e a de baixo ficou mole."},
        {"Os crachás caíram, e a porta do porão destravou sozinha.", "Fechei duas vezes. Ela abriu duas vezes."},
        {"Preciso que você me conte o que ler nos logs.", "Pode ser que alguém tenha descido às 3h12."},
        {"Tem uma escada a lápis no mapa do prédio. Ela vai até o fundo.", "Eu desci. Voltei. Não volto de novo."},
        {"Hoje eu fico aqui em cima. Lá embaixo já tem gente demais.", "Se descer, leve o crachá da Mariana. Ela esqueceu."}},
    [NPC_VASCO] = {
        {"Nem todo chamado urgente precisa passar na frente.", "Aprenda isso cedo e o resto do ano fica mais leve."},
        {"A norma mudou? Então a prioridade também mudou.", "Separe o que está escrito do que apenas foi dito."},
        {"O Financeiro quer assinatura até as seis. Já li três versões.", "Conta que encurta demais costuma esconder uma linha."},
        {"A ata é o único documento em que o silêncio tem peso.", "Registre quem falou, e não quem foi citado."},
        {"Escolher os dados é decidir o que a IARA vai valorizar.", "Priorize o que não identifica ninguém."},
        {"Cada crachá da lista tem um prazo. Alguns já passaram.", "Prioridade é saber quem saiu e quem só foi esquecido."},
        {"Sem a chefia, as prioridades ficaram comigo, e eu não aguento tudo.", "Escolha o que fecha o raciocínio sem sobra."},
        {"A lista do Dia 100 saiu, nome por nome, e eu fiquei de fora.", "Quem nunca recebe convite costuma ser quem sobrevive."},
        {"Cheguei cedo para arrumar a sala. Ela já estava arrumada.", "Alguém me esperava na cabeceira. Saí antes de ver quem."}},
    [NPC_ZITO] = {
        {"Se algo não fecha, a culpa raramente é sua.", "Quando duvidar, volte às premissas."},
        {"A norma é uma conta com parênteses. O difícil é achar o parêntese certo.", "Quem erra a ordem manda gente demais para o TI. Eu sofro."},
        {"Duas frases diferentes podem dizer a mesma coisa. Eu uso isso no TI.", "Mas trocar P por Q no lugar errado muda tudo."},
        {"Em sala de verdade-ou-mentira, comece supondo que um deles é sincero.", "O resto cai em cadeia, como dominó."},
        {"Todo registro vira regra quando entra no treino.", "Um único dado indevido e a regra inteira cai."},
        {"Do 'se' sai o 'então', e do 'então' sai o 'portanto'.", "Se a consequência é falsa, quem a causou também é."},
        {"Dois requisitos, uma resposta. Quando um é certo, o resto se organiza.", "Comece pelo que não tem como ser diferente."},
        {"Desenhe os conjuntos. Dentro, fora: tudo vira círculo.", "Quando a figura incomoda, ela está dizendo a verdade."},
        {"O Renê não gosta de ser contrariado. Fatos são contrários o bastante.", "Procure o par: uma coisa e o seu oposto, na mesma resposta."}}};

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

#define NPC_POSE_DIRECTION_COUNT 3
static const int npc_pose_directions[NPC_POSE_DIRECTION_COUNT] = {SOUTH, SOUTHEAST, SOUTHWEST};

typedef struct {
    Vector2 position;
    int direction;
    int seat;                 // Índice em npc_definitions do lugar (posição, direção e se está sentado) que ocupa hoje
    float blocked_time;
    int movement_sign;
    float walk_time;
    Texture2D walk_frames[DIRECTION_COUNT][WALK_FRAME_COUNT];
    bool walk_loaded[DIRECTION_COUNT];
    Texture2D pose_textures[DIRECTION_COUNT];   // Só as direções de npc_pose_directions são carregadas
} NpcState;

static NpcState npc_states[NPC_COUNT];
static bool npcs_loaded = false;

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

int npc_get_daily_hint_owner(int day)
{
    int playable_day_index = npc_get_playable_day_index(day);
    return playable_day_index < 0 ? -1 : playable_day_index % NPC_COUNT;
}

bool npc_has_hint(int npc_id, int day)
{
    return npc_id >= 0 && npc_id < NPC_COUNT && npc_id == npc_get_daily_hint_owner(day);
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
    if (npc_id < 0 || npc_id >= NPC_COUNT) return false;
    // Sem os NPCs carregados, cada um está no seu lugar de origem
    return !npc_definitions[npcs_loaded ? npc_states[npc_id].seat : npc_id].seated_pose;
}

int npc_get_dialogue_line_count(int npc_id, int day, bool first_today)
{
    if (npc_get_day_line(npc_id, day, 0) == NULL) return 0;

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

    compute_seats_for_day(day, seats);
    for (int npc = 0; npc < NPC_COUNT; npc++)
    {
        const NpcDefinition *seat = &npc_definitions[seats[npc]];

        npc_states[npc].seat = seats[npc];
        npc_states[npc].position = (Vector2){map_center.x + seat->position.x * scale,
                                              map_center.y + seat->position.y * scale};
        npc_states[npc].direction = seat->facing;
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

        if (definition->movement == NPC_STATIONARY)
        {
            // Como o lugar muda a cada dia, o NPC carrega a pose de todas as direções que os lugares usam
            for (int pose = 0; pose < NPC_POSE_DIRECTION_COUNT; pose++)
            {
                int direction = npc_pose_directions[pose];
                const char *path = TextFormat("%s%s/walk/%s-0.png", PERSONAGENS_PATH,
                                              definition->sprite_folder, direction_names[direction]);

                if (!FileExists(path))
                {
                    TraceLog(LOG_ERROR, "Pose do NPC %s nao encontrada: %s", definition->name, path);
                    all_frames_loaded = false;
                    continue;
                }

                state->pose_textures[direction] = LoadTexture(path);
                if (state->pose_textures[direction].id == 0) all_frames_loaded = false;
            }
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
    return all_frames_loaded;
}

static bool position_is_in_movement_area(int npc_id, Vector2 position, float scale, Vector2 map_center)
{
    Rectangle area = scale_reference_rect(npc_definitions[npc_id].movement_area, scale, map_center);
    return position.x >= area.x && position.y >= area.y &&
           position.x <= area.x + area.width && position.y <= area.y + area.height;
}

void update_npcs(float delta_time, float scale, Vector2 map_center, Rectangle map_bounds,
                 const Rectangle *obstacles, int obstacle_count, Rectangle player_hitbox)
{
    if (!npcs_loaded || scale <= 0.0f) return;

    for (int i = 0; i < NPC_COUNT; i++)
    {
        NpcState *state = &npc_states[i];
        const NpcDefinition *definition = &npc_definitions[i];
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

        if (!position_is_in_movement_area(i, candidate, scale, map_center) ||
            collides_with_any(get_npc_hitbox_at(candidate, scale), obstacles, obstacle_count) ||
            !CheckCollisionRecs(get_npc_hitbox_at(candidate, scale), map_bounds) ||
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
            if (other != i && npc_is_moving(other) &&
                CheckCollisionRecs(candidate_hitbox, npc_get_hitbox(other, scale)))
            {
                blocked_by_npc = true;
                break;
            }
        }

        if (blocked_by_npc)
        {
            state->blocked_time += delta_time;
            if (state->blocked_time < 2.0f)
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
    const NpcState *state = &npc_states[npc_id];
    float size = NPC_SIZE_AT_REFERENCE * scale;
    Texture2D texture;
    Rectangle source;
    Rectangle destination;

    if (!npc_is_moving(npc_id))
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
        if (!state->walk_loaded[state->direction]) return;
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
        npc_states[i] = (NpcState){0};
    }

    npcs_loaded = false;
}
