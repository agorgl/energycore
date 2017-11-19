#include "world.h"
#include "components.h"

struct world* world_create()
{
    struct world* wrld = malloc(sizeof(struct world));
    wrld->ecs = ecs_init();
    register_component_maps(wrld->ecs);
    return wrld;
}

void world_update(struct world* wrld, float dt)
{
    (void) dt, (void) wrld;
}

void world_destroy(struct world* wrld)
{
    ecs_destroy(wrld->ecs);
    free(wrld);
}
