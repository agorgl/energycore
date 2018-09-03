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
#ifndef _GIRNDR_H_
#define _GIRNDR_H_

#include <stdlib.h>
#include <linalgb.h>

struct gi_rndr {
    /* Sub renderers */
    struct probe_rndr* probe_rndr;
    /* Mini gbuffer used when updating probes */
    struct gbuffer* probe_gbuf;
    /* Probes */
    struct gi_probe_data {
        vec3 pos;
        double sh_coeffs[25][3];
        struct probe* p;
    }* pdata, fallback_probe;
    size_t num_probes;
    /* Running state */
    struct {
        unsigned int pidx;
        unsigned int side;
    } rs;
};

/* Global illumination renderer interface */
void gi_rndr_init(struct gi_rndr* r);
void gi_rndr_destroy(struct gi_rndr* r);
void gi_add_probe(struct gi_rndr* r, vec3 pos);
/* Updates global illumination data */
void gi_update_begin(struct gi_rndr* r);
int  gi_update_pass_begin(struct gi_rndr* r, mat4* view, mat4* proj);
void gi_update_pass_end(struct gi_rndr* r);
void gi_update_end(struct gi_rndr* r);
void gi_preprocess(struct gi_rndr* r, unsigned int irr_conv_shdr, unsigned int prefilter_shdr);
void gi_upload_sh_coeffs(unsigned int shdr, double sh_coef[25][3]);
/* Visualizes light probes, for debugging purposes */
void gi_vis_probes(struct gi_rndr* r, unsigned int shdr, float view[16], float proj[16], unsigned int mode);

/* Convenience macros */
#define gi_render_passes(gir, pview, pproj) \
    for (int _break = (gi_update_begin(gir), 1); _break; gi_update_end(gir), _break = 0) \
        for (;gi_update_pass_begin(gir, &pview, &pproj); gi_update_pass_end(gir))

#endif /* ! _GIRNDR_H_ */
