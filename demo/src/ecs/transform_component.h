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
#ifndef _TRANSFORM_COMPONENT_H_
#define _TRANSFORM_COMPONENT_H_

#include <stdlib.h>
#include <linalgb.h>
#include "entity.h"

/* Type used to index a transform component
 * into the compound data array */
struct transform_handle { size_t offs; };
struct transform_handle invalid_transform_handle;

/* Packed transform component data */
struct transform_pose {
    vec3 scale;
    quat rotation;
    vec3 translation;
};

/* SOA containing transform components data and relations */
struct transform_data_buffer {
    /* TODO: struct hashset                    improve lookup with separate hashset data structure */
    size_t size;                               /** Number of used entries in arrays */
    size_t capacity;                           /** Number of allocated entries in arrays */
    void* buffer;                              /** Raw buffer for data */

    entity_t* entities;                        /** The entities owning each instance (used by gc) */
    struct transform_pose* poses;              /** Individual components that compose local transform */
    mat4* local_mats;                          /** Cached local transform with respect to parent */
    mat4* world_mats;                          /** World transform */
    struct transform_handle* parents;          /** The parent instance of each instance */
    struct transform_handle* first_childs;     /** The first child of each instance */
    struct transform_handle* next_siblings;    /** The next sibling of each instance */
    struct transform_handle* prev_siblings;    /** The previous sibling of each instance */
};

/* Transform data buffer interface */
void transform_data_buffer_init(struct transform_data_buffer* tdb);
void transform_data_buffer_destroy(struct transform_data_buffer* tdb);
void transform_data_buffer_gc(struct transform_data_buffer* tdb, struct entity_mgr* emgr);
/* Transform components interface */
struct transform_handle transform_component_lookup(struct transform_data_buffer* tdb, entity_t e);
struct transform_handle transform_component_create(struct transform_data_buffer* tdb, entity_t e);
/* Transform data interface */
struct transform_pose* transform_pose_data(struct transform_data_buffer* tdb, struct transform_handle th);
void transform_set_pose_data(struct transform_data_buffer* tdb, struct transform_handle th, vec3 scale, quat rotation, vec3 translation);
mat4* transform_local_mat(struct transform_data_buffer* tdb, struct transform_handle th);
mat4* transform_world_mat(struct transform_data_buffer* tdb, struct transform_handle th);

#endif /* ! _TRANSFORM_COMPONENT_H_ */
