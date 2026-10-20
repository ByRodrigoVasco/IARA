// Compilar e rodar: gcc code/tests/test_collisions.c code/src/game/collisions.c -Icode/src/game -o test_collisions.exe -lraylib -lopengl32 -lgdi32 -lwinmm && ./test_collisions.exe
#include <assert.h>
#include <stdio.h>
#include "collisions.h"

int main(void)
{
    Rectangle obstacles[] = {{0, 0, 10, 10}, {50, 50, 10, 10}};
    Rectangle hits_second = {55, 55, 5, 5};
    Rectangle hits_none = {20, 20, 5, 5};

    assert(collides_with_any(hits_second, obstacles, 2));
    assert(!collides_with_any(hits_none, obstacles, 2));
    assert(!collides_with_any(hits_second, obstacles, 0)); // sem obstáculos nunca bate

    // Escala 2 e origem (100, 200): posição e tamanho dobram, e a origem soma
    Rectangle scaled = scale_reference_rect((Rectangle){10, 20, 30, 40}, 2.0f, (Vector2){100, 200});
    assert(scaled.x == 120 && scaled.y == 240 && scaled.width == 60 && scaled.height == 80);

    printf("Todos os testes passaram\n");
    return 0;
}
