#include "scene_ext.h"
#include <string.h>
#include <stdio.h>
#include <prof.h>
#include <hashmap.h>
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
    return -2;
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
            for (int j = 0; j < STT_MAX; ++j) {
                struct material_attr* mattr = mat->attrs + j; /* Implicit mapping STT_TYPE <-> MAT_TYPE */
                const char* tex_ref = m->textures[j].tex_ref;
                struct tex_hndl* p = tex_ref ? res_mngr_tex_get(rmgr, tex_ref) : 0;
                if (p) {
                    mattr->dtype = MAT_DTYPE_TEX;
                    mattr->d.tex.hndl.id = p->id;
                    mattr->d.tex.scl[0]  = m->textures[j].scale[0];
                    mattr->d.tex.scl[1]  = m->textures[j].scale[1];
                } else {
                    switch (j) {
                        case MAT_ALBEDO:
                            mattr->dtype = MAT_DTYPE_VAL3F;
                            mattr->d.val3f[0] = 0.0f;
                            mattr->d.val3f[1] = 0.0f;
                            mattr->d.val3f[2] = 0.0f;
                            break;
                        case MAT_ROUGHNESS:
                            mattr->dtype = MAT_DTYPE_VALF;
                            mattr->d.valf = 0.8f;
                            break;
                        case MAT_METALLIC:
                            mattr->dtype = MAT_DTYPE_VALF;
                            mattr->d.valf = 0.0f;
                            break;
                        case MAT_GLOSSINESS:
                            mattr->dtype = MAT_DTYPE_VALF;
                            mattr->d.valf = 0.2f;
                            break;
                        case MAT_SPECULAR:
                            mattr->dtype = MAT_DTYPE_VAL3F;
                            mattr->d.val3f[0] = 0.0f;
                            mattr->d.val3f[1] = 0.0f;
                            mattr->d.val3f[2] = 0.0f;
                            break;
                        case MAT_EMISSION:
                            mattr->dtype = MAT_DTYPE_VAL3F;
                            mattr->d.val3f[0] = 0.0f;
                            mattr->d.val3f[1] = 0.0f;
                            mattr->d.val3f[2] = 0.0f;
                            break;
                        case MAT_OCCLUSION:
                            mattr->dtype = MAT_DTYPE_VALF;
                            mattr->d.valf = 0.0f;
                            break;
                        default:
                            mattr->dtype = MAT_DTYPE_VALF;
                            mattr->d.valf = 0.0f;
                            break;
                    }
                }
            }
            res_mngr_mat_put(rmgr, m->ref, mat);
        }
    }

    /* Initialize scene */
    struct scene* s = scene_create();
    struct world* world = s->world;
    /* Used to later populate parent relations */
    component_t* transform_handles = calloc(sc->num_objects, sizeof(component_t));
    struct hashmap transform_handles_map;
    hashmap_init(&transform_handles_map, hm_str_hash, hm_str_eql);
    /* Add all scene objects */
    for (size_t i = 0; i < sc->num_objects; ++i) {
        /* Scene object refering to */
        struct scene_object* so = sc->objects + i;
        /* Create entity */
        entity_t e = entity_create(world->ecs);
        /* Create and set render component */
        if (so->mdl_ref) {
            struct model_hndl* mhdl = res_mngr_mdl_get(rmgr, so->mdl_ref);
            int mgroup_idx = ~0;
            if (so->mgroup_name)
                mgroup_idx = mesh_group_offset_from_name(mhdl, so->mgroup_name);
            if (mgroup_idx != -2) {
                component_t rc = render_component_create(world->ecs, e);
                struct render_component* rendr_c = render_component_data(world->ecs, rc);
                rendr_c->model = mhdl;
                rendr_c->mesh_group_idx = mgroup_idx;
                for (unsigned int j = 0, cur_mat = 0; j < rendr_c->model->num_meshes; ++j) {
                    struct mesh_hndl* mh = rendr_c->model->meshes + j;
                    if (mh->mgroup_idx == rendr_c->mesh_group_idx || (int)mgroup_idx == ~0) {
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
        component_t tc = transform_component_create(world->ecs, e);
        transform_component_set_pose(world->ecs, tc, (struct transform_pose) {
            vec3_new(scl[0], scl[1], scl[2]),
            quat_new(rot[0], rot[1], rot[2], rot[3]),
            vec3_new(pos[0], pos[1], pos[2])
        });
        /* Store transform handle for later parent relations */
        transform_handles[i] = tc;
        hashmap_put(&transform_handles_map, hm_cast(so->ref), i);
    }

    /* Populate parent links */
    for (size_t i = 0; i < sc->num_objects; ++i) {
        struct scene_object* so = sc->objects + i;
        if (so->parent_ref) {
            uint32_t cidx = *(uint32_t*)hashmap_get(&transform_handles_map, hm_cast(so->ref));
            uint32_t pidx = *(uint32_t*)hashmap_get(&transform_handles_map, hm_cast(so->parent_ref));
            component_t tc_child = transform_handles[cidx];
            component_t tc_parnt = transform_handles[pidx];
            transform_component_set_parent(world->ecs, tc_child, tc_parnt);
        }
    }
    hashmap_destroy(&transform_handles_map);
    free(transform_handles);
    scene_file_destroy(sc);

    return s;
}
