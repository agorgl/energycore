#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glad/glad.h>
#include <gfxwnd/window.h>
#include "gpures.h"
#include "ecs/world.h"
#include "scene_file.h"

#define SCENE_FILE "ext/scenes/sample_scene.json"

static void APIENTRY gl_debug_proc(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* user_param)
{
    (void) source;
    (void) id;
    (void) severity;
    (void) length;
    (void) user_param;

    if (type == GL_DEBUG_TYPE_ERROR) {
        fprintf(stderr, "%s", message);
        exit(1);
    }
}

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
}

static void on_mouse_button(struct window* wnd, int button, int action, int mods)
{
    (void) mods;
    /* struct game_context* ctx = window_get_userdata(wnd); */
    if (action == KEY_ACTION_RELEASE && button == MOUSE_LEFT)
        window_grub_cursor(wnd, 1);
}

static void load_data(struct game_context* ctx, struct scene_object* scene_objects, size_t num_scene_objects)
{
    /* Initialize model store */
    hashmap_init(&ctx->model_store, hm_str_hash, hm_str_eql);
    /* Initialize world */
    ctx->world = world_create();

    /* Add all scene objects */
    struct vector transform_handles; /* Used to later populate parent relations */
    vector_init(&transform_handles, sizeof(struct transform_handle));
    for (size_t i = 0; i < num_scene_objects; ++i) {
        /* Create entity */
        entity_t e = entity_create(&ctx->world->emgr);
        /* Check if model exists on the store */
        const char* mdl_loc = scene_objects[i].model_loc;
        if (mdl_loc) {
            struct model_hndl* model;
            hm_ptr* p = hashmap_get(&ctx->model_store, hm_cast(mdl_loc));
            if (p)
                model = hm_pcast(*p);
            else {
                /* Load, parse and upload model */
                model = model_from_file_to_gpu(mdl_loc);
                hashmap_put(&ctx->model_store, hm_cast(mdl_loc), hm_cast(model));
            }
            /* Create and set render component */
            struct render_component* rendr_c = render_component_create(&ctx->world->render_comp_dbuf, e);
            rendr_c->model = model;
        }
        /* Create and set transform component */
        float* pos = scene_objects[i].translation;
        float* rot = scene_objects[i].rotation;
        float* scl = scene_objects[i].scaling;
        struct transform_handle th = transform_component_create(&ctx->world->transform_dbuf, e);
        transform_set_pose_data(&ctx->world->transform_dbuf, th,
            vec3_new(scl[0], scl[1], scl[2]),
            quat_new(rot[0], rot[1], rot[2], rot[3]),
            vec3_new(pos[0], pos[1], pos[2]));
        /* Store transform handle for later parent relations */
        vector_append(&transform_handles, &th);
    }

    /* Populate parent links */
    for (size_t i = 0; i < num_scene_objects; ++i) {
        long par_ofs = scene_objects[i].parent_ofs;
        if (par_ofs != -1) {
            struct transform_handle th_child = *(struct transform_handle*)vector_at(&transform_handles, i);
            struct transform_handle th_parnt = *(struct transform_handle*)vector_at(&transform_handles, par_ofs);
            transform_set_parent(&ctx->world->transform_dbuf, th_child, th_parnt);
        }
    }
    vector_destroy(&transform_handles);
}

static void model_store_free(hm_ptr k, hm_ptr v)
{
    (void) k;
    struct model_hndl* mh = hm_pcast(v);
    model_free_from_gpu(mh);
}

static void free_data(struct game_context* ctx)
{
    /* Destroy world */
    world_destroy(ctx->world);
    /* Free model store data */
    hashmap_iter(&ctx->model_store, model_store_free);
    /* Free model store */
    hashmap_destroy(&ctx->model_store);
}

void game_init(struct game_context* ctx)
{
    /* Create window */
    const char* title = "EnergyCore";
    int width = 800, height = 600, mode = 0;
    ctx->wnd = window_create(title, width, height, mode);

    /* Assosiate context to be accessed from callback functions */
    window_set_userdata(ctx->wnd, ctx);

    /* Set event callbacks */
    struct window_callbacks wnd_callbacks;
    memset(&wnd_callbacks, 0, sizeof(struct window_callbacks));
    wnd_callbacks.key_cb = on_key;
    wnd_callbacks.mouse_button_cb = on_mouse_button;
    window_set_callbacks(ctx->wnd, &wnd_callbacks);

    /* Setup OpenGL debug handler */
    glDebugMessageCallback(gl_debug_proc, ctx);

    /* Pick scene file, try environment variable first */
    const char* scene_file = getenv("EC_SCENE");
    if (!scene_file) /* Fallback to default */
        scene_file = SCENE_FILE;

    /* Load scene file */
    struct scene_object* scene_objects;
    size_t num_scene_objects;
    load_scene_file(&scene_objects, &num_scene_objects, scene_file);
    /* Load data into GPU and construct world entities */
    load_data(ctx, scene_objects, num_scene_objects);
    /* Free scene file data */
    for (size_t i = 0; i < num_scene_objects; ++i) {
        struct scene_object* so = scene_objects + i;
        free((void*)so->model_loc);
        free((void*)so->name);
    }
    free(scene_objects);

    /* Initialize camera */
    camera_defaults(&ctx->cam);
    ctx->cam.pos = vec3_new(0.0, 1.0, 3.0);
    ctx->cam.front = vec3_normalize(vec3_mul(ctx->cam.pos, -1));
    ctx->fast_move = 0;

    /* Load shader from files into the GPU */
    ctx->rndr_params.shdr_main = shader_from_files("../ext/shaders/main_vs.glsl", 0, "../ext/shaders/main_fs.glsl");
    ctx->rndr_params.shdr_probe_vis = shader_from_files("../ext/shaders/main_vs.glsl", 0, "../ext/shaders/probe_vis_fs.glsl");

    /* Initialize renderer */
    renderer_init(&ctx->rndr_state, &ctx->rndr_params);

    /* Load skybox from file into the GPU */
    ctx->skybox_tex = tex_env_from_file_to_gpu("ext/envmaps/stormydays_large.jpg");
}

void game_update(void* userdata, float dt)
{
    (void) dt;
    struct game_context* ctx = userdata;
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
    /* Update camera matrix */
    camera_update(&ctx->cam);
    /* Process input events */
    window_update(ctx->wnd);
}

static void prepare_renderer_input(struct game_context* ctx, struct renderer_input* ri)
{
    /* Count total meshes */
    const size_t num_ents = entity_mgr_size(&ctx->world->emgr);
    ri->num_meshes = 0;
    for (unsigned int i = 0; i < num_ents; ++i) {
        entity_t e = entity_mgr_at(&ctx->world->emgr, i);
        struct render_component* rc = render_component_lookup(&ctx->world->render_comp_dbuf, e);
        if (rc)
            ri->num_meshes += rc->model->num_meshes;
    }

    /* Populate renderer mesh inputs */
    ri->meshes = malloc(ri->num_meshes * sizeof(struct renderer_mesh));
    memset(ri->meshes, 0, ri->num_meshes * sizeof(struct renderer_mesh));
    unsigned int cur_mesh = 0;
    for (unsigned int i = 0; i < num_ents; ++i) {
        entity_t e = entity_mgr_at(&ctx->world->emgr, i);
        struct render_component* rc = render_component_lookup(&ctx->world->render_comp_dbuf, e);
        if (!rc)
            continue;
        mat4* transform = transform_world_mat(
            &ctx->world->transform_dbuf,
            transform_component_lookup(&ctx->world->transform_dbuf, e));
        struct model_hndl* mdlh = rc->model;
        for (unsigned int j = 0; j < mdlh->num_meshes; ++j) {
            struct mesh_hndl* mh = mdlh->meshes + j;
            struct renderer_mesh* rm = ri->meshes + cur_mesh;
            rm->vao = mh->vao;
            rm->ebo = mh->ebo;
            rm->indice_count = mh->indice_count;
            memcpy(rm->model_mat, transform, 16 * sizeof(float));
            ++cur_mesh;
        }
    }
}

void game_render(void* userdata, float interpolation)
{
    (void) interpolation;
    struct game_context* ctx = userdata;

    /* Fill in struct with renderer input */
    struct renderer_input ri;
    prepare_renderer_input(ctx, &ri);
    ri.skybox = ctx->skybox_tex->id;

    /* Render */
    mat4 iview = camera_interpolated_view(&ctx->cam, interpolation);
    renderer_render(&ctx->rndr_state, &ri, (float*)&iview);
    free(ri.meshes);

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
    /* Unbind GPU handles */
    glUseProgram(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    /* Free skybox */
    tex_free_from_gpu(ctx->skybox_tex);
    /* Free shaders */
    glDeleteProgram(ctx->rndr_params.shdr_probe_vis);
    glDeleteProgram(ctx->rndr_params.shdr_main);
    /* Destroy renderer */
    renderer_destroy(&ctx->rndr_state);
    /* Free any loaded resources */
    free_data(ctx);
    /* Close window */
    window_destroy(ctx->wnd);
}
