#include "entity.h"
#include <assert.h>
#define MINIMUM_FREE_INDICES 1024

void entity_mgr_init(struct entity_mgr* emgr)
{
    vector_init(&emgr->generation, sizeof(uint16_t));
    queue_init(&emgr->free_indices, sizeof(uint64_t));
}

entity_t entity_create(struct entity_mgr* emgr)
{
    uint64_t idx;
    if (emgr->free_indices.size > MINIMUM_FREE_INDICES) {
        idx = *(uint64_t*) queue_front(&emgr->free_indices);
        queue_pop(&emgr->free_indices);
    } else {
        uint16_t val = 0;
        vector_append(&emgr->generation, &val);
        idx = emgr->generation.size - 1;
        assert(idx < (1LL << ENTITY_INDEX_BITS));
    }
    uint64_t gen = *(uint16_t*)vector_at(&emgr->generation, idx);
    return entity_make(idx, gen);
}

int entity_alive(struct entity_mgr* emgr, entity_t e)
{
    uint16_t g = *(uint16_t*) vector_at(&emgr->generation, entity_index(e));
    return g == entity_generation(e);
}

void entity_destroy(struct entity_mgr* emgr, entity_t e)
{
    uint64_t idx = entity_index(e);
    uint16_t* g = (uint16_t*)vector_at(&emgr->generation, idx);
    ++(*g);
    queue_push(&emgr->free_indices, &idx);
}

size_t entity_mgr_size(struct entity_mgr* emgr)
{
    return emgr->generation.size;
}

size_t entity_mgr_at(struct entity_mgr* emgr, size_t idx)
{
    uint64_t gen = *(uint16_t*)vector_at(&emgr->generation, idx);
    entity_t e = entity_make(idx, gen);
    return e;
}

void entity_mgr_destroy(struct entity_mgr* emgr)
{
    queue_destroy(&emgr->free_indices);
    vector_destroy(&emgr->generation);
}
