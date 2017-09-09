#include "scene.h"
#include <stdlib.h>
#include "ecs/world.h"

struct scene* scene_create()
{
    struct scene* s = calloc(1, sizeof(struct scene));
    s->world = world_create();
    return s;
}

void scene_destroy(struct scene* s)
{
    world_destroy(s->world);
    free(s);
}
