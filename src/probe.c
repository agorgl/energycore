#include "probe.h"
#include <string.h>
#include <assert.h>
#include <math.h>
#include "opengl.h"
#include "glutils.h"
#include "shcomp.h"

/*-----------------------------------------------------------------
 * Probe
 *-----------------------------------------------------------------*/
#define PROBE_CUBEMAP_SIZE 128

void probe_init(struct probe* p)
{
    unsigned int side = PROBE_CUBEMAP_SIZE;
    GLuint cubemap;
    glGenTextures(1, &cubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    for (unsigned int i = 0; i < 6; ++i)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, side, side, 0, GL_RGB, GL_FLOAT, 0);
    p->cm = cubemap;
}

void probe_destroy(struct probe* p)
{
    glDeleteTextures(1, &p->cm);
    if (p->irr_diffuse_cm)
        glDeleteTextures(1, &p->irr_diffuse_cm);
    if (p->prefiltered_cm)
        glDeleteTextures(1, &p->prefiltered_cm);
}

/*-----------------------------------------------------------------
 * Probe Renderer
 *-----------------------------------------------------------------*/
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

static mat4 face_proj_mat()
{
    return mat4_perspective(-radians(90.0f), 0.1f, 300.0f, 1.0f);
}

static mat4 face_view_mat(unsigned int side, vec3 pos)
{
    /* Construct view matrix towards given face */
    vec3 ffront = vec3_new(view_fronts[side][0], view_fronts[side][1], view_fronts[side][2]);
    vec3 fup = vec3_new(view_ups[side][0], view_ups[side][1], view_ups[side][2]);
    mat4 fview = mat4_view_look_at(pos, vec3_add(pos, ffront), fup);
    return fview;
}

void probe_rndr_init(struct probe_rndr* pr)
{
    /* Projection matrix
     * NOTE: Negative FOV is used to let view up vectors be positive while rendering upside down to the cubemap */
    pr->fproj = face_proj_mat();
    /* Create fb */
    GLuint fb;
    glGenFramebuffers(1, &fb);
    glBindFramebuffer(GL_FRAMEBUFFER, fb);
    /* Renderer's framebuffer's depth attachment */
    int side = PROBE_CUBEMAP_SIZE;
    GLuint depth_rb;
    glGenRenderbuffers(1, &depth_rb);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_rb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, side, side);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rb);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    /* Save handles */
    pr->glh.fb = fb;
    pr->glh.depth_rb = depth_rb;
}

void probe_rndr_destroy(struct probe_rndr* pr)
{
    glBindFramebuffer(GL_FRAMEBUFFER, pr->glh.fb);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteRenderbuffers(1, &pr->glh.depth_rb);
    glDeleteFramebuffers(1, &pr->glh.fb);
}

void probe_render_begin(struct probe_rndr* pr)
{
    /* Store previous framebuffer */
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&pr->rs.prev_fb);
    /* Store previous viewport */
    GLint* viewport = pr->rs.prev_vp;
    glGetIntegerv(GL_VIEWPORT, viewport);
    /* Set new viewport */
    const int fbsize = PROBE_CUBEMAP_SIZE;
    glViewport(0, 0, fbsize, fbsize);
}

void probe_render_side_begin(struct probe_rndr* pr, unsigned int side, struct probe* p, vec3 pos, mat4* view, mat4* proj)
{
    /* Set framebuffer and bind side texture */
    glBindFramebuffer(GL_FRAMEBUFFER, pr->glh.fb);
    glBindTexture(GL_TEXTURE_CUBE_MAP, p->cm);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + side, p->cm, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    /* Check framebuffer completeness */
    GLenum fb_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    assert(fb_status == GL_FRAMEBUFFER_COMPLETE);
    /* Construct view matrix towards current face */
    mat4 fview = face_view_mat(side, pos);
    /* Render */
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    /* Return current face view and proj matrices */
    *view = fview;
    *proj = pr->fproj;
}

void probe_render_side_end(struct probe_rndr* pr, unsigned int side)
{
    glBindFramebuffer(GL_FRAMEBUFFER, pr->glh.fb);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + side, 0, 0);
}

void probe_render_end(struct probe_rndr* pr)
{
    /* Restore viewport */
    GLint* viewport = pr->rs.prev_vp;
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    /* Restore framebuffer */
    glBindFramebuffer(GL_FRAMEBUFFER, pr->rs.prev_fb);
}

/*-----------------------------------------------------------------
 * Probe Processing
 *-----------------------------------------------------------------*/
void probe_extract_shcoeffs(double sh_coef[25][3], struct probe* p)
{
    const int side = PROBE_CUBEMAP_SIZE;
    /* Allocate memory buffer for cubemap pixels */
    const size_t img_sz = side * side * 3;
    uint8_t* cm_buf = malloc(img_sz * 6);
    memset(cm_buf, 0, sizeof(img_sz * 6));

    /* Fetch cubemap pixel data */
    glBindTexture(GL_TEXTURE_CUBE_MAP, p->cm);
    for (unsigned int i = 0; i < 6; ++i) {
        GLuint target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + i;
        /* Read texture back */
        glGetTexImage(target, 0, GL_RGB, GL_UNSIGNED_BYTE, cm_buf + i * img_sz);
    }

    /* Calc sh coeffs */
    sh_coeffs(sh_coef, cm_buf, side);

    /* Free temporary pixel buffer */
    free(cm_buf);
}

unsigned int probe_convolute_irradiance_diff(struct probe* p, unsigned int irr_conv_shdr)
{
    /* Create an irradiance cubemap, and re-scale capture fbo to irradiance scale */
    const unsigned int res = 32;
    GLuint irradiance_map;
    glGenTextures(1, &irradiance_map);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradiance_map);
    for (unsigned int i = 0; i < 6; ++i)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, res, res, 0, GL_RGB, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    /* Temporary framebuffer */
    GLuint capture_fbo;
    GLuint capture_rbo;
    glGenFramebuffers(1, &capture_fbo);
    glGenRenderbuffers(1, &capture_rbo);
    glBindFramebuffer(GL_FRAMEBUFFER, capture_fbo);
    glBindRenderbuffer(GL_RENDERBUFFER, capture_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, res, res);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, capture_rbo);

    /* Solve diffuse integral by convolution to create an irradiance (cube)map */
    glUseProgram(irr_conv_shdr);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, p->cm);
    glUniform1i(glGetUniformLocation(irr_conv_shdr, "env_map"), 0);
    mat4 proj = face_proj_mat();
    glUniformMatrix4fv(glGetUniformLocation(irr_conv_shdr, "proj"), 1, GL_FALSE, proj.m);

    GLint prev_vp[4];
    glGetIntegerv(GL_VIEWPORT, prev_vp);
    glViewport(0, 0, res, res);
    for (unsigned int i = 0; i < 6; ++i) {
        mat4 fview = face_view_mat(i, vec3_zero());
        glUniformMatrix4fv(glGetUniformLocation(irr_conv_shdr, "view"), 1, GL_FALSE, fview.m);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                               irradiance_map, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        render_cube();
    }

    /* Cleanup / Restore */
    glUseProgram(0);
    glViewport(prev_vp[0], prev_vp[1], prev_vp[2], prev_vp[3]);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glDeleteRenderbuffers(1, &capture_rbo);
    glDeleteFramebuffers(1, &capture_fbo);
    return irradiance_map;
}

unsigned int probe_convolute_irradiance_spec(struct probe* p, unsigned int prefilter_shdr)
{
    /* Create a pre-filter cubemap, and re-scale capture fbo to prefilter scale */
    const unsigned int res = 128;
    GLuint prefilter_map;
    glGenTextures(1, &prefilter_map);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilter_map);
    for (unsigned int i = 0; i < 6; ++i)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, res, res, 0, GL_RGB, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); /* Important! */
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    /* Generate mipmaps for the cubemap so OpenGL automatically allocates the required memory */
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    /* Temporary framebuffer */
    GLuint capture_fbo;
    GLuint capture_rbo;
    glGenFramebuffers(1, &capture_fbo);
    glGenRenderbuffers(1, &capture_rbo);
    glBindFramebuffer(GL_FRAMEBUFFER, capture_fbo);
    glBindRenderbuffer(GL_RENDERBUFFER, capture_rbo);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, capture_rbo);

    /* Run a quasi monte-carlo simulation on the environment lighting to create a prefilter (cube)map */
    glUseProgram(prefilter_shdr);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, p->cm);
    glUniform1i(glGetUniformLocation(prefilter_shdr, "env_map"), 0);
    mat4 proj = face_proj_mat();
    glUniformMatrix4fv(glGetUniformLocation(prefilter_shdr, "proj"), 1, GL_FALSE, proj.m);

    GLint prev_vp[4];
    glGetIntegerv(GL_VIEWPORT, prev_vp);
    const unsigned int max_mip_levels = 5;
    for (unsigned int mip = 0; mip < max_mip_levels; ++mip) {
        /* Resize framebuffer according to mip-level size. */
        unsigned int mip_res = res * pow(0.5, mip);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mip_res, mip_res);
        glViewport(0, 0, mip_res, mip_res);

        float roughness = (float)mip/(float)(max_mip_levels - 1);
        glUniform1f(glGetUniformLocation(prefilter_shdr, "roughness"), roughness);
        glUniform1f(glGetUniformLocation(prefilter_shdr, "map_res"), mip_res);
        for (unsigned int i = 0; i < 6; ++i) {
            mat4 fview = face_view_mat(i, vec3_zero());
            glUniformMatrix4fv(glGetUniformLocation(prefilter_shdr, "view"), 1, GL_FALSE, fview.m);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                                   prefilter_map, mip);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            render_cube();
        }
    }

    /* Cleanup / Restore */
    glUseProgram(0);
    glViewport(prev_vp[0], prev_vp[1], prev_vp[2], prev_vp[3]);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glDeleteRenderbuffers(1, &capture_rbo);
    glDeleteFramebuffers(1, &capture_fbo);
    return prefilter_map;
}

void probe_preprocess(struct probe* p, unsigned int irr_conv_shdr, unsigned int prefilt_shdr)
{
    if (p->irr_diffuse_cm)
        glDeleteTextures(1, &p->irr_diffuse_cm);
    if (p->prefiltered_cm)
        glDeleteTextures(1, &p->prefiltered_cm);
    p->irr_diffuse_cm = probe_convolute_irradiance_diff(p, irr_conv_shdr);
    p->prefiltered_cm = probe_convolute_irradiance_spec(p, prefilt_shdr);
}

/*-----------------------------------------------------------------
 * Probe Visualization
 *-----------------------------------------------------------------*/
void probe_vis_render(struct probe* p, vec3 probe_pos, unsigned int shdr, mat4 view, mat4 proj, int mode)
{
    /* Calculate view position */
    mat4 inverse_view = mat4_inverse(view);
    vec3 view_pos = vec3_new(inverse_view.xw, inverse_view.yw, inverse_view.zw);
    /* Render setup */
    glUseProgram(shdr);
    glUniformMatrix4fv(glGetUniformLocation(shdr, "view"), 1, GL_FALSE, view.m);
    glUniformMatrix4fv(glGetUniformLocation(shdr, "proj"), 1, GL_FALSE, proj.m);
    glUniform1i(glGetUniformLocation(shdr, "u_mode"), mode);
    glUniform1i(glGetUniformLocation(shdr, "u_envmap"), 0);
    glUniform3f(glGetUniformLocation(shdr, "u_view_pos"), view_pos.x, view_pos.y, view_pos.z);
    const float scale = 0.2f;
    mat4 model = mat4_mul_mat4(
        mat4_translation(probe_pos),
        mat4_scale(vec3_new(scale, scale, scale)));
    glUniformMatrix4fv(glGetUniformLocation(shdr, "model"), 1, GL_FALSE, model.m);
    /* Render */
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, p->cm);
    render_sphere();
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glUseProgram(0);
}

/*-----------------------------------------------------------------
 * Misc
 *-----------------------------------------------------------------*/
unsigned int brdf_lut_generate(unsigned int brdf_lut_shdr)
{
    /* Setup lut texture */
    const int res = 512;
    GLuint brdf_lut_tex;
    glGenTextures(1, &brdf_lut_tex);
    glBindTexture(GL_TEXTURE_2D, brdf_lut_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, res, res, 0, GL_RG, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    /* Temporary framebuffer */
    GLuint capture_fbo;
    GLuint capture_rbo;
    glGenFramebuffers(1, &capture_fbo);
    glGenRenderbuffers(1, &capture_rbo);
    glBindFramebuffer(GL_FRAMEBUFFER, capture_fbo);
    glBindRenderbuffer(GL_RENDERBUFFER, capture_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, res, res);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, capture_rbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdf_lut_tex, 0);

    /* Screen pass */
    GLint prev_vp[4];
    glGetIntegerv(GL_VIEWPORT, prev_vp);
    glViewport(0, 0, res, res);
    glUseProgram(brdf_lut_shdr);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    render_quad();

    /* Cleanup / Restore */
    glUseProgram(0);
    glViewport(prev_vp[0], prev_vp[1], prev_vp[2], prev_vp[3]);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteRenderbuffers(1, &capture_rbo);
    glDeleteFramebuffers(1, &capture_fbo);
    return brdf_lut_tex;
}
