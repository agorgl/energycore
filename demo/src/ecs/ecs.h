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
#ifndef _ECS_H_
#define _ECS_H_

#include "slot_map.h"

/* ECS opaque data type */
typedef struct ecs* ecs_t;

/* Entity reference */
typedef sm_key entity_t;

/* Component type */
typedef uint32_t component_type_t;

/* Component data reference */
typedef sm_key component_data_t;

/* Component reference */
typedef struct {
    component_type_t type;
    component_data_t dref;
} component_t;

/* ECS initialization/deinitialization */
ecs_t ecs_init();
void ecs_destroy(ecs_t ecs);

/* Entity creation/deletion */
entity_t entity_create(ecs_t ecs);
void entity_remove(ecs_t ecs, entity_t e);
int entity_exists(ecs_t ecs, entity_t e);
entity_t entity_at(ecs_t ecs, size_t idx);
size_t entity_total(ecs_t ecs);

/* Component map creation/deletion */
void component_map_register(ecs_t ecs, component_type_t t, size_t component_size);
void component_map_unregister(ecs_t ecs, component_type_t t);

/* Component addition/removal/lookup */
component_t component_add(ecs_t ecs, entity_t e, component_type_t t, void* data);
component_t component_lookup(ecs_t ecs, entity_t e, component_type_t t);
void* component_lookup_data(ecs_t ecs, component_t c);
int component_remove(ecs_t ecs, entity_t e, component_type_t t);

/* Component null references */
#define INVALID_COMPONENT_TYPE (~0)
#define INVALID_COMPONENT ((component_t){INVALID_COMPONENT_TYPE, SM_INVALID_KEY})
int component_valid(component_t c);

#endif /* ! _ECS_H_ */
