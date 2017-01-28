#include <energycore/renderer.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <glad/glad.h>
#include "sky_texture.h"
#include "sky_preetham.h"
#include "probe_vis.h"
#include "lcl_cubemap.h"
#include "glutils.h"

void renderer_init(struct renderer_state* rs, struct renderer_params* rp)
{
    /* Populate renderer state according to init params */
    memset(rs, 0, sizeof(*rs));
    rs->shdr_main = rp->shdr_main;
    renderer_resize(rs, 1280, 720);

    /* Initialize internal texture sky state */
    rs->sky_rs.tex = malloc(sizeof(struct sky_texture));
    sky_texture_init(rs->sky_rs.tex);

    /* Initialize internal preetham sky state */
    rs->sky_rs.preeth = malloc(sizeof(struct sky_preetham));
    sky_preetham_init(rs->sky_rs.preeth);

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
    glEnable(GL_CULL_FACE);
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
        /* Set font face */
        float model_det = mat4_det(*(mat4*)rm->model_mat);
        glFrontFace(model_det < 0 ? GL_CW : GL_CCW);
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
    glFrontFace(GL_CCW);

    /* Render sky last */
    switch (ri->sky_type) {
        case RST_TEXTURE: {
            sky_texture_render(rs->sky_rs.tex, (mat4*)view, (mat4*)proj, ri->sky_tex);
            break;
        }
        case RST_PREETHAM: {
            struct sky_preetham_params sp_params;
            sky_preetham_default_params(&sp_params);
            sky_preetham_render(rs->sky_rs.preeth, &sp_params, (mat4*)proj, (mat4*)view);
            break;
        }
        case RST_NONE:
        default:
            break;
    }
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

    /* Render local cubemap */
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

void renderer_resize(struct renderer_state* rs, unsigned int width, unsigned int height)
{
    glViewport(0, 0, width, height);
    rs->proj = mat4_perspective(radians(60.0f), 0.1f, 300.0f, ((float)width / height));
}

void renderer_destroy(struct renderer_state* rs)
{
    lc_renderer_destroy(rs->lc_rs);
    free(rs->lc_rs);
    probe_vis_destroy(rs->probe_vis);
    free(rs->probe_vis);
    sky_preetham_destroy(rs->sky_rs.preeth);
    free(rs->sky_rs.preeth);
    sky_texture_destroy(rs->sky_rs.tex);
    free(rs->sky_rs.tex);
}
