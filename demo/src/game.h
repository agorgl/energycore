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
#ifndef _GAME_H_
#define _GAME_H_

#include <linalgb.h>
#include <energycore/renderer.h>
#include "camera.h"

struct game_context
{
    /* Window assiciated with the game */
    struct window* wnd;
    /* Master run flag, indicates when the game should exit */
    int* should_terminate;
    /* GPU Resource Manager */
    struct res_mngr* rmgr;
    /* Scene list */
    struct scene* main_scene;
    /* Active scene pointer */
    struct scene* active_scene;
    /* Camera */
    struct camera cam;
    int fast_move;
    /* Sky */
    struct tex_hndl* sky_tex;
    /* Renderer state and his cached input */
    struct renderer_state rndr_state;
    struct renderer_input cached_ri;
};

/* Initializes the game instance */
void game_init(struct game_context* ctx);
/* Update callback used by the main loop */
void game_update(void* userdata, float dt);
/* Render callback used by the main loop */
void game_render(void* userdata, float interpolation);
/* Performance update callback used by the main loop */
void game_perf_update(void* userdata, float msec, float fps);
/* De-initializes the game instance */
void game_shutdown(struct game_context* ctx);

#endif /* ! _GAME_H_ */
