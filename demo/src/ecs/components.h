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
#ifndef _COMPONENTS_H_
#define _COMPONENTS_H_

#include <linalgb.h>
#include <energycore/resource.h>
#include "ecs.h"

#define MAX_MATERIALS 16

enum component_type {
    TRANSFORM = 1,
    RENDER,
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

/* Registers common component */
void register_component_maps(ecs_t ecs);

/* Transform component interface */
struct transform_component* transform_component_create(ecs_t ecs, entity_t e);
struct transform_component* transform_component_lookup(ecs_t ecs, entity_t e);
void transform_component_set_pose(ecs_t ecs, entity_t e, struct transform_pose pose);
void transform_component_set_parent(ecs_t ecs, entity_t child, entity_t parent);
mat4 transform_world_mat(ecs_t ecs, entity_t e);

/* Render component interface */
struct render_component* render_component_create(ecs_t ecs, entity_t e);
struct render_component* render_component_lookup(ecs_t ecs, entity_t e);

#endif /* ! _COMPONENTS_H_ */
