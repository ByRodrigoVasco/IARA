#include "raylib.h"

bool is_mouse_hovering(Rectangle collision_rectangle)
{
    if(IsWindowFocused())
    { 
        Vector2 mouse_position = GetMousePosition();
        Rectangle mouse_pointer_rectangle = { mouse_position.x, mouse_position.y, 1, 1};
        
        if (CheckCollisionRecs(mouse_pointer_rectangle, collision_rectangle))
        {
            return true;
        }
    }
}