#include <energycore/renderer.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <glad/glad.h>
#include "skytex.h"
#include "skyprth.h"
#include "girndr.h"
#include "probe.h"
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
 * Internal state
 *-----------------------------------------------------------------*/
struct renderer_internal_state {
    /* Internal subrenderers */
    struct sky_renderer_state {
        struct sky_texture tex;
        struct sky_preetham preeth;
    } sky_rndr;
    struct gi_rndr gi_rndr;
    struct bbox_rndr bbox_rs;
    struct shadowmap shdwmap;
    struct postfx postfx;
    struct panicscr_rndr ps_rndr;
    /* Shader handle fetching */
    rndr_shdr_fetch_fn shdr_fetch_cb;
    void* shdr_fetch_userdata;
    /* Shaders */
    struct {
        unsigned int geom_pass;
        unsigned int dir_light;
        unsigned int env_light;
        struct {
            unsigned int tonemap;
            unsigned int gamma;
            unsigned int smaa;
        } fx;
        unsigned int nm_vis;
        unsigned int probe_vis;
        struct {
            unsigned int irr_gen;
            unsigned int brdf_lut;
            unsigned int prefilter;
        } ibl;
    } shdrs;
    /* Internal textures (Luts etc.) */
    struct {
        struct {
            unsigned int area;
            unsigned int search;
        } smaa;
        unsigned int brdf_lut;
    } textures;
    /* GBuffer */
    struct gbuffer main_gbuf;
    struct gbuffer* gbuf; /* Active */
    /* Occlusion culling */
    struct occull_state occl_st;
    /* Cached values */
    vec2 viewport;
    mat4 proj;
    /* Frame profiler and debug info's */
    struct frame_prof* fprof;
    struct {
        unsigned int num_visible_objs;
        float gpass_msec;
        float lpass_msec;
        float ppass_msec;
    } dbginfo;
};

/*-----------------------------------------------------------------
 * Initialization
 *-----------------------------------------------------------------*/
void renderer_init(struct renderer_state* rs, rndr_shdr_fetch_fn sfn, void* sh_ud)
{
    /* Populate renderer state according to init params */
    memset(rs, 0, sizeof(*rs));
    rs->internal = calloc(1, sizeof(*rs->internal));
    struct renderer_internal_state* is = rs->internal;
    is->shdr_fetch_cb = sfn;
    is->shdr_fetch_userdata = sh_ud;

    /* Initial dimensions */
    int width = 1280, height = 720;
    is->viewport.x = width; is->viewport.y = height;
    /* Initialize gbuffer */
    gbuffer_init(&is->main_gbuf, width, height); is->gbuf = &is->main_gbuf;
    /* Initialize internal postfx renderer */
    postfx_init(&is->postfx, width, height);
    /* Initial resize */
    renderer_resize(rs, width, height);
    /* Initialize gl utilities state */
    glutils_init();
    /* Initialize internal texture sky state */
    sky_texture_init(&is->sky_rndr.tex);
    /* Initialize internal preetham sky state */
    sky_preetham_init(&is->sky_rndr.preeth);
    /* Initialize internal Global Illumination renderer state */
    gi_rndr_init(&is->gi_rndr);
    /* Initialize internal bbox renderer state */
    bbox_rndr_init(&is->bbox_rs);
    /* Initialize internal occlusion state */
    occull_init(&is->occl_st);
    /* Initialize internal shadowmap state */
    const GLuint shmap_res = 2048;
    shadowmap_init(&is->shdwmap, shmap_res, shmap_res);
    /* Initialize internal panic screen state */
    panicscr_init(&is->ps_rndr);
    panicscr_register_gldbgcb(&is->ps_rndr);
    /* Fetch shaders */
    renderer_shdr_fetch(rs);
    /* Initialize frame profiler */
    is->fprof = frame_prof_init();
    /* Initialize debug text rendering */
    dbgtxt_init();
    /* Initialize multiple render targets debugger */
    mrtdbg_init();
    /* Load internal textures */
    glGenTextures(1, &is->textures.smaa.area);
    glBindTexture(GL_TEXTURE_2D, is->textures.smaa.area);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, AREATEX_WIDTH, AREATEX_HEIGHT, 0, GL_RG, GL_UNSIGNED_BYTE, areaTexBytes);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenTextures(1, &is->textures.smaa.search);
    glBindTexture(GL_TEXTURE_2D, is->textures.smaa.search);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, searchTexBytes);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    /* Setup IBL */
    is->textures.brdf_lut = brdf_lut_generate(is->shdrs.ibl.brdf_lut);

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
    rs->options.show_gidata = 0;
}

void renderer_shdr_fetch(struct renderer_state* rs)
{
    struct renderer_internal_state* is = rs->internal;
    is->shdrs.geom_pass         = is->shdr_fetch_cb("geom_pass", is->shdr_fetch_userdata);
    is->shdrs.dir_light         = is->shdr_fetch_cb("dir_light", is->shdr_fetch_userdata);
    is->shdrs.env_light         = is->shdr_fetch_cb("env_light", is->shdr_fetch_userdata);
    is->shdrs.fx.tonemap        = is->shdr_fetch_cb("tonemap_fx", is->shdr_fetch_userdata);
    is->shdrs.fx.gamma          = is->shdr_fetch_cb("gamma_fx", is->shdr_fetch_userdata);
    is->shdrs.fx.smaa           = is->shdr_fetch_cb("smaa_fx", is->shdr_fetch_userdata);
    is->shdrs.nm_vis            = is->shdr_fetch_cb("norm_vis", is->shdr_fetch_userdata);
    is->shdrs.probe_vis         = is->shdr_fetch_cb("probe_vis", is->shdr_fetch_userdata);
    is->shdrs.ibl.irr_gen       = is->shdr_fetch_cb("irr_conv", is->shdr_fetch_userdata);
    is->shdrs.ibl.brdf_lut      = is->shdr_fetch_cb("brdf_lut", is->shdr_fetch_userdata);
    is->shdrs.ibl.prefilter     = is->shdr_fetch_cb("prefilter", is->shdr_fetch_userdata);
    is->sky_rndr.preeth.shdr    = is->shdr_fetch_cb("sky_prth", is->shdr_fetch_userdata);
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
    struct renderer_internal_state* is = rs->internal;
    gbuffer_bind_for_geometry_pass(is->gbuf);

    /* Enable depth and culling */
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);

    /* Clear */
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* Pick shader for the mode */
    GLuint shdr = is->shdrs.geom_pass;
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
    is->dbginfo.num_visible_objs = 0;
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
            occull_object_begin(&is->occl_st, i);
            visible = occull_should_render(&is->occl_st, rm->aabb.min, rm->aabb.max);
            if (!visible) {
                occull_object_end(&is->occl_st);
                continue;
            }
        }
        if (visible)
            is->dbginfo.num_visible_objs++;

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
            occull_object_end(&is->occl_st);
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
            sky_texture_render(&rs->internal->sky_rndr.tex, (mat4*)view, (mat4*)proj, ri->sky_tex);
            break;
        }
        case RST_PREETHAM: {
            struct sky_preetham_params sp_params;
            sky_preetham_default_params(&sp_params);
            sp_params.inclination = ri->sky_pp.inclination;
            sp_params.azimuth     = ri->sky_pp.azimuth;
            sky_preetham_render(&rs->internal->sky_rndr.preeth, &sp_params, (mat4*)proj, (mat4*)view);
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
    struct renderer_internal_state* is = rs->internal;
    gbuffer_bind_for_light_pass(is->gbuf);

    /* Clear */
    glClear(GL_COLOR_BUFFER_BIT);

    /* Disable writting to depth buffer for screen space renders */
    glDepthMask(GL_FALSE);

    /* Enable blend for additive lighting */
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE);

    /* Setup common uniforms */
    GLuint shdr = is->shdrs.dir_light;
    glUseProgram(shdr);
    upload_gbuffer_uniforms(shdr, is->viewport.xy, view, proj);

    mat4 inverse_view = mat4_inverse(*(mat4*)view);
    vec3 view_pos = vec3_new(inverse_view.xw, inverse_view.yw, inverse_view.zw);
    glUniform3f(glGetUniformLocation(shdr, "view_pos"), view_pos.x, view_pos.y, view_pos.z);
    glUniformMatrix4fv(glGetUniformLocation(shdr, "view"), 1, GL_FALSE, view->m);

    /* Setup shadowmap inputs */
    glActiveTexture(GL_TEXTURE7);
    glUniform1i(glGetUniformLocation(shdr, "shadowmap"), 7);
    glUniform1i(glGetUniformLocation(shdr, "shadows_enabled"), rs->options.use_shadows);
    if (rs->options.use_shadows) {
        glBindTexture(GL_TEXTURE_2D_ARRAY, is->shdwmap.glh.tex_id);
        shadowmap_bind(&is->shdwmap, shdr);
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
        /* Pick probe */
        struct probe* pb = is->gi_rndr.fallback_probe.p;
        /* Setup environment light shader */
        GLuint shdr = is->shdrs.env_light;
        glUseProgram(shdr);
        upload_gbuffer_uniforms(shdr, is->viewport.xy, view, proj);
        glUniform3f(glGetUniformLocation(shdr, "view_pos"), view_pos.x, view_pos.y, view_pos.z);
        /* Diffuse irradiance map */
        glUniform1i(glGetUniformLocation(shdr, "irr_map"), 5);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_CUBE_MAP, pb->irr_diffuse_cm);
        /* Specular irradiance map */
        glUniform1i(glGetUniformLocation(shdr, "pf_map"), 6);
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_CUBE_MAP, pb->prefiltered_cm);
        /* Brdf Lut */
        glUniform1i(glGetUniformLocation(shdr, "brdf_lut"), 7);
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, is->textures.brdf_lut);
        /* Screen pass */
        render_quad();
    }
    glUseProgram(0);

    /* Restore gl values */
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    gbuffer_unbind_textures(is->gbuf);
}

static void postprocess_pass(struct renderer_state* rs, unsigned int cur_fb)
{
    struct renderer_internal_state* is = rs->internal;
    GLuint shdr = 0;
    postfx_blit_fb_to_read(&is->postfx, is->gbuf->fbo);
    if (rs->options.use_antialiasing) {
        shdr = is->shdrs.fx.smaa;
        glDisable(GL_BLEND);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glUseProgram(shdr);
        glUniform1i(glGetUniformLocation(shdr, "input_tex"),  0);
        glUniform1i(glGetUniformLocation(shdr, "input_tex2"), 1);
        glUniform1i(glGetUniformLocation(shdr, "input_tex3"), 2);

        /* Edge detection step */
        glUniform1i(glGetUniformLocation(shdr, "ustep"), 0);
        postfx_pass(&is->postfx);

        /* Blend weight calculation step */
        glUniform1i(glGetUniformLocation(shdr, "ustep"), 1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, is->textures.smaa.area);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, is->textures.smaa.search);
        postfx_pass(&is->postfx);

        /* Neighborhood blending step */
        glUniform1i(glGetUniformLocation(shdr, "ustep"), 2);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, postfx_orig_tex(&is->postfx));
        postfx_pass(&is->postfx);
    }
    if (rs->options.use_tonemapping) {
        shdr = is->shdrs.fx.tonemap;
        glUseProgram(shdr);
        postfx_pass(&is->postfx);
    }
    if (rs->options.use_gamma_correction) {
        shdr = is->shdrs.fx.gamma;
        glUseProgram(shdr);
        postfx_pass(&is->postfx);
    }
    glUseProgram(0);
    postfx_blit_read_to_fb(&is->postfx, cur_fb);
}

static void render_scene(struct renderer_state* rs, struct renderer_input* ri, mat4* view, mat4* proj, int direct_only)
{
    struct renderer_internal_state* is = rs->internal;
    /* Clear default buffers */
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    /* Store current fb reference */
    GLint cur_fb;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &cur_fb);
    /* Geometry pass */
    frame_prof_timepoint(is->fprof)
        geometry_pass(rs, ri, view->m, proj->m);
    /* Copy depth to fb */
    gbuffer_blit_depth_to_fb(is->gbuf, cur_fb);
    /* Shadowmap pass*/
    if (rs->options.use_shadows && !direct_only) {
        float* light_dir = ri->lights[0].type_data.dir.direction.xyz;
        shadowmap_render((&is->shdwmap), light_dir, view->m, proj->m) {
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
    frame_prof_timepoint(is->fprof)
        light_pass(rs, ri, (mat4*)view, (mat4*)proj, direct_only);
    /* Sky */
    render_sky(rs, ri, view->m, proj->m);
    /* PostFX pass */
    frame_prof_timepoint(is->fprof) {
        if (!direct_only)
            postprocess_pass(rs, cur_fb);
        else
            gbuffer_blit_accum_to_fb(is->gbuf, cur_fb);
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
        bbox_rndr_vis(&rs->internal->bbox_rs, rm->model_mat, view, rs->internal->proj.m, rm->aabb.min, rm->aabb.max);
    }
}

static void visualize_normals(struct renderer_state* rs, struct renderer_input* ri, mat4* view, mat4* proj)
{
    const GLuint shdr = rs->internal->shdrs.nm_vis;
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

void renderer_gi_update(struct renderer_state* rs, struct renderer_input* ri)
{
    /* Update fallback probe */
    struct gi_rndr* gir = &rs->internal->gi_rndr;
    mat4 pass_view, pass_proj;
    probe_render_faces(gir->probe_rndr, gir->fallback_probe.p, vec3_zero(), pass_view, pass_proj)
        render_sky(rs, ri, pass_view.m, pass_proj.m);

    /* HACK: Temporarily replace gbuffer reference in renderer state */
    struct gbuffer* old_gbuf = rs->internal->gbuf;
    rs->internal->gbuf = gir->probe_gbuf;
    /* Update probes */
    gi_render_passes(gir, pass_view, pass_proj)
        render_scene(rs, ri, &pass_view, &pass_proj, 1);
    /* Restore original gbuffer reference */
    rs->internal->gbuf = old_gbuf;

    /* Preprocess probes */
    gi_preprocess(gir, rs->internal->shdrs.ibl.irr_gen, rs->internal->shdrs.ibl.prefilter);
}

/*-----------------------------------------------------------------
 * Public interface
 *-----------------------------------------------------------------*/
void renderer_render(struct renderer_state* rs, struct renderer_input* ri, float view[16])
{
    struct renderer_internal_state* is = rs->internal;
    /* Show panic screen if any critical error occured */
    if (is->ps_rndr.should_show) {
        panicscr_show(&is->ps_rndr);
        return;
    }

    /* Render main scene */
    with_fprof(is->fprof, 1)
        render_scene(rs, ri, (mat4*)view, &is->proj, 0);

    /* Visualize normals */
    if (rs->options.show_normals)
        visualize_normals(rs, ri, (mat4*)view, &is->proj);

    /* Visualize bboxes */
    if (rs->options.show_bboxes)
        visualize_bboxes(rs, ri, view);

    /* Visualize GI probes */
    if (rs->options.show_gidata)
        gi_vis_probes(&is->gi_rndr, is->shdrs.probe_vis, view, is->proj.m, 1);

    /* Show gbuffer textures */
    if (rs->options.show_gbuf_textures) {
        const struct mrtdbg_tex_info tinfos[] = {
            {
                .handle = is->gbuf->albedo_buf,
                .type   = 0, .layer  = 0,
                .mode   = MRTDBG_MODE_RGB,
            },{
                .handle = is->gbuf->normal_buf,
                .type   = 0, .layer  = 0,
                .mode   = MRTDBG_MODE_RGB,
            },{
                .handle = is->gbuf->roughness_metallic_buf,
                .type   = 0, .layer  = 0,
                .mode   = MRTDBG_MODE_MONO_R,
            },{
                .handle = is->gbuf->roughness_metallic_buf,
                .type   = 0, .layer  = 0,
                .mode   = MRTDBG_MODE_MONO_A,
            }
        };
        mrtdbg_show_textures((struct mrtdbg_tex_info*)tinfos,
                             sizeof(tinfos) / sizeof(tinfos[0]));
    }

    /* Update debug info */
    frame_prof_flush(is->fprof);
    is->dbginfo.gpass_msec = frame_prof_timepoint_msec(is->fprof, 0);
    is->dbginfo.lpass_msec = frame_prof_timepoint_msec(is->fprof, 1);
    is->dbginfo.ppass_msec = frame_prof_timepoint_msec(is->fprof, 2);

    /* Show debug info */
    if (rs->options.show_fprof) {
        char buf[128];
        snprintf(buf, sizeof(buf), "GPass: %.3f\nLPass: %.3f\nPPass: %.3f\nVis/Tot: %u/%u",
                 is->dbginfo.gpass_msec, is->dbginfo.lpass_msec, is->dbginfo.ppass_msec,
                 is->dbginfo.num_visible_objs, ri->num_meshes);
        dbgtxt_setfnt(FNT_GOHU);
        dbgtxt_prnt(buf, 5, 15);
        dbgtxt_setfnt(FNT_SLKSCR);
        dbgtxt_prntc("EnergyCore", is->viewport.x - 130, is->viewport.y - 25, 0.08f, 0.08f, 0.08f, 1.0f);
    }
}

void renderer_resize(struct renderer_state* rs, unsigned int width, unsigned int height)
{
    struct renderer_internal_state* is = rs->internal;
    glViewport(0, 0, width, height);
    is->viewport.x = width;
    is->viewport.y = height;
    is->proj = mat4_perspective(radians(60.0f), 0.1f, 30000.0f, ((float)width / height));
    /* Recreate all fb textures */
    gbuffer_destroy(is->gbuf);
    gbuffer_init(is->gbuf, width, height);
    postfx_destroy(&is->postfx);
    postfx_init(&is->postfx, width, height);
}

/*-----------------------------------------------------------------
 * De-initialization
 *-----------------------------------------------------------------*/
void renderer_destroy(struct renderer_state* rs)
{
    struct renderer_internal_state* is = rs->internal;
    glDeleteTextures(1, &is->textures.brdf_lut);
    glDeleteTextures(1, &is->textures.smaa.area);
    glDeleteTextures(1, &is->textures.smaa.search);
    panicscr_destroy(&is->ps_rndr);
    mrtdbg_destroy();
    dbgtxt_destroy();
    frame_prof_destroy(is->fprof);
    shadowmap_destroy(&is->shdwmap);
    occull_destroy(&is->occl_st);
    bbox_rndr_destroy(&is->bbox_rs);
    gi_rndr_destroy(&is->gi_rndr);
    sky_preetham_destroy(&is->sky_rndr.preeth);
    sky_texture_destroy(&is->sky_rndr.tex);
    glutils_deinit();
    postfx_destroy(&is->postfx);
    gbuffer_destroy(&is->main_gbuf);
    free(rs->internal);
}
