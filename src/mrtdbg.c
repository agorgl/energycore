#include "mrtdbg.h"
#include <string.h>
#include "opengl.h"
#include "glutils.h"

#define GLSRCEXT(src) "#version 330 core\n" \
                      "#extension GL_ARB_sample_shading : enable\n" \
                      #src

/* Minimum constant defined by OpenGL spec */
#define MAX_MRT_BUFS 8

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

uniform sampler2D tex;
uniform sampler2DArray tex_arr;
uniform int array_mode;
uniform int channel_mode;
uniform int layer;

vec3 sampleSimple()
{
    ivec2 st = ivec2(textureSize(tex, 0) * uv);
    vec3 tc = vec3(0.0);
    switch (channel_mode) {
        case 0:
            tc = texelFetch(tex, st, 0).rgb;
            break;
        case 1:
            tc = texelFetch(tex, st, 0).rrr;
            break;
        case 2:
            tc = texelFetch(tex, st, 0).ggg;
            break;
        case 3:
            tc = texelFetch(tex, st, 0).bbb;
            break;
        case 4:
            tc = texelFetch(tex, st, 0).aaa;
            break;
    }
    return tc;
}

vec3 sampleArray()
{
    ivec2 st = ivec2(textureSize(tex_arr, 0).xy * uv);
    vec3 tc = vec3(0.0);
    switch (channel_mode) {
        case 0:
            tc = texelFetch(tex_arr, ivec3(st, layer), 0).rgb;
            break;
        case 1:
            tc = texelFetch(tex_arr, ivec3(st, layer), 0).rrr;
            break;
        case 2:
            tc = texelFetch(tex_arr, ivec3(st, layer), 0).ggg;
            break;
        case 3:
            tc = texelFetch(tex_arr, ivec3(st, layer), 0).bbb;
            break;
        case 4:
            tc = texelFetch(tex_arr, ivec3(st, layer), 0).aaa;
            break;
    }
    return tc;
}

void main()
{
    vec3 tc = vec3(0.0);
    switch (array_mode) {
        case 0:
            tc = sampleSimple();
            break;
        case 1:
            tc = sampleArray();
            break;
    }
    color = vec4(tc, 1.0);
}
);

static struct {
    GLuint shdr;
} st;

static size_t gather_mrt_textures(GLuint* textures)
{
    /* Attachments to query */
    GLenum attachments[MAX_MRT_BUFS];
    for (int i = 0; i < MAX_MRT_BUFS - 1; ++i)
        attachments[i] = GL_COLOR_ATTACHMENT0 + i;
    attachments[MAX_MRT_BUFS - 1] = GL_DEPTH_ATTACHMENT;

    unsigned int count = 0;
    for (int i = 0; i < MAX_MRT_BUFS; ++i) {
        GLenum attachment = attachments[i];
        /* Get current attachment's type */
        GLint attachment_type = GL_NONE;
        glGetFramebufferAttachmentParameteriv(
            GL_DRAW_FRAMEBUFFER, attachment,
            GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
            &attachment_type
        );
        /* Skip non texture attachments */
        if (attachment_type != GL_TEXTURE)
            continue;
        /* Get respective texture handle */
        GLuint tex = 0;
        glGetFramebufferAttachmentParameteriv(
            GL_DRAW_FRAMEBUFFER, attachment,
            GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
            (GLint*)&tex
        );
        textures[count++] = tex;
    }
    return count;
}

static void show_texture_list(struct mrtdbg_tex_info* tex_infos, size_t num_textures)
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
    glUniform1i(glGetUniformLocation(st.shdr, "tex_arr"), 1);

    /* Draw texture by texture */
    for (size_t i = 0; i < num_textures; ++i) {
        struct mrtdbg_tex_info* ti = tex_infos + i;
        /* Set subviewport parameters */
        glViewport(padding + i * (tex_w + padding), padding, tex_w, tex_h);
        /* Set draw mode */
        glUniform1i(glGetUniformLocation(st.shdr, "array_mode"), ti->type);
        glUniform1i(glGetUniformLocation(st.shdr, "channel_mode"), ti->mode);
        /* Upload layer if needed */
        if (ti->type == 1)
            glUniform1i(glGetUniformLocation(st.shdr, "layer"), ti->layer);
        /* Draw texture */
        glActiveTexture(ti->type == 1 ? GL_TEXTURE1 : GL_TEXTURE0);
        GLenum target = ti->type == 1 ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
        glBindTexture(target, ti->handle);
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

void mrtdbg_show_fbo_textures(GLuint fbo)
{
    /* Store previous bound framebuffer */
    GLuint prev_fbo;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, (GLint*)&prev_fbo);

    /* Gather target textures */
    GLuint fbo_textures[MAX_MRT_BUFS];
    memset(fbo_textures, 0, sizeof(fbo_textures));
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
    size_t num_textures = gather_mrt_textures(fbo_textures);

    /* Fill texture informations */
    struct mrtdbg_tex_info tex_infos[MAX_MRT_BUFS];
    memset(tex_infos, 0, sizeof(tex_infos));
    for (unsigned int i = 0; i < num_textures; ++i) {
        tex_infos[i].handle = fbo_textures[i];
        tex_infos[i].type = 0;
        tex_infos[i].mode = MRTDBG_MODE_RGB;
    }

    /* Draw them */
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    show_texture_list(tex_infos, num_textures);

    /* Restore framebuffer binding */
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prev_fbo);
}

void mrtdbg_show_textures(struct mrtdbg_tex_info* textures, unsigned int num_textures)
{
    /* Store previous bound framebuffer */
    GLuint prev_fbo;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, (GLint*)&prev_fbo);

    /* Draw them */
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    show_texture_list(textures, num_textures);

    /* Restore framebuffer binding */
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prev_fbo);
}

void mrtdbg_destroy()
{
    glDeleteProgram(st.shdr);
}
