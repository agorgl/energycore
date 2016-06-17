#include "hashmap.h"
#include <stdlib.h>
#include <string.h>

#define HASHMAP_DEFAULT_LOAD_FACTOR 0.75f
#define HASHMAP_DEFAULT_INITIAL_CAPACITY 10

void hashmap_init(struct hashmap* hm, hashmap_hash_fn hfn, hashmap_eql_fn efn)
{
    hm->capacity = HASHMAP_DEFAULT_INITIAL_CAPACITY;
    hm->size = 0;
    hm->buckets = malloc(sizeof(struct hashmap_node*) * hm->capacity);
    hm->hashfn = hfn;
    hm->eqlfn = efn;
    memset(hm->buckets, 0, sizeof(struct hashmap_node*) * hm->capacity);
}

static void free_buckets(struct hashmap_node** buckets, size_t num_buckets)
{
    struct hashmap_node* n;
    for (size_t i = 0; i < num_buckets; ++i) {
        n = buckets[i];
        while (n) {
            struct hashmap_node* next = n->next;
            free(n);
            n = next;
        }
    }
    free(buckets);
}

void hashmap_destroy(struct hashmap* hm)
{
    free_buckets(hm->buckets, hm->capacity);
    memset(hm, 0, sizeof(struct hashmap));
}

void hashmap_iter(struct hashmap* hm, hm_iter_fn iter_cb)
{
    struct hashmap_node* n;
    for (size_t i = 0; i < hm->capacity; ++i) {
        n = hm->buckets[i];
        while (n) {
            iter_cb(n->data.key, n->data.value);
            n = n->next;
        }
    }
}

static void hashmap_put_internal(struct hashmap* hm, void* key, void* value)
{
    /* Get index from the given key's hash */
    size_t index = hm->hashfn(key) % hm->capacity;
    struct hashmap_node** cur_node = &hm->buckets[index];
    if (*cur_node == 0) {
        /* Bucket with index "index" is empty, create it */
        *cur_node = malloc(sizeof(struct hashmap_node));
        (*cur_node)->next = 0;
    } else {
        struct hashmap_node* n = 0;
        for (n = *cur_node; n != 0; n = n->next) {
            if (hm->eqlfn(key, n->data.key)) {
                /* Key exists, overwrite data */
                n->data.value = value;
                return;
            }
        }
        /* Key does not exist in bucket. Prepend it */
        n = *cur_node;
        *cur_node = malloc(sizeof(struct hashmap_node));
        (*cur_node)->next = n;
    }
    /* Fill new node */
    (*cur_node)->data.key = key;
    (*cur_node)->data.value = value;
    hm->size++;
}

static void hashmap_resize(struct hashmap* hm)
{
    /* Increase the capacity by doubling it */
    size_t ocap = hm->capacity;
    hm->capacity *= 2;
    /* Store old bucket array to copy its contents to the new location */
    struct hashmap_node** old_buckets = hm->buckets;
    /* Allocate new bucket array */
    hm->buckets = malloc(sizeof(struct hashmap_node*) * hm->capacity);
    memset(hm->buckets, 0, sizeof(struct hashmap_node*) * hm->capacity);
    /* Copy the pairs from the old bucket array to the new one */
    for (size_t i = 0; i < ocap; ++i) {
        struct hashmap_node* n = old_buckets[i];
        while (n != 0) {
            hashmap_put_internal(hm, n->data.key, n->data.value);
            n = n->next;
        }
    }
    /* Free old buckets */
    free_buckets(old_buckets, ocap);
}

void hashmap_put(struct hashmap* hm, void* key, void* value)
{
    /* Check if resize needed */
    if (((float)hm->size / hm->capacity) >= HASHMAP_DEFAULT_LOAD_FACTOR)
        hashmap_resize(hm);

    /* Actual put */
    hashmap_put_internal(hm, key, value);
}

void* hashmap_get(struct hashmap* hm, void* key)
{
    /* Get the bucket index associated with given key's hash */
    size_t index = hm->hashfn(key) % hm->capacity;
    /* Pointer to start of bucket that contains key */
    struct hashmap_node* n = 0;
    for (n = hm->buckets[index]; n != 0; n = n->next) {
        if (hm->eqlfn(key, n->data.key)) {
            return &n->data.value;
        }
    }
    return 0;
}

int hashmap_exists(struct hashmap* hm, void* key)
{
    return hashmap_get(hm, key) != 0;
}
