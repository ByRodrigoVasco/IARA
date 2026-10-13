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
