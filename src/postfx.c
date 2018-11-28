#include "postfx.h"
#include <string.h>
#include <assert.h>
#include "opengl.h"
#include "glutils.h"

void postfx_init(struct postfx* pfx, unsigned int width, unsigned int height)
{
    memset(pfx, 0, sizeof(*pfx));
    pfx->cur_fbo = 0;

    /* Store dimensions */
    pfx->width = width;
    pfx->height = height;

    glGenFramebuffers(2, (GLuint*)&pfx->glh.fbo);
    glGenTextures(2, (GLuint*)&pfx->glh.color);
    glGenTextures(2, (GLuint*)&pfx->glh.depth);

    glGenTextures(1, (GLuint*)&pfx->glh.stashed);
    glBindTexture(GL_TEXTURE_2D, pfx->glh.stashed);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    struct {
        GLuint* tex_arr;
        GLint ifmt;
        GLenum fmt;
        GLenum pix_dtype;
        GLenum attachment;
    } tex_types[] = {
        {
            pfx->glh.color,
            GL_RGBA16F,
            GL_RGBA,
            GL_FLOAT,
            GL_COLOR_ATTACHMENT0
        },
        {
            pfx->glh.depth,
            GL_DEPTH24_STENCIL8,
            GL_DEPTH_STENCIL,
            GL_UNSIGNED_INT_24_8,
            GL_DEPTH_STENCIL_ATTACHMENT
        }
    };
    for (unsigned int i = 0; i < 2; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, pfx->glh.fbo[i]);
        for (unsigned int j = 0; j < 2; ++j) {
            GLuint tex = tex_types[j].tex_arr[i];
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexImage2D(GL_TEXTURE_2D, 0,
                         tex_types[j].ifmt,
                         width, height, 0,
                         tex_types[j].fmt,
                         tex_types[j].pix_dtype, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glFramebufferTexture2D(GL_FRAMEBUFFER, tex_types[j].attachment, GL_TEXTURE_2D, tex, 0);
        }
        assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    }

    /* Unbind stuff */
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void postfx_pass(struct postfx* pfx)
{
    GLuint draw_fbo = pfx->glh.fbo[!pfx->cur_fbo];
    GLuint read_fbo = pfx->glh.fbo[pfx->cur_fbo];
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw_fbo);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, read_fbo);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pfx->glh.color[pfx->cur_fbo]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    render_quad();
    pfx->cur_fbo = !pfx->cur_fbo;
}

void postfx_stash_cur(struct postfx* pfx)
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER, pfx->glh.fbo[pfx->cur_fbo]);
    glBindTexture(GL_TEXTURE_2D, pfx->glh.stashed);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, pfx->width, pfx->height);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void postfx_blit_fb_to_read(struct postfx* pfx, unsigned int fb)
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, pfx->glh.fbo[pfx->cur_fbo]);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fb);
    glBlitFramebuffer(
        0, 0, pfx->width, pfx->height,
        0, 0, pfx->width, pfx->height,
        GL_COLOR_BUFFER_BIT, GL_NEAREST
    );
    glBindFramebuffer(GL_FRAMEBUFFER, pfx->glh.fbo[pfx->cur_fbo]);
}

void postfx_blit_read_to_fb(struct postfx* pfx, unsigned int fb)
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, pfx->glh.fbo[pfx->cur_fbo]);
    glBlitFramebuffer(
        0, 0, pfx->width, pfx->height,
        0, 0, pfx->width, pfx->height,
        GL_COLOR_BUFFER_BIT, GL_NEAREST
    );
    glBindFramebuffer(GL_FRAMEBUFFER, fb);
}

unsigned int postfx_stashed_tex(struct postfx* pfx)
{
    return pfx->glh.stashed;
}

void postfx_destroy(struct postfx* pfx)
{
    glDeleteTextures(1, &pfx->glh.stashed);
    glDeleteTextures(2, pfx->glh.depth);
    glDeleteTextures(2, pfx->glh.color);
    glDeleteFramebuffers(2, pfx->glh.fbo);
}
