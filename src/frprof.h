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
#ifndef _FRPROF_H_
#define _FRPROF_H_

#include <stdlib.h>

#define FP_MAX_TIMEPOINTS 10
#define FP_NUM_SAMPLES 16
#define FP_NUM_BUFFERS 2

struct frame_prof {
    unsigned int enabled;
    struct frame_prof_tp {
        unsigned int query_ids[FP_NUM_BUFFERS][2];
        float samples[FP_NUM_SAMPLES];
        size_t cur_sample;
    } timepoints[FP_MAX_TIMEPOINTS];
    size_t cur_timepoint;
    size_t num_timepoints;
    size_t cur_buf, last_buf_rdy;
};

/* Init/deinit */
struct frame_prof* frame_prof_init();
void frame_prof_destroy(struct frame_prof* fp);
/* Called to populate last frame's async'd timer values */
void frame_prof_flush(struct frame_prof* fp);
/* Adds successive timepoint query, returns timepoint index */
unsigned int frame_prof_timepoint_start(struct frame_prof* fp);
void frame_prof_timepoint_end(struct frame_prof* fp);
/* Queries nth timepoint msec value */
float frame_prof_timepoint_msec(struct frame_prof* fp, unsigned int tp);

/* Convenience macros */
#define with_fprof(fp, state) \
    for (int _break = (fp->enabled = state, 1); (_break || (fp->enabled = 0, 0)); _break = 0)

#define frame_prof_timepoint(fp) \
    for (int _break = (frame_prof_timepoint_start(fp), 1); \
            (_break || (frame_prof_timepoint_end(fp), 0)); _break = 0)

#endif /* ! _FRPROF_H_ */
