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
struct lc_renderer {
    float* nsa_idx;
    struct {
        unsigned int fb;
        unsigned int depth_rb;
    } glh;
    mat4 fproj;
    struct {
        int prev_vp[4];
    } rs;
};

/* Local cubemap renderer interface */
void lc_renderer_init(struct lc_renderer* lcr);
unsigned int lc_create_cm(struct lc_renderer* lcr);
void lc_render_begin(struct lc_renderer* lcr);
void lc_render_side_begin(struct lc_renderer* lcr, unsigned int side, unsigned int lcl_cubemap, vec3 pos, mat4* view, mat4* proj);
void lc_render_side_end(struct lc_renderer* lcr, unsigned int side);
void lc_render_end(struct lc_renderer* lcr);
void lc_extract_sh_coeffs(struct lc_renderer* lcr, double sh_coef[SH_COEFF_NUM][3], unsigned int cm);
void lc_renderer_destroy(struct lc_renderer* lcr);

/* Convenience macros */
#define lc_render_faces(lcr, lcl_cubemap, pos, fview, fproj) \
    for (int _break = (lc_render_begin(lcr), 1); _break; lc_render_end(lcr), _break = 0) \
        for (unsigned int side = 0; (side < 6 && (lc_render_side_begin(lcr, side, lcl_cubemap, pos, &fview, &fproj), 1)); \
                (lc_render_side_end(lcr, side), ++side))

#endif /* ! _LCL_CUBEMAP_H_ */
