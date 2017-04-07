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
#ifndef _GPURES_H_
#define _GPURES_H_

#include <assets/model/model.h>
#include <assets/image/image.h>

struct mesh_hndl
{
    unsigned int vao;
    unsigned int vbo;
    unsigned int wbo;
    unsigned int ebo;
    unsigned int indice_count;
    unsigned int mat_idx;
    float aabb_min[3], aabb_max[3];
};

struct model_hndl
{
    struct mesh_hndl* meshes;
    unsigned int num_meshes;
    struct skeleton* skel;
    struct frameset* fset;
    struct mesh_group** mesh_groups;
    unsigned int num_mesh_groups;
};

struct tex_hndl { unsigned int id; };

enum material_attr {
    MAT_ALBEDO = 0,
    MAT_NORMAL,
    MAT_ROUGHNESS,
    MAT_METALLIC,
    MAT_MAX
};

struct material {
    struct {
        struct tex_hndl hndl;
        float scl[2];
    } tex[MAT_MAX];
};

/* Geometry gpu data */
struct model_hndl* model_to_gpu(struct model* model);
struct model_hndl* model_from_file_to_gpu(const char* filename);
void model_free_from_gpu(struct model_hndl* mdlh);
/* Texture gpu data */
struct tex_hndl* tex_to_gpu(struct image* im);
struct tex_hndl* tex_from_file_to_gpu(const char* filename);
struct tex_hndl* tex_env_to_gpu(struct image* im);
struct tex_hndl* tex_env_from_file_to_gpu(const char* filename);
void tex_free_from_gpu(struct tex_hndl* th);
/* Shader programs */
unsigned int shader_from_sources(const char* vs_src, const char* gs_src, const char* fs_src);
unsigned int shader_from_files(const char* vsp, const char* gsp, const char* fsp);

#endif /* ! _GPURES_H_ */
