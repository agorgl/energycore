#include <energycore/renderer.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <glad/glad.h>
#include <emproc/sh.h>
#include <emproc/filter_util.h>
#include "skybox.h"
#include "probe_vis.h"
#include "glutils.h"

#define LCL_CM_SIZE 128

void renderer_init(struct renderer_state* rs, struct renderer_params* rp)
{
    /* Populate renderer state according to init params */
    memset(rs, 0, sizeof(*rs));
    rs->shdr_main = rp->shdr_main;
    rs->proj = mat4_perspective(radians(45.0f), 0.1f, 300.0f, (800.0f / 600.0f));

    /* Initialize internal skybox state */
    rs->skybox = malloc(sizeof(struct skybox));
    skybox_init(rs->skybox);

    /* Initialize internal probe visualization state */
    rs->probe_vis = malloc(sizeof(struct probe_vis));
    probe_vis_init(rs->probe_vis, rp->shdr_probe_vis);

    float* nsa_idx = malloc(normal_solid_angle_index_sz(LCL_CM_SIZE));
    normal_solid_angle_index_build(nsa_idx, LCL_CM_SIZE, EM_TYPE_VSTRIP);
    rs->sh.nsa_idx = nsa_idx;
}

static void render_scene(struct renderer_state* rs, struct renderer_input* ri, float view[16], float proj[16], int render_mode)
{
    /* Clear default buffers */
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* Render */
    glEnable(GL_DEPTH_TEST);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glUseProgram(rs->shdr_main);
    glUniform1i(glGetUniformLocation(rs->shdr_main, "render_mode"), render_mode);

    /* Setup matrices */
    GLuint proj_mat_loc = glGetUniformLocation(rs->shdr_main, "proj");
    GLuint view_mat_loc = glGetUniformLocation(rs->shdr_main, "view");
    GLuint modl_mat_loc = glGetUniformLocation(rs->shdr_main, "model");
    glUniformMatrix4fv(proj_mat_loc, 1, GL_FALSE, proj);
    glUniformMatrix4fv(view_mat_loc, 1, GL_FALSE, (GLfloat*)view);
    /* Misc uniforms */
    mat4 inverse_view = mat4_inverse(*(mat4*)view);
    vec3 view_pos = vec3_new(inverse_view.xw, inverse_view.yw, inverse_view.zw);
    glUniform3f(glGetUniformLocation(rs->shdr_main, "view_pos"), view_pos.x, view_pos.y, view_pos.z);

    /* Loop through meshes */
    for (unsigned int i = 0; i < ri->num_meshes; ++i) {
        /* Setup mesh to be rendered */
        struct renderer_mesh* rm = ri->meshes + i;
        /* Upload model matrix */
        glUniformMatrix4fv(modl_mat_loc, 1, GL_FALSE, rm->model_mat);
        /* Render mesh */
        glBindVertexArray(rm->vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rm->ebo);
        glDrawElements(GL_TRIANGLES, rm->indice_count, GL_UNSIGNED_INT, (void*)0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    glUseProgram(0);

    /* Render skybox last */
    skybox_render(rs->skybox, (mat4*)view, (mat4*)proj, ri->skybox);
}

static unsigned int render_local_cubemap(struct renderer_state* rs, struct renderer_input* ri, vec3 pos)
{
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

    /* Vectors corresponding to cubemap faces */
    const float view_fronts[6][3] = {
        { 1.0f,  0.0f,  0.0f },
        {-1.0f,  0.0f,  0.0f },
        { 0.0f,  1.0f,  0.0f },
        { 0.0f, -1.0f,  0.0f },
        { 0.0f,  0.0f,  1.0f },
        { 0.0f,  0.0f, -1.0f }
    };
    const float view_ups[6][3] = {
        { 0.0f, -1.0f,  0.0f },
        { 0.0f, -1.0f,  0.0f },
        { 0.0f,  0.0f,  1.0f },
        { 0.0f,  0.0f, -1.0f },
        { 0.0f, -1.0f,  0.0f },
        { 0.0f, -1.0f,  0.0f }
    };

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
        render_scene(rs, ri, (float*)&fview, (float*)&fproj, 0);
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

static void sh_coeffs_from_local_cubemap(struct renderer_state* rs, double sh_coef[SH_COEFF_NUM][3], GLuint cm)
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
    sh_coeffs(sh_coef, &em, rs->sh.nsa_idx);

    /* Free temporary pixel buffer */
    free(cm_buf);
}

static void upload_sh_coeffs(GLuint prog, double sh_coef[SH_COEFF_NUM][3])
{
    const char* uniform_name = "sh_coeffs";
    size_t uniform_name_len = strlen(uniform_name);
    glUseProgram(prog);
    for (unsigned int i = 0; i < SH_COEFF_NUM; ++i) {
        /* Construct uniform name ("sh_coeffs" + "[" + i + "]" + '\0') */
        size_t uname_sz = uniform_name_len + 1 + 2 + 1 + 1;
        char* uname = calloc(uname_sz, 1);
        strcat(uname, uniform_name);
        strcat(uname, "[");
        snprintf(uname + uniform_name_len + 1, 3, "%u", i);
        strcat(uname, "]");
        /* Upload */
        GLuint uloc = glGetUniformLocation(prog, uname);
        float c[3] = { (float)sh_coef[i][0], (float)sh_coef[i][1], (float)sh_coef[i][2] };
        glUniform3f(uloc, c[0], c[1], c[2]);
        free(uname);
    }
    glUseProgram(0);
}

void renderer_render(struct renderer_state* rs, struct renderer_input* ri, float view[16])
{
    /* Calculate probe position */
    rs->prob_angle += 0.01f;
    vec3 probe_pos = vec3_new(cosf(rs->prob_angle), 1.0f, sinf(rs->prob_angle));

    /* Render local skybox */
    GLuint lcl_sky = render_local_cubemap(rs, ri, probe_pos);

    /* Calculate and upload SH coefficients from captured cubemap */
    double sh_coeffs[SH_COEFF_NUM][3];
    sh_coeffs_from_local_cubemap(rs, sh_coeffs, lcl_sky);
    upload_sh_coeffs(rs->shdr_main, sh_coeffs);

    /* Render from camera view */
    glUseProgram(rs->shdr_main);
    glUniform3f(glGetUniformLocation(rs->shdr_main, "probe_pos"), probe_pos.x, probe_pos.y, probe_pos.z);
    render_scene(rs, ri, view, (float*)&rs->proj, 1);

    /* Render sample probe */
    upload_sh_coeffs(rs->probe_vis->shdr, sh_coeffs);
    probe_vis_render(rs->probe_vis, lcl_sky, probe_pos, *(mat4*)view, rs->proj, 1);
    glDeleteTextures(1, &lcl_sky);
}

void renderer_destroy(struct renderer_state* rs)
{
    free(rs->sh.nsa_idx);
    probe_vis_destroy(rs->probe_vis);
    free(rs->probe_vis);
    skybox_destroy(rs->skybox);
    free(rs->skybox);
}
