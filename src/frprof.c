#include "frprof.h"
#include <assert.h>
#include <stdio.h>
#include "opengl.h"

struct frame_prof* frame_prof_init()
{
    struct frame_prof* fp = calloc(1, sizeof(struct frame_prof));
    fp->enabled = 0;
    fp->cur_buf = 0;
    fp->last_buf_rdy = 0;
    for (unsigned int i = 0; i < FP_MAX_TIMEPOINTS; ++i) {
        for (unsigned int j = 0; j < FP_NUM_BUFFERS; ++j) {
            glGenQueries(1, &fp->timepoints[i].query_ids[j][0]);
            glGenQueries(1, &fp->timepoints[i].query_ids[j][1]);
        }
    }
    return fp;
}

void frame_prof_destroy(struct frame_prof* fp)
{
    for (unsigned int i = 0; i < fp->num_timepoints; ++i) {
        for (unsigned int j = 0; j < FP_NUM_BUFFERS; ++j) {
            glDeleteQueries(1, &fp->timepoints[i].query_ids[j][0]);
            glDeleteQueries(1, &fp->timepoints[i].query_ids[j][1]);
        }
    }
    free(fp);
}

unsigned int frame_prof_timepoint_start(struct frame_prof* fp)
{
    if (!fp->enabled)
        return 0;
    assert(fp->num_timepoints + 1 <= FP_MAX_TIMEPOINTS);
    ++fp->num_timepoints;
    glQueryCounter(fp->timepoints[fp->num_timepoints - 1].query_ids[fp->cur_buf][0], GL_TIMESTAMP);
    return fp->num_timepoints - 1;
}

void frame_prof_timepoint_end(struct frame_prof* fp)
{
    if (!fp->enabled)
        return;
    glQueryCounter(fp->timepoints[fp->num_timepoints - 1].query_ids[fp->cur_buf][1], GL_TIMESTAMP);
}

void frame_prof_flush(struct frame_prof* fp)
{
    /* To avoid crash on first call */
    if (fp->last_buf_rdy == 0) {
        fp->last_buf_rdy = 1;
        goto new_frame;
    }

    /* Update samples */
    size_t last_buf = fp->cur_buf == 0 ? FP_NUM_BUFFERS - 1 : fp->cur_buf - 1;
    for (unsigned int i = 0; i < fp->num_timepoints; ++i) {
        struct frame_prof_tp* tp = fp->timepoints + i;
        GLuint64 tp_begin, tp_end;
        glGetQueryObjectui64v(fp->timepoints[i].query_ids[last_buf][0], GL_QUERY_RESULT, &tp_begin);
        glGetQueryObjectui64v(fp->timepoints[i].query_ids[last_buf][1], GL_QUERY_RESULT, &tp_end);
        tp->cur_sample = (tp->cur_sample == FP_NUM_SAMPLES - 1) ? 0 : tp->cur_sample + 1;
        tp->samples[tp->cur_sample] = (tp_end - tp_begin) / 1000000.0f;
    }

new_frame:
    fp->num_timepoints = 0;
    fp->cur_buf = fp->cur_buf == FP_NUM_BUFFERS - 1 ? 0 : fp->cur_buf + 1;
}

float frame_prof_timepoint_msec(struct frame_prof* fp, unsigned int tp)
{
    assert(tp < FP_MAX_TIMEPOINTS);
    struct frame_prof_tp* fptp = fp->timepoints + tp;
    float tot_msec = 0;
    for (unsigned int i = 0; i < FP_NUM_SAMPLES; ++i)
        tot_msec += fptp->samples[i];
    tot_msec /= FP_NUM_SAMPLES;
    return tot_msec;
}
