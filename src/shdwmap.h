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
#ifndef _SHDWMAP_H_
#define _SHDWMAP_H_

#include <linalgb.h>

#define SHADOWMAP_NSPLITS 4

struct shadowmap {
    /* Resolution */
    unsigned int width, height;
    /* GL handles */
    struct {
        unsigned int tex_id, fbo_id;
        unsigned int shdr;
    } glh;
    /* Split data */
    struct {
        mat4 proj_mat;
        mat4 view_mat;
        mat4 shdw_mat;
        vec2 plane;
        float near_plane, far_plane;
    } sd[SHADOWMAP_NSPLITS];
    /* Render state */
    struct {
        int prev_vp[4];
        unsigned int prev_fbo;
    } rs;
};

void shadowmap_init(struct shadowmap* sm, int width, int height);
void shadowmap_render_begin(struct shadowmap* sm, float light_pos[3], float view[16], float proj[16]);
void shadowmap_render_end(struct shadowmap* sm);
void shadowmap_bind(struct shadowmap* sm, unsigned int shdr);
void shadowmap_destroy(struct shadowmap* sm);

/* Convenience macros */
#define shadowmap_render(sm, light_pos, view, proj) \
    for (int _break = (shadowmap_render_begin(sm, light_pos, view, proj), 1), shdr = sm->glh.shdr; \
            (_break || (shadowmap_render_end(sm), 0)); _break = 0)

#endif /* ! _SHDWMAP_H_ */
