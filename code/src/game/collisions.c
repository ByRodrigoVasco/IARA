#include "raylib.h"
#include "collisions.h"

bool is_mouse_hovering(Rectangle collision_rectangle)
{
    // Só considera o mouse quando a janela do jogo está em foco
    if (!IsWindowFocused())
    {
        return false;
    }

    Vector2 mouse_position = GetMousePosition();
    Rectangle mouse_pointer_rectangle = {mouse_position.x, mouse_position.y, 1, 1};

    return CheckCollisionRecs(mouse_pointer_rectangle, collision_rectangle);
}

bool collides_with_any(Rectangle hitbox, const Rectangle *obstacles, int count)
{
    for (int i = 0; i < count; i++)
    {
        if (CheckCollisionRecs(hitbox, obstacles[i])) return true;
    }
    return false;
}

Rectangle scale_reference_rect(Rectangle ref, float scale, Vector2 origin)
{
    // Posição e tamanho acompanham a escala; a origem só desloca a posição
    return (Rectangle){origin.x + ref.x * scale, origin.y + ref.y * scale,
                       ref.width * scale, ref.height * scale};
}

float move_axis(float position, float step, Rectangle hitbox, bool is_x_axis, const Rectangle *obstacles, int count)
{
    // Testa a hitbox já deslocada só naquele eixo
    if (is_x_axis) hitbox.x += step;
    else           hitbox.y += step;

    // Se bateu, desfaz o passo (a posição não muda)
    if (collides_with_any(hitbox, obstacles, count)) return position;

    return position + step;
}