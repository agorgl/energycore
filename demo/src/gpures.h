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
#include <hashmap.h>

struct mesh_hndl
{
    unsigned int vao;
    unsigned int vbo;
    unsigned int wbo;
    unsigned int ebo;
    unsigned int indice_count;
    unsigned int mat_idx;
    unsigned int mgroup_idx;
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

enum material_attr_type {
    MAT_ALBEDO = 0,
    MAT_NORMAL,
    MAT_ROUGHNESS,
    MAT_METALLIC,
    MAT_SPECULAR,
    MAT_GLOSSINESS,
    MAT_EMISSION,
    MAT_OCCLUSION,
    MAT_DETAIL_ALBEDO,
    MAT_DETAIL_NORMAL,
    MAT_PARALLAX,
    MAT_MAX
};

struct material {
    struct material_attr {
        struct {
            float valf;
            float val3f[3];
            struct {
                struct tex_hndl hndl;
                float scl[2];
            } tex;
        } d;
        enum {
            MAT_DTYPE_VALF = 0,
            MAT_DTYPE_VAL3F,
            MAT_DTYPE_TEX
        } dtype;
    } attrs[MAT_MAX];
};

struct res_mngr {
    struct hashmap mdl_store;
    struct hashmap tex_store;
    struct hashmap mat_store;
    struct hashmap shdr_store;
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
/* Resource manager */
struct res_mngr* res_mngr_create();
struct model_hndl* res_mngr_mdl_get(struct res_mngr* rmgr, const char* mdl_name);
struct tex_hndl* res_mngr_tex_get(struct res_mngr* rmgr, const char* tex_name);
struct material* res_mngr_mat_get(struct res_mngr* rmgr, const char* mat_name);
unsigned int res_mngr_shdr_get(struct res_mngr* rmgr, const char* shdr_name);
void res_mngr_mdl_put(struct res_mngr* rmgr, const char* mdl_name, struct model_hndl* m);
void res_mngr_tex_put(struct res_mngr* rmgr, const char* tex_name, struct tex_hndl* t);
void res_mngr_mat_put(struct res_mngr* rmgr, const char* mat_name, struct material* mat);
void res_mngr_shdr_put(struct res_mngr* rmgr, const char* shdr_name, unsigned int shdr);
void res_mngr_destroy(struct res_mngr*);

#endif /* ! _GPURES_H_ */
