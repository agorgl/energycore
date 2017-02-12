#include <energycore/renderer.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <glad/glad.h>
#include "sky_texture.h"
#include "sky_preetham.h"
#include "probe_vis.h"
#include "sh_gi.h"
#include "gbuffer.h"
#include "glutils.h"
#include <assert.h>

/*-----------------------------------------------------------------
 * Initialization
 *-----------------------------------------------------------------*/
void renderer_init(struct renderer_state* rs, rndr_shdr_fetch_fn sfn, void* sh_ud)
{
    /* Populate renderer state according to init params */
    memset(rs, 0, sizeof(*rs));
    rs->shdr_fetch_cb = sfn;
    rs->shdr_fetch_userdata = sh_ud;

    /* Initial dimensions */
    int width = 1280, height = 720;
    rs->viewport.x = width; rs->viewport.y = height;

    /* Initialize gbuffer */
    rs->gbuf = malloc(sizeof(struct gbuffer));
    gbuffer_init(rs->gbuf, width, height, 1);

    /* Initial resize */
    renderer_resize(rs, width, height);

    /* Initialize gl utilities state */
    glutils_init();

    /* Initialize internal texture sky state */
    rs->sky_rs.tex = malloc(sizeof(struct sky_texture));
    sky_texture_init(rs->sky_rs.tex);

    /* Initialize internal preetham sky state */
    rs->sky_rs.preeth = malloc(sizeof(struct sky_preetham));
    sky_preetham_init(rs->sky_rs.preeth);

    /* Initialize internal sh_gi renderer state */
    rs->sh_gi_rs = malloc(sizeof(struct sh_gi_renderer));
    sh_gi_init(rs->sh_gi_rs);

    /* Fetch shaders */
    renderer_shdr_fetch(rs);
}

void renderer_shdr_fetch(struct renderer_state* rs)
{
    rs->shdrs.geom_pass  = rs->shdr_fetch_cb("geom_pass", rs->shdr_fetch_userdata);
    rs->shdrs.light_pass = rs->shdr_fetch_cb("light_pass", rs->shdr_fetch_userdata);
    rs->sh_gi_rs->shdr   = rs->shdr_fetch_cb("env_pass", rs->shdr_fetch_userdata);
    rs->sh_gi_rs->probe_vis->shdr = rs->shdr_fetch_cb("probe_vis", rs->shdr_fetch_userdata);
}

/*-----------------------------------------------------------------
 * Geometry pass
 *-----------------------------------------------------------------*/
static void geometry_pass(struct renderer_state* rs, struct renderer_input* ri, float view[16], float proj[16])
{
    /* Bind gbuf */
    gbuffer_bind_for_geometry_pass(rs->gbuf);

    /* Enable depth and culling */
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);

    /* Clear */
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* Pick shader for the mode */
    GLuint shdr = rs->shdrs.geom_pass;
    glUseProgram(shdr);

    /* Setup matrices */
    GLuint proj_mat_loc = glGetUniformLocation(shdr, "proj");
    GLuint view_mat_loc = glGetUniformLocation(shdr, "view");
    GLuint modl_mat_loc = glGetUniformLocation(shdr, "model");
    glUniformMatrix4fv(proj_mat_loc, 1, GL_FALSE, proj);
    glUniformMatrix4fv(view_mat_loc, 1, GL_FALSE, (GLfloat*)view);

    /* Set texture locations */
    glUniform1i(glGetUniformLocation(shdr, "mat.albedo_tex"), 0);

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
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, rm->material.diff_tex);
        glUniform3fv(glGetUniformLocation(shdr, "mat.albedo_col"), 1, rm->material.diff_col);
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
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void bind_gbuffer_textures(struct renderer_state* rs, GLuint shdr, mat4* view, mat4* proj)
{
    gbuffer_bind_for_light_pass(rs->gbuf);
    glUseProgram(shdr);
    glUniform1i(glGetUniformLocation(shdr, "gbuf.depth"), 0);
    glUniform1i(glGetUniformLocation(shdr, "gbuf.normal"), 1);
    glUniform1i(glGetUniformLocation(shdr, "gbuf.albedo"), 2);
    glUniform2fv(glGetUniformLocation(shdr, "u_screen"), 1, rs->viewport.xy);
    mat4 inv_view_proj = mat4_inverse(mat4_mul_mat4(*proj, *view));
    glUniformMatrix4fv(glGetUniformLocation(shdr, "u_inv_view_proj"), 1, GL_FALSE, inv_view_proj.m);
    glUseProgram(0);
}

/*-----------------------------------------------------------------
 * Light pass
 *-----------------------------------------------------------------*/
static void render_sky(struct renderer_state* rs, struct renderer_input* ri, float* view, float* proj)
{
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

static void light_pass(struct renderer_state* rs, struct renderer_input* ri, mat4* view, mat4* proj)
{
    /* Clear */
    glClear(GL_COLOR_BUFFER_BIT);

    /* Disable writting to depth buffer for screen space renders */
    glDepthMask(GL_FALSE);

    /* Enable blend for additive lighting */
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE);

    /* Setup gbuffer input textures */
    GLuint shdr = rs->shdrs.light_pass;
    bind_gbuffer_textures(rs, shdr, view, proj);

    /* Setup common uniforms */
    glUseProgram(shdr);
    mat4 inverse_view = mat4_inverse(*(mat4*)view);
    vec3 view_pos = vec3_new(inverse_view.xw, inverse_view.yw, inverse_view.zw);
    glUniform3f(glGetUniformLocation(shdr, "view_pos"), view_pos.x, view_pos.y, view_pos.z);

    /* Iterate through lights */
    for (size_t i = 0; i < ri->num_lights; ++i) {
        struct renderer_light* light = ri->lights + i;
        switch (light->type) {
            case LT_DIRECTIONAL: {
                /* Full screen quad for directional light */
                glUniform3fv(glGetUniformLocation(shdr, "dir_l.direction"), 1, light->type_data.dir.direction.xyz);
                glUniform3fv(glGetUniformLocation(shdr, "dir_l.color"), 1, light->color.xyz);
                render_quad();
                break;
            }
            case LT_POINT:
                /* Unimplemented */
                break;
        }
    }
    glUseProgram(0);

    /* Restore gl values */
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
}

static void render_scene(struct renderer_state* rs, struct renderer_input* ri, mat4* view, mat4* proj)
{
    /* Clear default buffers */
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    /* Store current fb reference */
    GLint cur_fb;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &cur_fb);
    /* Geometry pass */
    geometry_pass(rs, ri, view->m, proj->m);
    /* Copy depth to fb */
    gbuffer_blit_depth_to_fb(rs->gbuf, cur_fb);
    /* Direct Light pass */
    light_pass(rs, ri, (mat4*)view, (mat4*)proj);
    render_sky(rs, ri, view->m, proj->m);
}

#ifdef WITH_GI
/* Local callback data bundle */
struct sh_gi_render_scene_userdata {
    struct renderer_state* rs;
    struct renderer_input* ri;
};

/* Callback passed to local cubemap renderer, called when rendering each cubemap side */
static void sh_gi_render_lcm_side(mat4* view, mat4* proj, void* userdata)
{
    /* Temporarily disable multisample */
    glDisable(GL_MULTISAMPLE);
    /* Cast userdata */
    struct sh_gi_render_scene_userdata* ud = userdata;
    /* HACK: Temporarily replace gbuffer reference in renderer state */
    struct gbuffer* old_gbuf = ud->rs->gbuf;
    ud->rs->gbuf = ud->rs->sh_gi_rs->lcr_gbuf;
    /* Render scene */
    render_scene(ud->rs, ud->ri, view, proj);
    /* Restore original gbuffer reference */
    ud->rs->gbuf = old_gbuf;
    /* Re-enable multisample */
    glEnable(GL_MULTISAMPLE);
}

/* Callback passed to sh gi renderer, called when making the full screen pass render */
static void sh_gi_prepare_gi_render(mat4* view, mat4* proj, void* userdata)
{
    struct sh_gi_render_scene_userdata* ud = userdata;
    bind_gbuffer_textures(ud->rs, ud->rs->sh_gi_rs->shdr, view, proj);
}
#endif

/*-----------------------------------------------------------------
 * Public interface
 *-----------------------------------------------------------------*/
void renderer_render(struct renderer_state* rs, struct renderer_input* ri, float view[16])
{
    /* Render main scene */
    render_scene(rs, ri, (mat4*)view, &rs->proj);
    /* Render GI */
#ifdef WITH_GI
    struct sh_gi_render_scene_userdata ud;
    ud.rs = rs;
    ud.ri = ri;
    sh_gi_update(rs->sh_gi_rs, sh_gi_render_lcm_side, &ud);
    sh_gi_render(rs->sh_gi_rs, view, rs->proj.m, sh_gi_prepare_gi_render, &ud);
    sh_gi_vis_probes(rs->sh_gi_rs, view, rs->proj.m, 1);
#endif
}

void renderer_resize(struct renderer_state* rs, unsigned int width, unsigned int height)
{
    glViewport(0, 0, width, height);
    rs->viewport.x = width;
    rs->viewport.y = height;
    rs->proj = mat4_perspective(radians(60.0f), 0.1f, 30000.0f, ((float)width / height));
    /* GBuf */
    gbuffer_destroy(rs->gbuf);
    memset(rs->gbuf, 0, sizeof(struct gbuffer));
    gbuffer_init(rs->gbuf, width, height, 1);
}

/*-----------------------------------------------------------------
 * De-initialization
 *-----------------------------------------------------------------*/
void renderer_destroy(struct renderer_state* rs)
{
    sh_gi_destroy(rs->sh_gi_rs);
    free(rs->sh_gi_rs);
    sky_preetham_destroy(rs->sky_rs.preeth);
    free(rs->sky_rs.preeth);
    sky_texture_destroy(rs->sky_rs.tex);
    free(rs->sky_rs.tex);
    glutils_deinit();
    gbuffer_destroy(rs->gbuf);
    free(rs->gbuf);
}
