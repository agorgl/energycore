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
struct renderer_mesh {
    /* Geometry handles */
    unsigned int vao;
    unsigned int ebo;
    unsigned int indice_count;
    /* Model matrix */
    float model_mat[16];
    /* Material parameters */
    struct {
        float diff_col[3];
        unsigned int diff_tex;
        float diff_tex_scl[2];
        unsigned int norm_tex;
        float norm_tex_scl[2];
        unsigned int rough_tex;
        float rough_tex_scl[2];
        unsigned int metal_tex;
        float metal_tex_scl[2];
    } material;
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

/* Sky renderers */
struct sky_renderer_state {
    struct sky_texture* tex;
    struct sky_preetham* preeth;
};

/* Aggregate state */
struct renderer_state {
    /* Internal subrenderers */
    struct sky_renderer_state sky_rs;
    struct sh_gi_renderer* sh_gi_rs;
    struct bbox_rndr* bbox_rs;
    struct shadowmap* shdwmap;
    struct postfx* postfx;
    struct panicscr_rndr* ps_rndr;
    /* Shader handle fetching */
    rndr_shdr_fetch_fn shdr_fetch_cb;
    void* shdr_fetch_userdata;
    /* Shaders */
    struct {
        unsigned int geom_pass;
        unsigned int light_pass;
        struct {
            unsigned int tonemap;
            unsigned int gamma;
        } fx;
    } shdrs;
    /* GBuffer */
    struct gbuffer* gbuf;
    /* Occlusion culling */
    struct occull_state* occl_st;
    /* Cached values */
    vec2 viewport;
    mat4 proj;
    /* Frame profiler and debug info's */
    struct frame_prof* fprof;
    struct {
        unsigned int num_visible_objs;
        float gpass_msec;
        float lpass_msec;
    } dbginfo;
    /* Options */
    struct {
        unsigned int show_bboxes;
        unsigned int show_fprof;
        unsigned int show_gbuf_textures;
        unsigned int use_occlusion_culling;
        unsigned int use_normal_mapping;
        unsigned int use_rough_met_maps;
        unsigned int use_shadows;
        unsigned int use_tonemapping;
        unsigned int use_gamma_correction;
    } options;
};

/* Public interface */
void renderer_init(struct renderer_state* rs, rndr_shdr_fetch_fn sfn, void* sf_ud);
void renderer_render(struct renderer_state* rs, struct renderer_input* ri, float view_mat[16]);
void renderer_shdr_fetch(struct renderer_state* rs);
void renderer_resize(struct renderer_state* rs, unsigned int width, unsigned int height);
void renderer_destroy(struct renderer_state* rs);

#endif /* ! _RENDERER_H_ */
