#include "ecs.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MAX_COMPONENTS 64

struct ecs {
    /* Maps from entity to component references */
    struct slot_map entity_map;
    /* Component maps */
    struct component_map {
        component_type_t type;
        struct slot_map map;
    }* component_maps;
    size_t cap_component_maps;
    size_t num_component_maps;
};

struct component_reference_table {
    component_t references[MAX_COMPONENTS];
    size_t num_components;
};

static inline void default_reference_table(struct component_reference_table* rt)
{
    memset(rt, 0, sizeof(*rt));
    for (size_t i = 0; i < MAX_COMPONENTS; ++i)
        rt->references[i].type = INVALID_COMPONENT_TYPE;
    rt->num_components = 0;
}

static void initialize_allocated_component_maps(struct component_map* cmaps, size_t start, size_t end) {
    for (size_t i = start; i < end; ++i) {
        struct component_map* cm = &cmaps[i];
        cm->type = INVALID_COMPONENT_TYPE;
    }
}

ecs_t ecs_init()
{
    struct ecs* ecs = calloc(1, sizeof(struct ecs));
    slot_map_init(&ecs->entity_map, sizeof(struct component_reference_table));
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
    slot_map_destroy(&ecs->entity_map);
    free(ecs);
}

entity_t entity_create(ecs_t ecs)
{
    struct component_reference_table rt;
    default_reference_table(&rt);
    sm_key k = slot_map_insert(&ecs->entity_map, &rt);
    return k;
}

void entity_remove(ecs_t ecs, entity_t e)
{
    slot_map_remove(&ecs->entity_map, e);
}

int entity_exists(ecs_t ecs, entity_t e)
{
    return !!(slot_map_lookup(&ecs->entity_map, e));
}

entity_t entity_at(ecs_t ecs, size_t idx)
{
    return slot_map_data_to_key(&ecs->entity_map, idx);
}

size_t entity_total(ecs_t ecs)
{
    return ecs->entity_map.size;
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

static inline component_t* find_component_reference(struct component_reference_table* rt, component_type_t t)
{
    for (size_t i = 0; i < rt->num_components; ++i)
        if (rt->references[i].type == t)
            return &rt->references[i];
    return 0;
}

component_t component_add(ecs_t ecs, entity_t e, component_type_t t, void* data)
{
    struct component_reference_table* rt = slot_map_lookup(&ecs->entity_map, e);
    if (rt) {
        if (!find_component_reference(rt, t)) {
            assert(rt->num_components + 1 < MAX_COMPONENTS);
            component_t* r = &rt->references[rt->num_components++];
            struct component_map* component_map = find_component_map(ecs, t);
            r->dref = slot_map_insert(&component_map->map, data);
            r->type = t;
            return *r;
        }
    }
    return INVALID_COMPONENT;
}

component_t component_lookup(ecs_t ecs, entity_t e, component_type_t t)
{
    struct component_reference_table* rt = slot_map_lookup(&ecs->entity_map, e);
    if (rt) {
        component_t* r = find_component_reference(rt, t);
        return r ? *r : INVALID_COMPONENT;
    }
    return INVALID_COMPONENT;
}

void* component_lookup_data(ecs_t ecs, component_t c)
{
    if (component_valid(c)) {
        struct component_map* cm = find_component_map(ecs, c.type);
        void* data = slot_map_lookup(&cm->map, c.dref);
        return data;
    }
    return 0;
}

int component_remove(ecs_t ecs, entity_t e, component_type_t t)
{
    (void) ecs; (void) e; (void) t;
    assert(0 && "Unimplemented");
    return 0;
}

int component_valid(component_t c)
{
    return slot_map_key_valid(c.dref) && c.type != INVALID_COMPONENT.type;
}
