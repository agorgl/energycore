#include <energycore/renderer.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <glad/glad.h>
#include "skybox.h"
#include "probe_vis.h"
#include "lcl_cubemap.h"
#include "glutils.h"

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

    /* Initialize local cubemap renderer state */
    rs->lc_rs = malloc(sizeof(struct lc_renderer_state));
    lc_renderer_init(rs->lc_rs);
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

    /* Set texture locations */
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(glGetUniformLocation(rs->shdr_main, "albedo_tex"), 0);

    /* Loop through meshes */
    for (unsigned int i = 0; i < ri->num_meshes; ++i) {
        /* Setup mesh to be rendered */
        struct renderer_mesh* rm = ri->meshes + i;
        /* Upload model matrix */
        glUniformMatrix4fv(modl_mat_loc, 1, GL_FALSE, rm->model_mat);
        /* Set material parameters */
        glBindTexture(GL_TEXTURE_2D, rm->material.diff_tex);
        glUniform3fv(glGetUniformLocation(rs->shdr_main, "albedo_col"), 1, rm->material.diff_col);
        /* Render mesh */
        glBindVertexArray(rm->vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rm->ebo);
        glDrawElements(GL_TRIANGLES, rm->indice_count, GL_UNSIGNED_INT, (void*)0);
        /* Reset bindings */
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    glUseProgram(0);

    /* Render skybox last */
    skybox_render(rs->skybox, (mat4*)view, (mat4*)proj, ri->skybox);
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

struct lc_render_scene_params {
    struct renderer_state* rs;
    struct renderer_input* ri;
};

static void render_scene_for_lc(mat4* view, mat4* proj, void* userdata)
{
    struct lc_render_scene_params* p = userdata;
    render_scene(p->rs, p->ri, (float*)view, (float*)proj, 0);
}

void renderer_render(struct renderer_state* rs, struct renderer_input* ri, float view[16])
{
    /* Calculate probe position */
    rs->prob_angle += 0.01f;
    vec3 probe_pos = vec3_new(cosf(rs->prob_angle), 1.0f, sinf(rs->prob_angle));

    /* Render local skybox */
    struct lc_render_scene_params rsp;
    rsp.rs = rs;
    rsp.ri = ri;
    GLuint lcl_sky = lc_render(rs->lc_rs, probe_pos, render_scene_for_lc, &rsp);

    /* Calculate and upload SH coefficients from captured cubemap */
    double sh_coeffs[SH_COEFF_NUM][3];
    lc_extract_sh_coeffs(rs->lc_rs, sh_coeffs, lcl_sky);
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
    lc_renderer_destroy(rs->lc_rs);
    free(rs->lc_rs);
    probe_vis_destroy(rs->probe_vis);
    free(rs->probe_vis);
    skybox_destroy(rs->skybox);
    free(rs->skybox);
}
