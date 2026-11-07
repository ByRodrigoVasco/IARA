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

static const char *const anderson_dialogue[] = {
    "O relatório chegou antes da hora.",
    "Mesmo assim, confira de onde vieram os dados.",
    "Pressa não transforma suposição em evidência."};
static const char *const julio_dialogue[] = {
    "A equipe está correndo para fechar as tarefas.",
    "Vale a pena conferir se a conclusão segue das premissas.",
    "Uma pergunta bem feita pode revelar o que ficou de fora."};
static const char *const jv_dialogue[] = {
    "Tem gente tratando a sugestão da IARA como uma ordem.",
    "A ferramenta pode ajudar sem decidir por você.",
    "Leia a justificativa antes de confiar no resultado."};
static const char *const laura_dialogue[] = {
    "Os dados deste chamado envolvem pessoas de verdade.",
    "Pense em quem pode ser afetado por cada decisão.",
    "Se algo parecer estranho, investigue antes de enviar."};
static const char *const nicollas_dialogue[] = {
    "Ei, você pode me ajudar com um trabalho no porão hoje?",
    "Preciso de uma ajuda com uma coisa e não confio só em mim pra checar.",
    "Quando terminar, me conta o que você encontrou."};
static const char *const vasco_dialogue[] = {
    "Nem todo chamado urgente precisa passar na frente.",
    "A regra de prioridade depende do que aconteceu de fato.",
    "Separe os dados importantes das suposições."};
static const char *const zito_dialogue[] = {
    "Às vezes, duas afirmações parecem verdadeiras ao mesmo tempo.",
    "Procure um exemplo que teste cada possibilidade.",
    "Se uma regra contradiz outra, vale investigar a origem."};

static const char *const anderson_hint[] = {
    "Separe o que o enunciado afirma do que ele apenas sugere.",
    "Confira se cada conclusão tem apoio nas regras.",
    "Tente testar a ideia com um exemplo simples."};
static const char *const julio_hint[] = {
    "Comece identificando as premissas antes de tirar uma conclusão.",
    "Verifique se a conclusão realmente decorre da regra.",
    "Teste o raciocínio considerando uma exceção."};
static const char *const jv_hint[] = {
    "Leia a justificativa e confira cada etapa do raciocínio.",
    "Uma resposta confiante ainda pode partir de uma premissa falsa.",
    "Tente reformular a regra com suas próprias palavras."};
static const char *const laura_hint[] = {
    "Observe quais pessoas podem ser afetadas por cada alternativa.",
    "Confira se os dados necessários estão realmente presentes.",
    "Pense no que mudaria se uma premissa fosse diferente."};
static const char *const nicollas_hint[] = {
    "Não pule etapas: confronte o chamado com cada critério.",
    "Procure uma informação que possa contradizer a primeira impressão.",
    "Uma dica ajuda a investigar, mas a conclusão continua sendo sua."};
static const char *const vasco_hint[] = {
    "Distinguir urgência de importância pode ajudar a organizar o caso.",
    "Confira qual regra se aplica antes de escolher uma alternativa.",
    "Tente encontrar um contraexemplo para a conclusão."};
static const char *const zito_hint[] = {
    "Uma possibilidade e sua oposta podem ser testadas separadamente.",
    "Volte às premissas quando duas conclusões parecerem plausíveis.",
    "Procure o detalhe que distingue os casos."};

static const NpcDefinition npc_definitions[NPC_COUNT] = {
    [NPC_ANDERSON] = {
        "ANDERSON", "Anderson", {-258.0f, -263.0f}, {0}, NPC_STATIONARY, 0.0f, SOUTH, true,
        anderson_dialogue, 3, NULL, 0, anderson_hint, 3},
    [NPC_JULIO] = {
        "JULIO", "Julio", {280.0f, -90.0f}, {0}, NPC_STATIONARY, 0.0f, SOUTHEAST, false,
        julio_dialogue, 3, NULL, 0, julio_hint, 3},
    [NPC_JV] = {
        "JV", "JV", {310.0f, -120.0f}, {0}, NPC_STATIONARY, 0.0f, SOUTHWEST, false,
        jv_dialogue, 3, NULL, 0, jv_hint, 3},
    [NPC_LAURA] = {
        "LAURA", "Laura", {-262.0f, 0.0f}, {0}, NPC_STATIONARY, 0.0f, SOUTH, true,
        laura_dialogue, 3, NULL, 0, laura_hint, 3},
    [NPC_NICOLLAS] = {
        "NICOLLAS", "Nicollas", {76.0f, -7.0f}, {0}, NPC_STATIONARY, 0.0f, SOUTH, true,
        nicollas_dialogue, 3, NULL, 0, nicollas_hint, 3},
    [NPC_VASCO] = {
        "VASCO", "Vasco", {178.0f, -7.0f}, {0}, NPC_STATIONARY, 0.0f, SOUTH, true,
        vasco_dialogue, 3, NULL, 0, vasco_hint, 3},
    [NPC_ZITO] = {
        "ZITO", "Zito", {325.0f, -90.0f}, {0}, NPC_STATIONARY, 0.0f, SOUTHWEST, false,
        zito_dialogue, 3, NULL, 0, zito_hint, 3}};

typedef struct {
    Vector2 position;
    int direction;
    float blocked_time;
    int movement_sign;
    float walk_time;
    Texture2D walk_frames[DIRECTION_COUNT][WALK_FRAME_COUNT];
    bool walk_loaded[DIRECTION_COUNT];
    Texture2D pose_texture;
} NpcState;

static NpcState npc_states[NPC_COUNT];
static bool npcs_loaded = false;

static const NpcSpecialConversation *get_special_conversation(int npc_id, int day)
{
    const NpcDefinition *definition = npc_get_definition(npc_id);

    if (definition == NULL || day < 1) return NULL;

    for (int i = 0; i < definition->special_conversation_count; i++)
    {
        if (definition->special_conversations[i].day == day) return &definition->special_conversations[i];
    }

    return NULL;
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

int npc_get_daily_hint_owner(int day)
{
    if (day < 1) return -1;

    int playable_day_index = 0;
    for (int current_day = 1; current_day <= day; current_day++)
    {
        if (calendar_get_day_type(current_day) != PLAYABLE_DAY) continue;
        if (current_day == day) return playable_day_index % NPC_COUNT;
        playable_day_index++;
    }

    return -1;
}

bool npc_has_hint(int npc_id, int day)
{
    return npc_id >= 0 && npc_id < NPC_COUNT && npc_id == npc_get_daily_hint_owner(day);
}

bool npc_is_moving(int npc_id)
{
    const NpcDefinition *definition = npc_get_definition(npc_id);
    return definition != NULL && definition->movement != NPC_STATIONARY;
}

bool npc_blocks_player(int npc_id)
{
    const NpcDefinition *definition = npc_get_definition(npc_id);
    return definition != NULL && !definition->seated_pose;
}

int npc_get_dialogue_line_count(int npc_id, int day)
{
    const NpcDefinition *definition = npc_get_definition(npc_id);
    if (definition == NULL) return 0;

    const NpcSpecialConversation *special = get_special_conversation(npc_id, day);
    int line_count = special != NULL ? special->line_count : definition->dialogue_line_count;

    if (npc_has_hint(npc_id, day)) line_count += definition->hint_line_count;
    return line_count;
}

const char *npc_get_dialogue_line(int npc_id, int day, int line_index)
{
    const NpcDefinition *definition = npc_get_definition(npc_id);
    if (definition == NULL || line_index < 0) return NULL;

    const NpcSpecialConversation *special = get_special_conversation(npc_id, day);
    const char *const *conversation = special != NULL ? special->lines : definition->dialogue;
    int conversation_line_count = special != NULL ? special->line_count : definition->dialogue_line_count;

    if (line_index < conversation_line_count) return conversation[line_index];

    int hint_line = line_index - conversation_line_count;
    if (!npc_has_hint(npc_id, day) || hint_line >= definition->hint_line_count) return NULL;
    return definition->hint[hint_line];
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

    bool all_frames_loaded = true;

    for (int i = 0; i < NPC_COUNT; i++)
    {
        const NpcDefinition *definition = &npc_definitions[i];
        NpcState *state = &npc_states[i];
        state->position = (Vector2){map_center.x + definition->position.x * scale,
                                    map_center.y + definition->position.y * scale};
        state->direction = definition->facing;
        state->blocked_time = 0.0f;
        state->movement_sign = 1;
        state->walk_time = 0.0f;

        if (definition->movement == NPC_STATIONARY)
        {
            static const char *const direction_names[DIRECTION_COUNT] = {
                "east", "south", "north", "west", "south-east",
                "north-west", "south-west", "north-east"};
            const char *path = TextFormat("%s%s/walk/%s-0.png", PERSONAGENS_PATH,
                                          definition->sprite_folder, direction_names[definition->facing]);
            if (!FileExists(path) && definition->facing == WEST)
            {
                path = TextFormat("%s%s/west.png", PERSONAGENS_PATH, definition->sprite_folder);
            }
            if (!FileExists(path))
            {
                TraceLog(LOG_ERROR, "Pose do NPC %s nao encontrada: %s", definition->name, path);
                all_frames_loaded = false;
                continue;
            }

            state->pose_texture = LoadTexture(path);
            if (state->pose_texture.id == 0) all_frames_loaded = false;
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
        texture = state->pose_texture;
        if (texture.id == 0) return;
        source = (Rectangle){0, 0, (float)texture.width,
                             texture.height * (npc_definitions[npc_id].seated_pose
                                                   ? (npc_id == NPC_LAURA ? 0.625f : 0.75f)
                                                   : 1.0f)};
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

static void draw_hint_bubble(int npc_id, float scale, unsigned char alpha)
{
    if (npc_id < 0 || npc_id >= NPC_COUNT) return;

    const NpcDefinition *definition = &npc_definitions[npc_id];
    const char *text = TextFormat("%s tem uma dica para você!", definition->name);
    float font_size = 19.0f * scale;
    float padding = 8.0f * scale;
    float text_width = MeasureTextEx(default_game_font, text, font_size, 0.0f).x;
    float bubble_width = text_width + padding * 2.0f;
    float bubble_height = 30.0f * scale;
    Vector2 position = npc_states[npc_id].position;
    Rectangle bubble = {position.x + (NPC_SIZE_AT_REFERENCE * scale - bubble_width) / 2.0f,
                        position.y - bubble_height - 5.0f * scale, bubble_width, bubble_height};

    Color bubble_color = {255, 255, 255, alpha};
    Color text_color = {0, 0, 0, alpha};
    DrawTriangle((Vector2){position.x + NPC_SIZE_AT_REFERENCE * scale / 2.0f - 6.0f * scale, bubble.y + bubble.height},
                 (Vector2){position.x + NPC_SIZE_AT_REFERENCE * scale / 2.0f + 6.0f * scale, bubble.y + bubble.height},
                 (Vector2){position.x + NPC_SIZE_AT_REFERENCE * scale / 2.0f, bubble.y + bubble.height + 7.0f * scale},
                 bubble_color);
    DrawRectangleRounded(bubble, 0.25f, 4, bubble_color);
    DrawTextEx(default_game_font, text, (Vector2){bubble.x + padding, bubble.y + (bubble.height - font_size) / 2.0f},
               font_size, 0.0f, text_color);
}

void draw_npcs(float scale, Color tint, int day, float player_y, bool behind_player,
               bool hint_bubble_unlocked)
{
    if (!npcs_loaded) return;

    for (int i = 0; i < NPC_COUNT; i++)
    {
        bool npc_is_behind = npc_states[i].position.y < player_y;
        if (npc_is_behind == behind_player) draw_npc(i, scale, tint);
    }

    if (!behind_player && hint_bubble_unlocked)
        draw_hint_bubble(npc_get_daily_hint_owner(day), scale, tint.a);
}

void draw_npc_portrait(int npc_id, Rectangle destination)
{
    if (!npcs_loaded || npc_id < 0 || npc_id >= NPC_COUNT) return;

    Texture2D texture = npc_is_moving(npc_id)
        ? npc_states[npc_id].walk_frames[SOUTH][0]
        : npc_states[npc_id].pose_texture;
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
        }
        if (npc_states[i].pose_texture.id > 0)
            UnloadTexture(npc_states[i].pose_texture);
        npc_states[i] = (NpcState){0};
    }

    npcs_loaded = false;
}
