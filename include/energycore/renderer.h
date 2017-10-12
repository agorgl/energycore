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

/*-----------------------------------------------------------------
 * Renderer input
 *-----------------------------------------------------------------*/
enum renderer_material_attr_type {
    RMAT_ALBEDO = 0,
    RMAT_NORMAL,
    RMAT_ROUGHNESS,
    RMAT_METALLIC,
    RMAT_DETAIL_ALBEDO,
    RMAT_DETAIL_NORMAL,
    RMAT_MAX
};

struct renderer_material {
    struct renderer_material_attr {
        struct {
            float valf;
            float val3f[3];
            struct {
                unsigned int id;
                float scl[2];
            } tex;
        } d;
        enum {
            RMAT_DT_VALF = 0,
            RMAT_DT_VAL3F,
            RMAT_DT_TEX
        } dtype;
    } attrs[RMAT_MAX];
};

struct renderer_mesh {
    /* Geometry handles */
    unsigned int vao;
    unsigned int ebo;
    unsigned int indice_count;
    /* Model matrix */
    float model_mat[16];
    /* Material parameters */
    struct renderer_material material;
    /* AABB */
    struct {
        float min[3], max[3];
    } aabb;
};

enum renderer_sky_type {
    RST_TEXTURE,
    RST_PREETHAM,
    RST_NONE
};

struct renderer_light {
    /* Light type */
    enum renderer_light_type {
        LT_DIRECTIONAL,
        LT_POINT
    } type;
    /* Common light type data */
    vec3 color;
    /* Light type-specific data */
    union {
        struct {
            vec3 direction;
        } dir;
        struct {
            vec3 position;
            float radius;
        } pt;
    } type_data;
};

/* Scene description passed to render function */
struct renderer_input {
    /* Meshes to render */
    struct renderer_mesh* meshes;
    unsigned int num_meshes;
    /* Sky type and parameters */
    enum renderer_sky_type sky_type;
    unsigned int sky_tex; /* Used if sky_type is RST_TEXTURE */
    struct {
        float inclination;
        float azimuth;
    } sky_pp; /* Used if sky_type  is RST_PREETHAM */
    /* Lighting parameters */
    struct renderer_light* lights;
    unsigned int num_lights;
};

/*-----------------------------------------------------------------
 * Renderer state
 *-----------------------------------------------------------------*/
/* Shader fetch function, used by renderer_shdr_fetch.
 * Takes as input the name of a shader and returns shader handle */
typedef unsigned int(*rndr_shdr_fetch_fn)(const char*, void* userdata);

/* Aggregate state */
struct renderer_state {
    /* Internals */
    struct renderer_internal_state* internal;
    /* Options */
    struct {
        unsigned int show_bboxes;
        unsigned int show_fprof;
        unsigned int show_gbuf_textures;
        unsigned int show_normals;
        unsigned int show_gidata;
        unsigned int use_occlusion_culling;
        unsigned int use_normal_mapping;
        unsigned int use_rough_met_maps;
        unsigned int use_detail_maps;
        unsigned int use_shadows;
        unsigned int use_envlight;
        unsigned int use_tonemapping;
        unsigned int use_gamma_correction;
        unsigned int use_antialiasing;
    } options;
};

/* Public interface */
void renderer_init(struct renderer_state* rs, rndr_shdr_fetch_fn sfn, void* sf_ud);
void renderer_render(struct renderer_state* rs, struct renderer_input* ri, float view_mat[16]);
void renderer_gi_update(struct renderer_state* rs, struct renderer_input* ri);
void renderer_shdr_fetch(struct renderer_state* rs);
void renderer_resize(struct renderer_state* rs, unsigned int width, unsigned int height);
void renderer_destroy(struct renderer_state* rs);

#endif /* ! _RENDERER_H_ */
