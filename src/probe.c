#include "probe.h"
#include <string.h>
#include <assert.h>
#include <glad/glad.h>
#include <emproc/sh.h>
#include <emproc/filter_util.h>
#include "glutils.h"

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
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    for (unsigned int i = 0; i < 6; ++i)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA8, side, side, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    p->cm = cubemap;
}

void probe_destroy(struct probe* p)
{
    glDeleteTextures(1, &p->cm);
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

void probe_rndr_init(struct probe_rndr* pr)
{
    /* Projection matrix
     * NOTE: Negative FOV is used to let view up vectors be positive while rendering upside down to the cubemap */
    pr->fproj = mat4_perspective(-radians(90.0f), 0.1f, 300.0f, 1.0f);
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
    vec3 ffront = vec3_new(view_fronts[side][0], view_fronts[side][1], view_fronts[side][2]);
    vec3 fup = vec3_new(view_ups[side][0], view_ups[side][1], view_ups[side][2]);
    mat4 fview = mat4_view_look_at(pos, vec3_add(pos, ffront), fup);
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
void probe_proc_init(struct probe_proc* pp)
{
    /* Build normal/solid angle index */
    const unsigned int side = PROBE_CUBEMAP_SIZE;
    float* nsa_idx = malloc(normal_solid_angle_index_sz(side));
    normal_solid_angle_index_build(nsa_idx, side, EM_TYPE_VSTRIP);
    pp->nsa_idx = nsa_idx;
}

void probe_extract_shcoeffs(struct probe_proc* pp, double sh_coef[25][3], struct probe* p)
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
    struct envmap em;
    em.width = side;
    em.height = side * 6;
    em.channels = 3;
    em.data = cm_buf;
    em.type = EM_TYPE_VSTRIP;
    sh_coeffs(sh_coef, &em, pp->nsa_idx);

    /* Free temporary pixel buffer */
    free(cm_buf);
}

void probe_proc_destroy(struct probe_proc* pp)
{
    /* Free normal/solid angle index */
    free(pp->nsa_idx);
}

/*-----------------------------------------------------------------
 * Probe Visualization
 *-----------------------------------------------------------------*/
void probe_vis_init(struct probe_vis* pv)
{
    memset(pv, 0, sizeof(*pv));
    const unsigned int num_sectors = 64;
    /* Create sphere data */
    struct sphere_gdata* vdat = uv_sphere_create(1.0f, num_sectors, num_sectors);
    /* Create vao */
    GLuint sph_vao, sph_vbo, sph_ebo;
    glGenVertexArrays(1, &sph_vao);
    glBindVertexArray(sph_vao);
    /* Create vbo */
    glGenBuffers(1, &sph_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, sph_vbo);
    glBufferData(GL_ARRAY_BUFFER, vdat->num_verts * sizeof(struct sphere_vertice), vdat->vertices, GL_STATIC_DRAW);
    /* Setup vertex inputs */
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(struct sphere_vertice), (GLvoid*)offsetof(struct sphere_vertice, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(struct sphere_vertice), (GLvoid*)offsetof(struct sphere_vertice, uvs));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(struct sphere_vertice), (GLvoid*)offsetof(struct sphere_vertice, normal));
    /* Create ebo */
    glGenBuffers(1, &sph_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sph_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, vdat->num_indices * sizeof(uint32_t), vdat->indices, GL_STATIC_DRAW);
    /* Store gpu handles */
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    pv->vao = sph_vao;
    pv->vbo = sph_vbo;
    pv->ebo = sph_ebo;
    pv->num_indices = vdat->num_indices;
    uv_sphere_destroy(vdat);
}

void probe_vis_render(struct probe_vis* pv, struct probe* p, vec3 probe_pos, mat4 view, mat4 proj, int mode)
{
    /* Calculate view position */
    mat4 inverse_view = mat4_inverse(view);
    vec3 view_pos = vec3_new(inverse_view.xw, inverse_view.yw, inverse_view.zw);

    /* Render setup */
    glUseProgram(pv->shdr);
    glUniformMatrix4fv(glGetUniformLocation(pv->shdr, "view"), 1, GL_FALSE, view.m);
    glUniformMatrix4fv(glGetUniformLocation(pv->shdr, "proj"), 1, GL_FALSE, proj.m);
    glUniform1i(glGetUniformLocation(pv->shdr, "u_mode"), mode);
    glUniform1i(glGetUniformLocation(pv->shdr, "u_envmap"), 0);
    glUniform3f(glGetUniformLocation(pv->shdr, "u_view_pos"), view_pos.x, view_pos.y, view_pos.z);
    const float scale = 0.2f;
    mat4 model = mat4_mul_mat4(
        mat4_translation(probe_pos),
        mat4_scale(vec3_new(scale, scale, scale)));
    glUniformMatrix4fv(glGetUniformLocation(pv->shdr, "model"), 1, GL_FALSE, model.m);
    /* Render */
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, p->cm);
    glBindVertexArray(pv->vao);
    glDrawElements(GL_TRIANGLES, pv->num_indices, GL_UNSIGNED_INT, (void*)0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

void probe_vis_destroy(struct probe_vis* pv)
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDeleteBuffers(1, &pv->ebo);
    glDeleteBuffers(1, &pv->vbo);
    glDeleteVertexArrays(1, &pv->vao);
}
