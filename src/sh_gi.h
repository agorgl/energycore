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
#ifndef _SH_GI_H_
#define _SH_GI_H_

#include <linalgb.h>

/* Scene render callback signature (used by local cubemap renderer, and screen pass) */
typedef void(*rndr_scene_cb)(mat4* view, mat4* proj, void* userdata);

struct sh_gi_renderer {
    /* Sub renderers */
    struct probe_vis* probe_vis;
    struct lc_renderer_state* lc_rs;
    /* Local cubemap renderer gbuffer */
    struct gbuffer* lcr_gbuf;
    /* Shader */
    unsigned int shdr;
    /* Probes */
    struct {
        vec3 pos;
        double sh_coeffs[25][3];
        unsigned int cm;
    } probe;
    /* Misc */
    float angle; /* Time varying variable for debuging purposes */
};

/* Initializes internal renderer state */
void sh_gi_init(struct sh_gi_renderer* shgi_rndr);
/* Updates probes' local cubemaps and sh coefficients */
void sh_gi_update(struct sh_gi_renderer* shgi_rndr, rndr_scene_cb rndr_scn, void* rndr_scene_userdata);
/* Makes full screen pass to render gi light */
void sh_gi_render(struct sh_gi_renderer* shgi_rndr, float view[16], float proj[16], rndr_scene_cb rndr_scn, void* rndr_scene_userdata);
/* Visualizes light probes, for debugging purposes */
void sh_gi_vis_probes(struct sh_gi_renderer* shgi_rndr, float view[16], float proj[16], unsigned int mode);
/* Deinitializes internal renderer state */
void sh_gi_destroy(struct sh_gi_renderer* shgi_rndr);

#endif /* ! _SH_GI_H_ */