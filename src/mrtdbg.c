#include "mrtdbg.h"
#include <string.h>
#include <glad/glad.h>
#include "glutils.h"

#define GLSRCEXT(src) "#version 330 core\n" \
                      "#extension GL_ARB_sample_shading : enable\n" \
                      #src

/* Minimum constant defined by OpenGL spec */
#define MAX_DRAW_BUFS 8

static const char* vs_src = GLSRCEXT(
layout (location = 0) in vec3 position;
out vec2 uv;

void main()
{
    uv = position.xy * 0.5 + 0.5;
    gl_Position = vec4(position, 1.0);
}
);

static const char* fs_src = GLSRCEXT(
out vec4 color;
in vec2 uv;

uniform sampler2DMS tex;

void main()
{
    ivec2 st = ivec2(textureSize(tex) * uv);
    vec3 tc = texelFetch(tex, st, gl_SampleID).rgb;
    color = vec4(tc, 1.0);
}
);

static struct {
    GLuint shdr;
} st;

static size_t gather_target_textures(GLuint* textures)
{
    unsigned int count = 0;
    for (int i = 0; i < MAX_DRAW_BUFS; ++i) {
        /* Get current attachment's type */
        GLint attachment_type = GL_NONE;
        glGetFramebufferAttachmentParameteriv(
            GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
            GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
            &attachment_type
        );
        /* Skip non texture attachments */
        if (attachment_type != GL_TEXTURE)
            continue;
        /* Get respective texture handle */
        GLuint tex = 0;
        glGetFramebufferAttachmentParameteriv(
            GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
            GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
            (GLint*)&tex
        );
        textures[count++] = tex;
    }
    return count;
}

static void show_texture_list(GLuint* textures, size_t num_textures)
{
    /* Get default framebuffer size */
    GLint default_vp[4] = {0};
    glGetIntegerv(GL_VIEWPORT, default_vp);
    GLint w = default_vp[2];
    GLint h = default_vp[3];

    /* Calculate each subviewport's dimensions */
    const int shown_textures = 4;
    const int padding = 6;
    const int tot_padding = (shown_textures + 1) * padding;
    const float scl_fctor = ((float)(w - tot_padding) / shown_textures) / (float) w;
    GLint tex_w = w * scl_fctor;
    GLint tex_h = h * scl_fctor;

    /* Setup texture shader */
    glUseProgram(st.shdr);
    glUniform1i(glGetUniformLocation(st.shdr, "tex"), 0);
    glActiveTexture(GL_TEXTURE0);

    /* Draw texture by texture */
    for (size_t i = 0; i < num_textures; ++i) {
        /* Set subviewport parameters */
        glViewport(padding + i * (tex_w + padding), padding, tex_w, tex_h);
        /* Draw texture */
        GLenum target = GL_TEXTURE_2D_MULTISAMPLE;
        glBindTexture(target, textures[i]);
        render_quad();
        glBindTexture(target, 0);
    }
    glUseProgram(0);

    /* Restore viewport */
    glViewport(default_vp[0], default_vp[1], default_vp[2], default_vp[3]);
}

void mrtdbg_init()
{
    st.shdr = shader_from_srcs(vs_src, 0, fs_src);
}

void mrtdbg_show_textures(GLuint fbo)
{
    /* Store previous bound framebuffer */
    GLuint prev_fbo;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, (GLint*)&prev_fbo);

    /* Gather target textures */
    GLuint fbo_textures[MAX_DRAW_BUFS];
    memset(fbo_textures, 0, sizeof(fbo_textures));
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
    size_t num_textures = gather_target_textures(fbo_textures);

    /* Draw them */
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    show_texture_list(fbo_textures, num_textures);

    /* Restore framebuffer binding */
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prev_fbo);
}

void mrtdbg_destroy()
{
    glDeleteProgram(st.shdr);
}
