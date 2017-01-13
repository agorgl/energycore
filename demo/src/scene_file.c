#include "scene_file.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <assets/fileload.h>
#include <hashmap.h>
#include <json.h>

struct scene_prefab {
    const char* model;
    const char** materials;
    size_t num_materials;
};

static void json_parse_float_arr(float* out, struct json_array_element_s* arr_e, int lim)
{
    for (int i = 0; i < lim; ++i) {
        struct json_number_s* n = arr_e->value->payload;
        out[i] = atof(n->number);
        arr_e = arr_e->next;
    }
}

static void free_mat_data(hm_ptr k, hm_ptr v)
{
    (void) k;
    struct scene_material* mat = hm_pcast(v);
    if (mat->diff_tex)
        free((void*)mat->diff_tex);
    free((void*)mat->name);
    free(mat);
}

static void free_prefab_data(hm_ptr k, hm_ptr v)
{
    (void) k;
    struct scene_prefab* prefab = hm_pcast(v);
    if (prefab->model)
        free((void*)prefab->model);
    if (prefab->materials) {
        for (size_t i = 0; i < prefab->num_materials; ++i)
            free((void*)prefab->materials[i]);
        free(prefab->materials);
    }
    free(prefab);
}

void load_scene_file(struct scene_object** scene_objects, size_t* num_scene_objects, const char* filepath)
{
    *num_scene_objects = 0;
    *scene_objects = 0;
    size_t scene_objects_capacity = 0;

    /* Load file data */
    long data_buf_sz = filesize(filepath);
    if (data_buf_sz == -1) {
        printf("File not found: %s\n", filepath);
        return;
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

    /* Create model location map */
    struct hashmap modelmap;
    hashmap_init(&modelmap, hm_str_hash, hm_str_eql);
    /* Iterate through models map */
    for (struct json_object_element_s* em = models_jo->start; em; em = em->next) {
        const char* mdl_key = em->name->string;
        const char* mdl_loc = ((struct json_string_s*) em->value->payload)->string;
        hashmap_put(&modelmap, hm_cast(mdl_key), hm_cast(mdl_loc));
    }

    /* Create texture location map */
    struct hashmap texturemap;
    hashmap_init(&texturemap, hm_str_hash, hm_str_eql);
    /* Iterate through textures nodes */
    if (textures_jo) {
        for (struct json_object_element_s* em = textures_jo->start; em; em = em->next) {
            const char* tex_key = em->name->string;
            const char* tex_loc = ((struct json_string_s*) em->value->payload)->string;
            hashmap_put(&texturemap, hm_cast(tex_key), hm_cast(tex_loc));
        }
    }

    /* Create material map */
    struct hashmap materialmap;
    hashmap_init(&materialmap, hm_str_hash, hm_str_eql);
    /* Iterate through material nodes */
    if (materials_jo) {
        for (struct json_object_element_s* em = materials_jo->start; em; em = em->next) {
            const char* mat_key = em->name->string;
            struct scene_material* mat = calloc(1, sizeof(struct scene_material));
            mat->name = strdup(mat_key);
            /* Iterate through material attributes */
            struct json_object_s* mat_attr_jo = em->value->payload;
            for (struct json_object_element_s* emma = mat_attr_jo->start; emma; emma = emma->next) {
                if (strncmp(emma->name->string, "MainTex", emma->name->string_size) == 0) {
                    const char* tex_key = ((struct json_string_s*)(emma->value->payload))->string;
                    /* Search texture */
                    hm_ptr* p = hashmap_get(&texturemap, hm_cast(tex_key));
                    if (p) {
                        const char* tex = hm_pcast(*p);
                        mat->diff_tex = strdup(tex);
                    }
                }
            }
            hashmap_put(&materialmap, hm_cast(mat_key), hm_cast(mat));
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
                prefab->model = strdup(mdl_key);
            } else if (strncmp(epv->name->string, "materials", epv->name->string_size) == 0) {
                struct json_array_s* mats_ja = epv->value->payload;
                prefab->num_materials = mats_ja->length;
                prefab->materials = calloc(prefab->num_materials, sizeof(const char*));
                size_t i = 0;
                for (struct json_array_element_s* mae = mats_ja->start; mae; mae = mae->next) {
                    const char* mat_key = ((struct json_string_s*)mae->value->payload)->string;
                    prefab->materials[i] = strdup(mat_key);
                    ++i;
                }
            }
        }
        hashmap_put(&prefabmap, hm_cast(prfb_key), hm_cast(prefab));
    }

    /* Populate scene objects */
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
                *scene_objects = realloc(*scene_objects, scene_objects_capacity * sizeof(struct scene_object));
                /* Iterate through objects */
                for (struct json_object_element_s* eso = scene_objs_jo->start; eso; eso = eso->next) {
                    struct json_object_s* scene_obj_jo = eso->value->payload;
                    /* Iterate through object's attributes */
                    struct scene_object* scn_obj = *scene_objects + (*num_scene_objects)++;
                    memset(scn_obj, 0, sizeof(*scn_obj));
                    /* Store its key - array index pair to populate parent references later */
                    hashmap_put(&sobjmap, hm_cast(eso->name->string), hm_cast(*num_scene_objects - 1));
                    for (struct json_object_element_s* esoa = scene_obj_jo->start; esoa; esoa = esoa->next) {
                        if (strncmp(esoa->name->string, "name", esoa->name->string_size) == 0) {
                            const char* name = ((struct json_string_s*)esoa->value->payload)->string;
                            scn_obj->name = strdup(name);
                        } else if (strncmp(esoa->name->string, "prefab", esoa->name->string_size) == 0) {
                            const char* prefab_key = ((struct json_string_s*)esoa->value->payload)->string;
                            /* Resolve */
                            hm_ptr* p = hashmap_get(&prefabmap, hm_cast(prefab_key));
                            if (p) {
                                hm_ptr* pp = 0;
                                struct scene_prefab* prefab = hm_pcast(*p);
                                /* Resolve model */
                                pp = hashmap_get(&modelmap, hm_cast(prefab->model));
                                if (pp) {
                                    const char* mdl_loc_rel = hm_pcast(*pp);
                                    /* Create new buffer concating basename of scene file and above path */
                                    size_t path_sz = strlen(filepath) + strlen(mdl_loc_rel) + 1;
                                    scn_obj->model_loc = calloc(path_sz, sizeof(char));
                                    strncat((char*)scn_obj->model_loc, filepath, strrchr(filepath, '/') - filepath + 1);
                                    strncat((char*)scn_obj->model_loc, mdl_loc_rel, strlen(mdl_loc_rel));
                                }
                                /* Resolve materials */
                                if (prefab->materials) {
                                    scn_obj->num_materials = prefab->num_materials;
                                    scn_obj->materials = calloc(scn_obj->num_materials, sizeof(struct scene_material));
                                    for (size_t i = 0; i < prefab->num_materials; ++i) {
                                        hm_ptr* p = hashmap_get(&materialmap, hm_cast(prefab->materials[i]));
                                        if (!p)
                                            continue;
                                        struct scene_material* mat = hm_pcast(*p);
                                        scn_obj->materials[i].name = strdup(mat->name);
                                        /* Create new buffer concating basename of scene file and above path */
                                        size_t path_sz = strlen(filepath) + strlen(mat->diff_tex) + 1;
                                        scn_obj->materials[i].diff_tex = calloc(path_sz, sizeof(char));
                                        strncat((char*)scn_obj->materials[i].diff_tex, filepath, strrchr(filepath, '/') - filepath + 1);
                                        strncat((char*)scn_obj->materials[i].diff_tex, mat->diff_tex, strlen(mat->diff_tex));
                                    }
                                }
                            }
                        } else if (strncmp(esoa->name->string, "transform", esoa->name->string_size) == 0) {
                            /* Iterate through transform attributes */
                            struct json_object_s* scene_obj_tattr_jo = esoa->value->payload;
                            for (struct json_object_element_s* e = scene_obj_tattr_jo->start; e; e = e->next) {
                                struct json_array_s* ev_jar = e->value->payload;
                                if (strncmp(e->name->string, "scale", e->name->string_size) == 0) {
                                    json_parse_float_arr(scn_obj->scaling, ev_jar->start, 3);
                                }
                                else if (strncmp(e->name->string, "rotation", e->name->string_size) == 0) {
                                    json_parse_float_arr(scn_obj->rotation, ev_jar->start, 4);
                                }
                                else if (strncmp(e->name->string, "position", e->name->string_size) == 0) {
                                    json_parse_float_arr(scn_obj->translation, ev_jar->start, 3);
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
                    struct scene_object* scn_obj = *scene_objects + cur_obj++;
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

    /* Free */
    hashmap_iter(&materialmap, free_mat_data);
    hashmap_destroy(&materialmap);
    hashmap_destroy(&texturemap);
    hashmap_destroy(&sobjmap);
    hashmap_iter(&prefabmap, free_prefab_data);
    hashmap_destroy(&prefabmap);
    hashmap_destroy(&modelmap);
    free(root);
    free(data_buf);
}
