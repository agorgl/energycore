#include "eyeadapt.h"
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <prof.h>
#include "exposure.h"
#include "opengl.h"

#define NUM_HISTOGRAM_BINS 64

struct histogram_buf {
    float scale_offs[2];
    uint32_t histogram[NUM_HISTOGRAM_BINS];
    float exposure_val;
};

void eyeadapt_init(struct eyeadapt* s)
{
    memset(s, 0, sizeof(*s));
    s->last_exposure = exposure_from_ev100(16);
    float range[2] = { -10.0, 20 }; /* In EV */
    float scale = 1.0f / (range[1] - range[0]);
    float offst = -range[0] * scale;
    struct histogram_buf hist_buf = {
        .scale_offs   = { scale, offst },
        .exposure_val = s->last_exposure
    };
    GLuint sbuf;
    glGenBuffers(1, &sbuf);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sbuf);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(hist_buf), &hist_buf, GL_DYNAMIC_COPY);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    s->gl.ssbo = sbuf;
}

void eyeadapt_luminance_hist(struct eyeadapt* s)
{
    timepoint_t prev_tp = s->last_change, cur_tp = millisecs();
    s->last_change = cur_tp;

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, s->gl.ssbo);
    struct histogram_buf* hbuf = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    s->last_exposure = hbuf->exposure_val;
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    GLint draw_tex, src_width, src_height;
    glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &draw_tex);
    glBindTexture(GL_TEXTURE_2D, draw_tex);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &src_width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &src_height);

    glUseProgram(s->gl.shdr_clr);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, s->gl.ssbo);
    glDispatchCompute(NUM_HISTOGRAM_BINS / 16, 1, 1);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glUseProgram(s->gl.shdr_hist);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, draw_tex);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, s->gl.ssbo);
    glDispatchCompute(ceil(src_width / 16 / 2), ceil(src_height / 16 / 2), 1);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glUseProgram(s->gl.shdr_expo);
    glUniform1f(glGetUniformLocation(s->gl.shdr_expo, "delta_time"), (cur_tp - prev_tp) / 1000.0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, s->gl.ssbo);
    glDispatchCompute(1, 1, 1);

    glUseProgram(0);
}

float eyeadapt_exposure_mult(struct eyeadapt* s)
{
    return s->last_exposure;
}

void eyeadapt_destroy(struct eyeadapt* s)
{
    glDeleteBuffers(1, &s->gl.ssbo);
}
