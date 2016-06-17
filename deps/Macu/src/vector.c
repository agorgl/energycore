#include "vector.h"
#include <stdlib.h>
#include <string.h>

#define VECTOR_INIT_CAPACITY 10

void vector_init(struct vector* v, size_t item_sz)
{
    memset(v, 0, sizeof(struct vector));
    v->item_sz = item_sz;
    v->capacity = VECTOR_INIT_CAPACITY;
    v->size = 0;
    v->data = malloc(v->capacity * item_sz);
    memset(v->data, 0, v->capacity * item_sz);
}

void vector_destroy(struct vector* v)
{
    free(v->data);
    v->data = 0;
    v->capacity = 0;
    v->size = 0;
    v->item_sz = 0;
}

static void vector_grow(struct vector* v)
{
    v->capacity = (v->capacity * 2 + 1);
    v->data = realloc(v->data, v->capacity * v->item_sz);
    memset(v->data + v->size * v->item_sz, 0, (v->capacity - v->size) * v->item_sz);
}

void vector_append(struct vector* v, void* thing)
{
    if (v->capacity - v->size < 1) {
        vector_grow(v);
    }
    v->size++;
    memcpy(v->data + v->item_sz * (v->size - 1), thing, v->item_sz);
}

void* vector_at(struct vector* v, size_t index)
{
    return v->data + v->item_sz * index;
}
