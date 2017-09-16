#include <energycore/renderer.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <glad/glad.h>
#include "skytex.h"
#include "skyprth.h"
#include "probe_vis.h"
#include "sh_gi.h"
#include "bbrndr.h"
#include "gbuffer.h"
#include "occull.h"
#include "shdwmap.h"
#include "glutils.h"
#include "frprof.h"
#include "dbgtxt.h"
#include "mrtdbg.h"
#include "postfx.h"
#include "panicscr.h"
#include "smaa_area_tex.h"
#include "smaa_search_tex.h"

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
    gbuffer_init(rs->gbuf, width, height);

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

    /* Initialize internal bbox renderer state */
    rs->bbox_rs = malloc(sizeof(struct bbox_rndr));
    bbox_rndr_init(rs->bbox_rs);

    /* Initialize internal occlusion state */
    rs->occl_st = malloc(sizeof(struct occull_state));
    occull_init(rs->occl_st);

    /* Initialize internal shadowmap state */
    rs->shdwmap = malloc(sizeof(struct shadowmap));
    const GLuint shmap_res = 2048;
    shadowmap_init(rs->shdwmap, shmap_res, shmap_res);

    /* Initialize internal postfx renderer */
    rs->postfx = malloc(sizeof(struct postfx));
    postfx_init(rs->postfx, width, height);

    /* Initialize internal panic screen state */
    rs->ps_rndr = malloc(sizeof(struct panicscr_rndr));
    panicscr_init(rs->ps_rndr);
    panicscr_register_gldbgcb(rs->ps_rndr);

    /* Fetch shaders */
    renderer_shdr_fetch(rs);

    /* Initialize frame profiler */
    rs->fprof = frame_prof_init();

    /* Initialize debug text rendering */
    dbgtxt_init();

    /* Initialize multiple render targets debugger */
    mrtdbg_init();

    /* Load internal textures */
    glGenTextures(1, &rs->textures.smaa.area);
    glBindTexture(GL_TEXTURE_2D, rs->textures.smaa.area);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, AREATEX_WIDTH, AREATEX_HEIGHT, 0, GL_RG, GL_UNSIGNED_BYTE, areaTexBytes);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenTextures(1, &rs->textures.smaa.search);
    glBindTexture(GL_TEXTURE_2D, rs->textures.smaa.search);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, searchTexBytes);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    /* Default options */
    rs->options.use_occlusion_culling = 0;
    rs->options.use_rough_met_maps = 1;
    rs->options.use_shadows = 0;
    rs->options.use_envlight = 1;
    rs->options.use_tonemapping = 0;
    rs->options.use_gamma_correction = 0;
    rs->options.show_bboxes = 0;
    rs->options.show_normals = 0;
    rs->options.show_fprof = 1;
    rs->options.show_gbuf_textures = 0;
}

void renderer_shdr_fetch(struct renderer_state* rs)
{
    rs->shdrs.geom_pass  = rs->shdr_fetch_cb("geom_pass", rs->shdr_fetch_userdata);
    rs->shdrs.dir_light  = rs->shdr_fetch_cb("dir_light", rs->shdr_fetch_userdata);
    rs->shdrs.env_light  = rs->shdr_fetch_cb("env_light", rs->shdr_fetch_userdata);
    rs->shdrs.fx.tonemap = rs->shdr_fetch_cb("tonemap_fx", rs->shdr_fetch_userdata);
    rs->shdrs.fx.gamma   = rs->shdr_fetch_cb("gamma_fx", rs->shdr_fetch_userdata);
    rs->shdrs.fx.smaa    = rs->shdr_fetch_cb("smaa_fx", rs->shdr_fetch_userdata);
    rs->shdrs.nm_vis     = rs->shdr_fetch_cb("norm_vis", rs->shdr_fetch_userdata);
    rs->sh_gi_rs->shdr   = rs->shdr_fetch_cb("env_probe", rs->shdr_fetch_userdata);
    rs->sh_gi_rs->probe_vis->shdr = rs->shdr_fetch_cb("probe_vis", rs->shdr_fetch_userdata);
    rs->sky_rs.preeth->shdr = rs->shdr_fetch_cb("sky_prth", rs->shdr_fetch_userdata);
}

static void upload_gbuffer_uniforms(GLuint shdr, float viewport[2], mat4* view, mat4* proj)
{
    glUniform1i(glGetUniformLocation(shdr, "gbuf.depth"), 0);
    glUniform1i(glGetUniformLocation(shdr, "gbuf.normal"), 1);
    glUniform1i(glGetUniformLocation(shdr, "gbuf.albedo"), 2);
    glUniform1i(glGetUniformLocation(shdr, "gbuf.roughness_metallic"), 3);
    glUniform2fv(glGetUniformLocation(shdr, "u_screen"), 1, viewport);
    mat4 inv_view_proj = mat4_inverse(mat4_mul_mat4(*proj, *view));
    glUniformMatrix4fv(glGetUniformLocation(shdr, "u_inv_view_proj"), 1, GL_FALSE, inv_view_proj.m);
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
    glUniform1i(glGetUniformLocation(shdr, "mat.normal_tex"), 1);
    glUniform1i(glGetUniformLocation(shdr, "mat.rough_tex"), 2);
    glUniform1i(glGetUniformLocation(shdr, "mat.metal_tex"), 3);

    /* Loop through meshes */
    rs->dbginfo.num_visible_objs = 0;
    for (unsigned int i = 0; i < ri->num_meshes; ++i) {
        /* Setup mesh to be rendered */
        struct renderer_mesh* rm = ri->meshes + i;

        /* Upload model matrix */
        glUniformMatrix4fv(modl_mat_loc, 1, GL_FALSE, rm->model_mat);

        /* Set front face */
        float model_det = mat4_det(*(mat4*)rm->model_mat);
        glFrontFace(model_det < 0 ? GL_CW : GL_CCW);

        /* Determine object visibility using occlusion culling */
        unsigned int visible = 1;
        if (rs->options.use_occlusion_culling) {
            /* Begin occlusion query, use index i as handle */
            occull_object_begin(rs->occl_st, i);
            visible = occull_should_render(rs->occl_st, rm->aabb.min, rm->aabb.max);
            if (!visible) {
                occull_object_end(rs->occl_st);
                continue;
            }
        }
        if (visible)
            rs->dbginfo.num_visible_objs++;

        /* Albedo */
        struct renderer_material_attr* rma = 0;
        rma = rm->material.attrs + RMAT_ALBEDO;
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, rma->d.tex.id);
        glUniform3fv(glGetUniformLocation(shdr, "mat.albedo_col"), 1, rma->d.val3f);
        glUniform2fv(glGetUniformLocation(shdr, "mat.albedo_scl"), 1, rma->d.tex.scl);

        /* Normal map */
        int use_nm = 0;
        if (rs->options.use_normal_mapping) {
            rma = rm->material.attrs + RMAT_NORMAL;
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, rma->d.tex.id);
            glUniform2fv(glGetUniformLocation(shdr, "mat.normal_scl"), 1, rma->d.tex.scl);
            use_nm = rma->dtype == RMAT_DT_TEX;
        }
        glUniform1i(glGetUniformLocation(shdr, "use_nm"), use_nm);

        /* Roughness / Metallic */
        unsigned int roughness_tex = 0, metallic_tex = 0;
        float* roughness_scl = (float[]){1.0f, 1.0f}, *metallic_scl = (float[]){1.0f, 1.0f};
        float roughness_val = 0.0f, metallic_val = 0.0f;
        if (rs->options.use_rough_met_maps) {
            rma = rm->material.attrs + RMAT_ROUGHNESS;
            if (rma->dtype == RMAT_DT_TEX) {
                roughness_tex = rma->d.tex.id;
                roughness_scl = rma->d.tex.scl;
            } else if (rma->dtype == RMAT_DT_VALF) {
                roughness_val = rma->d.valf;
            }
            rma = rm->material.attrs + RMAT_METALLIC;
            if (rma->dtype == RMAT_DT_TEX) {
                metallic_tex = rma->d.tex.id;
                metallic_scl = rma->d.tex.scl;
            } else if (rma->dtype == RMAT_DT_VALF) {
                metallic_val = rma->d.valf;
            }
        }
        glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, roughness_tex);
        glUniform2fv(glGetUniformLocation(shdr, "mat.rough_scl"), 1, roughness_scl);
        glUniform1fv(glGetUniformLocation(shdr, "mat.roughness"), 1, &roughness_val);
        glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, metallic_tex);
        glUniform2fv(glGetUniformLocation(shdr, "mat.metal_scl"), 1, metallic_scl);
        glUniform1fv(glGetUniformLocation(shdr, "mat.metallic"),  1, &metallic_val);

        /* Render mesh */
        glBindVertexArray(rm->vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rm->ebo);
        glDrawElements(GL_TRIANGLES, rm->indice_count, GL_UNSIGNED_INT, (void*)0);

        /* Reset bindings */
        for (int i = 0; i < 4; ++i) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        /* End occlusion query for current object */
        if (rs->options.use_occlusion_culling)
            occull_object_end(rs->occl_st);
    }
    glUseProgram(0);
    glFrontFace(GL_CCW);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
            sp_params.inclination = ri->sky_pp.inclination;
            sp_params.azimuth     = ri->sky_pp.azimuth;
            sky_preetham_render(rs->sky_rs.preeth, &sp_params, (mat4*)proj, (mat4*)view);
            break;
        }
        case RST_NONE:
        default:
            break;
    }
}

static void light_pass(struct renderer_state* rs, struct renderer_input* ri, mat4* view, mat4* proj, int direct_only)
{
    /* Bind gbuffer input textures and target fbo */
    gbuffer_bind_for_light_pass(rs->gbuf);

    /* Clear */
    glClear(GL_COLOR_BUFFER_BIT);

    /* Disable writting to depth buffer for screen space renders */
    glDepthMask(GL_FALSE);

    /* Enable blend for additive lighting */
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE);

    /* Setup common uniforms */
    GLuint shdr = rs->shdrs.dir_light;
    glUseProgram(shdr);
    upload_gbuffer_uniforms(shdr, rs->viewport.xy, view, proj);

    mat4 inverse_view = mat4_inverse(*(mat4*)view);
    vec3 view_pos = vec3_new(inverse_view.xw, inverse_view.yw, inverse_view.zw);
    glUniform3f(glGetUniformLocation(shdr, "view_pos"), view_pos.x, view_pos.y, view_pos.z);
    glUniformMatrix4fv(glGetUniformLocation(shdr, "view"), 1, GL_FALSE, view->m);

    /* Setup shadowmap inputs */
    glActiveTexture(GL_TEXTURE7);
    glUniform1i(glGetUniformLocation(shdr, "shadowmap"), 7);
    glUniform1i(glGetUniformLocation(shdr, "shadows_enabled"), rs->options.use_shadows);
    if (rs->options.use_shadows) {
        glBindTexture(GL_TEXTURE_2D_ARRAY, rs->shdwmap->glh.tex_id);
        shadowmap_bind(rs->shdwmap, shdr);
    } else
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

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

    /* Environmental lighting */
    if (rs->options.use_envlight && !direct_only) {
#ifdef WITH_GI
        /* Use probes to render GI */
        GLuint shdr = rs->sh_gi_rs->shdr;
        glUseProgram(shdr);
        upload_gbuffer_uniforms(rs->sh_gi_rs->shdr, rs->viewport.xy, (mat4*)view, &rs->proj);
        glUniform3f(glGetUniformLocation(rs->sh_gi_rs->shdr, "view_pos"), view_pos.x, view_pos.y, view_pos.z);
        sh_gi_render(rs->sh_gi_rs);
#else
        GLuint shdr = rs->shdrs.env_light;
        glUseProgram(shdr);
        upload_gbuffer_uniforms(shdr, rs->viewport.xy, view, proj);
        glUniform3f(glGetUniformLocation(shdr, "view_pos"), view_pos.x, view_pos.y, view_pos.z);
        render_quad();
#endif
    }
    glUseProgram(0);

    /* Restore gl values */
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    gbuffer_unbind_textures(rs->gbuf);
}

static void postprocess_pass(struct renderer_state* rs, unsigned int cur_fb)
{
    GLuint shdr = 0;
    postfx_blit_fb_to_read(rs->postfx, rs->gbuf->fbo);
    if (rs->options.use_antialiasing) {
        shdr = rs->shdrs.fx.smaa;
        glDisable(GL_BLEND);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glUseProgram(shdr);
        glUniform1i(glGetUniformLocation(shdr, "input_tex"),  0);
        glUniform1i(glGetUniformLocation(shdr, "input_tex2"), 1);
        glUniform1i(glGetUniformLocation(shdr, "input_tex3"), 2);

        /* Edge detection step */
        glUniform1i(glGetUniformLocation(shdr, "ustep"), 0);
        postfx_pass(rs->postfx);

        /* Blend weight calculation step */
        glUniform1i(glGetUniformLocation(shdr, "ustep"), 1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, rs->textures.smaa.area);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, rs->textures.smaa.search);
        postfx_pass(rs->postfx);

        /* Neighborhood blending step */
        glUniform1i(glGetUniformLocation(shdr, "ustep"), 2);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, postfx_orig_tex(rs->postfx));
        postfx_pass(rs->postfx);
    }
    if (rs->options.use_tonemapping) {
        shdr = rs->shdrs.fx.tonemap;
        glUseProgram(shdr);
        postfx_pass(rs->postfx);
    }
    if (rs->options.use_gamma_correction) {
        shdr = rs->shdrs.fx.gamma;
        glUseProgram(shdr);
        postfx_pass(rs->postfx);
    }
    glUseProgram(0);
    postfx_blit_read_to_fb(rs->postfx, cur_fb);
}

static void render_scene(struct renderer_state* rs, struct renderer_input* ri, mat4* view, mat4* proj, int direct_only)
{
    /* Clear default buffers */
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    /* Store current fb reference */
    GLint cur_fb;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &cur_fb);
    /* Geometry pass */
    frame_prof_timepoint(rs->fprof)
        geometry_pass(rs, ri, view->m, proj->m);
    /* Copy depth to fb */
    gbuffer_blit_depth_to_fb(rs->gbuf, cur_fb);
    /* Shadowmap pass*/
    if (rs->options.use_shadows && !direct_only) {
        float* light_dir = ri->lights[0].type_data.dir.direction.xyz;
        shadowmap_render(rs->shdwmap, light_dir, view->m, proj->m) {
            for (size_t i = 0; i < ri->num_meshes; ++i) {
                struct renderer_mesh* rm = ri->meshes + i;
                glUniformMatrix4fv(glGetUniformLocation(shdr, "model"), 1, GL_FALSE, rm->model_mat);
                glBindVertexArray(rm->vao);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rm->ebo);
                glDrawElements(GL_TRIANGLES, rm->indice_count, GL_UNSIGNED_INT, 0);
            }
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
        }
    }
    /* Light pass */
    frame_prof_timepoint(rs->fprof)
        light_pass(rs, ri, (mat4*)view, (mat4*)proj, direct_only);
    /* Sky */
    render_sky(rs, ri, view->m, proj->m);
    /* PostFX pass */
    frame_prof_timepoint(rs->fprof) {
        if (!direct_only)
            postprocess_pass(rs, cur_fb);
        else
            gbuffer_blit_accum_to_fb(rs->gbuf, cur_fb);
    }
}

/*-----------------------------------------------------------------
 * Visualizations
 *-----------------------------------------------------------------*/
static void visualize_bboxes(struct renderer_state* rs, struct renderer_input* ri, float view[16])
{
    /* Loop through meshes */
    for (unsigned int i = 0; i < ri->num_meshes; ++i) {
        /* Setup mesh to be rendered */
        struct renderer_mesh* rm = ri->meshes + i;
        /* Upload model matrix */
        bbox_rndr_vis(rs->bbox_rs, rm->model_mat, view, rs->proj.m, rm->aabb.min, rm->aabb.max);
    }
}

static void visualize_normals(struct renderer_state* rs, struct renderer_input* ri, mat4* view, mat4* proj)
{
    const GLuint shdr = rs->shdrs.nm_vis;
    glUseProgram(shdr);
    glUniformMatrix4fv(glGetUniformLocation(shdr, "proj"), 1, GL_FALSE, proj->m);
    glUniformMatrix4fv(glGetUniformLocation(shdr, "view"), 1, GL_FALSE, view->m);
    GLuint mdl_mat_loc = glGetUniformLocation(shdr, "model");
    for (unsigned int i = 0; i < ri->num_meshes; ++i) {
        /* Setup mesh to be rendered */
        struct renderer_mesh* rm = ri->meshes + i;
        glUniformMatrix4fv(mdl_mat_loc, 1, GL_FALSE, rm->model_mat);
        glBindVertexArray(rm->vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rm->ebo);
        glDrawElements(GL_TRIANGLES, rm->indice_count, GL_UNSIGNED_INT, (void*)0);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

/*-----------------------------------------------------------------
 * Public interface
 *-----------------------------------------------------------------*/
void renderer_render(struct renderer_state* rs, struct renderer_input* ri, float view[16])
{
    /* Show panic screen if any critical error occured */
    if (rs->ps_rndr->should_show) {
        panicscr_show(rs->ps_rndr);
        return;
    }

#ifdef WITH_GI
    /* Update probes */
    if (rs->options.use_envlight) {
        mat4 pass_view, pass_proj;
        sh_gi_render_passes(rs->sh_gi_rs, pass_view, pass_proj) {
            /* HACK: Temporarily replace gbuffer reference in renderer state */
            struct gbuffer* old_gbuf = rs->gbuf;
            rs->gbuf = rs->sh_gi_rs->lcr_gbuf;
            /* Render scene */
            render_scene(rs, ri, &pass_view, &pass_proj, 1);
            /* Restore original gbuffer reference */
            rs->gbuf = old_gbuf;
        }
    }
#endif

    /* Render main scene */
    with_fprof(rs->fprof, 1)
        render_scene(rs, ri, (mat4*)view, &rs->proj, 0);

    /* Visualize normals */
    if (rs->options.show_normals)
        visualize_normals(rs, ri, (mat4*)view, &rs->proj);

    /* Visualize bboxes */
    if (rs->options.show_bboxes)
        visualize_bboxes(rs, ri, view);

#ifdef WITH_GI
    /* Visualize probes */
    if (rs->options.use_envlight)
        sh_gi_vis_probes(rs->sh_gi_rs, view, rs->proj.m, 0);
#endif

    /* Show gbuffer textures */
    if (rs->options.show_gbuf_textures) {
        const struct mrtdbg_tex_info tinfos[] = {
            {
                .handle = rs->gbuf->albedo_buf,
                .type   = 0, .layer  = 0,
                .mode   = MRTDBG_MODE_RGB,
            },{
                .handle = rs->gbuf->normal_buf,
                .type   = 0, .layer  = 0,
                .mode   = MRTDBG_MODE_RGB,
            },{
                .handle = rs->gbuf->roughness_metallic_buf,
                .type   = 0, .layer  = 0,
                .mode   = MRTDBG_MODE_MONO_R,
            },{
                .handle = rs->gbuf->roughness_metallic_buf,
                .type   = 0, .layer  = 0,
                .mode   = MRTDBG_MODE_MONO_A,
            }
        };
        mrtdbg_show_textures((struct mrtdbg_tex_info*)tinfos,
                             sizeof(tinfos) / sizeof(tinfos[0]));
    }

    /* Update debug info */
    frame_prof_flush(rs->fprof);
    rs->dbginfo.gpass_msec = frame_prof_timepoint_msec(rs->fprof, 0);
    rs->dbginfo.lpass_msec = frame_prof_timepoint_msec(rs->fprof, 1);
    rs->dbginfo.ppass_msec = frame_prof_timepoint_msec(rs->fprof, 2);

    /* Show debug info */
    if (rs->options.show_fprof) {
        char buf[128];
        snprintf(buf, sizeof(buf), "GPass: %.3f\nLPass: %.3f\nPPass: %.3f\nVis/Tot: %u/%u",
                 rs->dbginfo.gpass_msec, rs->dbginfo.lpass_msec, rs->dbginfo.ppass_msec,
                 rs->dbginfo.num_visible_objs, ri->num_meshes);
        dbgtxt_setfnt(FNT_GOHU);
        dbgtxt_prnt(buf, 5, 15);
        dbgtxt_setfnt(FNT_SLKSCR);
        dbgtxt_prntc("EnergyCore", rs->viewport.x - 130, rs->viewport.y - 25, 0.08f, 0.08f, 0.08f, 1.0f);
    }
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
    gbuffer_init(rs->gbuf, width, height);
}

/*-----------------------------------------------------------------
 * De-initialization
 *-----------------------------------------------------------------*/
void renderer_destroy(struct renderer_state* rs)
{
    glDeleteTextures(1, &rs->textures.smaa.area);
    glDeleteTextures(1, &rs->textures.smaa.search);
    panicscr_destroy(rs->ps_rndr);
    free(rs->ps_rndr);
    postfx_destroy(rs->postfx);
    free(rs->postfx);
    mrtdbg_destroy();
    dbgtxt_destroy();
    frame_prof_destroy(rs->fprof);
    shadowmap_destroy(rs->shdwmap);
    free(rs->shdwmap);
    occull_destroy(rs->occl_st);
    free(rs->occl_st);
    bbox_rndr_destroy(rs->bbox_rs);
    free(rs->bbox_rs);
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
