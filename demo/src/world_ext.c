#include "world_ext.h"
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <prof.h>
#include <hashmap.h>
#include <energycore/asset.h>
#include <stb_image.h>
#include "scene_file.h"
#include "util.h"

#define timed_load(fname) \
    for (timepoint_t _start_ms = millisecs(), _break = (printf("Loading: %-120s", fname), 1); \
         _break; _break = 0, printf("[ %3lu ms ]\n", (unsigned long)(millisecs() - _start_ms)))

struct submesh_info {
    const char* model;
    const char* mgroup_name;
};

static size_t submesh_info_hash(hm_ptr key)
{
    struct submesh_info* si = hm_pcast(key);
    size_t h1 = hm_str_hash(hm_cast(si->model));
    size_t h2 = hm_str_hash(hm_cast(si->mgroup_name));
    return h1 ^ h2;
}

static int submesh_info_eql(hm_ptr k1, hm_ptr k2)
{
    struct submesh_info* s1 = hm_pcast(k1);
    struct submesh_info* s2 = hm_pcast(k2);
    return strcmp(s1->model, s2->model) == 0
        && strcmp(s1->mgroup_name, s2->mgroup_name) == 0;
}

static image image_from_file_helper(const char* path)
{
    image im = image_from_file(path);
    if (!im.data) {
        /* Try with external loader */
        int width = 0, height = 0, channels = 0;
        void* data = 0;
        stbi_set_flip_vertically_on_load(1);
        data = stbi_load(path, &width, &height, &channels, 0);
        im  = (image) {
            .w                = width,
            .h                = height,
            .channels         = channels,
            .bit_depth        = 8,
            .data             = data,
            .sz               = 0,
            .compression_type = 0
        };
    }
    return im;
}

world_t world_external(const char* scene_file, struct resmgr* rmgr)
{
    /* Load scene file */
    struct scene_file* sc = scene_file_load(scene_file);

    /* Load models */
    struct hashmap model_handles_map;
    hashmap_init(&model_handles_map, submesh_info_hash, submesh_info_eql);
    bench("[+] Mdl time")
    for (size_t i = 0; i < sc->num_models; ++i) {
        struct scene_model* m = sc->models + i;
        /* Load, parse and upload model */
        struct scene* sc = 0;
        timed_load(m->path)
            sc = scene_from_file(m->path);
        for (size_t j = 0; j < sc->num_nodes; ++j) {
            /* Check if mesh node */
            struct node* node = sc->nodes[j];
            if (!node->ist)
                continue;
            /* Create mesh resource */
            struct mesh* mesh = node->ist->msh;
            rid id = resmgr_add_mesh(rmgr, mesh);
            struct render_mesh* rmsh = resmgr_get_mesh(rmgr, id);
            /* Setup material indexes */
            uint32_t cnt = 0;
            struct hashmap mat_idx_map;
            hashmap_init(&mat_idx_map, hm_u64_hash, hm_u64_eql);
            for (size_t k = 0; k < rmsh->num_shapes; ++k) {
                struct material* mptr = mesh->shapes[k]->mat;
                hm_ptr* p = hashmap_get(&mat_idx_map, hm_cast(mptr));
                if (p) {
                    rmsh->shapes[k].mat_idx = *p;
                } else {
                    uint32_t nidx = cnt++;
                    hashmap_put(&mat_idx_map, hm_cast(mptr), hm_cast(nidx));
                    rmsh->shapes[k].mat_idx = nidx;
                }
            }
            hashmap_destroy(&mat_idx_map);
            /* Store handle to temporary hashmap to use by object references later */
            struct submesh_info* si = calloc(1, sizeof(*si));
            si->model = strdup(m->ref);
            si->mgroup_name = strdup(node->name);
            hashmap_put(&model_handles_map, hm_cast(si), *((hm_ptr*)&id));
        }
        scene_destroy(sc);
    }

    /* Load textures */
    struct hashmap texture_handles_map;
    hashmap_init(&texture_handles_map, hm_str_hash, hm_str_eql);
    bench("[+] Tex time")
    for (size_t i = 0; i < sc->num_textures; ++i) {
        struct scene_texture* t = sc->textures + i;
        if (!hashmap_exists(&texture_handles_map, hm_cast(t->ref))) {
            /* Load, parse and upload texture */
            image im;
            timed_load(t->path)
                im = image_from_file_helper(t->path);
            struct texture tex = {
                .name = 0,
                .path = 0,
                .img  = im,
            };
            rid id = resmgr_add_texture(rmgr, &tex);
            hashmap_put(&texture_handles_map, hm_cast(t->ref), *((hm_ptr*)&id));
            free(im.data);
        }
    }

    /* Load materials */
    struct hashmap material_handles_map;
    hashmap_init(&material_handles_map, hm_str_hash, hm_str_eql);
    for (size_t i = 0; i < sc->num_materials; ++i) {
        struct scene_material* m = sc->materials + i;
        if (!hashmap_exists(&material_handles_map, hm_cast(m->ref))) {
            struct render_material rmat;
            resmgr_default_rmat(&rmat);
            rmat.rs = 0.9;
            rmat.type = MATERIAL_TYPE_METALLIC_ROUGHNESS;

            int using_gloss = 0, using_spec = 0;
            for (int j = 0; j < STT_MAX; ++j) {
                const char* tex_ref = m->textures[j].tex_ref;
                float scl = m->textures[j].scale[0];
                hm_ptr* p = tex_ref ? hashmap_get(&texture_handles_map, hm_cast(tex_ref)) : 0;
                if (!p)
                    continue;
                rid tid = *(rid*)p;
                switch (j) {
                    case STT_ALBEDO:
                        rmat.kd_txt = tid;
                        rmat.kd_txt_info.scale = scl;
                        break;
                    case STT_NORMAL:
                        rmat.norm_txt = tid;
                        rmat.norm_txt_info.scale = scl;
                        break;
                    case STT_ROUGHNESS:
                        rmat.rs_txt = tid;
                        rmat.rs_txt_info.scale = scl;
                        using_gloss = 0;
                        break;
                    case STT_METALLIC:
                        rmat.ks_txt = tid;
                        rmat.ks_txt_info.scale = scl;
                        using_spec = 0;
                        break;
                    case STT_SPECULAR:
                        rmat.ks_txt = tid;
                        rmat.ks_txt_info.scale = scl;
                        using_spec = 1;
                        break;
                    case STT_GLOSSINESS:
                        rmat.rs_txt = tid;
                        rmat.rs_txt_info.scale = scl;
                        using_gloss = 1;
                        break;
                    case STT_EMISSION:
                        rmat.ke_txt = tid;
                        rmat.ke_txt_info.scale = scl;
                        break;
                    case STT_OCCLUSION:
                        rmat.occ_txt = tid;
                        rmat.occ_txt_info.scale = scl;
                        break;
                    case STT_DETAIL_ALBEDO:
                        rmat.kdd_txt = tid;
                        rmat.kdd_txt_info.scale = scl;
                        break;
                    case STT_DETAIL_NORMAL:
                        rmat.normd_txt = tid;
                        rmat.normd_txt_info.scale = scl;
                        break;
                }
            }
            if (using_spec) {
                rmat.type = MATERIAL_TYPE_SPECULAR_GLOSSINESS;
                if (!using_gloss)
                    rmat.type = MATERIAL_TYPE_SPECULAR_ROUGHNESS;
            }
            rid id = resmgr_add_material(rmgr, &rmat);
            hashmap_put(&material_handles_map, hm_cast(m->ref), *((hm_ptr*)&id));
        }
    }

    /* Initialize world */
    world_t world = world_create();
    /* Used to later populate parent relations */
    entity_t* transform_handles = calloc(sc->num_objects, sizeof(entity_t));
    struct hashmap transform_handles_map;
    hashmap_init(&transform_handles_map, hm_str_hash, hm_str_eql);
    /* Add all scene objects */
    for (size_t i = 0; i < sc->num_objects; ++i) {
        /* Scene object refering to */
        struct scene_object* so = sc->objects + i;
        /* Create entity */
        entity_t e = entity_create(world);
        /* Create and set render component */
        if (so->mdl_ref) {
            struct submesh_info si = { .model = so->mdl_ref, .mgroup_name = so->mgroup_name };
            hm_ptr* p = hashmap_get(&model_handles_map, hm_cast(&si));
            if (p) {
                rid id = *(rid*)hm_pcast(p);
                struct render_component* rendr_c = render_component_create(world, e);
                rendr_c->mesh = id;
                for (unsigned int j = 0; j < MAX_MATERIALS && j < so->num_mat_refs; ++j) {
                    rid* mid = (rid*)hashmap_get(&material_handles_map, hm_cast(so->mat_refs[j]));
                    rendr_c->materials[j] = *mid;
                }
            } else {
                printf("Mesh group offset not found for %s!\n", so->mgroup_name);
            }
        }
        /* Create and set transform component */
        float* pos = so->transform.translation;
        float* rot = so->transform.rotation;
        float* scl = so->transform.scaling;
        transform_component_create(world, e);
        transform_component_set_pose(world, e, (struct transform_pose) {
            vec3_new(scl[0], scl[1], scl[2]),
            quat_new(rot[0], rot[1], rot[2], rot[3]),
            vec3_new(pos[0], pos[1], pos[2])
        });
        /* Store transform handle for later parent relations */
        transform_handles[i] = e;
        hashmap_put(&transform_handles_map, hm_cast(so->ref), i);
    }

    /* Populate parent links */
    for (size_t i = 0; i < sc->num_objects; ++i) {
        struct scene_object* so = sc->objects + i;
        if (so->parent_ref) {
            uint32_t cidx = *(uint32_t*)hashmap_get(&transform_handles_map, hm_cast(so->ref));
            uint32_t pidx = *(uint32_t*)hashmap_get(&transform_handles_map, hm_cast(so->parent_ref));
            entity_t tc_child = transform_handles[cidx];
            entity_t tc_parnt = transform_handles[pidx];
            transform_component_set_parent(world, tc_child, tc_parnt);
        }
    }

    /* Add all scene lights */
    for (size_t i = 0; i < sc->num_lights; ++i) {
        struct scene_light* sl = sc->lights + i;
        entity_t e = entity_create(world);
        struct light_component* light_c = light_component_create(world, e);
        memcpy(light_c->color.rgb, sl->color, sizeof(light_c->color.rgb));
        light_c->intensity = sl->intensity;
        switch (sl->type) {
            case SLT_DIRECTIONAL:
                light_c->type = LC_DIRECTIONAL;
                memcpy(light_c->direction.xyz, sl->direction, sizeof(light_c->direction.xyz));
                break;
            case SLT_POINT:
                light_c->type = LC_POINT;
                memcpy(light_c->position.xyz, sl->position, sizeof(light_c->position.xyz));
                light_c->falloff = sl->falloff;
                break;
            case SLT_SPOT:
                light_c->type = LC_SPOT;
                memcpy(light_c->position.xyz, sl->position, sizeof(light_c->position.xyz));
                memcpy(light_c->direction.xyz, sl->direction, sizeof(light_c->direction.xyz));
                light_c->inner_cone = sl->inner_cone;
                light_c->outer_cone = sl->outer_cone;
                break;
        }
    }

    /* Add all scene cameras */
    for (size_t i = 0; i < sc->num_cameras; ++i) {
        struct scene_camera* scm = sc->cameras + i;
        entity_t e = entity_create(world);
        struct camera_component* cam_c = camera_component_create(world, e);
        camctrl_setpos(&cam_c->camctrl, *(vec3*)scm->position);
        camctrl_setdir(&cam_c->camctrl, *(vec3*)scm->target);
    }

    hashmap_destroy(&material_handles_map);
    hashmap_destroy(&texture_handles_map);
    struct hashmap_iter it;
    hashmap_for(model_handles_map, it) {
        struct submesh_info* si = hm_pcast(it.p->key);
        free((void*)si->mgroup_name);
        free((void*)si->model);
        free(si);
    }
    hashmap_destroy(&model_handles_map);
    hashmap_destroy(&transform_handles_map);
    free(transform_handles);
    scene_file_destroy(sc);

    return world;
}
