#include "game.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <glad/glad.h>
#include <gfxwnd/window.h>
#include "gpures.h"
#include "util.h"
#include "ecs/world.h"
#include "scene_ext.h"

#define SCENE_FILE "ext/scenes/sample_scene.json"

/* Fw declarations */
static void prepare_renderer_input(struct game_context* ctx, struct renderer_input* ri);

static void on_key(struct window* wnd, int key, int scancode, int action, int mods)
{
    (void)scancode; (void)mods;
    struct game_context* ctx = window_get_userdata(wnd);
    if (action == 0 && key == KEY_ESCAPE)
        *(ctx->should_terminate) = 1;
    else if (action == KEY_ACTION_RELEASE && key == KEY_RIGHT_CONTROL)
        window_grub_cursor(wnd, 0);
    else if (key == KEY_LEFT_SHIFT)
        ctx->fast_move = action != KEY_ACTION_RELEASE;
    else if (action == KEY_ACTION_RELEASE && key == KEY_N)
        ctx->rndr_state.options.show_normals = !ctx->rndr_state.options.show_normals;
    else if (action == KEY_ACTION_RELEASE && key == KEY_M)
        ctx->dynamic_sky = !ctx->dynamic_sky;
    else if (action == KEY_ACTION_RELEASE && key == KEY_KP_ADD)
        ctx->cam.move_speed *= (ctx->cam.move_speed < 10e2) ? 2.0f : 1.0f;
    else if (action == KEY_ACTION_RELEASE && key == KEY_KP_SUBTRACT)
        ctx->cam.move_speed /= (ctx->cam.move_speed > 10e-2) ? 2.0f : 1.0f;
    else if (action == KEY_ACTION_RELEASE && key == KEY_B)
        ctx->rndr_state.options.show_bboxes = !ctx->rndr_state.options.show_bboxes;
    else if (action == KEY_ACTION_RELEASE && key == KEY_C)
        ctx->rndr_state.options.use_occlusion_culling = !ctx->rndr_state.options.use_occlusion_culling;
    else if (action == KEY_ACTION_RELEASE && key == KEY_K)
        ctx->rndr_state.options.use_normal_mapping = !ctx->rndr_state.options.use_normal_mapping;
    else if (action == KEY_ACTION_RELEASE && key == KEY_T)
        ctx->rndr_state.options.show_gbuf_textures = !ctx->rndr_state.options.show_gbuf_textures;
    else if (action == KEY_ACTION_RELEASE && key == KEY_R)
        ctx->rndr_state.options.use_rough_met_maps = !ctx->rndr_state.options.use_rough_met_maps;
    else if (action == KEY_ACTION_RELEASE && key == KEY_O)
        ctx->rndr_state.options.use_shadows = !ctx->rndr_state.options.use_shadows;
    else if (action == KEY_ACTION_RELEASE && key == KEY_X)
        ctx->rndr_state.options.use_envlight = !ctx->rndr_state.options.use_envlight;
    else if (action == KEY_ACTION_RELEASE && key == KEY_H)
        ctx->rndr_state.options.use_tonemapping = !ctx->rndr_state.options.use_tonemapping;
    else if (action == KEY_ACTION_RELEASE && key == KEY_G)
        ctx->rndr_state.options.use_gamma_correction = !ctx->rndr_state.options.use_gamma_correction;
    else if (action == KEY_ACTION_RELEASE && key == KEY_V)
        ctx->rndr_state.options.use_antialiasing = !ctx->rndr_state.options.use_antialiasing;
}

static void on_mouse_button(struct window* wnd, int button, int action, int mods)
{
    (void) mods;
    /* struct game_context* ctx = window_get_userdata(wnd); */
    if (action == KEY_ACTION_RELEASE && button == MOUSE_LEFT)
        window_grub_cursor(wnd, 1);
}

static void on_fb_size(struct window* wnd, unsigned int width, unsigned int height)
{
    struct game_context* ctx = window_get_userdata(wnd);
    renderer_resize(&ctx->rndr_state, width, height);
}


static const struct shdr_info {
    const char* name;
    const char* vs_loc;
    const char* gs_loc;
    const char* fs_loc;
} shdrs [] = {
    {
        .name = "geom_pass",
        .vs_loc = "../ext/shaders/static_vs.glsl",
        .gs_loc = 0,
        .fs_loc = "../ext/shaders/geom_pass_fs.glsl"
    },
    {
        .name = "dir_light",
        .vs_loc = "../ext/shaders/passthrough_vs.glsl",
        .gs_loc = 0,
        .fs_loc = "../ext/shaders/dir_light_fs.glsl"
    },
    {
        .name = "env_light",
        .vs_loc = "../ext/shaders/passthrough_vs.glsl",
        .gs_loc = 0,
        .fs_loc = "../ext/shaders/env_light_fs.glsl"
    },
    {
        .name = "env_probe",
        .vs_loc = "../ext/shaders/passthrough_vs.glsl",
        .gs_loc = 0,
        .fs_loc = "../ext/shaders/env_probe_fs.glsl"
    },
    {
        .name = "probe_vis",
        .vs_loc = "../ext/shaders/static_vs.glsl",
        .gs_loc = 0,
        .fs_loc = "../ext/shaders/vis/probe_fs.glsl"
    },
    {
        .name = "norm_vis",
        .vs_loc = "../ext/shaders/vis/normal_vs.glsl",
        .gs_loc = "../ext/shaders/vis/normal_gs.glsl",
        .fs_loc = "../ext/shaders/vis/normal_fs.glsl"
    },
    {
        .name = "tonemap_fx",
        .vs_loc = "../ext/shaders/passthrough_vs.glsl",
        .gs_loc = 0,
        .fs_loc = "../ext/shaders/fx/tonemap.glsl"
    },
    {
        .name = "gamma_fx",
        .vs_loc = "../ext/shaders/passthrough_vs.glsl",
        .gs_loc = 0,
        .fs_loc = "../ext/shaders/fx/gamma.glsl"
    },
    {
        .name = "smaa_fx",
        .vs_loc = "../ext/shaders/fx/smaa_pass_vs.glsl",
        .gs_loc = 0,
        .fs_loc = "../ext/shaders/fx/smaa_pass_fs.glsl"
    }
};

static unsigned int rndr_shdr_fetch(const char* shdr_name, void* userdata)
{
    struct game_context* ctx = userdata;
    unsigned int shdr = res_mngr_shdr_get(ctx->rmgr, shdr_name);
    return shdr;
}

void game_init(struct game_context* ctx)
{
    /* Create window */
    const char* title = "EnergyCore";
    int width = 1280, height = 720, mode = 0;
    ctx->wnd = window_create(title, width, height, mode);

    /* Assosiate context to be accessed from callback functions */
    window_set_userdata(ctx->wnd, ctx);

    /* Set event callbacks */
    struct window_callbacks wnd_callbacks;
    memset(&wnd_callbacks, 0, sizeof(struct window_callbacks));
    wnd_callbacks.key_cb = on_key;
    wnd_callbacks.mouse_button_cb = on_mouse_button;
    wnd_callbacks.fb_size_cb = on_fb_size;
    window_set_callbacks(ctx->wnd, &wnd_callbacks);

    /* Create resource manager */
    ctx->rmgr = res_mngr_create();

    /* Pick scene file, try environment variable first */
    const char* scene_file = getenv("EC_SCENE");
    if (!scene_file) /* Fallback to default */
        scene_file = SCENE_FILE;

    /* Load data into GPU and construct world entities */
    bench("[+] Tot time")
        ctx->main_scene = scene_external(scene_file, ctx->rmgr);
    ctx->active_scene = ctx->main_scene;

    /* Internal shaders */
    for (unsigned int i = 0; i < sizeof(shdrs) / sizeof(struct shdr_info); ++i) {
        const struct shdr_info* si = shdrs + i;
        const char* name = si->name;
        unsigned int shdr = shader_from_files(si->vs_loc, si->gs_loc, si->fs_loc);
        res_mngr_shdr_put(ctx->rmgr, name, shdr);
    }

    /* Initialize camera */
    camera_defaults(&ctx->cam);
    ctx->cam.pos = vec3_new(0.0, 1.0, 3.0);
    camera_setdir(&ctx->cam, vec3_normalize(vec3_mul(ctx->cam.pos, -1)));
    ctx->fast_move = 0;

    /* Initialize renderer */
    renderer_init(&ctx->rndr_state, rndr_shdr_fetch, ctx);

    /* Load sky texture from file into the GPU */
    ctx->sky_tex = tex_env_from_file_to_gpu("ext/envmaps/stormydays_large.jpg");
    ctx->dynamic_sky = 0;

    /* Build initial renderer input */
    prepare_renderer_input(ctx, &ctx->cached_ri);
}

void game_update(void* userdata, float dt)
{
    (void) dt;
    struct game_context* ctx = userdata;
    /* Update camera matrix */
    camera_update(&ctx->cam);
    /* Update camera position */
    int cam_mov_flags = 0x0;
    if (window_key_state(ctx->wnd, KEY_W) == KEY_ACTION_PRESS)
        cam_mov_flags |= cmd_forward;
    if (window_key_state(ctx->wnd, KEY_A) == KEY_ACTION_PRESS)
        cam_mov_flags |= cmd_left;
    if (window_key_state(ctx->wnd, KEY_S) == KEY_ACTION_PRESS)
        cam_mov_flags |= cmd_backward;
    if (window_key_state(ctx->wnd, KEY_D) == KEY_ACTION_PRESS)
        cam_mov_flags |= cmd_right;
    if (ctx->fast_move) {
        float old_move_speed = ctx->cam.move_speed;
        /* Temporarily increase move speed, make the move and restore it */
        ctx->cam.move_speed = old_move_speed * 10.0f;
        camera_move(&ctx->cam, cam_mov_flags);
        ctx->cam.move_speed = old_move_speed;
    } else {
        camera_move(&ctx->cam, cam_mov_flags);
    }
    /* Update camera look */
    float cur_diff_x = 0, cur_diff_y = 0;
    window_get_cursor_diff(ctx->wnd, &cur_diff_x, &cur_diff_y);
    if (window_is_cursor_grubbed(ctx->wnd))
        camera_look(&ctx->cam, cur_diff_x, cur_diff_y);
    /* Process input events */
    window_update(ctx->wnd);
}

static void prepare_renderer_input_lights(struct renderer_input* ri)
{
    /* Sample directional light */
    ri->num_lights = 1;
    ri->lights = calloc(ri->num_lights, sizeof(struct renderer_light));
    ri->lights[0].type = LT_DIRECTIONAL;
    ri->lights[0].color = vec3_new(1, 1, 1);
    ri->lights[0].type_data.dir.direction = vec3_new(0.8, 1.0, 0.8);
}

static void prepare_renderer_input(struct game_context* ctx, struct renderer_input* ri)
{
    struct world* world = ctx->active_scene->world;
    /* Count total meshes */
    const size_t num_ents = entity_mgr_size(&world->emgr);
    ri->num_meshes = 0;
    for (unsigned int i = 0; i < num_ents; ++i) {
        entity_t e = entity_mgr_at(&world->emgr, i);
        struct render_component* rc = render_component_lookup(&world->render_comp_dbuf, e);
        if (rc) {
            for (unsigned int j = 0; j < rc->model->num_meshes; ++j) {
                struct mesh_hndl* mh = rc->model->meshes + j;
                if (mh->mgroup_idx == rc->mesh_group_idx)
                    ++ri->num_meshes;
            }
        }
    }

    /* Populate renderer mesh inputs */
    ri->meshes = malloc(ri->num_meshes * sizeof(struct renderer_mesh));
    memset(ri->meshes, 0, ri->num_meshes * sizeof(struct renderer_mesh));
    unsigned int cur_mesh = 0;
    for (unsigned int i = 0; i < num_ents; ++i) {
        entity_t e = entity_mgr_at(&world->emgr, i);
        struct render_component* rc = render_component_lookup(&world->render_comp_dbuf, e);
        if (!rc)
            continue;
        mat4* transform = transform_world_mat(
            &world->transform_dbuf,
            transform_component_lookup(&world->transform_dbuf, e));
        struct model_hndl* mdlh = rc->model;
        for (unsigned int j = 0; j < mdlh->num_meshes; ++j) {
            /* Source */
            struct mesh_hndl* mh = mdlh->meshes + j;
            /* Skip meshes not in assigned mesh group */
            if (mh->mgroup_idx != rc->mesh_group_idx)
                continue;
            /* Target */
            struct renderer_mesh* rm = ri->meshes + cur_mesh;
            rm->vao = mh->vao;
            rm->ebo = mh->ebo;
            rm->indice_count = mh->indice_count;
            /* Material properties */
            struct material* mat = rc->materials[mh->mat_idx];
            if (mat) {
                for (unsigned int k = 0; k < MAT_MAX; ++k) {
                    struct material_attr* ma = mat->attrs + k;
                    /* Defaults */
                    struct {unsigned int id; float* scl;} tex = {0, (float[]){1.0f, 1.0f}};
                    float valf = 0.0f; float val3f[3] = {0.0f, 0.0f, 0.0f};
                    /* Fill in local */
                    switch (ma->dtype) {
                        case MAT_DTYPE_VALF:
                            valf = ma->d.valf;
                            break;
                        case MAT_DTYPE_VAL3F:
                            val3f[0] = ma->d.val3f[0];
                            val3f[1] = ma->d.val3f[1];
                            val3f[2] = ma->d.val3f[2];
                            break;
                        case MAT_DTYPE_TEX:
                            tex.id  = ma->d.tex.hndl.id;
                            tex.scl = ma->d.tex.scl;
                            break;
                    }
                    /* Copy to target */
                    struct renderer_material_attr* rma = rm->material.attrs + k; /* Implicit mapping MAT_TYPE <-> RMAT_TYPE */
                    rma->dtype = ma->dtype;
                    rma->d.tex.id = tex.id;
                    rma->d.valf = valf;
                    memcpy(rma->d.tex.scl, tex.scl, 2 * sizeof(float));
                    memcpy(rma->d.val3f, val3f, sizeof(val3f));
                }
            } else {
                /* Default to white color */
                struct renderer_material_attr* rma = rm->material.attrs + RMAT_ALBEDO;
                rma->dtype = RMAT_DT_VAL3F;
                rma->d.val3f[0] = 1.0f;
                rma->d.val3f[1] = 1.0f;
                rma->d.val3f[2] = 1.0f;
                rma = rm->material.attrs + RMAT_ROUGHNESS;
                rma->dtype = RMAT_DT_VALF;
                rma->d.valf = 0.8f;
            }
            memcpy(rm->aabb.min, mh->aabb_min, 3 * sizeof(float));
            memcpy(rm->aabb.max, mh->aabb_max, 3 * sizeof(float));
            memcpy(rm->model_mat, transform, 16 * sizeof(float));
            ++cur_mesh;
        }
    }
    prepare_renderer_input_lights(ri);
}

void game_render(void* userdata, float interpolation)
{
    struct game_context* ctx = userdata;

    /* Update renderer input dynamic parameters */
    ctx->cached_ri.sky_tex = ctx->sky_tex->id;
    ctx->cached_ri.sky_type = ctx->dynamic_sky ? RST_PREETHAM : RST_TEXTURE;

    /* Render */
    mat4 iview = camera_interpolated_view(&ctx->cam, interpolation);
    renderer_render(&ctx->rndr_state, &ctx->cached_ri, (float*)&iview);

    /* Show rendered contents from the backbuffer */
    window_swap_buffers(ctx->wnd);
}

void game_perf_update(void* userdata, float msec, float fps)
{
    struct game_context* ctx = userdata;
    char suffix_buf[64];
    snprintf(suffix_buf, sizeof(suffix_buf), "[Msec: %.2f / Fps: %.2f]", msec, fps);
    window_set_title_suffix(ctx->wnd, suffix_buf);
}

void game_shutdown(struct game_context* ctx)
{
    /* Free cached renderer input */
    free(ctx->cached_ri.lights);
    free(ctx->cached_ri.meshes);

    /* Unbind GPU handles */
    glUseProgram(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    /* Free sky texture */
    tex_free_from_gpu(ctx->sky_tex);

    /* Destroy renderer */
    renderer_destroy(&ctx->rndr_state);

    /* Destroy main scene */
    scene_destroy(ctx->main_scene);

    /* Destroy resource manager */
    res_mngr_destroy(ctx->rmgr);

    /* Close window */
    window_destroy(ctx->wnd);
}
