#include "rooms.h"
#include "rooms_data.h"

const room_def *room_get(room_id id)
{
    if (id < 0 || id >= ROOM_COUNT) return NULL;
    return &room_defs[id];
}

int room_door_touching(room_id id, Rectangle hitbox_reference)
{
    const room_def *room = room_get(id);
    if (!room) return -1;

    for (int i = 0; i < room->door_count; i++)
    {
        if (CheckCollisionRecs(room->doors[i].zone, hitbox_reference)) return i;
    }
    return -1;
}

int room_door_toward_office(room_id id)
{
    const room_def *room = room_get(id);
    if (!room || id == ROOM_ESCRITORIO) return -1;

    // Do corredor a porta é a do escritório; dos outros cômodos, a que volta para o corredor
    room_id target = (id == ROOM_CORREDOR) ? ROOM_ESCRITORIO : ROOM_CORREDOR;
    for (int i = 0; i < room->door_count; i++)
    {
        if (room->doors[i].to_room == target) return i;
    }
    return -1;
}
