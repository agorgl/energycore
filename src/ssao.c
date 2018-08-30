#include "ssao.h"
#include <string.h>
#include <assert.h>
#include <linalgb.h>
#include "rand.h"
#include "opengl.h"
#include "glutils.h"

static float rf() { return genrand_real1(); }

void ssao_init(struct ssao* s, int width, int height)
{
    memset(s, 0, sizeof(*s));

    /* Populate framebuffers */
    struct { GLuint* fbo, *tex; } fbi[] = {
        {&s->gl.ao_fbo, &s->gl.ao_ctex},
        {&s->gl.blur_fbo, &s->gl.blur_ctex}
    };
    for (unsigned int i = 0; i < 2; ++i) {
        GLuint* fbo = fbi[i].fbo;
        GLuint* tex = fbi[i].tex;
        glGenFramebuffers(1, fbo);
        glGenTextures(1, tex);
        glBindTexture(GL_TEXTURE_2D, *tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RGB, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindFramebuffer(GL_FRAMEBUFFER, *fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *tex, 0);
        assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    }

    /* Generate noise texture */
    vec3 ssao_noise[16];
    for (size_t i = 0; i < 16; ++i) {
        vec3 noise = {{
            rf() * 2.0 - 1.0,
            rf() * 2.0 - 1.0,
            0.0
        }};
        ssao_noise[i] = noise;
    }
    GLuint noise_tex;
    glGenTextures(1, &noise_tex);
    glBindTexture(GL_TEXTURE_2D, noise_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssao_noise);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    s->gl.noise_tex = noise_tex;

    /* Generate kernel values */
    ssao_populate_kernel(s, 16);
}

void ssao_populate_kernel(struct ssao* s, size_t sz)
{
    free(s->kernel);
    s->kernel_sz = sz;
    s->kernel = calloc(s->kernel_sz, sizeof(*s->kernel));
    for (size_t i = 0; i < s->kernel_sz; ++i) {
        vec3 sample = {{
            rf() * 2.0 - 1.0,
            rf() * 2.0 - 1.0,
            rf()
        }};
        sample = vec3_normalize(sample);
        sample = vec3_mul(sample, rf());

        /* Scale samples s.t. they're more aligned to center of kernel */
        float scale = (float)i / s->kernel_sz;
        scale = lerpp(0.1, 1.0, scale * scale);
        sample = vec3_mul(sample, scale);
        memcpy(s->kernel[i], sample.xyz, sizeof(s->kernel[i]));
    }
}

void ssao_ao_pass(struct ssao* s, float viewport[2], float view[16], float proj[16])
{
    GLint prev_fbo;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, s->gl.ao_fbo);
    glClear(GL_COLOR_BUFFER_BIT);

    GLuint shdr = s->gl.ao_shdr;
    glUseProgram(shdr);

    glUniform1i(glGetUniformLocation(shdr, "gbuf.depth"), 0);
    glUniform1i(glGetUniformLocation(shdr, "gbuf.normal"), 1);
    glUniform1i(glGetUniformLocation(shdr, "gbuf.albedo"), 2);
    glUniform1i(glGetUniformLocation(shdr, "gbuf.roughness_metallic"), 3);
    glUniform2fv(glGetUniformLocation(shdr, "u_screen"), 1, viewport);
    mat4 inv_view_proj = mat4_inverse(mat4_mul_mat4(*(mat4*)proj, *(mat4*)view));
    glUniformMatrix4fv(glGetUniformLocation(shdr, "u_inv_view_proj"), 1, GL_FALSE, inv_view_proj.m);

    glUniformMatrix4fv(glGetUniformLocation(shdr, "view"), 1, GL_FALSE, view);
    glUniformMatrix4fv(glGetUniformLocation(shdr, "proj"), 1, GL_FALSE, proj);
    glUniform3fv(glGetUniformLocation(shdr, "samples"), s->kernel_sz, (GLfloat*)s->kernel);
    glUniform1i(glGetUniformLocation(shdr, "kernel_sz"), s->kernel_sz);
    glUniform1i(glGetUniformLocation(shdr, "tex_noise"), 4);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, s->gl.noise_tex);
    render_quad();

    glBindFramebuffer(GL_FRAMEBUFFER, prev_fbo);
}

void ssao_blur_pass(struct ssao* s)
{
    GLint prev_fbo;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, s->gl.blur_fbo);
    glClear(GL_COLOR_BUFFER_BIT);

    GLuint shdr = s->gl.blur_shdr;
    glUseProgram(shdr);
    glUniform1i(glGetUniformLocation(shdr, "blur_sz"), 4);
    glUniform1i(glGetUniformLocation(shdr, "tex"), 4);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, s->gl.ao_ctex);
    render_quad();

    glBindFramebuffer(GL_FRAMEBUFFER, prev_fbo);
}

void ssao_destroy(struct ssao* s)
{
    free(s->kernel);
    glDeleteTextures(1, &s->gl.noise_tex);
    glDeleteTextures(1, &s->gl.ao_ctex);
    glDeleteTextures(1, &s->gl.blur_ctex);
    glDeleteFramebuffers(1, &s->gl.ao_fbo);
    glDeleteFramebuffers(1, &s->gl.blur_fbo);
}
