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
#ifndef _RENDERER_H_
#define _RENDERER_H_

#include <linalgb.h>

/* Shader fetch function, used by renderer_shdr_fetch.
 * Takes as input the name of a shader and returns shader handle */
typedef unsigned int(*rndr_shdr_fetch_fn)(const char*, void* userdata);

enum renderer_sky_type {
    RST_TEXTURE = 0,
    RST_PREETHAM,
    RST_NONE
};

struct sky_renderer_state {
    struct sky_texture* tex;
    struct sky_preetham* preeth;
};

struct renderer_state {
    rndr_shdr_fetch_fn shdr_fetch_cb;
    void* shdr_fetch_userdata;
    unsigned int shdr_main;
    mat4 proj;
    struct sky_renderer_state sky_rs;
    struct probe_vis* probe_vis;
    struct lc_renderer_state* lc_rs;
    float prob_angle;
};

struct renderer_material {
    float diff_col[3];
    unsigned int diff_tex;
};

struct renderer_mesh {
    unsigned int vao;
    unsigned int ebo;
    unsigned int indice_count;
    float model_mat[16];
    struct renderer_material material;
};

struct renderer_input {
    struct renderer_mesh* meshes;
    unsigned int num_meshes;
    unsigned int sky_tex;
    enum renderer_sky_type sky_type;
};

void renderer_init(struct renderer_state* rs, rndr_shdr_fetch_fn sfn, void* sf_ud);
void renderer_render(struct renderer_state* rs, struct renderer_input* ri, float view_mat[16]);
void renderer_shdr_fetch(struct renderer_state* rs);
void renderer_resize(struct renderer_state* rs, unsigned int width, unsigned int height);
void renderer_destroy(struct renderer_state* rs);

#endif /* ! _RENDERER_H_ */
