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
#ifndef _ENTITY_H_
#define _ENTITY_H_

#include <stdint.h>
#include <vector.h>
#include <queue.h>

typedef uint64_t entity_t;

#define ENTITY_INDEX_BITS 48
#define ENTITY_GENERATION_BITS 16

#define entity_index(e) ((e) & ((1LL << ENTITY_INDEX_BITS) - 1))
#define entity_generation(e) (((e) >> ENTITY_INDEX_BITS) & ((1LL << ENTITY_GENERATION_BITS) - 1))
#define entity_make(idx, gen) (((gen) << ENTITY_INDEX_BITS) | (idx))

struct entity_mgr {
    struct vector generation;
    struct queue free_indices;
};

void entity_mgr_init(struct entity_mgr* emgr);
entity_t entity_create(struct entity_mgr* emgr);
int entity_alive(struct entity_mgr* emgr, entity_t e);
void entity_destroy(struct entity_mgr* emgr, entity_t e);
size_t entity_mgr_size(struct entity_mgr* emgr);
size_t entity_mgr_at(struct entity_mgr* emgr, size_t idx);
void entity_mgr_destroy(struct entity_mgr* emgr);

#endif /* ! _ENTITY_H_ */
