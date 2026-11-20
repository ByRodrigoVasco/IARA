#include <math.h>

#include "office_symptoms.h"

// Tamanho do office.png. Todas as posições abaixo são em pixels dessa imagem
// (canto superior esquerdo = 0, 0), medidas sobre o asset.
#define OFFICE_MAP_WIDTH 1062.0f

// ---------------------------------------------------------------- automação
#define COLD_LIGHT_RED_LOSS 0.22f     // Quanto de vermelho a luz perde na automação máxima
#define COLD_LIGHT_GREEN_LOSS 0.08f   // Quanto de verde ela perde (o azul fica)
#define FLICKER_PERIOD 3.7f           // Segundos entre um ciclo de falhas e o próximo
#define FLICKER_DIM 0.78f             // Brilho durante a falha (1 = sem falha)

// Monitores das mesas: o primeiro grupo (superior esquerdo) e os deslocamentos para os outros três.
static const Rectangle monitor_screens[] = {
    {287.0f, 242.0f, 39.0f, 21.0f},  // monitor da esquerda
    {385.0f, 242.0f, 39.0f, 21.0f},  // monitor da direita
};
static const Vector2 desk_group_offsets[] = {
    {0.0f, 0.0f}, {338.0f, 0.0f}, {0.0f, 255.0f}, {338.0f, 255.0f}
};
static const Rectangle server_racks = {919.0f, 20.0f, 130.0f, 80.0f};
#define MONITOR_GLOW_COLOR_RGB 90, 200, 255

// ---------------------------------------------------------------- percepção
static const Rectangle board_check_people = {509.0f, 66.0f, 13.0f, 12.0f}; // ✓ do "PESSOAS"
static const Rectangle board_check_data = {509.0f, 53.0f, 13.0f, 12.0f};   // ✓ do "DADOS"
static const Vector2 mural_notes[] = {
    {58.0f, 248.0f}, {77.0f, 249.0f}, {95.0f, 250.0f},
    {58.0f, 267.0f}, {79.0f, 265.0f}, {96.0f, 268.0f}
};
static const int mural_threads[][2] = {{0, 4}, {4, 2}, {1, 3}, {3, 5}, {1, 5}};
static const Rectangle corridor_door_gap = {777.0f, 113.0f, 51.0f, 9.0f};  // Logo abaixo da porta do corredor
#define PERCEPTION_RED_RGB 200, 20, 30

// ---------------------------------------------------------------- vínculo
typedef struct {
    Vector2 center;
    float radius;
    SymptomLevel from; // Nível a partir do qual a luz acende
} WarmLight;

static const WarmLight warm_lights[] = {
    {{140.0f, 668.0f}, 95.0f, SYMPTOM_MILD},    // sofá e poltronas
    {{235.0f, 85.0f}, 70.0f, SYMPTOM_STRONG},   // copa
    {{730.0f, 105.0f}, 55.0f, SYMPTOM_STRONG},  // bebedouro
};
#define WARM_LIGHT_RGB 255, 180, 100
#define WARM_LIGHT_MAX_ALPHA 70.0f

// ---------------------------------------------------------------- desempenho
typedef struct {
    Vector2 position;
    float rotation;
    SymptomLevel from;
} FloorPaper;

// No chão livre abaixo das mesas da direita, perto da mesa do jogador.
static const FloorPaper floor_papers[] = {
    {{612.0f, 652.0f}, -12.0f, SYMPTOM_MILD},
    {{668.0f, 676.0f}, 18.0f, SYMPTOM_MILD},
    {{742.0f, 658.0f}, -4.0f, SYMPTOM_MILD},
    {{700.0f, 702.0f}, 31.0f, SYMPTOM_STRONG},
    {{785.0f, 688.0f}, -22.0f, SYMPTOM_STRONG},
    {{560.0f, 690.0f}, 9.0f, SYMPTOM_STRONG},
    {{640.0f, 714.0f}, -35.0f, SYMPTOM_STRONG},
};
static const Rectangle printer_screen = {70.0f, 317.0f, 22.0f, 20.0f};
#define PAPER_WIDTH 12.0f
#define PAPER_HEIGHT 15.0f

#define ARRAY_COUNT(array) ((int)(sizeof(array) / sizeof((array)[0])))

// Converte um retângulo do office.png para a tela.
static Rectangle to_screen(Rectangle office, Rectangle map)
{
    float scale = office.width / OFFICE_MAP_WIDTH;

    return (Rectangle){office.x + (map.x * scale), office.y + (map.y * scale), map.width * scale, map.height * scale};
}

static Vector2 point_to_screen(Rectangle office, Vector2 map)
{
    float scale = office.width / OFFICE_MAP_WIDTH;

    return (Vector2){office.x + (map.x * scale), office.y + (map.y * scale)};
}

// Aplica a opacidade da cena a um alfa próprio do efeito.
static unsigned char faded(float effect_alpha, unsigned char scene_alpha)
{
    float value = effect_alpha * ((float)scene_alpha / 255.0f);

    if (value < 0.0f) value = 0.0f;
    if (value > 255.0f) value = 255.0f;

    return (unsigned char)value;
}

// Pulsação lenta entre 0 e 1.
static float pulse(double time, float speed, float phase)
{
    return 0.5f + (0.5f * sinf((float)time * speed + phase));
}

// 1 no normal, FLICKER_DIM durante as quedas curtas da luz.
static float flicker_factor(double time)
{
    float t = fmodf((float)time, FLICKER_PERIOD);

    if (t < 0.12f) return FLICKER_DIM;
    if (t > 0.30f && t < 0.38f) return FLICKER_DIM + ((1.0f - FLICKER_DIM) / 2.0f);

    return 1.0f;
}

Color office_symptoms_light_tint(const SceneSymptoms *symptoms, unsigned char brightness, unsigned char alpha, double time)
{
    float cold = symptoms->automation_intensity;
    float light = (float)brightness;

    if (symptoms->automation == SYMPTOM_STRONG) light *= flicker_factor(time);

    return (Color){
        (unsigned char)(light * (1.0f - (COLD_LIGHT_RED_LOSS * cold))),
        (unsigned char)(light * (1.0f - (COLD_LIGHT_GREEN_LOSS * cold))),
        (unsigned char)light,
        alpha
    };
}

// Automação: os monitores acendem sozinhos, a IARA trabalhando nas mesas.
static void draw_automation(const SceneSymptoms *symptoms, Rectangle office, unsigned char alpha, double time)
{
    if (symptoms->automation == SYMPTOM_NONE) return;

    float scale = office.width / OFFICE_MAP_WIDTH;
    int screens_per_group = (symptoms->automation == SYMPTOM_STRONG) ? ARRAY_COUNT(monitor_screens) : 1;
    float base = (symptoms->automation == SYMPTOM_STRONG) ? 120.0f : 80.0f;

    BeginBlendMode(BLEND_ADDITIVE);

    for (int group = 0; group < ARRAY_COUNT(desk_group_offsets); group++)
    {
        for (int i = 0; i < screens_per_group; i++)
        {
            Rectangle map = monitor_screens[i];

            map.x += desk_group_offsets[group].x;
            map.y += desk_group_offsets[group].y;

            Rectangle screen = to_screen(office, map);
            float glow = base * (0.75f + (0.25f * pulse(time, 1.6f, (float)(group * 2 + i))));
            Rectangle halo = {screen.x - (4.0f * scale), screen.y - (4.0f * scale),
                              screen.width + (8.0f * scale), screen.height + (8.0f * scale)};

            DrawRectangleRec(halo, (Color){MONITOR_GLOW_COLOR_RGB, faded(glow * 0.25f, alpha)});
            DrawRectangleRec(screen, (Color){MONITOR_GLOW_COLOR_RGB, faded(glow, alpha)});
        }
    }

    if (symptoms->automation == SYMPTOM_STRONG)
    {
        float glow = 40.0f + (50.0f * pulse(time, 2.2f, 0.0f));

        DrawRectangleRec(to_screen(office, server_racks), (Color){MONITOR_GLOW_COLOR_RGB, faded(glow, alpha)});
    }

    EndBlendMode();
}

// Um X vermelho de caneta sobre um ✓ do quadro de valores.
static void draw_board_cross(Rectangle office, Rectangle check, unsigned char alpha)
{
    float scale = office.width / OFFICE_MAP_WIDTH;
    Rectangle mark = to_screen(office, check);
    float thickness = 2.0f * scale;
    Color red = {PERCEPTION_RED_RGB, faded(230.0f, alpha)};

    DrawLineEx((Vector2){mark.x, mark.y}, (Vector2){mark.x + mark.width, mark.y + mark.height}, thickness, red);
    DrawLineEx((Vector2){mark.x + mark.width, mark.y}, (Vector2){mark.x, mark.y + mark.height}, thickness, red);
}

// Percepção: o jogador enxerga o que não bate no discurso da empresa.
static void draw_perception(const SceneSymptoms *symptoms, Rectangle office, unsigned char alpha, double time)
{
    if (symptoms->perception == SYMPTOM_NONE) return;

    draw_board_cross(office, board_check_people, alpha);

    if (symptoms->perception != SYMPTOM_STRONG) return;

    draw_board_cross(office, board_check_data, alpha);

    // Mural: os bilhetes ligados por linha vermelha, como num quadro de investigação.
    float scale = office.width / OFFICE_MAP_WIDTH;
    Color thread = {PERCEPTION_RED_RGB, faded(210.0f, alpha)};

    for (int i = 0; i < ARRAY_COUNT(mural_threads); i++)
    {
        Vector2 from = point_to_screen(office, mural_notes[mural_threads[i][0]]);
        Vector2 to = point_to_screen(office, mural_notes[mural_threads[i][1]]);

        DrawLineEx(from, to, 1.0f * scale, thread);
    }

    for (int i = 0; i < ARRAY_COUNT(mural_notes); i++)
    {
        DrawCircleV(point_to_screen(office, mural_notes[i]), 1.5f * scale, thread);
    }

    // Luz vermelha por baixo da porta do corredor: tem alguém (ou algo) do outro lado.
    Rectangle gap = to_screen(office, corridor_door_gap);
    float glow = 70.0f + (60.0f * pulse(time, 1.2f, 0.0f));

    BeginBlendMode(BLEND_ADDITIVE);
    DrawRectangleGradientV((int)gap.x, (int)gap.y, (int)gap.width, (int)gap.height,
                           (Color){PERCEPTION_RED_RGB, faded(glow, alpha)}, (Color){PERCEPTION_RED_RGB, 0});
    EndBlendMode();
}

// Vínculo: luz quente onde as pessoas se encontram.
static void draw_human_bond(const SceneSymptoms *symptoms, Rectangle office, unsigned char alpha)
{
    if (symptoms->human_bond == SYMPTOM_NONE) return;

    float scale = office.width / OFFICE_MAP_WIDTH;
    float strength = WARM_LIGHT_MAX_ALPHA * symptoms->human_bond_intensity;

    BeginBlendMode(BLEND_ADDITIVE);

    for (int i = 0; i < ARRAY_COUNT(warm_lights); i++)
    {
        if (symptoms->human_bond < warm_lights[i].from) continue;

        DrawCircleGradient(point_to_screen(office, warm_lights[i].center),
                           warm_lights[i].radius * scale,
                           (Color){WARM_LIGHT_RGB, faded(strength, alpha)}, (Color){WARM_LIGHT_RGB, 0});
    }

    EndBlendMode();
}

// Desempenho baixo: pendências espalhadas pelo chão e a impressora com erro.
static void draw_performance(const SceneSymptoms *symptoms, Rectangle office, unsigned char alpha, double time)
{
    if (symptoms->performance == SYMPTOM_NONE) return;

    float scale = office.width / OFFICE_MAP_WIDTH;
    Color paper = {236, 232, 220, faded(255.0f, alpha)};
    Color ink = {120, 120, 130, faded(255.0f, alpha)};

    for (int i = 0; i < ARRAY_COUNT(floor_papers); i++)
    {
        if (symptoms->performance < floor_papers[i].from) continue;

        Vector2 center = point_to_screen(office, floor_papers[i].position);
        Rectangle sheet = {center.x, center.y, PAPER_WIDTH * scale, PAPER_HEIGHT * scale};
        Vector2 origin = {sheet.width / 2.0f, sheet.height / 2.0f};
        float rotation = floor_papers[i].rotation;

        // Sombra, folha e duas linhas de texto. As linhas giram em volta do centro da folha.
        DrawRectanglePro((Rectangle){sheet.x + scale, sheet.y + scale, sheet.width, sheet.height}, origin, rotation, (Color){0, 0, 0, faded(70.0f, alpha)});
        DrawRectanglePro(sheet, origin, rotation, paper);

        float radians = rotation * (3.14159265f / 180.0f);
        float line_offsets[] = {-3.0f * scale, 1.0f * scale};

        for (int line = 0; line < 2; line++)
        {
            Vector2 line_center = {center.x - (line_offsets[line] * sinf(radians)), center.y + (line_offsets[line] * cosf(radians))};

            DrawRectanglePro((Rectangle){line_center.x, line_center.y, sheet.width * 0.6f, scale},
                             (Vector2){sheet.width * 0.3f, scale / 2.0f}, rotation, ink);
        }
    }

    if (symptoms->performance == SYMPTOM_STRONG && fmodf((float)time, 1.0f) < 0.5f)
    {
        // Pintada por cima, não somada: somar ao azul da tela deixaria a luz rosa
        DrawRectangleRec(to_screen(office, printer_screen), (Color){220, 30, 30, faded(200.0f, alpha)});
    }
}

void draw_office_symptoms(const SceneSymptoms *symptoms, Rectangle office, unsigned char alpha, double time)
{
    draw_human_bond(symptoms, office, alpha);
    draw_performance(symptoms, office, alpha, time);
    draw_perception(symptoms, office, alpha, time);
    draw_automation(symptoms, office, alpha, time);
}
