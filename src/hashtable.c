#include "hashtable.h"
#include <string.h>
#include <math.h>

#define HASH_TABLE_DEFAULT_INITIAL_CAPACITY 32
#define HASH_TABLE_UPSIZE_LOAD_FACTOR 0.5

struct hash_table* hash_table_create(hash_fn_t hash_fn, eql_fn_t eql_fn, size_t capacity)
{
    struct hash_table* ht = malloc(sizeof(*ht));
    ht->size     = 0;
    ht->capacity = capacity ? capacity : HASH_TABLE_DEFAULT_INITIAL_CAPACITY;
    ht->hash_fn  = hash_fn;
    ht->eql_fn   = eql_fn;
    ht->table    = calloc(ht->capacity, sizeof(*ht->table));
    ht->states   = calloc(ht->capacity, sizeof(*ht->states));
    return ht;
}

void hash_table_destroy(struct hash_table* ht)
{
    free(ht->states);
    free(ht->table);
    free(ht);
}

#define entry_is_free(ht, index) (ht->states[index] == 0)
#define entry_is_present(ht, index) (ht->states[index] == 1)
#define entry_is_deleted(ht, index) (ht->states[index] == 2)
#define entry_set_present(ht, index) (ht->states[index] = 1)
#define entry_set_deleted(ht, index) (ht->states[index] = 2)

static void hash_table_resize(struct hash_table* ht, size_t new_capacity)
{
    struct hash_table* nht = hash_table_create(ht->hash_fn, ht->eql_fn, new_capacity);
    hash_table_foreach(ht, e)
        hash_table_insert(nht, e->key, e->val);
    free(ht->states);
    free(ht->table);
    *ht = *nht;
    free(nht);
}

#define hash_table_index(ht, k, attempt) ((ht->hash_fn(k) + attempt) % ht->capacity)

void hash_table_insert(struct hash_table* ht, hash_key_t k, hash_val_t v)
{
    float load_factor = (float)ht->size / ht->capacity;
    if (load_factor > HASH_TABLE_UPSIZE_LOAD_FACTOR) {
        size_t new_cap = ht->capacity * 2 + 1;
        hash_table_resize(ht, new_cap);
    }

    struct hash_entry* e = 0;
    size_t index = hash_table_index(ht, k, 0);
    for (size_t i = 1; entry_is_present(ht, index); ++i) {
        e = ht->table + index;
        if (ht->eql_fn(e->key, k)) {
            e->val = v;
            return;
        }
        index = hash_table_index(ht, k, i);
    }

    entry_set_present(ht, index);
    e = ht->table + index;
    e->key = k;
    e->val = v;
    ++ht->size;
}

struct hash_entry* hash_table_search(struct hash_table* ht, hash_key_t k)
{
    size_t index = hash_table_index(ht, k, 0);
    for (size_t i = 1; index < ht->capacity && !entry_is_free(ht, index); ++i) {
        struct hash_entry* e = ht->table + index;
        if (entry_is_present(ht, index) && ht->eql_fn(e->key, k))
            return e;
        index = hash_table_index(ht, k, i);
    }
    return 0;
}

struct hash_entry* hash_table_remove(struct hash_table* ht, hash_key_t k)
{
    size_t index = hash_table_index(ht, k, 0);
    for (size_t i = 1; index < ht->capacity && !entry_is_free(ht, index); ++i) {
        struct hash_entry* e = ht->table + index;
        if (entry_is_present(ht, index) && ht->eql_fn(e->key, k)) {
            entry_set_deleted(ht, index);
            --(ht->size);
            return e;
        }
        index = hash_table_index(ht, k, i);
    }
    return 0;
}

struct hash_entry* hash_table_next_entry(struct hash_table* ht, struct hash_entry* entry)
{
    struct hash_entry* e = 0;
    size_t index = !entry ? 0 : ((entry - ht->table) + 1);
    for (; index < ht->capacity; ++index) {
        if (entry_is_present(ht, index)) {
            e = ht->table + index;
            break;
        }
    }
    return e;
}

/* Fowler/Noll/Vo (FNV) hash function, variant 1a */
uint32_t hash_table_string_hash(hash_key_t k)
{
    const char* cp = (const char*) k;
    size_t hash = 0x811c9dc5;
    while (*cp) {
        hash ^= (unsigned char)*cp++;
        hash *= 0x01000193;
    }
    return hash;
}

int hash_table_string_eql(hash_key_t k1, hash_key_t k2)
{
    return strcmp((const char*)k1, (const char*)k2) == 0;
}
