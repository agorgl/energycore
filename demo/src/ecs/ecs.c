#include "ecs.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct entity_data {
    uint64_t component_mask;
};

struct ecs {
    /* Entity pool */
    struct slot_map entity_pool;
    /* Component maps */
    struct component_map {
        component_type_t type;
        /* Maps from component references to component data */
        struct slot_map map;
    }* component_maps;
    size_t cap_component_maps;
    size_t num_component_maps;
};

static void initialize_allocated_component_maps(struct component_map* cmaps, size_t start, size_t end) {
    for (size_t i = start; i < end; ++i) {
        struct component_map* cm = &cmaps[i];
        cm->type = INVALID_COMPONENT_TYPE;
    }
}

ecs_t ecs_init()
{
    struct ecs* ecs = calloc(1, sizeof(struct ecs));
    slot_map_init(&ecs->entity_pool, sizeof(struct entity_data));
    ecs->cap_component_maps = 4;
    ecs->num_component_maps = 0;
    ecs->component_maps = calloc(ecs->cap_component_maps, sizeof(struct component_map));
    initialize_allocated_component_maps(ecs->component_maps, ecs->num_component_maps, ecs->cap_component_maps);
    return ecs;
}

void ecs_destroy(ecs_t ecs)
{
    for (size_t i = 0; i < ecs->num_component_maps; ++i)
        slot_map_destroy(&ecs->component_maps[i].map);
    free(ecs->component_maps);
    slot_map_destroy(&ecs->entity_pool);
    free(ecs);
}

entity_t entity_create(ecs_t ecs)
{
    struct entity_data ed;
    ed.component_mask = 0;
    sm_key k = slot_map_insert(&ecs->entity_pool, &ed);
    return k;
}

void entity_remove(ecs_t ecs, entity_t e)
{
    slot_map_remove(&ecs->entity_pool, e);
}

int entity_exists(ecs_t ecs, entity_t e)
{
    return !!(slot_map_lookup(&ecs->entity_pool, e));
}

entity_t entity_at(ecs_t ecs, size_t idx)
{
    return slot_map_data_to_key(&ecs->entity_pool, idx);
}

size_t entity_total(ecs_t ecs)
{
    return ecs->entity_pool.size;
}

static inline struct component_map* find_component_map(ecs_t ecs, component_type_t t)
{
    for (size_t i = 0; i < ecs->num_component_maps; ++i)
        if (ecs->component_maps[i].type == t)
            return &ecs->component_maps[i];
    return 0;
}

void component_map_register(ecs_t ecs, component_type_t t, size_t component_size)
{
    if (!find_component_map(ecs, t)) {
        if (ecs->cap_component_maps - ecs->num_component_maps < 1) {
            ecs->cap_component_maps = ecs->cap_component_maps * 2 + 1;
            ecs->component_maps = realloc(ecs->component_maps, ecs->cap_component_maps * sizeof(struct component_map));
            initialize_allocated_component_maps(ecs->component_maps, ecs->num_component_maps, ecs->cap_component_maps);
        }
        struct component_map* cm = &ecs->component_maps[ecs->num_component_maps++];
        slot_map_init(&cm->map, component_size);
        cm->type = t;
    }
}

void component_map_unregister(ecs_t ecs, component_type_t t)
{
    (void) ecs; (void) t;
    assert(0 && "Unimplemented");
}

void* component_add(ecs_t ecs, entity_t e, component_type_t t, void* data)
{
    struct component_map* cm = find_component_map(ecs, t);
    if (entity_exists(ecs, e) && cm) {
        struct entity_data* ed = slot_map_lookup(&ecs->entity_pool, e);
        ed->component_mask |= 1 << t;
        return slot_map_foreign_add(&cm->map, e, data);
    }
    return 0;
}

void* component_lookup(ecs_t ecs, entity_t e, component_type_t t)
{
    struct component_map* cm = find_component_map(ecs, t);
    if (entity_exists(ecs, e) && cm) {
        struct entity_data* ed = slot_map_lookup(&ecs->entity_pool, e);
        if (ed->component_mask & (1 << t))
            return slot_map_lookup(&cm->map, e);
    }
    return 0;
}

int component_remove(ecs_t ecs, entity_t e, component_type_t t)
{
    (void) ecs; (void) e; (void) t;
    assert(0 && "Unimplemented");
    return 0;
}

int entity_valid(entity_t e)
{
    return slot_map_key_valid(e);
}

entity_t component_parent(ecs_t ecs, component_type_t t, size_t data_idx)
{
    struct component_map* cm = find_component_map(ecs, t);
    if (cm)
        return slot_map_data_to_key(&cm->map, data_idx);
    return INVALID_ENTITY;
}

size_t components_count(ecs_t ecs, component_type_t t)
{
    struct component_map* cm = find_component_map(ecs, t);
    if (cm)
        return cm->map.size;
    return 0;
}

void* components_get(ecs_t ecs, component_type_t t)
{
    struct component_map* cm = find_component_map(ecs, t);
    if (cm)
        return cm->map.data;
    return 0;
}
