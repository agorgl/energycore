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
#include "resource.h"

/*-----------------------------------------------------------------
 * Renderer input
 *-----------------------------------------------------------------*/
struct render_object {
    rid mesh;
    rid materials[16];
    float model_mat[16];
};

struct render_light {
    /* Light type */
    enum render_light_type {
        LT_DIRECTIONAL,
        LT_POINT,
        LT_SPOT
    } type;
    /* Common light type data */
    vec3 color;
    float intensity;
    /* Light type-specific data */
    union {
        struct {
            vec3 direction;
        } dir;
        struct {
            vec3 position;
            float radius;
        } pt;
        struct {
            vec3 position;
            vec3 direction;
            float inner_cone;
            float outer_cone;
        } spt;
    } type_data;
};

/* Scene description passed to render function */
struct render_scene {
    /* Objects to render */
    struct render_object* objects;
    unsigned int num_objects;
    /* Sky type and parameters */
    enum {
        RST_TEXTURE,
        RST_PREETHAM,
        RST_NONE
    } sky_type;
    rid sky_tex; /* Used if sky_type is RST_TEXTURE */
    struct {
        float inclination;
        float azimuth;
    } sky_pp; /* Used if sky_type  is RST_PREETHAM */
    /* Lighting parameters */
    struct render_light* lights;
    unsigned int num_lights;
};

/*-----------------------------------------------------------------
 * Renderer state
 *-----------------------------------------------------------------*/
/* Aggregate state */
struct renderer_state {
    /* Internals */
    struct renderer_internal_state* internal;
    /* Resource manager */
    struct resmgr rmgr;
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
        unsigned int use_bloom;
        unsigned int use_tonemapping;
        unsigned int use_gamma_correction;
        unsigned int use_antialiasing;
        unsigned int use_ssao;
    } options;
};

/* Public interface */
void renderer_init(struct renderer_state* rs);
void renderer_render(struct renderer_state* rs, struct render_scene* rscn, float view_mat[16]);
void renderer_gi_update(struct renderer_state* rs, struct render_scene* rscn);
void renderer_resize(struct renderer_state* rs, unsigned int width, unsigned int height);
void renderer_destroy(struct renderer_state* rs);

#endif /* ! _RENDERER_H_ */
