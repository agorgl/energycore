#include "scene_file.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <assets/fileload.h>
#include <hashmap.h>
#include <json.h>

struct scene_prefab {
    struct scene_object* objects;
    size_t num_objects;
};

/* Fw declarations */
static void free_material(struct scene_material* m);
static void free_texture(struct scene_texture* t);
static void free_model(struct scene_model* m);
static void free_scene_object(struct scene_object* o);
static void scene_cleanup(struct scene* sc);

static void json_parse_float_arr(float* out, struct json_array_element_s* arr_e, int lim)
{
    for (int i = 0; i < lim; ++i) {
        struct json_number_s* n = arr_e->value->payload;
        out[i] = atof(n->number);
        arr_e = arr_e->next;
    }
}

static void parse_transform_component(struct json_object_s* j_transf, float trans[3], float rot[4], float scl[3])
{
    /* Iterate through transform attributes */
    for (struct json_object_element_s* e = j_transf->start; e; e = e->next) {
        struct json_array_s* ev_jar = e->value->payload;
        if (strncmp(e->name->string, "scale", e->name->string_size) == 0) {
            json_parse_float_arr(scl, ev_jar->start, 3);
        }
        else if (strncmp(e->name->string, "rotation", e->name->string_size) == 0) {
            json_parse_float_arr(rot, ev_jar->start, 4);
        }
        else if (strncmp(e->name->string, "position", e->name->string_size) == 0) {
            json_parse_float_arr(trans, ev_jar->start, 3);
        }
    }
}

static void parse_scene_object(struct json_object_element_s* eso, struct scene_object* scn_obj)
{
    scn_obj->ref = strdup(eso->name->string);
    struct json_object_s* scene_obj_jo = eso->value->payload;
    for (struct json_object_element_s* attr = scene_obj_jo->start; attr; attr = attr->next) {
        if (strncmp(attr->name->string, "name", attr->name->string_size) == 0) {
            const char* name = ((struct json_string_s*)attr->value->payload)->string;
            scn_obj->name = strdup(name);
        } else if (strncmp(attr->name->string, "transform", attr->name->string_size) == 0) {
            struct json_object_s* trans_jo = attr->value->payload;
            /* Iterate through transform attributes */
            parse_transform_component(
                trans_jo,
                scn_obj->transform.translation,
                scn_obj->transform.rotation,
                scn_obj->transform.scaling);
            for (struct json_object_element_s* e = trans_jo->start; e; e = e->next) {
                if (strncmp(e->name->string, "parent", e->name->string_size) == 0) {
                    const char* par_ref = ((struct json_string_s*)e->value->payload)->string;
                    scn_obj->parent_ref = strdup(par_ref);
                }
            }
        } else if (strncmp(attr->name->string, "model", attr->name->string_size) == 0) {
            const char* mdl_ref = ((struct json_string_s*)attr->value->payload)->string;
            scn_obj->mdl_ref = strdup(mdl_ref);
        } else if (strncmp(attr->name->string, "materials", attr->name->string_size) == 0) {
            struct json_array_s* mats_ja = attr->value->payload;
            scn_obj->num_mat_refs = mats_ja->length;
            scn_obj->mat_refs = calloc(scn_obj->num_mat_refs, sizeof(const char*));
            size_t i = 0;
            for (struct json_array_element_s* mae = mats_ja->start; mae; mae = mae->next) {
                const char* mat_key = ((struct json_string_s*)mae->value->payload)->string;
                scn_obj->mat_refs[i] = strdup(mat_key);
                ++i;
            }
        } else if (strncmp(attr->name->string, "mgroup_name", attr->name->string_size) == 0) {
            const char* mgroup = ((struct json_string_s*)attr->value->payload)->string;
            scn_obj->mgroup_name = strdup(mgroup);
        }
    }
}

static const char* parse_prefab_parent(struct json_object_s* scene_obj_jo)
{
    for (struct json_object_element_s* attr = scene_obj_jo->start; attr; attr = attr->next)
        if (strncmp(attr->name->string, "prefab", attr->name->string_size) == 0)
            return ((struct json_string_s*)attr->value->payload)->string;
    return 0;
}

static void parse_scene_chunk(struct json_object_s* scene_objs_jo, struct scene_object* scn_objs)
{
    /* Iterate through objects */
    size_t i = 0;
    for (struct json_object_element_s* eso = scene_objs_jo->start; eso; eso = eso->next) {
        struct scene_object* scn_obj = scn_objs + (i++);
        memset(scn_obj, 0, sizeof(struct scene_object));
        parse_scene_object(eso, scn_obj);
    }
}

static const char* combine_path(const char* p1, const char* p2)
{
    size_t path_sz = strlen(p1) + strlen(p2) + 1;
    char* np = calloc(path_sz, sizeof(char));
    strncat(np, p1, strrchr(p1, '/') - p1 + 1);
    strncat(np, p2, strlen(p2));
    return np;
}

static void copy_scene_object(struct scene_object* dst, struct scene_object* src)
{
    dst->ref = strdup(src->ref);
    dst->name = strdup(src->name);
    dst->mdl_ref = src->mdl_ref ? strdup(src->mdl_ref) : 0;
    dst->parent_ref = src->parent_ref ? strdup(src->parent_ref) : 0;
    dst->num_mat_refs = src->num_mat_refs;
    dst->mat_refs = calloc(dst->num_mat_refs, sizeof(const char*));
    dst->mgroup_name = src->mgroup_name ? strdup(src->mgroup_name) : 0;
    for (size_t i = 0; i < src->num_mat_refs; ++i)
        dst->mat_refs[i] = strdup(src->mat_refs[i]);
    memcpy(&dst->transform, &src->transform, sizeof(((struct scene_object*)0)->transform));
}

static void free_prefab_data(hm_ptr k, hm_ptr v)
{
    (void) k;
    struct scene_prefab* sp = (struct scene_prefab*)hm_pcast(v);
    for (size_t i = 0; i < sp->num_objects; ++i)
        free_scene_object(sp->objects + i);
    free(sp->objects);
    free(sp);
}

static const char* complex_ref(const char* guid, const char* file_id)
{
    size_t sz = strlen(guid) + strlen(file_id) + 3;
    char* nr = calloc(sz, sizeof(char));
    strncat(nr, guid, strlen(guid));
    strncat(nr, "::", 2);
    strncat(nr, file_id, strlen(file_id));
    return nr;
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
                int type = -1;
                if (strncmp(emma->name->string, "MainTex", emma->name->string_size) == 0)
                    type = STT_ALBEDO;
                if (type != -1) {
                    sc->materials[i].textures[type].type = type;
                    for (struct json_object_element_s* tex_attr = ((struct json_object_s*)(emma->value->payload))->start;
                            tex_attr; tex_attr = tex_attr->next) {
                        if (strncmp(tex_attr->name->string, "texture", tex_attr->name->string_size) == 0) {
                            const char* tex_key = ((struct json_string_s*)(tex_attr->value->payload))->string;
                            sc->materials[i].textures[type].tex_ref = strdup(tex_key);
                        } else if (strncmp(tex_attr->name->string, "scale", tex_attr->name->string_size) == 0) {
                            struct json_array_element_s* scl_arr = ((struct json_array_s*)(tex_attr->value->payload))->start;
                            json_parse_float_arr(sc->materials[i].textures[type].scale, scl_arr, 2);
                        }
                    }
                }
            }
            ++i;
        }
    }

    /* Create prefab map */
    struct hashmap prefabmap;
    hashmap_init(&prefabmap, hm_str_hash, hm_str_eql);
    /* Iterate through prefab mappings */
    for (struct json_object_element_s* ep = prefabs_jo->start; ep; ep = ep->next) {
        const char* prfb_key = ep->name->string;
        struct json_object_s* prfb_jo = ep->value->payload;
        struct scene_prefab* prefab = calloc(1, sizeof(struct scene_prefab));
        prefab->num_objects = prfb_jo->length;
        prefab->objects = calloc(prefab->num_objects, sizeof(struct scene_object));
        parse_scene_chunk(prfb_jo, prefab->objects);
        hashmap_put(&prefabmap, hm_cast(prfb_key), hm_cast(prefab));
    }

    /* Populate scene objects */
    size_t scene_objects_capacity = 0;
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
                    const char* scn_obj_ref = eso->name->string;
                    /* Populate object's attributes */
                    struct scene_object* scn_obj = sc->objects + (sc->num_objects)++;
                    memset(scn_obj, 0, sizeof(*scn_obj));
                    parse_scene_object(eso, scn_obj);
                    /* Check if it inherits a prefab */
                    const char* prfb_key = parse_prefab_parent(eso->value->payload);
                    if (prfb_key) {
                        hm_ptr* p = hashmap_get(&prefabmap, hm_cast(prfb_key));
                        if (p) {
                            struct scene_prefab* sprefb = hm_pcast(*p);
                            scene_objects_capacity += sprefb->num_objects;
                            sc->objects = realloc(sc->objects, scene_objects_capacity * sizeof(struct scene_object));
                            for (size_t i = 0; i < sprefb->num_objects; ++i) {
                                struct scene_object* src_prfb_obj = sprefb->objects + i;
                                struct scene_object* dst_prfb_obj = sc->objects + sc->num_objects++;
                                copy_scene_object(dst_prfb_obj, src_prfb_obj);
                                free((void*)dst_prfb_obj->ref);
                                dst_prfb_obj->ref = complex_ref(src_prfb_obj->ref, scn_obj_ref);
                                if (dst_prfb_obj->parent_ref)
                                    free((void*)dst_prfb_obj->parent_ref);
                                dst_prfb_obj->parent_ref = strdup(scn_obj_ref);
                            }
                        }
                    }
                }
            }
        }
    }

    hashmap_iter(&prefabmap, free_prefab_data);
    hashmap_destroy(&prefabmap);
    free(root);
    free(data_buf);

    /* Remove unused stuff from scene */
    scene_cleanup(sc);
    return sc;
}

static void scene_cleanup(struct scene* sc)
{
    struct hashmap used_materials, used_models, used_textures;
    hashmap_init(&used_materials, hm_str_hash, hm_str_eql);
    hashmap_init(&used_models, hm_str_hash, hm_str_eql);
    hashmap_init(&used_textures, hm_str_hash, hm_str_eql);

    /* Populate used models and materials using scene objects */
    for (size_t i = 0; i < sc->num_objects; ++i) {
        hm_ptr* p = 0;
        struct scene_object* so = sc->objects + i;
        if (so->mdl_ref) {
            p = hashmap_get(&used_models, hm_cast(so->mdl_ref));
            if (!p)
                hashmap_put(&used_models, hm_cast(so->mdl_ref), 0);
        }
        if (so->mat_refs) {
            for (size_t j = 0; j < so->num_mat_refs; ++j) {
                p = hashmap_get(&used_materials, hm_cast(so->mat_refs[j]));
                if (!p)
                    hashmap_put(&used_materials, hm_cast(so->mat_refs[j]), 0);
            }
        }
    }

    /* Populate used textures from used materials */
    for (size_t i = 0; i < sc->num_materials; ++i) {
        struct scene_material* sm = sc->materials + i;
        hm_ptr* p = hashmap_get(&used_materials, hm_cast(sm->ref));
        if (!p)
            continue;
        for (int j = 0; j < STT_MAX; ++j) {
            const char* tex_ref = sc->materials[i].textures[j].tex_ref;
            if (tex_ref) {
                hm_ptr* p = hashmap_get(&used_textures, hm_cast(tex_ref));
                if (!p)
                    hashmap_put(&used_textures, hm_cast(tex_ref), 0);
            }
        }
    }

    /* Remove unused models */
    for (size_t i = 0; i < sc->num_models; ++i) {
        const char* ref = sc->models[i].ref;
        hm_ptr* p = hashmap_get(&used_models, hm_cast(ref));
        if (!p) {
            free_model(sc->models + i);
            --sc->num_models;
            memmove(sc->models + i, sc->models + i + 1, (sc->num_models - i) * sizeof(struct scene_model));
            --i;
        }
    }

    /* Remove unused materials */
    for (size_t i = 0; i < sc->num_materials; ++i) {
        const char* ref = sc->materials[i].ref;
        hm_ptr* p = hashmap_get(&used_materials, hm_cast(ref));
        if (!p) {
            free_material(sc->materials + i);
            --sc->num_materials;
            memmove(sc->materials + i, sc->materials + i + 1, (sc->num_materials - i) * sizeof(struct scene_material));
            --i;
        }
    }

    /* Remove unused textures */
    for (size_t i = 0; i < sc->num_textures; ++i) {
        const char* ref = sc->textures[i].ref;
        hm_ptr* p = hashmap_get(&used_textures, hm_cast(ref));
        if (!p) {
            free_texture(sc->textures + i);
            --sc->num_textures;
            memmove(sc->textures + i, sc->textures + i + 1, (sc->num_textures - i) * sizeof(struct scene_texture));
            --i;
        }
    }

    hashmap_destroy(&used_textures);
    hashmap_destroy(&used_models);
    hashmap_destroy(&used_materials);
}

static void free_material(struct scene_material* m)
{
    free((void*)m->ref);
    for (size_t i = 0; i < STT_MAX; ++i) {
        const char* tex_ref = m->textures[i].tex_ref;
        if (tex_ref)
            free((void*)m->textures[i].tex_ref);
    }
}

static void free_texture(struct scene_texture* t)
{
    free((void*)t->ref);
    free((void*)t->path);
}

static void free_model(struct scene_model* m)
{
    free((void*)m->ref);
    free((void*)m->path);
}

static void free_scene_object(struct scene_object* o)
{
    free((void*)o->ref);
    if (o->parent_ref)
        free((void*)o->parent_ref);
    if (o->mdl_ref)
        free((void*)o->mdl_ref);
    if (o->mgroup_name)
        free((void*)o->mgroup_name);
    if (o->mat_refs) {
        for (size_t j = 0; j < o->num_mat_refs; ++j)
            free((void*)o->mat_refs[j]);
        free(o->mat_refs);
    }
    if (o->name)
        free((void*)o->name);
}

void scene_destroy(struct scene* sc)
{
    for (size_t i = 0; i < sc->num_objects; ++i)
        free_scene_object(sc->objects + i);
    free(sc->objects);
    for (size_t i = 0; i < sc->num_materials; ++i)
        free_material(sc->materials + i);
    free(sc->materials);
    for (size_t i = 0; i < sc->num_textures; ++i)
        free_texture(sc->textures + i);
    free(sc->textures);
    for (size_t i = 0; i < sc->num_models; ++i)
        free_model(sc->models + i);
    free(sc->models);
    free(sc);
}
