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
#ifndef _LCL_CUBEMAP_H_
#define _LCL_CUBEMAP_H_

#include <linalgb.h>
#include <emproc/sh.h>

/* Local cubemap side dimension */
#define LCL_CM_SIZE 128

/* Shared local cubemap renderer state */
struct lc_renderer_state {
    float* nsa_idx;
    unsigned int fb;
    unsigned int depth_rb;
    mat4 fproj;
};

/* Callback function called to render the scene for a local cubemap's side */
typedef void(*render_scene_fn)(mat4* view, mat4* proj, void* userdata);

/* Local cubemap renderer interface */
void lc_renderer_init(struct lc_renderer_state* lcrs);
unsigned int lc_create_cm(struct lc_renderer_state* lcrs);
void lc_render(struct lc_renderer_state* lcrs, unsigned int lcl_cubemap, vec3 pos, render_scene_fn rsf, void* userdata);
void lc_extract_sh_coeffs(struct lc_renderer_state* lcrs, double sh_coef[SH_COEFF_NUM][3], unsigned int cm);
void lc_renderer_destroy(struct lc_renderer_state* lcrs);

#endif /* ! _LCL_CUBEMAP_H_ */
