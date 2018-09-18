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
#ifndef _CAMCTRL_H_
#define _CAMCTRL_H_

#include <linalgb.h>

struct camctrl {
    /* Position and Orientation */
    vec3 pos;
    quat rot;
    vec3 eul;
    vec3 vel;
    vec3 angvel;
    /* Attributes */
    float pitch_lim;
    float accel;
    float max_vel;
    float ang_accel;
    float max_angvel;
    float sensitivity;
    /* Cached values */
    vec3 prev_pos;
    quat prev_rot;
    mat4 view_mat;
};

enum camctrl_move_dir {
    cmd_forward  = 1 << 0,
    cmd_left     = 1 << 1,
    cmd_backward = 1 << 2,
    cmd_right    = 1 << 3
};

void camctrl_defaults(struct camctrl* cam);
void camctrl_move(struct camctrl* cam, int move_directions, float dt);
void camctrl_look(struct camctrl* cam, float offx, float offy, float dt);
void camctrl_update(struct camctrl* cam, float dt);
void camctrl_setpos(struct camctrl* cam, vec3 pos);
void camctrl_setdir(struct camctrl* cam, vec3 dir);
mat4 camctrl_interpolated_view(struct camctrl* cam, float interpolaton);

#endif /* ! _CAMCTRL_H_ */
