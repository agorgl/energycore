/*********************************************************************************************************************/
/*                                                  /===-_---~~~~~~~~~------____                                     */
/*                                                 |===-~___                _,-'                                     */
/*                  -==\\                         `//~\\   ~~~~`---.___.-~~                                          */
/*              ______-==|                         | |  \\           _-~`                                            */
/*        __--~~~  ,-/-==\\                        | |   `\        ,'                                                */
/*     _-~       /'    |  \\                      / /      \      /                                                  */
/*   .'        /       |   \\                   /' /        \   /'                                                   */
/*  /  ____  /         |    \`\.__/-~~ ~ \ _ _/'  /          \/'                                                     */
/* /-'~    ~~~~~---__  |     ~-/~         ( )   /'        _--~`                                                      */
/*                   \_|      /        _)   ;  ),   __--~~                                                           */
/*                     '~~--_/      _-~/-  / \   '-~ \                                                               */
/*                    {\__--_/}    / \\_>- )<__\      \                                                              */
/*                    /'   (_/  _-~  | |__>--<__|      |                                                             */
/*                   |0  0 _/) )-~     | |__>--<__|     |                                                            */
/*                   / /~ ,_/       / /__>---<__/      |                                                             */
/*                  o o _//        /-~_>---<__-~      /                                                              */
/*                  (^(~          /~_>---<__-      _-~                                                               */
/*                 ,/|           /__>--<__/     _-~                                                                  */
/*              ,//('(          |__>--<__|     /                  .----_                                             */
/*             ( ( '))          |__>--<__|    |                 /' _---_~\                                           */
/*          `-)) )) (           |__>--<__|    |               /'  /     ~\`\                                         */
/*         ,/,'//( (             \__>--<__\    \            /'  //        ||                                         */
/*       ,( ( ((, ))              ~-__>--<_~-_  ~--____---~' _/'/        /'                                          */
/*     `~/  )` ) ,/|                 ~-_~>--<_/-__       __-~ _/                                                     */
/*   ._-~//( )/ )) `                    ~~-'_/_/ /~~~~~~~__--~                                                       */
/*    ;'( ')/ ,)(                              ~~~~~~~~~~                                                            */
/*   ' ') '( (/                                                                                                      */
/*     '   '  `                                                                                                      */
/*********************************************************************************************************************/
#ifndef _WORLD_H_
#define _WORLD_H_

#include <linalgb.h>
#include <energycore/resource.h>
#include "ecs.h"
#include "camctrl.h"

#define MAX_MATERIALS 16

typedef ecs_t world_t;

enum component_type {
    TRANSFORM = 1,
    RENDER,
    LIGHT,
    CAMERA,
    MAX_COMPONENT_TYPE
};

struct transform_component {
    struct transform_pose {
        vec3 scale;
        quat rotation;
        vec3 translation;
    } pose;
    mat4 local_mat;
    mat4 world_mat;
    entity_t parent;
    entity_t first_child;
    entity_t next_sibling;
    entity_t prev_sibling;
};

struct render_component {
    rid mesh;
    rid materials[MAX_MATERIALS];
};

struct light_component {
    enum {
        LC_DIRECTIONAL,
        LC_POINT,
        LC_SPOT
    } type;
    vec3 color;
    float intensity;
    vec3 position;
    float falloff;
    vec3 direction;
    float inner_cone;
    float outer_cone;
};

struct camera_component {
    struct camctrl camctrl;
};

/* World interface */
world_t world_create();
void world_update(world_t w, float dt);
void world_destroy(world_t w);

/* Transform component interface */
struct transform_component* transform_component_create(world_t w, entity_t e);
struct transform_component* transform_component_lookup(world_t w, entity_t e);
void transform_component_set_pose(world_t w, entity_t e, struct transform_pose pose);
void transform_component_set_parent(world_t w, entity_t child, entity_t parent);
mat4 transform_world_mat(world_t w, entity_t e);

/* Render component interface */
struct render_component* render_component_create(world_t w, entity_t e);
struct render_component* render_component_lookup(world_t w, entity_t e);

/* Light component interface */
struct light_component* light_component_create(world_t w, entity_t e);
struct light_component* light_component_lookup(world_t w, entity_t e);

/* Camera component interface */
struct camera_component* camera_component_create(world_t w, entity_t e);
struct camera_component* camera_component_lookup(world_t w, entity_t e);

#endif /* ! _WORLD_H_ */
