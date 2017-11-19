#include "components.h"
#include <string.h>

void register_component_maps(ecs_t ecs)
{
    component_map_register(ecs, TRANSFORM, sizeof(struct transform_component));
    component_map_register(ecs, RENDER, sizeof(struct render_component));
}

component_t transform_component_create(ecs_t ecs, entity_t e)
{
    component_t c = component_add(ecs, e, TRANSFORM, 0);
    struct transform_component* d = component_lookup_data(ecs, c);
    d->parent       = INVALID_COMPONENT;
    d->first_child  = INVALID_COMPONENT;
    d->next_sibling = INVALID_COMPONENT;
    d->prev_sibling = INVALID_COMPONENT;
    return c;
}

static void transform_update_cached_wrld_mats(ecs_t ecs, component_t c)
{
    struct transform_component* par_data = transform_component_data(ecs, c);
    component_t child = par_data->first_child;
    while (component_valid(child)) {
        struct transform_component* cld_data = transform_component_data(ecs, child);
        mat4* par = &par_data->world_mat;
        mat4* lcl = &cld_data->local_mat;
        mat4* wrld = &cld_data->world_mat;
        *wrld = mat4_mul_mat4(*par, *lcl);
        transform_update_cached_wrld_mats(ecs, child);
        child = cld_data->next_sibling;
    }
}

void transform_component_set_pose(ecs_t ecs, component_t c, struct transform_pose pose)
{
    struct transform_component* d = component_lookup_data(ecs, c);
    d->pose.rotation    = pose.rotation;
    d->pose.scale       = pose.scale;
    d->pose.translation = pose.translation;

    mat4 m = mat4_world(pose.translation, pose.scale, pose.rotation);
    d->local_mat = m;
    d->world_mat = m;
    transform_update_cached_wrld_mats(ecs, c);
}

component_t transform_component_lookup(ecs_t ecs, entity_t e)
{
    return component_lookup(ecs, e, TRANSFORM);
}

struct transform_component* transform_component_data(ecs_t ecs, component_t c)
{
    return component_lookup_data(ecs, c);
}

void transform_component_set_parent(ecs_t ecs, component_t child, component_t parent)
{
    if (!(component_valid(child) && component_valid(parent)))
        return;
    struct transform_component* ct = transform_component_data(ecs, child);
    ct->parent = parent;
    /* Set child relations */
    struct transform_component* pt = transform_component_data(ecs, parent);
    ct->next_sibling = pt->first_child;
    if (component_valid(pt->first_child)) {
        struct transform_component* fct = transform_component_data(ecs, pt->first_child);
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
    struct transform_component* td = transform_component_data(ecs, transform_component_lookup(ecs, e));
    return td->world_mat;
}

component_t render_component_create(ecs_t ecs, entity_t e)
{
    component_t c = component_add(ecs, e, RENDER, 0);
    struct render_component* d = component_lookup_data(ecs, c);
    memset(d, 0, sizeof(*d));
    return c;
}

component_t render_component_lookup(ecs_t ecs, entity_t e)
{
    return component_lookup(ecs, e, RENDER);
}

struct render_component* render_component_data(ecs_t ecs, component_t c)
{
    return component_lookup_data(ecs, c);
}
