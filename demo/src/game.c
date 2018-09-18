#include "game.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <gfxwnd/window.h>
#include "util.h"
#include "world_ext.h"

#define SCENE_FILE "ext/scenes/sample_scene.json"

/* Fw declarations */
static void prepare_render_scene(struct game_context* ctx, struct render_scene* rscn);

static void on_key(struct window* wnd, int key, int scancode, int action, int mods)
{
    (void)scancode; (void)mods;
    struct game_context* ctx = window_get_userdata(wnd);
    if (action == 0 && key == KEY_ESCAPE)
        *(ctx->should_terminate) = 1;
    else if (action == KEY_ACTION_RELEASE && key == KEY_RIGHT_CONTROL)
        window_grub_cursor(wnd, 0);
    else if (action == KEY_ACTION_RELEASE && key == KEY_N)
        ctx->rndr_state.options.show_normals = !ctx->rndr_state.options.show_normals;
    else if (action == KEY_ACTION_RELEASE && key == KEY_M)
        ctx->cached_scene.sky_type = !ctx->cached_scene.sky_type;
    else if (action == KEY_ACTION_RELEASE && key == KEY_B)
        ctx->rndr_state.options.show_bboxes = !ctx->rndr_state.options.show_bboxes;
    else if (action == KEY_ACTION_RELEASE && key == KEY_C)
        ctx->rndr_state.options.use_occlusion_culling = !ctx->rndr_state.options.use_occlusion_culling;
    else if (action == KEY_ACTION_RELEASE && key == KEY_K)
        ctx->rndr_state.options.use_normal_mapping = !ctx->rndr_state.options.use_normal_mapping;
    else if (action == KEY_ACTION_RELEASE && key == KEY_T)
        ctx->rndr_state.options.show_gbuf_textures = !ctx->rndr_state.options.show_gbuf_textures;
    else if (action == KEY_ACTION_RELEASE && key == KEY_P)
        ctx->rndr_state.options.use_detail_maps = !ctx->rndr_state.options.use_detail_maps;
    else if (action == KEY_ACTION_RELEASE && key == KEY_Y)
        ctx->rndr_state.options.show_gidata = !ctx->rndr_state.options.show_gidata;
    else if (action == KEY_ACTION_RELEASE && key == KEY_O)
        ctx->rndr_state.options.use_shadows = !ctx->rndr_state.options.use_shadows;
    else if (action == KEY_ACTION_RELEASE && key == KEY_X)
        ctx->rndr_state.options.use_envlight = !ctx->rndr_state.options.use_envlight;
    else if (action == KEY_ACTION_RELEASE && key == KEY_Z)
        ctx->rndr_state.options.use_ssao = !ctx->rndr_state.options.use_ssao;
    else if (action == KEY_ACTION_RELEASE && key == KEY_F)
        ctx->rndr_state.options.use_bloom = !ctx->rndr_state.options.use_bloom;
    else if (action == KEY_ACTION_RELEASE && key == KEY_H)
        ctx->rndr_state.options.use_tonemapping = !ctx->rndr_state.options.use_tonemapping;
    else if (action == KEY_ACTION_RELEASE && key == KEY_G)
        ctx->rndr_state.options.use_gamma_correction = !ctx->rndr_state.options.use_gamma_correction;
    else if (action == KEY_ACTION_RELEASE && key == KEY_V)
        ctx->rndr_state.options.use_antialiasing = !ctx->rndr_state.options.use_antialiasing;
    else if (action == KEY_ACTION_RELEASE && key == KEY_Y)
        ctx->rndr_state.options.show_gidata = !ctx->rndr_state.options.show_gidata;
    else if (action == KEY_ACTION_RELEASE && key == KEY_NUM0)
        ctx->gi_dirty = 1;
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

void game_init(struct game_context* ctx)
{
    /* Create window */
    const char* title = "EnergyCore";
    int width = 1280, height = 720, mode = 0;
    ctx->wnd = window_create(title, width, height, mode, (struct context_params){OPENGL, {4, 3}, 1});

    /* Assosiate context to be accessed from callback functions */
    window_set_userdata(ctx->wnd, ctx);

    /* Set event callbacks */
    struct window_callbacks wnd_callbacks;
    memset(&wnd_callbacks, 0, sizeof(struct window_callbacks));
    wnd_callbacks.key_cb = on_key;
    wnd_callbacks.mouse_button_cb = on_mouse_button;
    wnd_callbacks.fb_size_cb = on_fb_size;
    window_set_callbacks(ctx->wnd, &wnd_callbacks);

    /* Initialize renderer */
    renderer_init(&ctx->rndr_state);
    ctx->gi_dirty = 1;

    /* Pick scene file, try environment variable first */
    const char* scene_file = getenv("EC_SCENE");
    if (!scene_file) /* Fallback to default */
        scene_file = SCENE_FILE;

    /* Load data into GPU and construct world entities */
    bench("[+] Tot time")
        ctx->world = world_external(scene_file, &ctx->rndr_state.rmgr);

    /* Find camera entity */
    ctx->camera = INVALID_ENTITY;
    size_t num_entities = entity_total(ctx->world);
    for (size_t i = 0; i < num_entities; ++i) {
        entity_t e = entity_at(ctx->world, i);
        struct camera_component* cc = camera_component_lookup(ctx->world, e);
        if (cc) {
            ctx->camera = e;
            break;
        }
    }
    /* If scene file did not provide, create one */
    if (!entity_valid(ctx->camera)) {
        ctx->camera = entity_create(ctx->world);
        struct camera_component* cc = camera_component_create(ctx->world, ctx->camera);
        vec3 pos = vec3_new(0.0, 1.0, 3.0);
        camctrl_setpos(&cc->camctrl, pos);
        camctrl_setdir(&cc->camctrl, vec3_normalize(vec3_mul(pos, -1)));
    }

    /* Load sky texture from file into the GPU */
    ctx->cached_scene.sky_tex = resmgr_add_texture_file(&ctx->rndr_state.rmgr, "ext/envmaps/sun_clouds.hdr");
    ctx->cached_scene.sky_type = RST_TEXTURE;
    ctx->cached_scene.sky_pp.inclination = 0.8f;
    ctx->cached_scene.sky_pp.azimuth = 0.6f;

    /* Build initial renderer input */
    prepare_render_scene(ctx, &ctx->cached_scene);
}

static vec3 sun_dir_from_params(float inclination, float azimuth)
{
    const float theta = 2.0f * M_PI * (azimuth - 0.5);
    const float phi = M_PI * (inclination - 0.5);
    return vec3_new(
        sin(phi) * sin(theta),
        cos(phi),
        sin(phi) * cos(theta)
    );
}

void game_update(void* userdata, float dt)
{
    struct game_context* ctx = userdata;
    struct camera_component* camc = camera_component_lookup(ctx->world, ctx->camera);

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
    camctrl_move(&camc->camctrl, cam_mov_flags, dt);

    /* Update camera look */
    float cur_diff_x = 0, cur_diff_y = 0;
    window_get_cursor_diff(ctx->wnd, &cur_diff_x, &cur_diff_y);
    if (window_is_cursor_grubbed(ctx->wnd))
        camctrl_look(&camc->camctrl, cur_diff_x, cur_diff_y, dt);
    /* Update camera matrix */
    if (window_key_state(ctx->wnd, KEY_LEFT_SHIFT) == KEY_ACTION_PRESS) {
        float old_max_vel = camc->camctrl.max_vel;
        /* Temporarily increase move speed, make the calculations and restore it */
        camc->camctrl.max_vel = old_max_vel * 10.0f;
        camctrl_update(&camc->camctrl, dt);
        camc->camctrl.max_vel = old_max_vel;
    } else {
        camctrl_update(&camc->camctrl, dt);
    }

    /* Update sun position */
    if (window_key_state(ctx->wnd, KEY_KP2) == KEY_ACTION_PRESS)
        ctx->cached_scene.sky_pp.inclination = clamp(ctx->cached_scene.sky_pp.inclination + 10e-3f, 0.0f, 1.0f);
    if (window_key_state(ctx->wnd, KEY_KP8) == KEY_ACTION_PRESS)
        ctx->cached_scene.sky_pp.inclination = clamp(ctx->cached_scene.sky_pp.inclination - 10e-3f, 0.0f, 1.0f);
    if (window_key_state(ctx->wnd, KEY_KP4) == KEY_ACTION_PRESS)
        ctx->cached_scene.sky_pp.azimuth = clamp(ctx->cached_scene.sky_pp.azimuth + 10e-3f, 0.0f, 1.0f);
    if (window_key_state(ctx->wnd, KEY_KP6) == KEY_ACTION_PRESS)
        ctx->cached_scene.sky_pp.azimuth = clamp(ctx->cached_scene.sky_pp.azimuth - 10e-3f, 0.0f, 1.0f);
    ctx->cached_scene.lights[0].type_data.dir.direction = sun_dir_from_params(ctx->cached_scene.sky_pp.inclination, ctx->cached_scene.sky_pp.azimuth);

    /* Process input events */
    window_update(ctx->wnd);
}

static void prepare_render_scene(struct game_context* ctx, struct render_scene* rscn)
{
    world_t world = ctx->world;

    /* Count total meshes and lights */
    const size_t num_ents = entity_total(world);
    rscn->num_objects = 0; rscn->num_lights = 0;
    for (unsigned int i = 0; i < num_ents; ++i) {
        entity_t e = entity_at(world, i);
        struct render_component* rc = render_component_lookup(world, e);
        struct light_component* lc = light_component_lookup(world, e);
        if (rc)
            ++rscn->num_objects;
        if (lc)
            ++rscn->num_lights;
    }

    /* Populate renderer mesh and light inputs */
    rscn->objects = calloc(rscn->num_objects, sizeof(*rscn->objects));
    rscn->lights  = calloc(rscn->num_lights,  sizeof(*rscn->lights));
    for (unsigned int i = 0, cur_obj = 0, cur_lgt = 0; i < num_ents; ++i) {
        entity_t e = entity_at(world, i);
        struct render_component* rc = render_component_lookup(world, e);
        struct light_component* lc = light_component_lookup(world, e);
        if (rc) {
            mat4 transform = transform_world_mat(world, e);
            /* Target */
            struct render_object* ro = &rscn->objects[cur_obj++];
            memcpy(ro->model_mat, transform.m, 16 * sizeof(float));
            memcpy(ro->materials, rc->materials, sizeof(ro->materials));
            ro->mesh = rc->mesh;
        }
        if (lc) {
            struct render_light* rl = &rscn->lights[cur_lgt++];
            rl->color = lc->color;
            rl->intensity = lc->intensity;
            switch (lc->type) {
                case LC_DIRECTIONAL:
                    rl->type = LT_DIRECTIONAL;
                    rl->type_data.dir.direction = lc->direction;
                    break;
                case LC_POINT:
                    rl->type = LT_POINT;
                    rl->type_data.pt.position = lc->position;
                    rl->type_data.pt.radius = lc->falloff;
                    break;
                case LC_SPOT:
                    rl->type = LT_SPOT;
                    rl->type_data.spt.position = lc->position;
                    rl->type_data.spt.direction = lc->direction;
                    rl->type_data.spt.inner_cone = lc->inner_cone;
                    rl->type_data.spt.outer_cone = lc->outer_cone;
                    break;
            }
        }
    }
}

void game_render(void* userdata, float interpolation)
{
    struct game_context* ctx = userdata;

    /* Update GI */
    if (ctx->gi_dirty) {
        renderer_gi_update(&ctx->rndr_state, &ctx->cached_scene);
        ctx->gi_dirty = 0;
    }

    /* Render */
    struct camera_component* camc = camera_component_lookup(ctx->world, ctx->camera);
    mat4 iview = camctrl_interpolated_view(&camc->camctrl, interpolation);
    renderer_render(&ctx->rndr_state, &ctx->cached_scene, (float*)&iview);

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
    free(ctx->cached_scene.lights);
    free(ctx->cached_scene.objects);

    /* Destroy renderer */
    renderer_destroy(&ctx->rndr_state);

    /* Destroy world */
    world_destroy(ctx->world);

    /* Close window */
    window_destroy(ctx->wnd);
}
