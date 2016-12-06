#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glad/glad.h>
#include <gfxwnd/window.h>
#include "gpures.h"

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
}

static void on_mouse_button(struct window* wnd, int button, int action, int mods)
{
    (void) mods;
    /* struct game_context* ctx = window_get_userdata(wnd); */
    if (action == KEY_ACTION_RELEASE && button == MOUSE_LEFT)
        window_grub_cursor(wnd, 1);
}

static struct {
    const char* model_loc;
    float translation[3];
    float rotation[3];
    float scaling;
} scene_objects[] = {
    {
        /* Gazebo */
        .model_loc     = "ext/models/gazebo/gazebo.obj",
        .translation   = {0.0f, -0.6f, 0.0f},
        .rotation      = {0.0f, 0.0f, 0.0f},
        .scaling       = 2.0f
    }
};

static void load_data(struct game_context* ctx)
{
    /* Add all scene objects */
    size_t num_scene_objects = sizeof(scene_objects) / sizeof(scene_objects[0]);
    vector_init(&ctx->gobjects, sizeof(struct game_object));
    for (size_t i = 0; i < num_scene_objects; ++i) {
        struct game_object go;
        /* Load, parse and upload model */
        go.model = model_from_file_to_gpu(scene_objects[i].model_loc);
        /* Construct model matrix */
        float* pos = scene_objects[i].translation;
        float* rot = scene_objects[i].rotation;
        float scl = scene_objects[i].scaling;
        go.transform = mat4_mul_mat4(
            mat4_mul_mat4(
                mat4_translation(vec3_new(pos[0], pos[1], pos[2])),
                mat4_rotation_euler(radians(rot[0]), radians(rot[1]), radians(rot[2]))
            ),
            mat4_scale(vec3_new(scl, scl, scl))
        );
        vector_append(&ctx->gobjects, &go);
    }
}

static void free_data(struct game_context* ctx)
{
    /* Loop through objects */
    for (unsigned int i = 0; i < ctx->gobjects.size; ++i) {
        struct game_object* gobj = vector_at(&ctx->gobjects, i);
        model_free_from_gpu(gobj->model);
        gobj->model = 0;
    }
    vector_destroy(&ctx->gobjects);
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

    /* Load data from files into the GPU */
    load_data(ctx);

    /* Initialize camera */
    camera_defaults(&ctx->cam);
    ctx->cam.pos = vec3_new(0.0, 1.0, 3.0);
    ctx->cam.front = vec3_normalize(vec3_mul(ctx->cam.pos, -1));

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
    camera_move(&ctx->cam, cam_mov_flags);
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
    ri->num_meshes = 0;
    for (unsigned int i = 0; i < ctx->gobjects.size; ++i) {
        struct game_object* gobj = vector_at(&ctx->gobjects, i);
        struct model_hndl* mdlh = gobj->model;
        ri->num_meshes += mdlh->num_meshes;
    }

    /* Populate renderer mesh inputs */
    ri->meshes = malloc(ri->num_meshes * sizeof(struct renderer_mesh));
    memset(ri->meshes, 0, ri->num_meshes * sizeof(struct renderer_mesh));
    unsigned int cur_mesh = 0;
    for (unsigned int i = 0; i < ctx->gobjects.size; ++i) {
        struct game_object* gobj = vector_at(&ctx->gobjects, i);
        struct model_hndl* mdlh = gobj->model;
        for (unsigned int j = 0; j < mdlh->num_meshes; ++j) {
            struct mesh_hndl* mh = mdlh->meshes + j;
            struct renderer_mesh* rm = ri->meshes + cur_mesh;
            rm->vao = mh->vao;
            rm->ebo = mh->ebo;
            rm->indice_count = mh->indice_count;
            memcpy(rm->model_mat, &gobj->transform, 16 * sizeof(float));
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
