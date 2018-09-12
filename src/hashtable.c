#include "hashtable.h"
#include <string.h>
#include <assert.h>
#include <math.h>

#define HASH_TABLE_DEFAULT_INITIAL_CAPACITY 32
#define HASH_TABLE_UPSIZE_LOAD_FACTOR 0.5

/* MSB set indicates that this hash is deleted */
#define is_deleted(hash) ((hash >> 31) != 0)
#define is_power_of_two(n) (n > 0 && ((n & (n - 1)) == 0))
#define index_from_hash(hash, capacity) (hash & (capacity - 1))

static void hash_table_init(struct hash_table* ht, hash_fn_t hash_fn, eql_fn_t eql_fn, size_t capacity)
{
    ht->size     = 0;
    ht->capacity = capacity;
    ht->hash_fn  = hash_fn;
    ht->eql_fn   = eql_fn;
    ht->table    = calloc(ht->capacity, sizeof(*ht->table));
    ht->hashes   = calloc(ht->capacity, sizeof(*ht->hashes));
    assert(is_power_of_two(ht->capacity));
}

struct hash_table* hash_table_create(hash_fn_t hash_fn, eql_fn_t eql_fn)
{
    struct hash_table* ht = calloc(1, sizeof(*ht));
    hash_table_init(ht, hash_fn, eql_fn, HASH_TABLE_DEFAULT_INITIAL_CAPACITY);
    return ht;
}

void hash_table_destroy(struct hash_table* ht)
{
    free(ht->hashes);
    free(ht->table);
    free(ht);
}

static uint32_t hash_key(struct hash_table* ht, hash_key_t key)
{
    uint32_t h = ht->hash_fn(key);
    /* MSB is used to indicate a deleted elem, so clear it */
    h &= 0x7fffffff;
    /* Ensure that we never return 0 as a hash,
     * since we use 0 to indicate that the elem has never
     * been used at all */
    h |= h == 0;
    return h;
}

static void hash_table_resize(struct hash_table* ht, size_t new_capacity)
{
    struct hash_table* nht = calloc(1, sizeof(*nht));
    hash_table_init(nht, ht->hash_fn, ht->eql_fn, new_capacity);
    hash_table_foreach(ht, e)
        hash_table_insert(nht, e->key, e->val);
    free(ht->hashes);
    free(ht->table);
    *ht = *nht;
    free(nht);
}

void hash_table_insert(struct hash_table* ht, hash_key_t k, hash_val_t v)
{
    float load_factor = (float)ht->size / ht->capacity;
    if (load_factor >= HASH_TABLE_UPSIZE_LOAD_FACTOR) {
        size_t new_cap = ht->capacity * 2;
        hash_table_resize(ht, new_cap);
    }

    uint32_t hash = hash_key(ht, k);
    for (size_t i = 0; ; ++i) {
        size_t index         = index_from_hash((hash + i), ht->capacity);
        uint32_t cur_hash    = ht->hashes[index];
        struct hash_entry* e = ht->table + index;

        /* Found empty or deleted slot, insert */
        if (cur_hash == 0 || is_deleted(cur_hash)) {
            e->key = k;
            e->val = v;
            ht->hashes[index] = hash;
            ++ht->size;
            return;
        }

        /* Found occupied slot with same hash and key, replace value */
        if (hash == cur_hash && ht->eql_fn(e->key, k)) {
            e->val = v;
            return;
        }
    }
}

static inline size_t lookup_index(struct hash_table* ht, hash_key_t k)
{
    uint32_t hash = hash_key(ht, k);
    for(size_t i = 0; ; ++i) {
        size_t index = index_from_hash((hash + i), ht->capacity);
        uint32_t cur_hash = ht->hashes[index];
        if (cur_hash == 0)
            break;
        if (!is_deleted(cur_hash) && hash == cur_hash && ht->eql_fn(ht->table[index].key, k))
            return index;
    }
    return -1;
}

hash_val_t* hash_table_search(struct hash_table* ht, hash_key_t k)
{
    size_t index = lookup_index(ht, k);
    return (ssize_t)index == -1 ? 0 : &ht->table[index].val;
}

int hash_table_remove(struct hash_table* ht, hash_key_t k)
{
    size_t index = lookup_index(ht, k);
    if ((ssize_t)index == -1)
        return 0;
    else {
        ht->hashes[index] |= 0x80000000; /* Mark as deleted */
        --(ht->size);
        return 1;
    }
}

struct hash_entry* hash_table_next_entry(struct hash_table* ht, struct hash_entry* entry)
{
    struct hash_entry* e = 0;
    size_t index = !entry ? 0 : ((entry - ht->table) + 1);
    for (; index < ht->capacity; ++index) {
        uint32_t cur_hash = ht->hashes[index];
        if ((cur_hash != 0 && !is_deleted(cur_hash))) {
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

uint32_t hash_table_int_hash(hash_key_t k)
{
    return (2166136261ul ^ k) * 0x01000193;
}

int hash_table_int_eql(hash_key_t k1, hash_key_t k2)
{
    return k1 == k2;
}
