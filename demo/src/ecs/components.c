#include "components.h"
#include <string.h>

void register_component_maps(ecs_t ecs)
{
    component_map_register(ecs, TRANSFORM, sizeof(struct transform_component));
    component_map_register(ecs, RENDER, sizeof(struct render_component));
}

struct transform_component* transform_component_create(ecs_t ecs, entity_t e)
{
    struct transform_component* d = component_add(ecs, e, TRANSFORM, 0);
    d->parent       = INVALID_ENTITY;
    d->first_child  = INVALID_ENTITY;
    d->next_sibling = INVALID_ENTITY;
    d->prev_sibling = INVALID_ENTITY;
    return d;
}

static void transform_update_cached_wrld_mats(ecs_t ecs, entity_t e)
{
    struct transform_component* par_data = transform_component_lookup(ecs, e);
    entity_t child = par_data->first_child;
    while (entity_valid(child)) {
        struct transform_component* cld_data = transform_component_lookup(ecs, child);
        mat4* par = &par_data->world_mat;
        mat4* lcl = &cld_data->local_mat;
        mat4* wrld = &cld_data->world_mat;
        *wrld = mat4_mul_mat4(*par, *lcl);
        transform_update_cached_wrld_mats(ecs, child);
        child = cld_data->next_sibling;
    }
}

void transform_component_set_pose(ecs_t ecs, entity_t e, struct transform_pose pose)
{
    struct transform_component* d = component_lookup(ecs, e, TRANSFORM);
    d->pose.rotation    = pose.rotation;
    d->pose.scale       = pose.scale;
    d->pose.translation = pose.translation;

    mat4 m = mat4_world(pose.translation, pose.scale, pose.rotation);
    d->local_mat = m;
    d->world_mat = m;
    transform_update_cached_wrld_mats(ecs, e);
}

struct transform_component* transform_component_lookup(ecs_t ecs, entity_t e)
{
    return component_lookup(ecs, e, TRANSFORM);
}

void transform_component_set_parent(ecs_t ecs, entity_t child, entity_t parent)
{
    if (!(entity_valid(child) && entity_valid(parent)))
        return;
    struct transform_component* ct = transform_component_lookup(ecs, child);
    ct->parent = parent;
    /* Set child relations */
    struct transform_component* pt = transform_component_lookup(ecs, parent);
    ct->next_sibling = pt->first_child;
    if (entity_valid(pt->first_child)) {
        struct transform_component* fct = transform_component_lookup(ecs, pt->first_child);
        fct->prev_sibling = child;
    }
    pt->first_child = child;
    /* Update child wrld mat */
    mat4* par  = &pt->world_mat;
    mat4* lcl  = &ct->local_mat;
    mat4* wrld = &ct->world_mat;
    *wrld = mat4_mul_mat4(*par, *lcl);
    /* Update cached child world mats */
    transform_update_cached_wrld_mats(ecs, child);
}

mat4 transform_world_mat(ecs_t ecs, entity_t e)
{
    struct transform_component* td = transform_component_lookup(ecs, e);
    return td->world_mat;
}

struct render_component* render_component_create(ecs_t ecs, entity_t e)
{
    struct render_component* d = component_add(ecs, e, RENDER, 0);
    memset(d, 0, sizeof(*d));
    for (size_t i = 0; i < MAX_MATERIALS; ++i)
        d->materials[i] = INVALID_RID;
    return d;
}

struct render_component* render_component_lookup(ecs_t ecs, entity_t e)
{
    return component_lookup(ecs, e, RENDER);
}
