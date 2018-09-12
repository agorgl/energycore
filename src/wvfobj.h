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
#ifndef _WVFOBJ_H_
#define _WVFOBJ_H_

#include <stdlib.h>

typedef struct {
    const char* name; /* Group name or object name. */
    size_t face_offset;
    size_t length;
} obj_shape_t;

typedef struct {
    int v_idx, vt_idx, vn_idx;
} obj_vertex_index_t;

typedef struct {
    float* vertices;
    size_t num_vertices;
    float* normals;
    size_t num_normals;
    float* texcoords;
    size_t num_texcoords;
    obj_vertex_index_t* faces;
    size_t num_faces;
    int* face_num_verts;
    size_t num_face_num_verts;
    int* material_ids;
} obj_attrib_t;

typedef struct {
    obj_attrib_t attrib;
    obj_shape_t* shapes;
    size_t num_shapes;
} obj_model_t;

obj_model_t* obj_model_parse(const char* buf, size_t len);
void obj_model_free(obj_model_t* obj);
struct scene* obj_to_scene(obj_model_t* obj);

#endif /* ! _WVFOBJ_H_ */
