#include "scene_ext.h"
#include <string.h>
#include <stdio.h>
#include <prof.h>
#include "gpures.h"
#include "ecs/world.h"
#include "scene_file.h"
#include "util.h"

static inline int mesh_group_offset_from_name(struct model_hndl* m, const char* mgroup_name)
{
    for (unsigned int i = 0; i <m->num_mesh_groups; ++i) {
        if (strcmp(m->mesh_groups[i]->name, mgroup_name) == 0) {
            return i;
        }
    }
    return -1;
}

struct scene* scene_external(const char* scene_file, struct res_mngr* rmgr)
{
    /* Load scene file */
    struct scene_file* sc = scene_file_load(scene_file);

    /* Load models */
    bench("[+] Mdl time")
    for (size_t i = 0; i < sc->num_models; ++i) {
        struct scene_model* m = sc->models + i;
        struct model_hndl* p = res_mngr_mdl_get(rmgr, m->ref);
        if (!p) {
            /* Load, parse and upload model */
            struct model_hndl* model = model_from_file_to_gpu(m->path);
            res_mngr_mdl_put(rmgr, m->ref, model);
        }
    }

    /* Load textures */
    bench("[+] Tex time")
    for (size_t i = 0; i < sc->num_textures; ++i) {
        struct scene_texture* t = sc->textures + i;
        struct tex_hndl* p = res_mngr_tex_get(rmgr, t->ref);
        if (!p) {
            /* Load, parse and upload texture */
            struct tex_hndl* tex = tex_from_file_to_gpu(t->path);
            res_mngr_tex_put(rmgr, t->ref, tex);
        }
    }

    /* Load materials */
    for (size_t i = 0; i < sc->num_materials; ++i) {
        struct scene_material* m = sc->materials + i;
        struct material* p = res_mngr_mat_get(rmgr, m->ref);
        if (!p) {
            /* Store material */
            struct material* mat = calloc(1, sizeof(struct material));
            /* Set texture parameters */
            for (int j = 0; j < MAT_MAX; ++j) {
                const char* tex_ref = m->textures[j].tex_ref;
                if (tex_ref) {
                    struct tex_hndl* p = res_mngr_tex_get(rmgr, tex_ref);
                    if (p)
                        mat->tex[j].hndl = *p;
                    mat->tex[j].scl[0] = m->textures[j].scale[0];
                    mat->tex[j].scl[1] = m->textures[j].scale[1];
                }
            }
            res_mngr_mat_put(rmgr, m->ref, mat);
        }
    }

    /* Initialize scene */
    struct scene* s = scene_create();
    struct world* world = s->world;
    /* Add all scene objects */
    struct hashmap transform_handles; /* Used to later populate parent relations */
    hashmap_init(&transform_handles, hm_str_hash, hm_str_eql);
    for (size_t i = 0; i < sc->num_objects; ++i) {
        /* Scene object refering to */
        struct scene_object* so = sc->objects + i;
        /* Create entity */
        entity_t e = entity_create(&world->emgr);
        /* Create and set render component */
        if (so->mdl_ref) {
            struct model_hndl* mhdl = res_mngr_mdl_get(rmgr, so->mdl_ref);
            int mgroup_idx = -1;
            if (so->mgroup_name)
                mgroup_idx = mesh_group_offset_from_name(mhdl, so->mgroup_name);
            if (mgroup_idx != -1) {
                struct render_component* rendr_c = render_component_create(&world->render_comp_dbuf, e);
                rendr_c->model = mhdl;
                rendr_c->mesh_group_idx = mgroup_idx;
                for (unsigned int j = 0, cur_mat = 0; j < rendr_c->model->num_meshes; ++j) {
                    struct mesh_hndl* mh = rendr_c->model->meshes + j;
                    if (mh->mgroup_idx == rendr_c->mesh_group_idx) {
                        if (!(cur_mat < so->num_mat_refs && cur_mat < MAX_MATERIALS))
                            break;
                        if (!rendr_c->materials[mh->mat_idx]) {
                            struct material* p = res_mngr_mat_get(rmgr, so->mat_refs[cur_mat++]);
                            if (p)
                                rendr_c->materials[mh->mat_idx] = p;
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
        struct transform_handle th = transform_component_create(&world->transform_dbuf, e);
        transform_set_pose_data(&world->transform_dbuf, th,
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
            transform_set_parent(&world->transform_dbuf, th_child, th_parnt);
        }
    }
    hashmap_destroy(&transform_handles);
    scene_file_destroy(sc);

    return s;
}
