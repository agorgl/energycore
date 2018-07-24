#include "world_ext.h"
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <prof.h>
#include <hashmap.h>
#include <energycore/asset.h>
#include "ecs/world.h"
#include "scene_file.h"
#include "asset_data.h"
#include "util.h"

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

static struct shape* mesh_data_to_shape(struct mesh_data* mdata)
{
    struct shape* shp = calloc(1, sizeof(*shp));
    shp->num_pos = shp->num_norm = shp->num_texcoord = shp->num_tangsp = mdata->num_verts;
    shp->pos      = calloc(shp->num_pos,      sizeof(*shp->pos));
    shp->norm     = calloc(shp->num_norm,     sizeof(*shp->norm));
    shp->texcoord = calloc(shp->num_texcoord, sizeof(*shp->texcoord));
    shp->tangsp   = calloc(shp->num_tangsp,   sizeof(*shp->tangsp));
    memcpy(shp->pos,      mdata->positions, shp->num_pos * sizeof(*shp->pos));
    memcpy(shp->norm,     mdata->normals,   shp->num_norm * sizeof(*shp->norm));
    memcpy(shp->texcoord, mdata->texcoords, shp->num_texcoord * sizeof(*shp->texcoord));
    for (size_t i = 0; i < shp->num_tangsp; ++i) {
        for (size_t k = 0; k < 3; ++k)
            ((float*)&shp->tangsp[i])[k] = mdata->tangents[i][k];
    }
    shp->num_triangles = mdata->num_triangles;
    shp->triangles = calloc(shp->num_triangles, sizeof(*shp->triangles));
    memcpy(shp->triangles, mdata->triangles, shp->num_triangles * sizeof(*shp->triangles));
    return shp;
}

struct world* world_external(const char* scene_file, struct resmgr* rmgr)
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
        struct scene* sc = scene_from_file(m->path);
        if (sc) {
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
        } else {
            struct model_data mdl;
            model_data_from_file(&mdl, m->path);
            for (size_t j = 0; j < mdl.num_group_names; ++j) {
                struct mesh mesh;
                _mesh_init(&mesh);
                for (size_t k = 0; k < mdl.num_meshes; ++k) {
                    struct mesh_data* mdata = &mdl.meshes[k];
                    if (mdata->group_idx == j) {
                        ++mesh.num_shapes;
                        mesh.shapes = realloc(mesh.shapes, mesh.num_shapes * sizeof(*mesh.shapes));
                        mesh.shapes[mesh.num_shapes - 1] = mesh_data_to_shape(mdata);
                    }
                }
                rid id = resmgr_add_mesh(rmgr, &mesh);
                struct render_mesh* rmsh = resmgr_get_mesh(rmgr, id);
                for (size_t k = 0; k < rmsh->num_shapes; ++k)
                    rmsh->shapes[k].mat_idx = mdl.meshes[k].mat_idx;
                _mesh_destroy(&mesh);
                /* Store handle to temporary hashmap to use by object references later */
                struct submesh_info* si = calloc(1, sizeof(*si));
                si->model = strdup(m->ref);
                si->mgroup_name = strdup(mdl.group_names[j]);
                hashmap_put(&model_handles_map, hm_cast(si), *((hm_ptr*)&id));
            }
            model_data_free(&mdl);
        }
    }

    /* Load textures */
    struct hashmap texture_handles_map;
    hashmap_init(&texture_handles_map, hm_str_hash, hm_str_eql);
    bench("[+] Tex time")
    for (size_t i = 0; i < sc->num_textures; ++i) {
        struct scene_texture* t = sc->textures + i;
        if (!hashmap_exists(&texture_handles_map, hm_cast(t->ref))) {
            /* Load, parse and upload texture */
            struct image_data img_data;
            image_data_from_file(&img_data, t->path);
            struct texture tex = {
                .name = 0,
                .path = 0,
                .img  = (image) {
                    .w                = img_data.width,
                    .h                = img_data.height,
                    .channels         = img_data.channels,
                    .bit_depth        = img_data.bit_depth,
                    .data             = img_data.data,
                    .sz               = img_data.data_sz,
                    .compression_type = img_data.compression_type
                }
            };
            rid id = resmgr_add_texture(rmgr, &tex);
            hashmap_put(&texture_handles_map, hm_cast(t->ref), *((hm_ptr*)&id));
            image_data_free(&img_data);
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
    struct world* world = world_create();
    /* Used to later populate parent relations */
    entity_t* transform_handles = calloc(sc->num_objects, sizeof(entity_t));
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
            struct submesh_info si = { .model = so->mdl_ref, .mgroup_name = so->mgroup_name };
            hm_ptr* p = hashmap_get(&model_handles_map, hm_cast(&si));
            if (p) {
                rid id = *(rid*)hm_pcast(p);
                struct render_component* rendr_c = render_component_create(world->ecs, e);
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
        transform_component_create(world->ecs, e);
        transform_component_set_pose(world->ecs, e, (struct transform_pose) {
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
            transform_component_set_parent(world->ecs, tc_child, tc_parnt);
        }
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
