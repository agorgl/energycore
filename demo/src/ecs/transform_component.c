#include "transform_component.h"
#include <string.h>
#include <assert.h>

struct transform_handle invalid_transform_handle = { ~0ul };

static void transform_data_buffer_allocate(struct transform_data_buffer* tdb, size_t sz)
{
    assert(sz > tdb->size);
    const size_t buf_sz = sz * (
        1 * sizeof(entity_t) +
        1 * sizeof(struct transform_pose) +
        2 * sizeof(mat4) +
        4 * sizeof(struct transform_handle)
    );
    struct transform_data_buffer new_tdb;
    /* Header info */
    new_tdb.buffer          = malloc(buf_sz);
    new_tdb.size            = tdb->size;
    new_tdb.capacity        = sz;
    /* Data pointers */
    new_tdb.entities      = (entity_t*)(new_tdb.buffer);
    new_tdb.poses         = (struct transform_pose*)(new_tdb.entities + sz);
    new_tdb.local_mats    = (mat4*)(new_tdb.poses + sz);
    new_tdb.world_mats    = new_tdb.local_mats + sz;
    /* Instance relation pointers */
    new_tdb.parents       = (struct transform_handle*)(new_tdb.world_mats + sz);
    new_tdb.first_childs  = new_tdb.parents + sz;
    new_tdb.next_siblings = new_tdb.first_childs + sz;
    new_tdb.prev_siblings = new_tdb.next_siblings + sz;
    /* Copy data */
    memcpy(new_tdb.entities, tdb->entities, tdb->size * sizeof(entity_t));
    memcpy(new_tdb.poses, tdb->poses, tdb->size * sizeof(struct transform_pose));
    memcpy(new_tdb.local_mats, tdb->local_mats, tdb->size * sizeof(mat4));
    memcpy(new_tdb.world_mats, tdb->world_mats, tdb->size * sizeof(mat4));
    /* Copy transform relations */
    memcpy(new_tdb.parents, tdb->parents, tdb->size * sizeof(struct transform_handle));
    memcpy(new_tdb.first_childs, tdb->first_childs, tdb->size * sizeof(struct transform_handle));
    memcpy(new_tdb.next_siblings, tdb->next_siblings, tdb->size * sizeof(struct transform_handle));
    memcpy(new_tdb.prev_siblings, tdb->prev_siblings, tdb->size * sizeof(struct transform_handle));
    /* Free old buffer data and update his members */
    free(tdb->buffer);
    *tdb = new_tdb;
}

void transform_data_buffer_init(struct transform_data_buffer* tdb)
{
    memset(tdb, 0, sizeof(struct transform_data_buffer));
}

void transform_data_buffer_destroy(struct transform_data_buffer* tdb)
{
    free(tdb->buffer);
    memset(tdb, 0, sizeof(struct transform_data_buffer));
}

/* Maintains packed array by moving last component to removed component's place */
static void transform_component_destroy_at(struct transform_data_buffer* tdb, size_t offs)
{
    size_t last = tdb->size - 1;
    /* TODO: implement set and remove on hashset future optimization */
    /*
    entity_t e = tdb->entities[offs];
    entity_t last_e = tdb->entities[last];
     */
    tdb->entities[offs]      = tdb->entities[last];
    tdb->local_mats[offs]    = tdb->local_mats[last];
    tdb->world_mats[offs]    = tdb->world_mats[last];
    tdb->parents[offs]       = tdb->parents[last];
    tdb->first_childs[offs]  = tdb->first_childs[last];
    tdb->next_siblings[offs] = tdb->next_siblings[last];
    tdb->prev_siblings[offs] = tdb->prev_siblings[last];
    --tdb->size;
}

/* We pick random component indices and destroy them if the corresponding entity has been destroyed.
 * We do this until we hit four living entities in a row. That costs almost nothing if there are no
 * destroyed entities (just four passes of the loop). But when there are a lot of destroyed entities
 * the components will be quickly destroyed. */
void transform_data_buffer_gc(struct transform_data_buffer* tdb, struct entity_mgr* emgr)
{
    unsigned int alive_in_row = 0;
    while (tdb->size > 0 && alive_in_row < 4) {
        unsigned i = rand() % tdb->size;
        entity_t e = tdb->entities[i];
        if (entity_alive(emgr, e)) {
            ++alive_in_row;
            continue;
        }
        alive_in_row = 0;
        transform_component_destroy_at(tdb, i);
    }
}

struct transform_handle transform_component_lookup(struct transform_data_buffer* tdb, entity_t e)
{
    for (size_t i = 0; i < tdb->size; ++i) {
        if (tdb->entities[i] == e) {
            struct transform_handle th = {i};
            return th;
        }
    }
    return invalid_transform_handle;
}

struct transform_handle transform_component_create(struct transform_data_buffer* tdb, entity_t e)
{
    if (tdb->size == tdb->capacity)
        transform_data_buffer_allocate(tdb, tdb->size * 3.0f/2.0f + 1);
    struct transform_handle th = { tdb->size++ };
    tdb->entities[th.offs] = e;
    return th;
}

/* Compare transform handles */
static int cmp_th(struct transform_handle h1, struct transform_handle h2) { return h1.offs == h2.offs; }

struct transform_pose* transform_pose_data(struct transform_data_buffer* tdb, struct transform_handle th)
{
    if (cmp_th(th, invalid_transform_handle))
        return 0;
    return tdb->poses + th.offs;
}

void transform_set_pose_data(struct transform_data_buffer* tdb, struct transform_handle th, vec3 scale, quat rotation, vec3 translation)
{
    if (cmp_th(th, invalid_transform_handle))
        return;
    struct transform_pose* pose = tdb->poses + th.offs;
    pose->scale = scale;
    pose->rotation = rotation;
    pose->translation = translation;
    mat4 m = mat4_world(pose->translation, pose->scale, pose->rotation);
    /* Set local matrix */
    mat4* lcl_mat = tdb->local_mats + th.offs;
    *lcl_mat = m;
    /* TODO: */
    mat4* wrld_mat = tdb->world_mats + th.offs;
    *wrld_mat = m;
}

mat4* transform_local_mat(struct transform_data_buffer* tdb, struct transform_handle th)
{
    if (cmp_th(th, invalid_transform_handle))
        return 0;
    return tdb->local_mats + th.offs;
}

mat4* transform_world_mat(struct transform_data_buffer* tdb, struct transform_handle th)
{
    if (cmp_th(th, invalid_transform_handle))
        return 0;
    return tdb->world_mats + th.offs;
}
