#include "raylib.h"

#define EAST      0
#define SOUTH     1
#define NORTH     2
#define WEST      3
#define SOUTHEAST 4
#define NORTHWEST 5
#define SOUTHWEST 6
#define NORTHEAST 7
#define DIRECTION_COUNT 8

typedef Texture2D char_textures[DIRECTION_COUNT];

void initialize_estagiario_1_textures(char_textures out);