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

    // Movimento por eixo: hitbox em (0,0) 10x10, obstáculo em (20,0) 10x10
    Rectangle wall[] = {{20, 0, 10, 10}};
    Rectangle hitbox = {0, 0, 10, 10};
    assert(move_axis(100, 5, hitbox, true, wall, 1) == 105);  // eixo livre é mantido
    assert(move_axis(100, 15, hitbox, true, wall, 1) == 100); // eixo bloqueado é desfeito
    assert(move_axis(100, 15, hitbox, false, wall, 1) == 115); // o outro eixo continua livre
    Rectangle inside = {15, 0, 10, 10};                       // já começa sobreposto ao obstáculo
    assert(move_axis(100, 1, inside, true, wall, 1) == 100);  // continua preso, sem andar

    printf("Todos os testes passaram\n");
    return 0;
}
