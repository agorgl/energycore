#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <prof.h>
#include <glad/glad.h>
#include <gfxwnd/window.h>
#include "gpures.h"
#include "ecs/world.h"
#include "scene_file.h"

#define bench(msg) \
    for (timepoint_t _start_ms = millisecs(), _break = 1;         \
         _break || (printf("%s %lu:%03lu\n", msg,                 \
                           (millisecs() - _start_ms) / 1000,      \
                           (millisecs() - _start_ms) % 1000), 0); \
         _break = 0)

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
        ctx->visualize_normals = !ctx->visualize_normals;
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

static int mesh_group_offset_from_name(struct model_hndl* m, const char* mgroup_name)
{
    for (unsigned int i = 0; i <m->num_mesh_groups; ++i) {
        if (strcmp(m->mesh_groups[i]->name, mgroup_name) == 0) {
            return i;
        }
    }
    return -1;
}

static void load_data(struct game_context* ctx, struct scene* sc)
{
    /* Initialize model, texture and material stores */
    hashmap_init(&ctx->model_store, hm_str_hash, hm_str_eql);
    hashmap_init(&ctx->tex_store, hm_str_hash, hm_str_eql);
    hashmap_init(&ctx->mat_store, hm_str_hash, hm_str_eql);

    /* Load models */
    bench("[+] Mdl time")
    for (size_t i = 0; i < sc->num_models; ++i) {
        struct scene_model* m = sc->models + i;
        hm_ptr* p = hashmap_get(&ctx->model_store, hm_cast(m->ref));
        if (!p) {
            /* Load, parse and upload model */
            struct model_hndl* model = model_from_file_to_gpu(m->path);
            hashmap_put(&ctx->model_store, hm_cast(m->ref), hm_cast(model));
        }
    }

    /* Load textures */
    bench("[+] Tex time")
    for (size_t i = 0; i < sc->num_textures; ++i) {
        struct scene_texture* t = sc->textures + i;
        hm_ptr* p = hashmap_get(&ctx->tex_store, hm_cast(t->ref));
        if (!p) {
            /* Load, parse and upload texture */
            struct tex_hndl* tex = tex_from_file_to_gpu(t->path);
            hashmap_put(&ctx->tex_store, hm_cast(t->ref), hm_cast(tex));
        }
    }

    /* Load materials */
    for (size_t i = 0; i < sc->num_materials; ++i) {
        struct scene_material* m = sc->materials + i;
        hm_ptr* p = hashmap_get(&ctx->mat_store, hm_cast(m->ref));
        if (!p) {
            /* Store material */
            struct material* mat = calloc(1, sizeof(struct material));
            /* Set texture parameters */
            for (int j = 0; j < MAT_MAX; ++j) {
                const char* tex_ref = m->textures[j].tex_ref;
                if (tex_ref) {
                    hm_ptr* p = hashmap_get(&ctx->tex_store, hm_cast(tex_ref));
                    if (p)
                        mat->tex[j].hndl = *(struct tex_hndl*)hm_pcast(*p);
                    mat->tex[j].scl[0] = m->textures[j].scale[0];
                    mat->tex[j].scl[1] = m->textures[j].scale[1];
                }
            }
            hashmap_put(&ctx->mat_store, hm_cast(m->ref), hm_cast(mat));
        }
    }

    /* Initialize world */
    ctx->world = world_create();
    /* Add all scene objects */
    struct hashmap transform_handles; /* Used to later populate parent relations */
    hashmap_init(&transform_handles, hm_str_hash, hm_str_eql);
    for (size_t i = 0; i < sc->num_objects; ++i) {
        /* Scene object refering to */
        struct scene_object* so = sc->objects + i;
        /* Create entity */
        entity_t e = entity_create(&ctx->world->emgr);
        /* Create and set render component */
        if (so->mdl_ref) {
            struct model_hndl* mhdl = (struct model_hndl*)hm_pcast(*hashmap_get(&ctx->model_store, hm_cast(so->mdl_ref)));
            int mgroup_idx = -1;
            if (so->mgroup_name)
                mgroup_idx = mesh_group_offset_from_name(mhdl, so->mgroup_name);
            if (mgroup_idx != -1) {
                struct render_component* rendr_c = render_component_create(&ctx->world->render_comp_dbuf, e);
                rendr_c->model = mhdl;
                rendr_c->mesh_group_idx = mgroup_idx;
                for (unsigned int j = 0, cur_mat = 0; j < rendr_c->model->num_meshes; ++j) {
                    struct mesh_hndl* mh = rendr_c->model->meshes + j;
                    if (mh->mgroup_idx == rendr_c->mesh_group_idx) {
                        if (!(cur_mat < so->num_mat_refs && cur_mat < MAX_MATERIALS))
                            break;
                        if (!rendr_c->materials[mh->mat_idx]) {
                            hm_ptr* p = hashmap_get(&ctx->mat_store, hm_cast(so->mat_refs[cur_mat++]));
                            if (p)
                                rendr_c->materials[mh->mat_idx] = (struct material*)hm_pcast(*p);
                        }
                    }
                }
            } else {
                printf("Mesh group offset not found for %s!\n", so->mgroup_name);
            }
        }
        /* Create and set transform component */
        float* pos = so->transform.translation;
        float* rot = so->transform.rotation;
        float* scl = so->transform.scaling;
        struct transform_handle th = transform_component_create(&ctx->world->transform_dbuf, e);
        transform_set_pose_data(&ctx->world->transform_dbuf, th,
            vec3_new(scl[0], scl[1], scl[2]),
            quat_new(rot[0], rot[1], rot[2], rot[3]),
            vec3_new(pos[0], pos[1], pos[2]));
        /* Store transform handle for later parent relations */
        hashmap_put(&transform_handles, hm_cast(so->ref), hm_cast(th.offs));
    }

    /* Populate parent links */
    for (size_t i = 0; i < sc->num_objects; ++i) {
        struct scene_object* so = sc->objects + i;
        if (so->parent_ref) {
            struct transform_handle th_child = *(struct transform_handle*)hashmap_get(&transform_handles, hm_cast(so->ref));
            struct transform_handle th_parnt = *(struct transform_handle*)hashmap_get(&transform_handles, hm_cast(so->parent_ref));
            transform_set_parent(&ctx->world->transform_dbuf, th_child, th_parnt);
        }
    }
    hashmap_destroy(&transform_handles);
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
        .name = "light_pass",
        .vs_loc = "../ext/shaders/passthrough_vs.glsl",
        .gs_loc = 0,
        .fs_loc = "../ext/shaders/dir_light_fs.glsl"
    },
    {
        .name = "env_pass",
        .vs_loc = "../ext/shaders/passthrough_vs.glsl",
        .gs_loc = 0,
        .fs_loc = "../ext/shaders/env_light_fs.glsl"
    },
    {
        .name = "probe_vis",
        .vs_loc = "../ext/shaders/static_vs.glsl",
        .gs_loc = 0,
        .fs_loc = "../ext/shaders/probe_vis_fs.glsl"
    },
    {
        .name = "norm_vis",
        .vs_loc = "ext/shaders/nm_vis_vs.glsl",
        .gs_loc = "ext/shaders/nm_vis_gs.glsl",
        .fs_loc = "ext/shaders/nm_vis_fs.glsl"
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

static void load_shdrs(struct game_context* ctx)
{
    hashmap_init(&ctx->shdr_store, hm_str_hash, hm_str_eql);
    /* Load shader from files into the GPU */
    for (unsigned int i = 0; i < sizeof(shdrs) / sizeof(struct shdr_info); ++i) {
        const struct shdr_info* si = shdrs + i;
        const char* name = si->name;
        unsigned int shdr = shader_from_files(si->vs_loc, si->gs_loc, si->fs_loc);
        hashmap_put(&ctx->shdr_store, hm_cast(name), hm_cast(shdr));
    }
}

static unsigned int rndr_shdr_fetch(const char* shdr_name, void* userdata)
{
    struct game_context* ctx = userdata;
    hm_ptr* p = hashmap_get(&ctx->shdr_store, hm_cast(shdr_name));
    unsigned int shdr = 0;
    if (p)
        shdr = (unsigned int)(*p);
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

    /* Pick scene file, try environment variable first */
    const char* scene_file = getenv("EC_SCENE");
    if (!scene_file) /* Fallback to default */
        scene_file = SCENE_FILE;

    /* Load scene file */
    struct scene* sc = scene_from_file(scene_file);
    /* Load data into GPU and construct world entities */
    bench("[+] Tot time")
        load_data(ctx, sc);
    scene_destroy(sc);

    /* Load shaders */
    load_shdrs(ctx);

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

    /* Visualizations setup */
    ctx->visualize_normals = 0;

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
    /* Count total meshes */
    const size_t num_ents = entity_mgr_size(&ctx->world->emgr);
    ri->num_meshes = 0;
    for (unsigned int i = 0; i < num_ents; ++i) {
        entity_t e = entity_mgr_at(&ctx->world->emgr, i);
        struct render_component* rc = render_component_lookup(&ctx->world->render_comp_dbuf, e);
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
        entity_t e = entity_mgr_at(&ctx->world->emgr, i);
        struct render_component* rc = render_component_lookup(&ctx->world->render_comp_dbuf, e);
        if (!rc)
            continue;
        mat4* transform = transform_world_mat(
            &ctx->world->transform_dbuf,
            transform_component_lookup(&ctx->world->transform_dbuf, e));
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
            struct material* mat = rc->materials[mh->mat_idx];
            if (mat) {
                rm->material.diff_tex = mat->tex[MAT_ALBEDO].hndl.id;
                float* scl = mat->tex[MAT_ALBEDO].scl;
                rm->material.diff_tex_scl[0] = scl[0];
                rm->material.diff_tex_scl[1] = scl[1];
                scl = mat->tex[MAT_NORMAL].scl;
                rm->material.norm_tex = mat->tex[MAT_NORMAL].hndl.id;
                rm->material.norm_tex_scl[0] = scl[0];
                rm->material.norm_tex_scl[1] = scl[1];
                scl = mat->tex[MAT_ROUGHNESS].scl;
                rm->material.rough_tex = mat->tex[MAT_ROUGHNESS].hndl.id;
                rm->material.rough_tex_scl[0] = scl[0];
                rm->material.rough_tex_scl[1] = scl[1];
                scl = mat->tex[MAT_METALLIC].scl;
                rm->material.metal_tex = mat->tex[MAT_METALLIC].hndl.id;
                rm->material.metal_tex_scl[0] = scl[0];
                rm->material.metal_tex_scl[1] = scl[1];
            } else {
                /* Default to white color */
                rm->material.diff_col[0] = 1.0f;
                rm->material.diff_col[1] = 1.0f;
                rm->material.diff_col[2] = 1.0f;
            }
            memcpy(rm->aabb.min, mh->aabb_min, 3 * sizeof(float));
            memcpy(rm->aabb.max, mh->aabb_max, 3 * sizeof(float));
            memcpy(rm->model_mat, transform, 16 * sizeof(float));
            ++cur_mesh;
        }
    }
    prepare_renderer_input_lights(ri);
}

static void visualize_normals(struct game_context* ctx, mat4* view, mat4* proj)
{
    const GLuint shdr = *hashmap_get(&ctx->shdr_store, hm_cast("norm_vis"));
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
    struct game_context* ctx = userdata;

    /* Update renderer input dynamic parameters */
    ctx->cached_ri.sky_tex = ctx->sky_tex->id;
    ctx->cached_ri.sky_type = ctx->dynamic_sky ? RST_PREETHAM : RST_TEXTURE;

    /* Render */
    mat4 iview = camera_interpolated_view(&ctx->cam, interpolation);
    renderer_render(&ctx->rndr_state, &ctx->cached_ri, (float*)&iview);

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
    struct hashmap_iter it;

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

    /* Destroy world */
    world_destroy(ctx->world);

    /* Free shaders */
    hashmap_for(ctx->shdr_store, it) {
        GLuint shdr = it.p->value;
        glDeleteProgram(shdr);
    }

    /* Free model store data */
    hashmap_for(ctx->model_store, it) {
        struct model_hndl* mh = hm_pcast(it.p->value);
        model_free_from_gpu(mh);
    }

    /* Free texture store data */
    hashmap_for(ctx->tex_store, it) {
        struct tex_hndl* th = hm_pcast(it.p->value);
        tex_free_from_gpu(th);
    }

    /* Free material store data */
    hashmap_for(ctx->mat_store, it) {
        free((void*)hm_pcast(it.p->value));
    }

    /* Free stores */
    hashmap_destroy(&ctx->shdr_store);
    hashmap_destroy(&ctx->model_store);
    hashmap_destroy(&ctx->mat_store);
    hashmap_destroy(&ctx->tex_store);

    /* Close window */
    window_destroy(ctx->wnd);
}
