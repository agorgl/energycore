#include "gbuffer.h"
#include <string.h>
#include <assert.h>
#include "opengl.h"

#define array_length(a) (sizeof(a)/sizeof(a[0]))

void gbuffer_init(struct gbuffer* gb, int width, int height)
{
    memset(gb, 0, sizeof(struct gbuffer));

    /* Store dimensions */
    gb->width = width;
    gb->height = height;

    /* Create framebuffer */
    glGenFramebuffers(1, &gb->fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, gb->fbo);

    /* Create data textures */
    struct {
        GLuint* id;
        GLint ifmt;
        GLenum fmt;
        GLenum pix_dtype;
        GLenum attachment;
    } data_texs[] = {
        {
            &gb->accum_buf,
            GL_RGB16F,
            GL_RGB,
            GL_FLOAT,
            GL_COLOR_ATTACHMENT0
        },
        {
            &gb->normal_buf,
            GL_RG16F,
            GL_RG,
            GL_FLOAT,
            GL_COLOR_ATTACHMENT1
        },
        {
            &gb->albedo_buf,
            GL_RGBA,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            GL_COLOR_ATTACHMENT2
        },
        {
            &gb->roughness_metallic_buf,
            GL_RG8,
            GL_RG,
            GL_UNSIGNED_BYTE,
            GL_COLOR_ATTACHMENT3
        },
        {
            &gb->depth_stencil_buf,
            GL_DEPTH24_STENCIL8,
            GL_DEPTH_STENCIL,
            GL_UNSIGNED_INT_24_8,
            GL_DEPTH_STENCIL_ATTACHMENT
        }
    };
    for (size_t i = 0; i < array_length(data_texs); ++i) {
        glGenTextures(1, data_texs[i].id);
        GLenum target = GL_TEXTURE_2D;
        glBindTexture(target, *(data_texs[i].id));
        glTexImage2D(target, 0,
                     data_texs[i].ifmt,
                     width, height, 0,
                     data_texs[i].fmt,
                     data_texs[i].pix_dtype, 0);
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, data_texs[i].attachment, target, *(data_texs[i].id), 0);
    }
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    /* Unbind stuff */
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void gbuffer_bind_for_geometry_pass(struct gbuffer* gb)
{
    glBindFramebuffer(GL_FRAMEBUFFER, gb->fbo);
    GLuint attachments[] = {
        GL_COLOR_ATTACHMENT1,
        GL_COLOR_ATTACHMENT2,
        GL_COLOR_ATTACHMENT3,
    };
    glDrawBuffers(array_length(attachments), attachments);
}

void gbuffer_bind_for_light_pass(struct gbuffer* gb)
{
    glBindFramebuffer(GL_FRAMEBUFFER, gb->fbo);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    gbuffer_bind_textures(gb);
}

void gbuffer_bind_textures(struct gbuffer* gb)
{
    /* Bind geometry pass textures */
    GLuint geom_tex[] = {
        gb->depth_stencil_buf,
        gb->normal_buf,
        gb->albedo_buf,
        gb->roughness_metallic_buf
    };
    for (unsigned int i = 0; i < array_length(geom_tex); ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, geom_tex[i]);
    }
}

void gbuffer_unbind_textures(struct gbuffer* gb)
{
    (void) gb;
    for (unsigned int i = 0; i < 4; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void gbuffer_blit_accum_to_fb(struct gbuffer* gb, unsigned int fb)
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, gb->fbo);
    glBlitFramebuffer(
        0, 0, gb->width, gb->height,
        0, 0, gb->width, gb->height,
        GL_COLOR_BUFFER_BIT,
        GL_NEAREST
    );
    glBindFramebuffer(GL_FRAMEBUFFER, fb);
}

void gbuffer_blit_depth_to_fb(struct gbuffer* gb, unsigned int fb)
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, gb->fbo);
    glBlitFramebuffer(
        0, 0, gb->width, gb->height,
        0, 0, gb->width, gb->height,
        GL_DEPTH_BUFFER_BIT,
        GL_NEAREST
    );
    glBindFramebuffer(GL_FRAMEBUFFER, fb);
}

void gbuffer_destroy(struct gbuffer* gb)
{
    GLuint textures[] = { gb->accum_buf, gb->normal_buf, gb->albedo_buf, gb->roughness_metallic_buf, gb->depth_stencil_buf };
    glDeleteTextures(array_length(textures), textures);
    glDeleteFramebuffers(1, &gb->fbo);
}
