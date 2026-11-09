// Compilar e rodar: gcc code/tests/test_characters.c code/src/game/characters.c -Icode/src/game -Icode/src/core -o test_characters.exe -lraylib -lopengl32 -lgdi32 -lwinmm && ./test_characters.exe
#include <assert.h>
#include <stdio.h>
#include "characters.h"

int main(void)
{
    assert(get_walk_frame(0.0f) == 0);                                  // começa no primeiro frame
    assert(get_walk_frame(WALK_FRAME_TIME * 1.5f) == 1);                // passou um frame
    assert(get_walk_frame(WALK_FRAME_TIME * 3.5f) == 3);                // último frame
    assert(get_walk_frame(WALK_FRAME_TIME * 4.5f) == 0);                // volta ao primeiro
    assert(get_walk_frame(WALK_FRAME_TIME * 5.5f) == 1);                // e segue o ciclo

    // Personagem sem os arquivos: não carrega nada e avisa com false (não precisa de janela)
    Texture2D frames[WALK_FRAME_COUNT];
    assert(!load_walk_frames("Inexistente", EAST, frames));

    // Todo personagem tem os 4 frames nas 8 direções (rodar na raiz do repositório)
    const char *characters[] = {"Anderson", "Estagiarios", "Julio", "JV", "Laura", "Nicollas", "Vasco", "Zito"};
    for (int c = 0; c < 8; c++)
    {
        for (int direction = 0; direction < DIRECTION_COUNT; direction++)
        {
            assert(has_walk_frames(characters[c], direction));
        }
    }

    printf("Todos os testes passaram\n");
    return 0;
}
