#include "world.h"

struct world* world_create()
{
    struct world* wrld = malloc(sizeof(struct world));
    entity_mgr_init(&wrld->emgr);
    transform_data_buffer_init(&wrld->transform_dbuf);
    render_comp_data_buffer_init(&wrld->render_comp_dbuf);
    return wrld;
}

void world_update(struct world* wrld, float dt)
{
    (void) dt;
    transform_data_buffer_gc(&wrld->transform_dbuf, &wrld->emgr);
}

void world_destroy(struct world* wrld)
{
    render_comp_data_buffer_destroy(&wrld->render_comp_dbuf);
    transform_data_buffer_destroy(&wrld->transform_dbuf);
    entity_mgr_destroy(&wrld->emgr);
    free(wrld);
}
