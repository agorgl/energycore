#include "scene_file.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <assets/fileload.h>
#include <hashmap.h>
#include <json.h>

struct scene_prefab {
    const char* model_ref;
    const char** material_refs;
    size_t num_material_refs;
};

static void json_parse_float_arr(float* out, struct json_array_element_s* arr_e, int lim)
{
    for (int i = 0; i < lim; ++i) {
        struct json_number_s* n = arr_e->value->payload;
        out[i] = atof(n->number);
        arr_e = arr_e->next;
    }
}

static void free_prefab_data(hm_ptr k, hm_ptr v)
{
    (void) k;
    struct scene_prefab* prefab = hm_pcast(v);
    if (prefab->model_ref)
        free((void*)prefab->model_ref);
    if (prefab->material_refs) {
        for (size_t i = 0; i < prefab->num_material_refs; ++i)
            free((void*)prefab->material_refs[i]);
        free(prefab->material_refs);
    }
    free(prefab);
}

static const char* combine_path(const char* p1, const char* p2)
{
    size_t path_sz = strlen(p1) + strlen(p2) + 1;
    char* np = calloc(path_sz, sizeof(char));
    strncat(np, p1, strrchr(p1, '/') - p1 + 1);
    strncat(np, p2, strlen(p2));
    return np;
}

struct scene* scene_from_file(const char* filepath)
{
    /* Load file data */
    long data_buf_sz = filesize(filepath);
    if (data_buf_sz == -1) {
        printf("File not found: %s\n", filepath);
        return 0;
    }
    unsigned char* data_buf = malloc(data_buf_sz * sizeof(char));
    read_file_to_mem(filepath, data_buf, data_buf_sz);

    /* Parse json */
    struct json_value_s* root = json_parse(data_buf, data_buf_sz);
    struct json_object_s* root_obj = root->payload;

    /* Locate json object references */
    struct json_object_s* models_jo = 0;
    struct json_object_s* prefabs_jo = 0;
    struct json_object_s* scenes_jo = 0;
    struct json_object_s* textures_jo = 0;
    struct json_object_s* materials_jo = 0;
    for (struct json_object_element_s* e = root_obj->start; e; e = e->next) {
        if (strncmp(e->name->string, "models", e->name->string_size) == 0) {
            models_jo = e->value->payload;
        } else if (strncmp(e->name->string, "prefabs", e->name->string_size) == 0) {
            prefabs_jo = e->value->payload;
        } else if (strncmp(e->name->string, "scenes", e->name->string_size) == 0) {
            scenes_jo = e->value->payload;
        } else if (strncmp(e->name->string, "textures", e->name->string_size) == 0) {
            textures_jo = e->value->payload;
        } else if (strncmp(e->name->string, "materials", e->name->string_size) == 0) {
            materials_jo = e->value->payload;
        }
    }

    /* Allocate empty scene object */
    struct scene* sc = calloc(1, sizeof(struct scene));
    size_t i;

    /* Fill in models */
    sc->num_models = models_jo->length;
    sc->models = calloc(sc->num_models, sizeof(struct scene_model));
    i = 0;
    for (struct json_object_element_s* em = models_jo->start; em; em = em->next) {
        const char* mdl_key = em->name->string;
        const char* mdl_loc = ((struct json_string_s*) em->value->payload)->string;
        sc->models[i].ref = strdup(mdl_key);
        sc->models[i].path = combine_path(filepath, mdl_loc);
        ++i;
    }

    /* Fill in textures */
    if (textures_jo) {
        sc->num_textures = textures_jo->length;
        sc->textures = calloc(sc->num_textures, sizeof(struct scene_texture));
        i = 0;
        for (struct json_object_element_s* em = textures_jo->start; em; em = em->next) {
            const char* tex_key = em->name->string;
            const char* tex_loc = ((struct json_string_s*) em->value->payload)->string;
            sc->textures[i].ref = strdup(tex_key);
            sc->textures[i].path = combine_path(filepath, tex_loc);
            ++i;
        }
    }

    /* Fill in materials */
    if (materials_jo) {
        sc->num_materials = materials_jo->length;
        sc->materials = calloc(sc->num_materials, sizeof(struct scene_material));
        i = 0;
        for (struct json_object_element_s* em = materials_jo->start; em; em = em->next) {
            const char* mat_key = em->name->string;
            sc->materials[i].ref = strdup(mat_key);
            /* Iterate through material attributes */
            struct json_object_s* mat_attr_jo = em->value->payload;
            for (struct json_object_element_s* emma = mat_attr_jo->start; emma; emma = emma->next) {
                if (strncmp(emma->name->string, "MainTex", emma->name->string_size) == 0) {
                    const char* tex_key = ((struct json_string_s*)(emma->value->payload))->string;
                    sc->materials[i].albedo_tex_ref = strdup(tex_key);
                }
            }
            ++i;
        }
    }

    /* Create prefab map */
    struct hashmap prefabmap;
    hashmap_init(&prefabmap, hm_str_hash, hm_str_eql);
    /* Iterate through prefabs map */
    for (struct json_object_element_s* ep = prefabs_jo->start; ep; ep = ep->next) {
        const char* prfb_key = ep->name->string;
        if (ep->value->type == json_type_null)
            continue;
        struct json_object_s* prfb_jo = ep->value->payload;
        struct scene_prefab* prefab = calloc(1, sizeof(struct scene_prefab));
        for (struct json_object_element_s* epv = prfb_jo->start; epv; epv = epv->next) {
            if (strncmp(epv->name->string, "model", epv->name->string_size) == 0) {
                const char* mdl_key = ((struct json_string_s*)epv->value->payload)->string;
                prefab->model_ref = strdup(mdl_key);
            } else if (strncmp(epv->name->string, "materials", epv->name->string_size) == 0) {
                struct json_array_s* mats_ja = epv->value->payload;
                prefab->num_material_refs = mats_ja->length;
                prefab->material_refs = calloc(prefab->num_material_refs, sizeof(const char*));
                size_t i = 0;
                for (struct json_array_element_s* mae = mats_ja->start; mae; mae = mae->next) {
                    const char* mat_key = ((struct json_string_s*)mae->value->payload)->string;
                    prefab->material_refs[i] = strdup(mat_key);
                    ++i;
                }
            }
        }
        hashmap_put(&prefabmap, hm_cast(prfb_key), hm_cast(prefab));
    }

    /* Populate scene objects */
    size_t scene_objects_capacity = 0;
    struct hashmap sobjmap; /* Scene obj key to array index mapping */
    hashmap_init(&sobjmap, hm_str_hash, hm_str_eql);
    for (struct json_object_element_s* es = scenes_jo->start; es; es = es->next) {
        /* Iterate through scene object types */
        struct json_object_s* scene_jo = es->value->payload;
        for (struct json_object_element_s* est = scene_jo->start; est; est = est->next) {
            if (strncmp(est->name->string, "objects", est->name->string_size) == 0) {
                struct json_object_s* scene_objs_jo = est->value->payload;
                /* Extend scene_objects array */
                scene_objects_capacity += scene_objs_jo->length;
                sc->objects = realloc(sc->objects, scene_objects_capacity * sizeof(struct scene_object));
                /* Iterate through objects */
                for (struct json_object_element_s* eso = scene_objs_jo->start; eso; eso = eso->next) {
                    struct json_object_s* scene_obj_jo = eso->value->payload;
                    /* Iterate through object's attributes */
                    struct scene_object* scn_obj = sc->objects + (sc->num_objects)++;
                    memset(scn_obj, 0, sizeof(*scn_obj));
                    /* Store its key - array index pair to populate parent references later */
                    hashmap_put(&sobjmap, hm_cast(eso->name->string), hm_cast(sc->num_objects - 1));
                    for (struct json_object_element_s* esoa = scene_obj_jo->start; esoa; esoa = esoa->next) {
                        if (strncmp(esoa->name->string, "name", esoa->name->string_size) == 0) {
                            const char* name = ((struct json_string_s*)esoa->value->payload)->string;
                            scn_obj->name = strdup(name);
                        } else if (strncmp(esoa->name->string, "prefab", esoa->name->string_size) == 0) {
                            const char* prefab_key = ((struct json_string_s*)esoa->value->payload)->string;
                            /* Resolve */
                            hm_ptr* p = hashmap_get(&prefabmap, hm_cast(prefab_key));
                            if (p) {
                                struct scene_prefab* prefab = hm_pcast(*p);
                                /* Model refs */
                                scn_obj->mdl_ref = strdup(prefab->model_ref);
                                /* Mat refs */
                                if (prefab->material_refs) {
                                    scn_obj->num_mat_refs = prefab->num_material_refs;
                                    scn_obj->mat_refs = calloc(scn_obj->num_mat_refs, sizeof(struct scene_material));
                                    for (size_t i = 0; i < prefab->num_material_refs; ++i)
                                        scn_obj->mat_refs[i] = strdup(prefab->material_refs[i]);
                                }
                            }
                        } else if (strncmp(esoa->name->string, "transform", esoa->name->string_size) == 0) {
                            /* Iterate through transform attributes */
                            struct json_object_s* scene_obj_tattr_jo = esoa->value->payload;
                            for (struct json_object_element_s* e = scene_obj_tattr_jo->start; e; e = e->next) {
                                struct json_array_s* ev_jar = e->value->payload;
                                if (strncmp(e->name->string, "scale", e->name->string_size) == 0) {
                                    json_parse_float_arr(scn_obj->transform.scaling, ev_jar->start, 3);
                                }
                                else if (strncmp(e->name->string, "rotation", e->name->string_size) == 0) {
                                    json_parse_float_arr(scn_obj->transform.rotation, ev_jar->start, 4);
                                }
                                else if (strncmp(e->name->string, "position", e->name->string_size) == 0) {
                                    json_parse_float_arr(scn_obj->transform.translation, ev_jar->start, 3);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

#define for_obj_elem(e, o) for (struct json_object_element_s* e =      \
                                    ((struct json_object_s*)o)->start; \
                                    e; e = e->next)

    /* Populate parent links */
    size_t cur_obj = 0;
    for_obj_elem(es, scenes_jo) {
        for_obj_elem(est, es->value->payload) { /* Iterate through scene object types */
            if (strncmp(est->name->string, "objects", est->name->string_size) == 0) {
                for_obj_elem(eso, est->value->payload) { /* Iterate through objects */
                    struct scene_object* scn_obj = sc->objects + cur_obj++;
                    scn_obj->parent_ofs = -1L;
                    for_obj_elem(esoa, eso->value->payload) { /* Iterate through object's attributes */
                        if (strncmp(esoa->name->string, "transform", esoa->name->string_size) == 0) {
                            for_obj_elem(e, esoa->value->payload) { /* Iterate through transform attributes */
                                if (strncmp(e->name->string, "parent", e->name->string_size) == 0) {
                                    struct json_string_s* n = e->value->payload;
                                    hm_ptr* p = hashmap_get(&sobjmap, hm_cast(n->string));
                                    long par_ofs = p ? *(long*)p : -1;
                                    scn_obj->parent_ofs = par_ofs;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    hashmap_destroy(&sobjmap);
    hashmap_iter(&prefabmap, free_prefab_data);
    hashmap_destroy(&prefabmap);
    free(root);
    free(data_buf);
    return sc;
}

void scene_destroy(struct scene* sc)
{
    for (size_t i = 0; i < sc->num_objects; ++i) {
        struct scene_object* o = sc->objects + i;
        if (o->mdl_ref)
            free((void*)o->mdl_ref);
        if (o->mat_refs) {
            for (size_t i = 0; i < o->num_mat_refs; ++i)
                free((void*)o->mat_refs[i]);
            free(o->mat_refs);
        }
        if (o->name)
            free((void*)o->name);
    }
    free(sc->objects);
    for (size_t i = 0; i < sc->num_materials; ++i) {
        struct scene_material* m = sc->materials + i;
        free((void*)m->ref);
        free((void*)m->albedo_tex_ref);
    }
    free(sc->materials);
    for (size_t i = 0; i < sc->num_textures; ++i) {
        struct scene_texture* t = sc->textures + i;
        free((void*)t->ref);
        free((void*)t->path);
    }
    free(sc->textures);
    for (size_t i = 0; i < sc->num_models; ++i) {
        struct scene_model* m = sc->models + i;
        free((void*)m->ref);
        free((void*)m->path);
    }
    free(sc->models);
    free(sc);
}
