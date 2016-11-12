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
#ifndef _GEOM_DATA_H_
#define _GEOM_DATA_H_

#include <stdint.h>
#include "linalgb.h"

/* Vertex */
typedef struct {
    vec3 position;
    vec3 normal;
    vec3 tangent;
    vec3 binormal;
    vec4 color;
    vec2 uvs;
} vertex;

vertex vertex_new();
bool vertex_equal(vertex v1, vertex v2);
void vertex_print(vertex v);

/* Mesh */
typedef struct {
    int num_verts;
    int num_triangles;
    vertex* verticies;
    uint32_t* triangles;
} mesh;

mesh* mesh_new();
void mesh_delete(mesh* m);

void mesh_generate_normals(mesh* m);
void mesh_generate_tangents(mesh* m);
void mesh_generate_orthagonal_tangents(mesh* m);
void mesh_generate_texcoords_cylinder(mesh* m);

void mesh_print(mesh* m);
float mesh_surface_area(mesh* m);

void mesh_transform(mesh* m, mat4 transform);
void mesh_translate(mesh* m, vec3 translation);
void mesh_scale(mesh* m, float scale);

sphere mesh_bounding_sphere(mesh* m);

/* Model */
typedef struct {
    int num_meshes;
    mesh** meshes;
} model;

model* model_new();
void model_delete(model* m);

void model_generate_normals(model* m);
void model_generate_tangents(model* m);
void model_generate_orthagonal_tangents(model* m);
void model_generate_texcoords_cylinder(model* m);

void model_print(model* m);
float model_surface_area(model* m);

void model_transform(model* m, mat4 transform);
void model_translate(model* m, vec3 translation);
void model_scale(model* m, float scale);

#endif /* ! _GEOM_DATA_H_ */
