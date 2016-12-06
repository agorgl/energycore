#include "lcl_cubemap.h"
#include <string.h>
#include <assert.h>
#include <glad/glad.h>
#include <emproc/filter_util.h>

#define LCL_CM_SIZE 128

/* Vectors corresponding to cubemap faces */
static const float view_fronts[6][3] = {
    { 1.0f,  0.0f,  0.0f },
    {-1.0f,  0.0f,  0.0f },
    { 0.0f,  1.0f,  0.0f },
    { 0.0f, -1.0f,  0.0f },
    { 0.0f,  0.0f,  1.0f },
    { 0.0f,  0.0f, -1.0f }
};

static const float view_ups[6][3] = {
    { 0.0f, -1.0f,  0.0f },
    { 0.0f, -1.0f,  0.0f },
    { 0.0f,  0.0f,  1.0f },
    { 0.0f,  0.0f, -1.0f },
    { 0.0f, -1.0f,  0.0f },
    { 0.0f, -1.0f,  0.0f }
};

void lc_renderer_init(struct lc_renderer_state* lcrs)
{
    float* nsa_idx = malloc(normal_solid_angle_index_sz(LCL_CM_SIZE));
    normal_solid_angle_index_build(nsa_idx, LCL_CM_SIZE, EM_TYPE_VSTRIP);
    lcrs->nsa_idx = nsa_idx;
}

unsigned int lc_render(struct lc_renderer_state* lcrs, vec3 pos, render_scene_fn rsf, void* userdata)
{
    (void) lcrs;
    /* Temporary framebuffer */
    int fbwidth = LCL_CM_SIZE, fbheight = LCL_CM_SIZE;
    GLuint fb;
    glGenFramebuffers(1, &fb);
    glBindFramebuffer(GL_FRAMEBUFFER, fb);

    /* Store previous viewport and set the new one */
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    glViewport(0, 0, fbwidth, fbheight);

    /* Cubemap result */
    GLuint lcl_cubemap;
    glGenTextures(1, &lcl_cubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, lcl_cubemap);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    for (unsigned int i = 0; i < 6; ++i)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA8, fbwidth, fbheight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

    /* Temporary framebuffer's depth attachment */
    GLuint depth_rb;
    glGenRenderbuffers(1, &depth_rb);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_rb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, fbwidth, fbheight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rb);

    /* Projection matrix */
    mat4 fproj = mat4_perspective(radians(90.0f), 0.1f, 300.0f, 1.0f);

    for (unsigned int i = 0; i < 6; ++i) {
        /* Create and set texture face */
        glBindTexture(GL_TEXTURE_CUBE_MAP, lcl_cubemap);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, lcl_cubemap, 0);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        /* Check framebuffer completeness */
        GLenum fb_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        assert(fb_status == GL_FRAMEBUFFER_COMPLETE);
        /* Construct view matrix towards current face */
        vec3 ffront = vec3_new(view_fronts[i][0], view_fronts[i][1], view_fronts[i][2]);
        vec3 fup = vec3_new(view_ups[i][0], view_ups[i][1], view_ups[i][2]);
        mat4 fview = mat4_view_look_at(pos, vec3_add(pos, ffront), fup);
        /* Render */
        rsf(&fview, &fproj, userdata);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, 0);
    }

    /* Free temporary resources */
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteRenderbuffers(1, &depth_rb);
    glDeleteFramebuffers(1, &fb);

    /* Restore viewport */
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    return lcl_cubemap;
}

void lc_extract_sh_coeffs(struct lc_renderer_state* lcrs, double sh_coef[25][3], unsigned int cm)
{
    const int im_size = LCL_CM_SIZE;
    /* Allocate memory buffer for cubemap pixels */
    const size_t img_sz = im_size * im_size * 3;
    uint8_t* cm_buf = malloc(img_sz * 6);
    memset(cm_buf, 0, sizeof(img_sz * 6));

    /* Fetch cubemap pixel data */
    glBindTexture(GL_TEXTURE_CUBE_MAP, cm);
    for (unsigned int i = 0; i < 6; ++i) {
        GLuint target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + i;
        /* Read texture back */
        glGetTexImage(target, 0, GL_RGB, GL_UNSIGNED_BYTE, cm_buf + i * img_sz);
    }

    /* Calc sh coeffs */
    struct envmap em;
    em.width = im_size;
    em.height = im_size * 6;
    em.channels = 3;
    em.data = cm_buf;
    em.type = EM_TYPE_VSTRIP;
    sh_coeffs(sh_coef, &em, lcrs->nsa_idx);

    /* Free temporary pixel buffer */
    free(cm_buf);
}

void lc_renderer_destroy(struct lc_renderer_state* lcrs)
{
    free(lcrs->nsa_idx);
}
