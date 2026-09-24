#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "game_state.h"
#include "macros.h"

#include "state_machine.h"

// Definição dos estados possíveis para transição de telas

void handle_state_machine()
{
    switch(current_game_state_variables.current_screen)
    {
        case PRE_MENU:
            printf("Transição para a tela de pre-menu\n");
            break;
        case MENU:
            printf("Transição para a tela de menu\n");
            break;
        case PLAYING:
            printf("Transição para a tela de dia jogavel\n");
            break;
            
        // IMPORTANTE: TALVEZ NÃO PRECISE DE FLASHING, JUMPING
        // PRINCIPALMENTE POR QUE DA PARA LIDAR DE OUTRAS FORMAS DENTRO DOS OUTROS LOOPS DE "PRE_MENU", "MENU" E "PLAYING"
        case FLASHING:
            printf("Transição para a tela de dia de flash\n");
            break;
        case JUMPING:
            printf("Transição para a tela de dia de salto\n");
            break;
        default:
            printf("Tipo de dia não reconhecido\n");
            break;
    }
}