#include "render_component.h"
#include <string.h>

void render_comp_data_buffer_init(struct render_comp_data_buffer* rcdb)
{
    vector_init(&rcdb->buffer, sizeof(struct render_component));
    vector_init(&rcdb->entities, sizeof(entity_t));
}

void render_comp_data_buffer_destroy(struct render_comp_data_buffer* rcdb)
{
    vector_destroy(&rcdb->entities);
    vector_destroy(&rcdb->buffer);
}

struct render_component* render_component_create(struct render_comp_data_buffer* rcdb, entity_t e)
{
    /* Append empty value */
    struct render_component rc;
    memset(&rc, 0, sizeof(rc));
    vector_append(&rcdb->buffer, &rc);
    vector_append(&rcdb->entities, &e);
    return vector_at(&rcdb->buffer, rcdb->buffer.size - 1);
}

struct render_component* render_component_lookup(struct render_comp_data_buffer* rcdb, entity_t e)
{
    for (size_t i = 0; i < rcdb->entities.size; ++i) {
        entity_t stored_entity = *(entity_t*)vector_at(&rcdb->entities, i);
        if (stored_entity == e) {
            return vector_at(&rcdb->buffer, i);
        }
    }
    return 0;
}

void render_component_destroy(struct render_comp_data_buffer* rcdb, entity_t e)
{
    for (size_t i = 0; i < rcdb->entities.size; ++i) {
        entity_t stored_entity = *(entity_t*)vector_at(&rcdb->entities, i);
        if (stored_entity == e) {
            vector_remove(&rcdb->buffer, i);
            vector_remove(&rcdb->entities, i);
            return;
        }
    }
}
