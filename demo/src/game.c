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
    else if (action == KEY_ACTION_RELEASE && key == KEY_N)
        ctx->visualize_normals = !ctx->visualize_normals;
    else if (action == KEY_ACTION_RELEASE && key == KEY_M)
        ctx->dynamic_sky = !ctx->dynamic_sky;
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

static void load_data(struct game_context* ctx, struct scene_object* scene_objects, size_t num_scene_objects)
{
    /* Initialize model, texture and material stores */
    hashmap_init(&ctx->model_store, hm_str_hash, hm_str_eql);
    hashmap_init(&ctx->tex_store, hm_str_hash, hm_str_eql);
    hashmap_init(&ctx->mat_store, hm_str_hash, hm_str_eql);
    /* Initialize world */
    ctx->world = world_create();
    /* Add all scene objects */
    struct vector transform_handles; /* Used to later populate parent relations */
    vector_init(&transform_handles, sizeof(struct transform_handle));
    for (size_t i = 0; i < num_scene_objects; ++i) {
        /* Create entity */
        entity_t e = entity_create(&ctx->world->emgr);
        struct render_component* rendr_c = 0;
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
            rendr_c = render_component_create(&ctx->world->render_comp_dbuf, e);
            rendr_c->model = model;
        }
        /* Check if materials exist on the store */
        if (rendr_c && scene_objects[i].materials) {
            for (size_t j = 0; j < scene_objects[i].num_materials; ++j) {
                struct scene_material* mtrl = scene_objects[i].materials + j;
                struct material* material;
                hm_ptr* p = hashmap_get(&ctx->mat_store, hm_cast(mtrl->name));
                if (p)
                    material = hm_pcast(*p);
                else {
                    /* Create material */
                    material = calloc(1, sizeof(struct material));
                    /* Check each texture if on texture map */
                    const char* tex_loc = mtrl->diff_tex;
                    if (tex_loc) {
                        struct tex_hndl* th;
                        hm_ptr* p = hashmap_get(&ctx->tex_store, hm_cast(tex_loc));
                        if (p)
                            th = hm_pcast(*p);
                        else {
                            /* Load parse and upload texture */
                            th = tex_from_file_to_gpu(tex_loc);
                            hashmap_put(&ctx->tex_store, hm_cast(tex_loc), hm_cast(th));
                        }
                        material->diff_tex = *th;
                    }
                    hashmap_put(&ctx->mat_store, hm_cast(mtrl->name), hm_cast(material));
                }
                rendr_c->materials[j] = material;
            }
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

static void tex_store_free(hm_ptr k, hm_ptr v)
{
    (void) k;
    struct tex_hndl* th = hm_pcast(v);
    tex_free_from_gpu(th);
}

static void mat_store_free(hm_ptr k, hm_ptr v)
{
    (void) k;
    free((void*)v);
}

static void free_data(struct game_context* ctx)
{
    /* Destroy world */
    world_destroy(ctx->world);
    /* Free model store data */
    hashmap_iter(&ctx->model_store, model_store_free);
    /* Free texture store data */
    hashmap_iter(&ctx->tex_store, tex_store_free);
    /* Free material store data */
    hashmap_iter(&ctx->mat_store, mat_store_free);
    /* Free model, texture and material store */
    hashmap_destroy(&ctx->mat_store);
    hashmap_destroy(&ctx->tex_store);
    hashmap_destroy(&ctx->model_store);
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
        if (so->materials) {
            for (size_t i = 0; i < so->num_materials; ++i) {
                if (so->materials[i].diff_tex)
                    free((void*)so->materials[i].diff_tex);
                free((void*)so->materials->name);
            }
            free(so->materials);
        }
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
    ctx->dynamic_sky = 0;

    /* Visualizations setup */
    ctx->visualize_normals = 0;
    ctx->vis_nm_prog = shader_from_files("ext/shaders/nm_vis_vs.glsl", "ext/shaders/nm_vis_gs.glsl", "ext/shaders/nm_vis_fs.glsl");
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
            if (rc->materials[mh->mat_idx])
                rm->material.diff_tex = rc->materials[mh->mat_idx]->diff_tex.id;
            else {
                /* Default to white color */
                rm->material.diff_col[0] = 1.0f;
                rm->material.diff_col[1] = 1.0f;
                rm->material.diff_col[2] = 1.0f;
            }
            memcpy(rm->model_mat, transform, 16 * sizeof(float));
            ++cur_mesh;
        }
    }
}

static void visualize_normals(struct game_context* ctx, mat4* view, mat4* proj)
{
    const GLuint shdr = ctx->vis_nm_prog;
    glUseProgram(shdr);
    GLuint proj_mat_loc = glGetUniformLocation(shdr, "proj");
    GLuint view_mat_loc = glGetUniformLocation(shdr, "view");
    GLuint modl_mat_loc = glGetUniformLocation(shdr, "model");
    glUniformMatrix4fv(proj_mat_loc, 1, GL_FALSE, proj->m);
    glUniformMatrix4fv(view_mat_loc, 1, GL_FALSE, view->m);

    const size_t num_ents = entity_mgr_size(&ctx->world->emgr);
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
            glUniformMatrix4fv(modl_mat_loc, 1, GL_FALSE, transform->m);
            glBindVertexArray(mh->vao);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mh->ebo);
            glDrawElements(GL_TRIANGLES, mh->indice_count, GL_UNSIGNED_INT, (void*)0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
        }
    }
    glUseProgram(0);
}

void game_render(void* userdata, float interpolation)
{
    (void) interpolation;
    struct game_context* ctx = userdata;

    /* Fill in struct with renderer input */
    struct renderer_input ri;
    prepare_renderer_input(ctx, &ri);
    ri.skybox = ctx->skybox_tex->id;
    ri.sky_type = ctx->dynamic_sky ? RST_PREETHAM : RST_TEXTURE;

    /* Render */
    mat4 iview = camera_interpolated_view(&ctx->cam, interpolation);
    renderer_render(&ctx->rndr_state, &ri, (float*)&iview);
    free(ri.meshes);

    if (ctx->visualize_normals)
        visualize_normals(ctx, &iview, &ctx->rndr_state.proj);

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
    glDeleteProgram(ctx->vis_nm_prog);
    glDeleteProgram(ctx->rndr_params.shdr_probe_vis);
    glDeleteProgram(ctx->rndr_params.shdr_main);
    /* Destroy renderer */
    renderer_destroy(&ctx->rndr_state);
    /* Free any loaded resources */
    free_data(ctx);
    /* Close window */
    window_destroy(ctx->wnd);
}
