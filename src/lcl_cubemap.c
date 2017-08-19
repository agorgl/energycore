#include "lcl_cubemap.h"
#include <string.h>
#include <assert.h>
#include <glad/glad.h>
#include <emproc/filter_util.h>

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
    { 0.0f,  1.0f,  0.0f },
    { 0.0f,  1.0f,  0.0f },
    { 0.0f,  0.0f, -1.0f },
    { 0.0f,  0.0f,  1.0f },
    { 0.0f,  1.0f,  0.0f },
    { 0.0f,  1.0f,  0.0f }
};

void lc_renderer_init(struct lc_renderer* lcr)
{
    /* Build normal/solid angle index */
    float* nsa_idx = malloc(normal_solid_angle_index_sz(LCL_CM_SIZE));
    normal_solid_angle_index_build(nsa_idx, LCL_CM_SIZE, EM_TYPE_VSTRIP);
    lcr->nsa_idx = nsa_idx;
    /* Projection matrix
     * NOTE: Negative FOV is used to let view up vectors be positive while rendering upside down to the cubemap */
    lcr->fproj = mat4_perspective(-radians(90.0f), 0.1f, 300.0f, 1.0f);
    /* Create fb */
    GLuint fb;
    glGenFramebuffers(1, &fb);
    glBindFramebuffer(GL_FRAMEBUFFER, fb);
    /* Renderer's framebuffer's depth attachment */
    int side = LCL_CM_SIZE;
    GLuint depth_rb;
    glGenRenderbuffers(1, &depth_rb);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_rb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, side, side);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rb);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    /* Save handles */
    lcr->glh.fb = fb;
    lcr->glh.depth_rb = depth_rb;
}

unsigned int lc_create_cm(struct lc_renderer* lcr)
{
    (void) lcr;
    unsigned int side = LCL_CM_SIZE;
    GLuint lcl_cubemap;
    glGenTextures(1, &lcl_cubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, lcl_cubemap);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    for (unsigned int i = 0; i < 6; ++i)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA8, side, side, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    return lcl_cubemap;
}

void lc_render_begin(struct lc_renderer* lcr)
{
    glBindFramebuffer(GL_FRAMEBUFFER, lcr->glh.fb);
    /* Store previous viewport and set the new one */
    int fbwidth = LCL_CM_SIZE, fbheight = LCL_CM_SIZE;
    GLint* viewport = lcr->rs.prev_vp;
    glGetIntegerv(GL_VIEWPORT, viewport);
    glViewport(0, 0, fbwidth, fbheight);
}

void lc_render_side_begin(struct lc_renderer* lcr, unsigned int side, unsigned int lcl_cubemap, vec3 pos, mat4* view, mat4* proj)
{
    (void) lcr;
    /* Create and set texture face */
    glBindTexture(GL_TEXTURE_CUBE_MAP, lcl_cubemap);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + side, lcl_cubemap, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    /* Check framebuffer completeness */
    GLenum fb_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    assert(fb_status == GL_FRAMEBUFFER_COMPLETE);
    /* Construct view matrix towards current face */
    vec3 ffront = vec3_new(view_fronts[side][0], view_fronts[side][1], view_fronts[side][2]);
    vec3 fup = vec3_new(view_ups[side][0], view_ups[side][1], view_ups[side][2]);
    mat4 fview = mat4_view_look_at(pos, vec3_add(pos, ffront), fup);
    /* Render */
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    /* Return current face view and proj matrices */
    *view = fview;
    *proj = lcr->fproj;
}

void lc_render_side_end(struct lc_renderer* lcr, unsigned int side)
{
    (void) lcr;
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + side, 0, 0);
}

void lc_render_end(struct lc_renderer* lcr)
{
    /* Restore viewport */
    GLint* viewport = lcr->rs.prev_vp;
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void lc_extract_sh_coeffs(struct lc_renderer* lcr, double sh_coef[25][3], unsigned int cm)
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
    sh_coeffs(sh_coef, &em, lcr->nsa_idx);

    /* Free temporary pixel buffer */
    free(cm_buf);
}

void lc_renderer_destroy(struct lc_renderer* lcr)
{
    /* Free renderer gpu resources */
    glBindFramebuffer(GL_FRAMEBUFFER, lcr->glh.fb);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteRenderbuffers(1, &lcr->glh.depth_rb);
    glDeleteFramebuffers(1, &lcr->glh.fb);
    /* Free normal/sa index */
    free(lcr->nsa_idx);
}
