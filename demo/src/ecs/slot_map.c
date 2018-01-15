#include "slot_map.h"
#include <string.h>
#include <assert.h>

#define POISON_POINTER ((void *)(0xDEAD000000000000UL))
#define INITIAL_CAPACITY 4
#define INVALID_IDX ~0u

static inline void initialize_allocated_slots(struct sm_slot* slots, size_t start, size_t end)
{
    for (size_t i = start; i < end; ++i) {
        struct sm_slot* s = &slots[i];
        s->generation     = 0;
        s->data_idx       = INVALID_IDX;
        s->free_list_next = INVALID_IDX;
    }
}

static inline void initialize_allocated_inverse_mappings(size_t* a, size_t start, size_t end)
{
    for (size_t i = start; i < end; ++i)
        a[i] = INVALID_IDX;
}

void slot_map_init(struct slot_map* sm, size_t esz)
{
    sm->esz            = esz;
    sm->num_slots      = 0;
    sm->size           = 0;
    sm->cap_slots      = INITIAL_CAPACITY;
    sm->capacity       = INITIAL_CAPACITY;
    sm->slots          = calloc(sm->cap_slots, sizeof(struct sm_slot));
    sm->data_to_slot   = calloc(sm->cap_slots, sizeof(size_t));
    sm->data           = calloc(sm->capacity, sm->esz);
    sm->free_list_head = INVALID_IDX;
    sm->free_list_tail = INVALID_IDX;
    initialize_allocated_slots(sm->slots, 0, sm->cap_slots);
    initialize_allocated_inverse_mappings(sm->data_to_slot, 0, sm->cap_slots);
}

void slot_map_destroy(struct slot_map* sm)
{
    free(sm->slots);
    free(sm->data);
    sm->slots = POISON_POINTER;
    sm->data  = POISON_POINTER;
}

int slot_map_keys_equal(sm_key k1, sm_key k2)
{
    return k1.index      == k2.index
        && k1.generation == k2.generation;
}

int slot_map_key_valid(sm_key k)
{
    return !slot_map_keys_equal(k, SM_INVALID_KEY);
}

static inline size_t free_list_pop(struct slot_map* sm)
{
    size_t idx = sm->free_list_head;
    sm->free_list_head = sm->slots[idx].free_list_next;
    sm->slots[idx].free_list_next = INVALID_IDX;
    return idx;
}

static inline void free_list_push(struct slot_map* sm, size_t idx)
{
    sm->slots[idx].free_list_next = INVALID_IDX;
    if (sm->free_list_head == INVALID_IDX) {
        sm->free_list_head = sm->free_list_tail = idx;
    } else {
        sm->slots[sm->free_list_tail].free_list_next = idx;
        sm->free_list_tail = idx;
    }
}

static inline size_t data_append(struct slot_map* sm, void* data)
{
    if (sm->capacity - sm->size < 1) {
        sm->capacity = sm->capacity * 2 + 1;
        sm->data = realloc(sm->data, sm->capacity * sm->esz);
    }
    if (data)
        memcpy(sm->data + sm->size * sm->esz, data, sm->esz);
    return sm->size++;
}

static inline size_t slot_allocate(struct slot_map* sm)
{
    if (sm->cap_slots - sm->num_slots < 1) {
        sm->cap_slots = sm->cap_slots * 2 + 1;
        sm->slots = realloc(sm->slots, sm->cap_slots * sizeof(struct sm_slot));
        sm->data_to_slot = realloc(sm->data_to_slot, sm->cap_slots * sizeof(size_t));
        initialize_allocated_slots(sm->slots, sm->num_slots, sm->cap_slots);
        initialize_allocated_inverse_mappings(sm->data_to_slot, sm->num_slots, sm->cap_slots);
    }
    return sm->num_slots++;
}

sm_key slot_map_insert(struct slot_map* sm, void* data)
{
    sm_key k;
    if (sm->free_list_head != INVALID_IDX) {
        /* Recycle indexes */
        k.index = free_list_pop(sm);
    } else {
        /* Allocate new index */
        k.index = slot_allocate(sm);
        assert(k.index < (1LL << SLOT_MAP_INDEX_BITS));
    }
    k.generation = sm->slots[k.index].generation;
    size_t ndata_idx = data_append(sm, data);
    sm->slots[k.index].data_idx = ndata_idx;
    sm->data_to_slot[ndata_idx] = k.index;
    return k;
}

void* slot_map_lookup(struct slot_map* sm, sm_key k)
{
    struct sm_slot* s = &sm->slots[k.index];
    if (k.generation == s->generation) {
        void* data = sm->data + s->data_idx * sm->esz;
        return data;
    }
    return 0;
}

static inline size_t data_to_slot(struct slot_map* sm, size_t data_idx)
{
    return sm->data_to_slot[data_idx];
}

void* slot_map_data(struct slot_map* sm, size_t idx)
{
    return sm->data + idx * sm->esz;
}

sm_key slot_map_data_to_key(struct slot_map* sm, size_t idx)
{
    sm_key k;
    k.index = data_to_slot(sm, idx);
    k.generation = sm->slots[k.index].generation;
    return k;
}

static inline void swap_with_last(struct slot_map* sm, size_t data_idx)
{
    memcpy(sm->data + data_idx * sm->esz, sm->data + (sm->size - 1) * sm->esz, sm->esz);
    sm->data_to_slot[data_idx] = sm->data_to_slot[sm->size - 1];
    sm->slots[data_to_slot(sm, sm->size - 1)].data_idx = data_idx;
}

int slot_map_remove(struct slot_map* sm, sm_key k)
{
    struct sm_slot* s = &sm->slots[k.index];
    if (k.generation == s->generation) {
        ++(s->generation);
        free_list_push(sm, k.index);
        if (s->data_idx != sm->size - 1)
            swap_with_last(sm, s->data_idx);
        s->data_idx = INVALID_IDX;
        --sm->size;
        --sm->num_slots;
        return 1;
    }
    return 0;
}
