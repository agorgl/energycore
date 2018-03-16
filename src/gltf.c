#include "gltf.h"
#include <string.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <json.h>
#include <energycore/scene_asset.h>

/*---------------------------------------------------------------------------
 * Parsing Utils
 *---------------------------------------------------------------------------*/
/* Reads file from disk to memory allocating needed space */
static void* read_file_to_mem_buf(const char* fpath)
{
    /* Check file for existence */
    FILE* f = 0;
    f = fopen(fpath, "rb");
    if (!f)
        return 0;

    /* Gather size */
    fseek(f, 0, SEEK_END);
    long file_sz = ftell(f);
    if (file_sz == -1) {
        fclose(f);
        return 0;
    }

    /* Read contents into memory buffer */
    rewind(f);
    void* data_buf = calloc(1, file_sz + 1);
    fread(data_buf, 1, file_sz, f);
    fclose(f);

    return data_buf;
}

/* Get the directory part of a filepath */
static const char* dirname(const char* filepath) { return strrchr(filepath, '/'); }

/* Error handling */
static char parse_err_buf[128];
#define parse_error(msg) snprintf(parse_err_buf, sizeof(parse_err_buf), "%s", msg);
#define parse_error_msg parse_err_buf

/*---------------------------------------------------------------------------
 * Parsing
 *---------------------------------------------------------------------------*/
static int gltf_parse_int(struct json_value_s* j)
{
    if (j->type != json_type_number) {
        assert(0 && "Expected json number");
        return 0;
    }
    struct json_number_s* n = j->payload;
    return strtol(n->number, 0, 10);
}

static float gltf_parse_float(struct json_value_s* j)
{
    if (j->type != json_type_number) {
        assert(0 && "Expected json number");
        return 0.0;
    }
    struct json_number_s* n = j->payload;
    return strtod(n->number, 0);
}

static const char* gltf_parse_string(struct json_value_s* j)
{
    if (j->type != json_type_string) {
        assert(0 && "Expected json string");
        return 0;
    }
    struct json_string_s* s = j->payload;
    return strdup(s->string);
}

static size_t gltf_parse_array_size(struct json_value_s* j)
{
    if (j->type != json_type_array) {
        assert(0 && "Expected json array");
        return 0;
    }
    struct json_array_s* a = j->payload;
    return a->length;
}

static size_t gltf_parse_object_size(struct json_value_s* j)
{
    if (j->type != json_type_object) {
        assert(0 && "Expected json object");
        return 0;
    }
    struct json_object_s* o = j->payload;
    return o->length;
}

static void gltf_parse_int_array(int* out, size_t num_elems, struct json_value_s* j)
{
    if (j->type != json_type_array) {
        assert(0 && "Expected json array");
        return;
    }
    struct json_array_s* a = j->payload; size_t i = 0;
    for (struct json_array_element_s* e = a->start; e && i < num_elems; e = e->next)
        out[i++] = gltf_parse_int(e->value);
}

static void gltf_parse_float_array(float* out, size_t num_elems, struct json_value_s* j)
{
    if (j->type != json_type_array) {
        assert(0 && "Expected json array");
        return;
    }
    struct json_array_s* a = j->payload; size_t i = 0;
    for (struct json_array_element_s* e = a->start; e && i < num_elems; e = e->next)
        out[i++] = gltf_parse_float(e->value);
}

static void gltf_parse_string_array(const char** out, size_t num_elems, struct json_value_s* j)
{
    if (j->type != json_type_array) {
        assert(0 && "Expected json array");
        return;
    }
    struct json_array_s* a = j->payload; size_t i = 0;
    for (struct json_array_element_s* e = a->start; e && i < num_elems; e = e->next)
        out[i++] = gltf_parse_string(e->value);
}

static void gltf_parse_camera_perspective(struct gltf_camera_perspective* c, struct json_value_s* j)
{
    /* Check that value is an object */
    if (j->type != json_type_object) {
        assert(0 && "Expected json object");
        return;
    }

    /* Iterate through attributes */
    struct json_object_s* o = j->payload;
    for (struct json_object_element_s* e = o->start; e; e = e->next) {
        const char* attr_name = e->name->string;
        if (strcmp(attr_name, "aspectRatio") == 0) {
            c->aspect_ratio = gltf_parse_float(e->value);
        } else if (strcmp(attr_name, "yfov") == 0) {
            c->yfov = gltf_parse_float(e->value);
        } else if (strcmp(attr_name, "zfar") == 0) {
            c->zfar = gltf_parse_float(e->value);
        } else if (strcmp(attr_name, "znear") == 0) {
            c->znear = gltf_parse_float(e->value);
        }
    }
}

static void gltf_parse_camera_orthographic(struct gltf_camera_orthographic* c, struct json_value_s* j)
{
    /* Check that value is an object */
    if (j->type != json_type_object) {
        assert(0 && "Expected json object");
        return;
    }

    /* Iterate through attributes */
    struct json_object_s* o = j->payload;
    for (struct json_object_element_s* e = o->start; e; e = e->next) {
        const char* attr_name = e->name->string;
        if (strcmp(attr_name, "xmag") == 0) {
            c->xmag = gltf_parse_float(e->value);
        } else if (strcmp(attr_name, "ymag") == 0) {
            c->ymag = gltf_parse_float(e->value);
        } else if (strcmp(attr_name, "zfar") == 0) {
            c->zfar = gltf_parse_float(e->value);
        } else if (strcmp(attr_name, "znear") == 0) {
            c->znear = gltf_parse_float(e->value);
        }
    }
}

static void gltf_parse_camera(struct gltf_camera* c, struct json_value_s* j)
{
    /* Check that value is an object */
    if (j->type != json_type_object) {
        assert(0 && "Expected json object");
        return;
    }

    /* Iterate through attributes */
    struct json_object_s* o = j->payload;
    for (struct json_object_element_s* e = o->start; e; e = e->next) {
        const char* attr_name = e->name->string;
        if (strcmp(attr_name, "name") == 0) {
            c->name = gltf_parse_string(e->value);
        } else if (strcmp(attr_name, "type") == 0) {
            c->type = gltf_parse_string(e->value);
        } else if (strcmp(attr_name, "orthographic") == 0) {
            c->orthographic = calloc(1, sizeof(*c->orthographic));
            gltf_parse_camera_orthographic(c->orthographic, e->value);
        } else if (strcmp(attr_name, "perspective") == 0) {
            c->perspective = calloc(1, sizeof(*c->perspective));
            gltf_parse_camera_perspective(c->perspective, e->value);
        }
    }
}

static void gltf_parse_image(struct gltf_image* i, struct json_value_s* j)
{
    /* Check that value is an object */
    if (j->type != json_type_object) {
        assert(0 && "Expected json object");
        return;
    }

    /* Iterate through attributes */
    struct json_object_s* o = j->payload;
    for (struct json_object_element_s* e = o->start; e; e = e->next) {
        const char* attr_name = e->name->string;
        if (strcmp(attr_name, "name") == 0) {
            i->name = gltf_parse_string(e->value);
        } else if (strcmp(attr_name, "uri") == 0) {
            i->uri = gltf_parse_string(e->value);
        } else if (strcmp(attr_name, "mimeType") == 0) {
            i->mime_type = gltf_parse_string(e->value);
        } else if (strcmp(attr_name, "bufferView") == 0) {
            i->buffer_view = gltf_parse_int(e->value);
        }
    }
}

static void gltf_parse_sampler(struct gltf_sampler* s, struct json_value_s* j)
{
    /* Check that value is an object */
    if (j->type != json_type_object) {
        assert(0 && "Expected json object");
        return;
    }

    /* Iterate through attributes */
    struct json_object_s* o = j->payload;
    for (struct json_object_element_s* e = o->start; e; e = e->next) {
        const char* attr_name = e->name->string;
        if (strcmp(attr_name, "name") == 0) {
            s->name = gltf_parse_string(e->value);
        } else if (strcmp(attr_name, "magFilter") == 0) {
            s->mag_filter = gltf_parse_int(e->value);
        } else if (strcmp(attr_name, "minFilter") == 0) {
            s->min_filter = gltf_parse_int(e->value);
        } else if (strcmp(attr_name, "wrapS") == 0) {
            s->wrap_s = gltf_parse_int(e->value);
        } else if (strcmp(attr_name, "wrapT") == 0) {
            s->wrap_t = gltf_parse_int(e->value);
        }
    }
}

static void gltf_parse_texture(struct gltf_texture* t, struct json_value_s* j)
{
    /* Check that value is an object */
    if (j->type != json_type_object) {
        assert(0 && "Expected json object");
        return;
    }

    /* Iterate through attributes */
    struct json_object_s* o = j->payload;
    for (struct json_object_element_s* e = o->start; e; e = e->next) {
        const char* attr_name = e->name->string;
        if (strcmp(attr_name, "name") == 0) {
            t->name = gltf_parse_string(e->value);
        } else if (strcmp(attr_name, "sampler") == 0) {
            t->sampler = gltf_parse_int(e->value);
        } else if (strcmp(attr_name, "source") == 0) {
            t->source = gltf_parse_int(e->value);
        }
    }
}

static void gltf_parse_texture_info(struct gltf_texture_info* ti, struct json_value_s* j)
{
    /* Check that value is an object */
    if (j->type != json_type_object) {
        assert(0 && "Expected json object");
        return;
    }

    /* Iterate through attributes */
    struct json_object_s* o = j->payload;
    for (struct json_object_element_s* e = o->start; e; e = e->next) {
        const char* attr_name = e->name->string;
        if (strcmp(attr_name, "index") == 0) {
            ti->index = gltf_parse_int(e->value);
        } else if (strcmp(attr_name, "texCoord") == 0) {
            ti->tex_coord = gltf_parse_int(e->value);
        }
    }
}

static void gltf_parse_material_normal_texture_info(struct gltf_material_normal_texture_info* ti, struct json_value_s* j)
{
    /* Check that value is an object */
    if (j->type != json_type_object) {
        assert(0 && "Expected json object");
        return;
    }

    /* Iterate through attributes */
    struct json_object_s* o = j->payload;
    for (struct json_object_element_s* e = o->start; e; e = e->next) {
        const char* attr_name = e->name->string;
        if (strcmp(attr_name, "index") == 0) {
            ti->index = gltf_parse_int(e->value);
        } else if (strcmp(attr_name, "texCoord") == 0) {
            ti->tex_coord = gltf_parse_int(e->value);
        } else if (strcmp(attr_name, "scale") == 0) {
            ti->scale = gltf_parse_float(e->value);
        }
    }
}

static void gltf_parse_material_occlusion_texture_info(struct gltf_material_occlusion_texture_info* ti, struct json_value_s* j)
{
    /* Check that value is an object */
    if (j->type != json_type_object) {
        assert(0 && "Expected json object");
        return;
    }

    /* Iterate through attributes */
    struct json_object_s* o = j->payload;
    for (struct json_object_element_s* e = o->start; e; e = e->next) {
        const char* attr_name = e->name->string;
        if (strcmp(attr_name, "index") == 0) {
            ti->index = gltf_parse_int(e->value);
        } else if (strcmp(attr_name, "texCoord") == 0) {
            ti->tex_coord = gltf_parse_int(e->value);
        } else if (strcmp(attr_name, "strength") == 0) {
            ti->strength = gltf_parse_float(e->value);
        }
    }
}

static void gltf_parse_material_pbr_metallic_roughness(struct gltf_material_pbr_metallic_roughness* m, struct json_value_s* j)
{
    /* Check that value is an object */
    if (j->type != json_type_object) {
        assert(0 && "Expected json object");
        return;
    }

    /* Iterate through attributes */
    struct json_object_s* o = j->payload;
    for (struct json_object_element_s* e = o->start; e; e = e->next) {
        const char* attr_name = e->name->string;
        if (strcmp(attr_name, "baseColorFactor") == 0) {
            gltf_parse_float_array(m->base_color_factor, 4, e->value);
        } else if (strcmp(attr_name, "baseColorTexture") == 0) {
            m->base_color_texture = calloc(1, sizeof(*m->base_color_texture));
            gltf_parse_texture_info(m->base_color_texture, e->value);
        } else if (strcmp(attr_name, "metallicFactor") == 0) {
            m->metallic_factor = gltf_parse_float(e->value);
        } else if (strcmp(attr_name, "roughnessFactor") == 0) {
            m->roughness_factor = gltf_parse_float(e->value);
        } else if (strcmp(attr_name, "baseColorTexture") == 0) {
            m->metallic_roughness_texture = calloc(1, sizeof(*m->metallic_roughness_texture));
            gltf_parse_texture_info(m->metallic_roughness_texture, e->value);
        }
    }
}

static void gltf_parse_material(struct gltf_material* m, struct json_value_s* j)
{
    /* Check that value is an object */
    if (j->type != json_type_object) {
        assert(0 && "Expected json object");
        return;
    }

    /* Iterate through attributes */
    struct json_object_s* o = j->payload;
    for (struct json_object_element_s* e = o->start; e; e = e->next) {
        const char* attr_name = e->name->string;
        if (strcmp(attr_name, "name") == 0) {
            m->name = gltf_parse_string(e->value);
        } else if (strcmp(attr_name, "pbrMetallicRoughness") == 0) {
            m->pbr_metallic_roughness = calloc(1, sizeof(*m->pbr_metallic_roughness));
            gltf_parse_material_pbr_metallic_roughness(m->pbr_metallic_roughness, e->value);
        } else if (strcmp(attr_name, "normalTexture") == 0) {
            m->normal_texture = calloc(1, sizeof(*m->normal_texture));
            gltf_parse_material_normal_texture_info(m->normal_texture, e->value);
        } else if (strcmp(attr_name, "occlusionTexture") == 0) {
            m->occlusion_texture = calloc(1, sizeof(*m->occlusion_texture));
            gltf_parse_material_occlusion_texture_info(m->occlusion_texture, e->value);
        } else if (strcmp(attr_name, "emissiveTexture") == 0) {
            m->emissive_texture = calloc(1, sizeof(*m->emissive_texture));
            gltf_parse_texture_info(m->emissive_texture, e->value);
        } else if (strcmp(attr_name, "emissiveFactor") == 0) {
            gltf_parse_float_array(m->emissive_factor, 3, e->value);
        } else if (strcmp(attr_name, "alphaMode") == 0) {
            m->alpha_mode = gltf_parse_string(e->value);
        } else if (strcmp(attr_name, "alphaCutoff") == 0) {
            m->alpha_cutoff = gltf_parse_float(e->value);
        } else if (strcmp(attr_name, "doubleSided") == 0) {
            m->double_sided = gltf_parse_int(e->value);
        }
    }
}

static void gltf_parse_mesh_primitive_attributes(struct gltf_mesh_primitive_attribute* arr, size_t num_elems, struct json_value_s* j)
{
    if (j->type != json_type_object) {
        assert(0 && "Expected json object");
        return;
    }
    struct json_object_s* o = j->payload; size_t i = 0;
    for (struct json_object_element_s* e = o->start; e && i < num_elems; e = e->next, ++i) {
        const char* attr_name = e->name->string;
        arr[i].semantic = strdup(attr_name);
        arr[i].accessor_index = gltf_parse_int(e->value);
    }
}

static void gltf_parse_mesh_primitive_targets(struct gltf_mesh_primitive_attribute (*arr)[3], size_t num_elems, struct json_value_s* j)
{
    if (j->type != json_type_object) {
        assert(0 && "Expected json object");
        return;
    }
    struct json_array_s* a = j->payload; size_t i = 0;
    for (struct json_array_element_s* e = a->start; e && i < num_elems; e = e->next)
        gltf_parse_mesh_primitive_attributes((struct gltf_mesh_primitive_attribute*)&arr[i++], 3, e->value);
}

static void gltf_parse_mesh_primitive(struct gltf_mesh_primitive* p, struct json_value_s* j)
{
    /* Check that value is an object */
    if (j->type != json_type_object) {
        assert(0 && "Expected json object");
        return;
    }

    /* Iterate through attributes */
    struct json_object_s* o = j->payload;
    for (struct json_object_element_s* e = o->start; e; e = e->next) {
        const char* attr_name = e->name->string;
        if (strcmp(attr_name, "attributes") == 0) {
            p->num_attributes = gltf_parse_object_size(e->value);
            p->attributes = calloc(p->num_attributes, sizeof(*p->attributes));
            gltf_parse_mesh_primitive_attributes(p->attributes, p->num_attributes, e->value);
        } else if (strcmp(attr_name, "indices") == 0) {
            p->indices = gltf_parse_int(e->value);
        } else if (strcmp(attr_name, "material") == 0) {
            p->material = gltf_parse_int(e->value);
        } else if (strcmp(attr_name, "mode") == 0) {
            p->mode = gltf_parse_int(e->value);
        } else if (strcmp(attr_name, "targets") == 0) {
            p->num_targets = gltf_parse_array_size(e->value);
            p->targets = calloc(p->num_targets, sizeof(*p->targets));
            gltf_parse_mesh_primitive_targets(p->targets, p->num_targets, e->value);
        }
    }
}

static void gltf_parse_mesh_primitive_array(struct gltf_mesh_primitive* arr, size_t num_elems, struct json_value_s* j)
{
    if (j->type != json_type_array) {
        assert(0 && "Expected json array");
        return;
    }
    struct json_array_s* a = j->payload; size_t i = 0;
    for (struct json_array_element_s* e = a->start; e && i < num_elems; e = e->next)
        gltf_parse_mesh_primitive(&arr[i++], e->value);
}

static void gltf_parse_mesh(struct gltf_mesh* m, struct json_value_s* j)
{
    /* Check that value is an object */
    if (j->type != json_type_object) {
        assert(0 && "Expected json object");
        return;
    }

    /* Iterate through attributes */
    struct json_object_s* o = j->payload;
    for (struct json_object_element_s* e = o->start; e; e = e->next) {
        const char* attr_name = e->name->string;
        if (strcmp(attr_name, "name") == 0) {
            m->name = gltf_parse_string(e->value);
        } else if (strcmp(attr_name, "primitives") == 0) {
            m->num_primitives = gltf_parse_array_size(e->value);
            m->primitives = calloc(m->num_primitives, sizeof(*m->primitives));
            gltf_parse_mesh_primitive_array(m->primitives, m->num_primitives, e->value);
        } else if (strcmp(attr_name, "weights") == 0) {
            m->num_weights = gltf_parse_array_size(e->value);
            m->weights = calloc(m->num_weights, sizeof(*m->weights));
            gltf_parse_float_array(m->weights, m->num_weights, e->value);
        }
    }
}

static void gltf_parse_skin(struct gltf_skin* s, struct json_value_s* j)
{
    /* Check that value is an object */
    if (j->type != json_type_object) {
        assert(0 && "Expected json object");
        return;
    }

    /* Iterate through attributes */
    struct json_object_s* o = j->payload;
    for (struct json_object_element_s* e = o->start; e; e = e->next) {
        const char* attr_name = e->name->string;
        if (strcmp(attr_name, "name") == 0) {
            s->name = gltf_parse_string(e->value);
        } else if (strcmp(attr_name, "inverseBindMatrices") == 0) {
            s->inverse_bind_matrices = gltf_parse_int(e->value);
        } else if (strcmp(attr_name, "skeleton") == 0) {
            s->skeleton = gltf_parse_int(e->value);
        } else if (strcmp(attr_name, "joints") == 0) {
            s->num_joints = gltf_parse_array_size(e->value);
            s->joints = calloc(s->num_joints, sizeof(*s->joints));
            gltf_parse_int_array(s->joints, s->num_joints, e->value);
        }
    }
}

static void gltf_parse_animation_channel_target(struct gltf_animation_channel_target* t, struct json_value_s* j)
{
    /* Check that value is an object */
    if (j->type != json_type_object) {
        assert(0 && "Expected json object");
        return;
    }

    /* Iterate through attributes */
    struct json_object_s* o = j->payload;
    for (struct json_object_element_s* e = o->start; e; e = e->next) {
        const char* attr_name = e->name->string;
        if (strcmp(attr_name, "name") == 0) {
            t->node = gltf_parse_int(e->value);
        } else if (strcmp(attr_name, "path") == 0) {
            t->path = gltf_parse_string(e->value);
        }
    }
}

static void gltf_parse_animation_channel(struct gltf_animation_channel* c, struct json_value_s* j)
{
    /* Check that value is an object */
    if (j->type != json_type_object) {
        assert(0 && "Expected json object");
        return;
    }

    /* Iterate through attributes */
    struct json_object_s* o = j->payload;
    for (struct json_object_element_s* e = o->start; e; e = e->next) {
        const char* attr_name = e->name->string;
        if (strcmp(attr_name, "sampler") == 0) {
            c->sampler = gltf_parse_int(e->value);
        } else if (strcmp(attr_name, "target") == 0) {
            c->target = calloc(1, sizeof(*c->target));
            gltf_parse_animation_channel_target(c->target, e->value);
        }
    }
}

static void gltf_parse_animation_sampler(struct gltf_animation_sampler* s, struct json_value_s* j)
{
    /* Check that value is an object */
    if (j->type != json_type_object) {
        assert(0 && "Expected json object");
        return;
    }

    /* Iterate through attributes */
    struct json_object_s* o = j->payload;
    for (struct json_object_element_s* e = o->start; e; e = e->next) {
        const char* attr_name = e->name->string;
        if (strcmp(attr_name, "input") == 0) {
            s->input = gltf_parse_int(e->value);
        } else if (strcmp(attr_name, "output") == 0) {
            s->output = gltf_parse_int(e->value);
        } else if (strcmp(attr_name, "interpolation") == 0) {
            s->interpolation = gltf_parse_string(e->value);
        }
    }
}

static void gltf_parse_animation_channel_array(struct gltf_animation_channel* arr, size_t num_elems, struct json_value_s* j)
{
    if (j->type != json_type_array) {
        assert(0 && "Expected json array");
        return;
    }
    struct json_array_s* a = j->payload; size_t i = 0;
    for (struct json_array_element_s* e = a->start; e && i < num_elems; e = e->next)
        gltf_parse_animation_channel(&arr[i++], e->value);
}

static void gltf_parse_animation_sampler_array(struct gltf_animation_sampler* arr, size_t num_elems, struct json_value_s* j)
{
    if (j->type != json_type_array) {
        assert(0 && "Expected json array");
        return;
    }
    struct json_array_s* a = j->payload; size_t i = 0;
    for (struct json_array_element_s* e = a->start; e && i < num_elems; e = e->next)
        gltf_parse_animation_sampler(&arr[i++], e->value);
}

static void gltf_parse_animation(struct gltf_animation* a, struct json_value_s* j)
{
    /* Check that value is an object */
    if (j->type != json_type_object) {
        assert(0 && "Expected json object");
        return;
    }

    /* Iterate through attributes */
    struct json_object_s* o = j->payload;
    for (struct json_object_element_s* e = o->start; e; e = e->next) {
        const char* attr_name = e->name->string;
        if (strcmp(attr_name, "name") == 0) {
            a->name = gltf_parse_string(e->value);
        } else if (strcmp(attr_name, "channels") == 0) {
            a->num_channels = gltf_parse_array_size(e->value);
            a->channels = calloc(a->num_channels, sizeof(*a->channels));
            gltf_parse_animation_channel_array(a->channels, a->num_channels, e->value);
        } else if (strcmp(attr_name, "samplers") == 0) {
            a->num_samplers = gltf_parse_array_size(e->value);
            a->samplers = calloc(a->num_samplers, sizeof(*a->samplers));
            gltf_parse_animation_sampler_array(a->samplers, a->num_samplers, e->value);
        }
    }
}

static void gltf_parse_node(struct gltf_node* n, struct json_value_s* j)
{
    /* Check that value is an object */
    if (j->type != json_type_object) {
        assert(0 && "Expected json object");
        return;
    }

    /* Iterate through attributes */
    struct json_object_s* o = j->payload;
    for (struct json_object_element_s* e = o->start; e; e = e->next) {
        const char* attr_name = e->name->string;
        if (strcmp(attr_name, "name") == 0) {
            n->name = gltf_parse_string(e->value);
        } else if (strcmp(attr_name, "camera") == 0) {
            n->camera = gltf_parse_int(e->value);
        } else if (strcmp(attr_name, "children") == 0) {
            n->num_children = gltf_parse_array_size(e->value);
            n->children = calloc(n->num_children, sizeof(*n->children));
            gltf_parse_int_array(n->children, n->num_children, e->value);
        } else if (strcmp(attr_name, "skin") == 0) {
            n->skin = gltf_parse_int(e->value);
        } else if (strcmp(attr_name, "matrix") == 0) {
            gltf_parse_float_array(n->matrix, 16, e->value);
        } else if (strcmp(attr_name, "mesh") == 0) {
            n->mesh = gltf_parse_int(e->value);
        } else if (strcmp(attr_name, "rotation") == 0) {
            gltf_parse_float_array(n->rotation, 4, e->value);
        } else if (strcmp(attr_name, "scale") == 0) {
            gltf_parse_float_array(n->scale, 3, e->value);
        } else if (strcmp(attr_name, "translation") == 0) {
            gltf_parse_float_array(n->translation, 3, e->value);
        } else if (strcmp(attr_name, "weights") == 0) {
            n->num_weights = gltf_parse_array_size(e->value);
            n->weights = calloc(n->num_weights, sizeof(*n->weights));
            gltf_parse_float_array(n->weights, n->num_weights, e->value);
        }
    }
}

static void gltf_parse_scene(struct gltf_scene* s, struct json_value_s* j)
{
    /* Check that value is an object */
    if (j->type != json_type_object) {
        assert(0 && "Expected json object");
        return;
    }

    /* Iterate through attributes */
    struct json_object_s* o = j->payload;
    for (struct json_object_element_s* e = o->start; e; e = e->next) {
        const char* attr_name = e->name->string;
        if (strcmp(attr_name, "name") == 0) {
            s->name = gltf_parse_string(e->value);
        } else if (strcmp(attr_name, "nodes") == 0) {
            s->num_nodes = gltf_parse_array_size(e->value);
            s->nodes = calloc(s->num_nodes, sizeof(*s->nodes));
            gltf_parse_int_array(s->nodes, s->num_nodes, e->value);
        }
    }
}

static void gltf_parse_buffer(struct gltf_buffer* b, struct json_value_s* j)
{
    /* Check that value is an object */
    if (j->type != json_type_object) {
        assert(0 && "Expected json object");
        return;
    }

    /* Iterate through attributes */
    struct json_object_s* o = j->payload;
    for (struct json_object_element_s* e = o->start; e; e = e->next) {
        const char* attr_name = e->name->string;
        if (strcmp(attr_name, "name") == 0) {
            b->name = gltf_parse_string(e->value);
        } else if (strcmp(attr_name, "uri") == 0) {
            b->uri = gltf_parse_string(e->value);
        } else if (strcmp(attr_name, "byteLength") == 0) {
            b->byte_length = gltf_parse_int(e->value);
        }
    }
}

static void gltf_parse_buffer_view(struct gltf_buffer_view* v, struct json_value_s* j)
{
    /* Check that value is an object */
    if (j->type != json_type_object) {
        assert(0 && "Expected json object");
        return;
    }

    /* Iterate through attributes */
    struct json_object_s* o = j->payload;
    for (struct json_object_element_s* e = o->start; e; e = e->next) {
        const char* attr_name = e->name->string;
        if (strcmp(attr_name, "name") == 0) {
            v->name = gltf_parse_string(e->value);
        } else if (strcmp(attr_name, "buffer") == 0) {
            v->buffer = gltf_parse_int(e->value);
        } else if (strcmp(attr_name, "byteOffset") == 0) {
            v->byte_offset = gltf_parse_int(e->value);
        } else if (strcmp(attr_name, "byteLength") == 0) {
            v->byte_length = gltf_parse_int(e->value);
        } else if (strcmp(attr_name, "byteStride") == 0) {
            v->byte_stride = gltf_parse_int(e->value);
        } else if (strcmp(attr_name, "target") == 0) {
            v->target = gltf_parse_int(e->value);
        }
    }
}

static void gltf_parse_accessor_sparse_indices(struct gltf_accessor_sparse_indices* i, struct json_value_s* j)
{
    /* Check that value is an object */
    if (j->type != json_type_object) {
        assert(0 && "Expected json object");
        return;
    }

    /* Iterate through attributes */
    struct json_object_s* o = j->payload;
    for (struct json_object_element_s* e = o->start; e; e = e->next) {
        const char* attr_name = e->name->string;
        if (strcmp(attr_name, "bufferView") == 0) {
            i->buffer_view = gltf_parse_int(e->value);
        } else if (strcmp(attr_name, "byteOffset") == 0) {
            i->byte_offset = gltf_parse_int(e->value);
        } else if (strcmp(attr_name, "componentType") == 0) {
            i->component_type = gltf_parse_int(e->value);
        }
    }
}

static void gltf_parse_accessor_sparse_values(struct gltf_accessor_sparse_values* v, struct json_value_s* j)
{
    /* Check that value is an object */
    if (j->type != json_type_object) {
        assert(0 && "Expected json object");
        return;
    }

    /* Iterate through attributes */
    struct json_object_s* o = j->payload;
    for (struct json_object_element_s* e = o->start; e; e = e->next) {
        const char* attr_name = e->name->string;
        if (strcmp(attr_name, "bufferView") == 0) {
            v->buffer_view = gltf_parse_int(e->value);
        } else if (strcmp(attr_name, "byteOffset") == 0) {
            v->byte_offset = gltf_parse_int(e->value);
        }
    }
}

static void gltf_parse_accessor_sparse(struct gltf_accessor_sparse* a, struct json_value_s* j)
{
    /* Check that value is an object */
    if (j->type != json_type_object) {
        assert(0 && "Expected json object");
        return;
    }

    /* Iterate through attributes */
    struct json_object_s* o = j->payload;
    for (struct json_object_element_s* e = o->start; e; e = e->next) {
        const char* attr_name = e->name->string;
        if (strcmp(attr_name, "count") == 0) {
            a->count = gltf_parse_int(e->value);
        } else if (strcmp(attr_name, "indices") == 0) {
            a->indices = calloc(1, sizeof(*a->indices));
            gltf_parse_accessor_sparse_indices(a->indices, e->value);
        } else if (strcmp(attr_name, "values") == 0) {
            a->values = calloc(1, sizeof(*a->values));
            gltf_parse_accessor_sparse_values(a->values, e->value);
        }
    }
}

static void gltf_parse_accessor(struct gltf_accessor* a, struct json_value_s* j)
{
    /* Check that value is an object */
    if (j->type != json_type_object) {
        assert(0 && "Expected json object");
        return;
    }

    /* Iterate through attributes */
    struct json_object_s* o = j->payload;
    for (struct json_object_element_s* e = o->start; e; e = e->next) {
        const char* attr_name = e->name->string;
        if (strcmp(attr_name, "name") == 0) {
            a->name = gltf_parse_string(e->value);
        } else if (strcmp(attr_name, "bufferView") == 0) {
            a->buffer_view = gltf_parse_int(e->value);
        } else if (strcmp(attr_name, "byteOffset") == 0) {
            a->byte_offset = gltf_parse_int(e->value);
        } else if (strcmp(attr_name, "componentType") == 0) {
            a->component_type = gltf_parse_int(e->value);
        } else if (strcmp(attr_name, "normalized") == 0) {
            a->normalized = gltf_parse_int(e->value);
        } else if (strcmp(attr_name, "count") == 0) {
            a->count = gltf_parse_int(e->value);
        } else if (strcmp(attr_name, "type") == 0) {
            a->type = gltf_parse_string(e->value);
        } else if (strcmp(attr_name, "max") == 0) {
            a->num_max = gltf_parse_array_size(e->value);
            a->max = calloc(a->num_max, sizeof(*a->max));
            gltf_parse_float_array(a->max, a->num_max, e->value);
        } else if (strcmp(attr_name, "min") == 0) {
            a->num_min = gltf_parse_array_size(e->value);
            a->min = calloc(a->num_min, sizeof(*a->min));
            gltf_parse_float_array(a->min, a->num_min, e->value);
        } else if (strcmp(attr_name, "sparse") == 0) {
            a->sparse = calloc(1, sizeof(*a->sparse));
            gltf_parse_accessor_sparse(a->sparse, e->value);
        }
    }
}

static void gltf_parse_asset(struct gltf_asset* a, struct json_value_s* j)
{
    /* Check that value is an object */
    if (j->type != json_type_object) {
        assert(0 && "Expected json object");
        return;
    }

    /* Iterate through attributes */
    struct json_object_s* o = j->payload;
    for (struct json_object_element_s* e = o->start; e; e = e->next) {
        const char* attr_name = e->name->string;
        if (strcmp(attr_name, "copyright") == 0) {
            a->copyright = gltf_parse_string(e->value);
        } else if (strcmp(attr_name, "generator") == 0) {
            a->generator = gltf_parse_string(e->value);
        } else if (strcmp(attr_name, "version") == 0) {
            a->version = gltf_parse_string(e->value);
        } else if (strcmp(attr_name, "minVersion") == 0) {
            a->min_version = gltf_parse_string(e->value);
        }
    }
}

typedef void(*gltf_object_parse_fn)(void*, struct json_value_s*);

static void gltf_parse_object_array(void* arr, size_t elem_sz, size_t num_elems, gltf_object_parse_fn pf, struct json_value_s* j)
{
    if (j->type != json_type_array) {
        assert(0 && "Expected json array");
        return;
    }
    struct json_array_s* a = j->payload; size_t i = 0;
    for (struct json_array_element_s* e = a->start; e && i < num_elems; e = e->next) {
        void* p = (unsigned char*)arr + i * elem_sz;
        pf(p, e->value);
        ++i;
    }
}

static void gltf_parse_root(struct gltf* g, struct json_value_s* j)
{
    /* Check that value is an object */
    if (j->type != json_type_object) {
        assert(0 && "Expected json object");
        return;
    }

    /* Iterate through attributes */
    struct json_object_s* o = j->payload;
    for (struct json_object_element_s* e = o->start; e; e = e->next) {
        const char* attr_name = e->name->string;
        if (strcmp(attr_name, "asset") == 0) {
            g->asset = calloc(1, sizeof(*g->asset));
            gltf_parse_asset(g->asset, e->value);
        } else if (strcmp(attr_name, "scene") == 0) {
            g->scene = gltf_parse_int(e->value);
        } else if (strcmp(attr_name, "accessors") == 0) {
            g->num_accessors = gltf_parse_array_size(e->value);
            g->accessors = calloc(g->num_accessors, sizeof(*g->accessors));
            gltf_parse_object_array(g->accessors, sizeof(*g->accessors), g->num_accessors,
                                    (gltf_object_parse_fn)gltf_parse_accessor, e->value);
        } else if (strcmp(attr_name, "animations") == 0) {
            g->num_animations = gltf_parse_array_size(e->value);
            g->animations = calloc(g->num_animations, sizeof(*g->animations));
            gltf_parse_object_array(g->animations, sizeof(*g->animations), g->num_animations,
                                    (gltf_object_parse_fn)gltf_parse_animation, e->value);
        } else if (strcmp(attr_name, "buffers") == 0) {
            g->num_buffers = gltf_parse_array_size(e->value);
            g->buffers = calloc(g->num_buffers, sizeof(*g->buffers));
            gltf_parse_object_array(g->buffers, sizeof(*g->buffers), g->num_buffers,
                                    (gltf_object_parse_fn)gltf_parse_buffer, e->value);
        } else if (strcmp(attr_name, "bufferViews") == 0) {
            g->num_buffer_views = gltf_parse_array_size(e->value);
            g->buffer_views = calloc(g->num_buffer_views, sizeof(*g->buffer_views));
            gltf_parse_object_array(g->buffer_views, sizeof(*g->buffer_views), g->num_buffer_views,
                                    (gltf_object_parse_fn)gltf_parse_buffer_view, e->value);
        } else if (strcmp(attr_name, "cameras") == 0) {
            g->num_cameras = gltf_parse_array_size(e->value);
            g->cameras = calloc(g->num_cameras, sizeof(*g->cameras));
            gltf_parse_object_array(g->cameras, sizeof(*g->cameras), g->num_cameras,
                                    (gltf_object_parse_fn)gltf_parse_camera, e->value);
        } else if (strcmp(attr_name, "images") == 0) {
            g->num_images = gltf_parse_array_size(e->value);
            g->images = calloc(g->num_images, sizeof(*g->images));
            gltf_parse_object_array(g->images, sizeof(*g->images), g->num_images,
                                    (gltf_object_parse_fn)gltf_parse_image, e->value);
        } else if (strcmp(attr_name, "materials") == 0) {
            g->num_materials = gltf_parse_array_size(e->value);
            g->materials = calloc(g->num_materials, sizeof(*g->materials));
            gltf_parse_object_array(g->materials, sizeof(*g->materials), g->num_materials,
                                    (gltf_object_parse_fn)gltf_parse_material, e->value);
        } else if (strcmp(attr_name, "meshes") == 0) {
            g->num_meshes = gltf_parse_array_size(e->value);
            g->meshes = calloc(g->num_meshes, sizeof(*g->meshes));
            gltf_parse_object_array(g->meshes, sizeof(*g->meshes), g->num_meshes,
                                    (gltf_object_parse_fn)gltf_parse_mesh, e->value);
        } else if (strcmp(attr_name, "nodes") == 0) {
            g->num_nodes = gltf_parse_array_size(e->value);
            g->nodes = calloc(g->num_nodes, sizeof(*g->nodes));
            gltf_parse_object_array(g->nodes, sizeof(*g->nodes), g->num_nodes,
                                    (gltf_object_parse_fn)gltf_parse_node, e->value);
        } else if (strcmp(attr_name, "samplers") == 0) {
            g->num_samplers = gltf_parse_array_size(e->value);
            g->samplers = calloc(g->num_samplers, sizeof(*g->samplers));
            gltf_parse_object_array(g->samplers, sizeof(*g->samplers), g->num_samplers,
                                    (gltf_object_parse_fn)gltf_parse_sampler, e->value);
        } else if (strcmp(attr_name, "scenes") == 0) {
            g->num_scenes = gltf_parse_array_size(e->value);
            g->scenes = calloc(g->num_scenes, sizeof(*g->scenes));
            gltf_parse_object_array(g->scenes, sizeof(*g->scenes), g->num_scenes,
                                    (gltf_object_parse_fn)gltf_parse_scene, e->value);
        } else if (strcmp(attr_name, "skins") == 0) {
            g->num_skins = gltf_parse_array_size(e->value);
            g->skins = calloc(g->num_skins, sizeof(*g->skins));
            gltf_parse_object_array(g->skins, sizeof(*g->skins), g->num_skins,
                                    (gltf_object_parse_fn)gltf_parse_skin, e->value);
        } else if (strcmp(attr_name, "textures") == 0) {
            g->num_textures = gltf_parse_array_size(e->value);
            g->textures = calloc(g->num_textures, sizeof(*g->textures));
            gltf_parse_object_array(g->textures, sizeof(*g->textures), g->num_textures,
                                    (gltf_object_parse_fn)gltf_parse_texture, e->value);
        } else if (strcmp(attr_name, "extensionsUsed") == 0) {
            g->num_extensions_used = gltf_parse_array_size(e->value);
            g->extensions_used = calloc(g->num_extensions_used, sizeof(*g->extensions_used));
            gltf_parse_string_array(g->extensions_used, g->num_extensions_used, e->value);
        } else if (strcmp(attr_name, "extensionsRequired") == 0) {
            g->num_extensions_required = gltf_parse_array_size(e->value);
            g->extensions_required = calloc(g->num_extensions_required, sizeof(*g->extensions_required));
            gltf_parse_string_array(g->extensions_required, g->num_extensions_required, e->value);
        }
    }
}

static int gltf_file_parse(struct gltf* gltf, void* data)
{
    /* Parse json */
    struct json_value_s* root = json_parse(data, strlen(data));
    if (!root) {
        parse_error("Invalid json data");
        return 0;
    }
    /* Parse gltf data */
    gltf_parse_root(gltf, root);
    free(root);
    return 1;
}

struct gltf* gltf_file_load(const char* filepath)
{
    /* Load json data */
    void* json_data = read_file_to_mem_buf(filepath);
    if (!json_data)
        return 0;

    /* Parse gltf */
    struct gltf* gltf = calloc(1, sizeof(*gltf));
    if (!gltf_file_parse(gltf, json_data)) {
        free(gltf);
        free(json_data);
        return 0;
    }

    /* Load external resources */
    const char* dname = dirname(filepath);
    (void) dname;
    /*
    gltf_file_load_buffers(gltf, dname);
    gltf_file_load_images(gltf, dname);
     */
    free(json_data);
    return gltf;
}

void gltf_destroy(struct gltf* gltf)
{
    /* Asset */
    if (gltf->asset) {
        struct gltf_asset* a = gltf->asset;
        free((void*)a->copyright);
        free((void*)a->generator);
        free((void*)a->version);
        free((void*)a->min_version);
        free(a);
    }

    /* Accessors */
    for (size_t i = 0; i < gltf->num_accessors; ++i) {
        struct gltf_accessor* a = &gltf->accessors[i];
        free((void*)a->name);
        free((void*)a->type);
        free(a->max);
        free(a->min);
        if (a->sparse) {
            struct gltf_accessor_sparse* sparse = a->sparse;
            free(sparse->indices);
            free(sparse->values);
            free(sparse);
        }
    }
    free(gltf->accessors);

    /* Animations */
    for (size_t i = 0; i < gltf->num_animations; ++i) {
        struct gltf_animation* a = & gltf->animations[i];
        free((void*)a->name);
        for (size_t j = 0; j < a->num_channels; ++j) {
            struct gltf_animation_channel* channel = &a->channels[j];
            if (channel->target) {
                struct gltf_animation_channel_target* target = channel->target;
                free((void*)target->path);
                free(target);
            }
        }
        free(a->channels);
        for (size_t j = 0; j < a->num_samplers; ++j)
            free((void*)a->samplers[j].interpolation);
        free(a->samplers);
    }
    free(gltf->animations);

    /* Buffers */
    for (size_t i = 0; i < gltf->num_buffers; ++i) {
        struct gltf_buffer* b = &gltf->buffers[i];
        free((void*)b->name);
        free((void*)b->uri);
    }
    free(gltf->buffers);

    /* Buffer views */
    for (size_t i = 0; i < gltf->num_buffer_views; ++i) {
        struct gltf_buffer_view* v = &gltf->buffer_views[i];
        free((void*)v->name);
    }
    free(gltf->buffer_views);

    /* Cameras */
    for (size_t i = 0; i < gltf->num_cameras; ++i) {
        struct gltf_camera* c = &gltf->cameras[i];
        free((void*)c->name);
        free((void*)c->type);
        free(c->orthographic);
        free(c->perspective);
    }
    free(gltf->cameras);

    /* Images */
    for (size_t i = 0; i < gltf->num_images; ++i) {
        struct gltf_image* im = &gltf->images[i];
        free((void*)im->name);
        free((void*)im->uri);
        free((void*)im->mime_type);
    }
    free(gltf->images);

    /* Materials */
    for (size_t i = 0; i < gltf->num_materials; ++i) {
        struct gltf_material* m = &gltf->materials[i];
        free((void*)m->name);
        if (m->pbr_metallic_roughness) {
            struct gltf_material_pbr_metallic_roughness* mr = m->pbr_metallic_roughness;
            free(mr->base_color_texture);
            free(mr->metallic_roughness_texture);
            free(m->pbr_metallic_roughness);
        }
        free(m->normal_texture);
        free(m->occlusion_texture);
        free(m->emissive_texture);
        free((void*)m->alpha_mode);
    }
    free(gltf->materials);

    /* Meshes */
    for (size_t i = 0; i < gltf->num_meshes; ++i) {
        struct gltf_mesh* m = &gltf->meshes[i];
        free((void*)m->name);
        for (size_t j = 0; j < m->num_primitives; ++j) {
            struct gltf_mesh_primitive* p = &m->primitives[j];
            for (size_t k = 0; k < p->num_attributes; ++k)
                free((void*)p->attributes[k].semantic);
            free(p->attributes);
            for (size_t k = 0; k < p->num_targets; ++k)
                for (size_t l = 0; l < 3; ++l)
                    free((void*)p->targets[k][l].semantic);
            free(p->targets);
        }
        free(m->primitives);
        free(m->weights);
    }
    free(gltf->meshes);

    /* Nodes */
    for (size_t i = 0; i < gltf->num_nodes; ++i) {
        struct gltf_node* n = &gltf->nodes[i];
        free((void*)n->name);
        free(n->children);
        free(n->weights);
    }
    free(gltf->nodes);

    /* Samplers */
    for (size_t i = 0; i < gltf->num_samplers; ++i) {
        struct gltf_sampler* s = &gltf->samplers[i];
        free((void*)s->name);
    }
    free(gltf->samplers);

    /* Scenes */
    for (size_t i = 0; i < gltf->num_scenes; ++i) {
        struct gltf_scene* s = &gltf->scenes[i];
        free((void*)s->name);
        free(s->nodes);
    }
    free(gltf->scenes);

    /* Skins */
    for (size_t i = 0; i < gltf->num_skins; ++i) {
        struct gltf_skin* s = &gltf->skins[i];
        free((void*)s->name);
        free(s->joints);
    }
    free(gltf->skins);

    /* Textures */
    for (size_t i = 0; i < gltf->num_textures; ++i) {
        struct gltf_texture* t = &gltf->textures[i];
        free((void*)t->name);
    }
    free(gltf->textures);

    /* Extensions */
    for (size_t i = 0; i < gltf->num_extensions_used; ++i)
        free((void*)gltf->extensions_used[i]);
    free(gltf->extensions_used);
    for (size_t i = 0; i < gltf->num_extensions_required; ++i)
        free((void*)gltf->extensions_required[i]);
    free(gltf->extensions_required);
    free(gltf);
}

/*---------------------------------------------------------------------------
 * Data population
 *---------------------------------------------------------------------------*/
static int startswith(const char* pre, const char* str)
{
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? 0 : strncmp(pre, str, lenpre) == 0;
}

static enum texture_filter filter_min_map(int f)
{
    static const struct { int g; enum texture_filter e;} m[] = {
        { GLTF_LINEAR,                 TEXTURE_FILTER_LINEAR                 },
        { GLTF_NEAREST,                TEXTURE_FILTER_NEAREST                },
        { GLTF_LINEAR_MIPMAP_LINEAR,   TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR   },
        { GLTF_LINEAR_MIPMAP_NEAREST,  TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST  },
        { GLTF_NEAREST_MIPMAP_LINEAR,  TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR  },
        { GLTF_NEAREST_MIPMAP_NEAREST, TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST },
    };
    for (size_t i = 0; i < sizeof(m)/sizeof(m[0]); ++i)
        if (m[i].g == f)
            return m[i].e;
    return TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR;
}

static enum texture_filter filter_mag_map(int f)
{
    static const struct { int g; enum texture_filter e;} m[] = {
        { GLTF_LINEAR,  TEXTURE_FILTER_LINEAR  },
        { GLTF_NEAREST, TEXTURE_FILTER_NEAREST },
    };
    for (size_t i = 0; i < sizeof(m)/sizeof(m[0]); ++i)
        if (m[i].g == f)
            return m[i].e;
    return TEXTURE_FILTER_LINEAR;
}

static enum texture_wrap wrap_map(int f)
{
    static const struct { int g; enum texture_wrap e;} m[] = {
        { GLTF_REPEAT,          TEXTURE_WRAP_REPEAT },
        { GLTF_CLAMP_TO_EDGE,   TEXTURE_WRAP_CLAMP  },
        { GLTF_MIRRORED_REPEAT, TEXTURE_WRAP_MIRROR },
    };
    for (size_t i = 0; i < sizeof(m)/sizeof(m[0]); ++i)
        if (m[i].g == f)
            return m[i].e;
    return TEXTURE_WRAP_REPEAT;
}

static int gltf_accessor_num_components(const char* type)
{
    static const struct {const char* t; int nc;} m[] = {
        { "SCALAR", 1 },
        { "VEC2",   2 },
        { "VEC3",   3 },
        { "VEC4",   4 },
        { "MAT2",   4 },
        { "MAT3",   9 },
        { "MAT4",  16 },
    };
    for (size_t i = 0; i < sizeof(m)/sizeof(m[0]); ++i)
        if (strcmp(m[i].t, type) == 0)
            return m[i].nc;
    assert(0);
    return 0;
}

static size_t gltf_accessor_ctype_size(int ctype)
{
    static const struct {int ct; size_t sz;} m[] = {
        { GLTF_BYTE,           1 },
        { GLTF_UNSIGNED_BYTE,  1 },
        { GLTF_SHORT,          2 },
        { GLTF_UNSIGNED_SHORT, 2 },
        { GLTF_UNSIGNED_INT,   4 },
        { GLTF_FLOAT,          4 },
    };
    for (size_t i = 0; i < sizeof(m)/sizeof(m[0]); ++i)
        if (m[i].ct == ctype)
            return m[i].sz;
    assert(0);
    return 0;
}


/* A view for gltf array buffers that allows for typed access */
struct gltf_accessor_view {
    /* Number of elements in the view */
    size_t size;
    /* Number of components per element */
    size_t ncomp;
    /* Component type */
    int ctype;
    /* Component type size */
    size_t ctype_size;
    /* Whether to normalize output data */
    int normalize;
    /* Data stride */
    size_t stride;
    /* Data ptr */
    void* data;
};

static void gltf_accessor_view_init(struct gltf_accessor_view* accessor_view, const struct gltf* gltf, struct gltf_accessor* accessor)
{
    accessor_view->size = accessor->count;
    accessor_view->ncomp = gltf_accessor_num_components(accessor->type);
    accessor_view->ctype = accessor->component_type;
    accessor_view->normalize = accessor->normalized;
    struct gltf_buffer_view* buffer_view = &gltf->buffer_views[accessor->buffer_view];
    accessor_view->ctype_size = gltf_accessor_ctype_size(accessor_view->ctype);
    accessor_view->stride = (buffer_view->byte_stride) ? (size_t)buffer_view->byte_stride : (accessor_view->ctype_size * accessor_view->ncomp);
    //struct gltf_buffer* buffer = &gltf->buffers[buffer_view->buffer];
    void* bdata = 0; // TODO!
    accessor_view->data = bdata + accessor->byte_offset + buffer_view->byte_offset;
}

static float gltf_accessor_view_get(struct gltf_accessor_view* av, int idx, int c)
{
    size_t i = fmin(fmax(c, 0), av->ncomp - 1);
    void* valb = av->data + av->stride * idx + i * av->ctype_size;
    if (!av->normalize) {
        switch (av->ctype) {
            case GLTF_FLOAT:          return (float)(*(float*)valb);
            case GLTF_BYTE:           return (float)(*(char*)valb);
            case GLTF_UNSIGNED_BYTE:  return (float)(*(unsigned char*)valb);
            case GLTF_SHORT:          return (float)(*(short*)valb);
            case GLTF_UNSIGNED_SHORT: return (float)(*(unsigned short*)valb);
            case GLTF_UNSIGNED_INT:   return (float)(*(unsigned int*)valb);
            default: assert(0); break;
        }
    } else {
        switch (av->ctype) {
            case GLTF_FLOAT:          return (float)(*(float*)valb);
            case GLTF_BYTE:           return (float)(fmax((float)(c / 127.0), -1.0f));
            case GLTF_UNSIGNED_BYTE:  return (float)(c / 255.0);
            case GLTF_SHORT:          return (float)(fmax((float)(c / 32767.0), -1.0f));
            case GLTF_UNSIGNED_SHORT: return (float)(c / 65535.0);
            case GLTF_UNSIGNED_INT:   return (float)(fmax((float)(c / 2147483647.0), -1.0f));
            default: assert(0); break;
        }
    }
    return 0;
}

static int gltf_accessor_view_geti(struct gltf_accessor_view* av, int idx, int c)
{
    size_t i = fmin(fmax(c, 0), av->ncomp - 1);
    void* valb = av->data + av->stride * idx + i * av->ctype_size;
    switch (av->ctype) {
        case GLTF_FLOAT:          return (int)(*(float*)valb);
        case GLTF_BYTE:           return (int)(*(char*)valb);
        case GLTF_UNSIGNED_BYTE:  return (int)(*(unsigned char*)valb);
        case GLTF_SHORT:          return (int)(*(short*)valb);
        case GLTF_UNSIGNED_SHORT: return (int)(*(unsigned short*)valb);
        case GLTF_UNSIGNED_INT:   return (int)(*(unsigned int*)valb);
        default: assert(0); break;
    }
    return 0;
}

static vec2f gltf_accessor_view_getv2f(struct gltf_accessor_view* av, int idx)
{
    vec2f v = {0.0, 0.0};
    for (int i = 0; i < fmin(av->ncomp, 2); ++i)
        ((float*)&v)[i] = gltf_accessor_view_get(av, idx, i);
    return v;
}

static vec3f gltf_accessor_view_getv3f(struct gltf_accessor_view* av, int idx)
{
    vec3f v = {0.0, 0.0, 0.0};
    for (int i = 0; i < fmin(av->ncomp, 3); ++i)
        ((float*)&v)[i] = gltf_accessor_view_get(av, idx, i);
    return v;
}

static vec4f gltf_accessor_view_getv4f(struct gltf_accessor_view* av, int idx)
{
    vec4f v = {0.0, 0.0, 0.0, 0.0};
    for (int i = 0; i < fmin(av->ncomp, 4); ++i)
        ((float*)&v)[i] = gltf_accessor_view_get(av, idx, i);
    return v;
}

static vec4i gltf_accessor_view_getv4i(struct gltf_accessor_view* av, int idx)
{
    vec4i v = {0.0, 0.0, 0.0, 0.0};
    for (int i = 0; i < fmin(av->ncomp, 4); ++i)
        ((int*)&v)[i] = gltf_accessor_view_geti(av, idx, i);
    return v;
}

static void scene_add_texture_helper(const struct gltf* gltf,
                                     struct scene* scn,
                                     struct gltf_texture_info* ginfo,
                                     struct texture** txt,
                                     struct texture_info* info,
                                     int normal,   /* = 0 */
                                     int occlusion /* = 0 */) {
    struct gltf_texture* gtxt = &gltf->textures[ginfo->index];
    if (!gtxt)
        return;
    txt = (!gtxt->source) ? 0 : &scn->textures[gtxt->source];
    if (!txt)
        return;
    texture_info_default(info);
    struct gltf_sampler* gsmp = &gltf->samplers[gtxt->sampler];
    if (gsmp) {
        info->filter_mag = filter_mag_map(gsmp->mag_filter);
        info->filter_min = filter_min_map(gsmp->min_filter);
        info->wrap_s = wrap_map(gsmp->wrap_s);
        info->wrap_t = wrap_map(gsmp->wrap_t);
    }
    if (normal) {
        struct gltf_material_normal_texture_info* ninfo = (struct gltf_material_normal_texture_info*)ginfo;
        info->scale = ninfo->scale;
    }
    if (occlusion) {
        struct gltf_material_occlusion_texture_info* ninfo = (struct gltf_material_occlusion_texture_info*)ginfo;
        info->scale = ninfo->strength;
    }
};

/* Flattens a gltf file into a flattened asset. */
struct scene* gltf_to_scene(const struct gltf* gltf)
{
    /* Clear asset */
    struct scene* scn = calloc(1, sizeof(*scn));
    scene_init(scn);

    /* Convert images */
    for (size_t k = 0; k < gltf->num_images; ++k) {
        struct gltf_image* gtxt = &gltf->images[k];
        struct texture* txt = calloc(1, sizeof(*txt));
        texture_init(txt);
        txt->name = gtxt->name ? strdup(gtxt->name) : 0;
        txt->path = strdup(startswith(gtxt->uri, "data:") ? "inlines" : gtxt->uri);
        // TODO get data from gtxt
        void* datab = 0, *dataf = 0;
        int imw = 0, imh = 0, ncomp = 0;
        if (!datab) {
            txt->ldr = (image4b){.w = imw, .h = imh, .pixels = calloc(imw * imh, sizeof(txt->ldr.pixels))};
            for (int j = 0; j < txt->ldr.h; j++) {
                for (int i = 0; i < txt->ldr.w; ++i) {
                    unsigned char* v = datab + (imw * j + i) * ncomp;
                    switch (ncomp) {
                        case 1:
                            memcpy(&(txt->ldr.pixels[j * txt->ldr.w + i]), (unsigned char[]){v[0], v[0], v[0], 255}, sizeof(*txt->ldr.pixels));
                            break;
                        case 2:
                            memcpy(&(txt->ldr.pixels[j * txt->ldr.w + i]), (unsigned char[]){v[0], v[1], 0, 255}, sizeof(*txt->ldr.pixels));
                            break;
                        case 3:
                            memcpy(&(txt->ldr.pixels[j * txt->ldr.w + i]), (unsigned char[]){v[0], v[1], v[2], 255}, sizeof(*txt->ldr.pixels));
                            break;
                        case 4:
                            memcpy(&(txt->ldr.pixels[j * txt->ldr.w + i]), (unsigned char[]){v[0], v[1], v[2], v[3]}, sizeof(*txt->ldr.pixels));
                            break;
                        default:
                            assert(0);
                            break;
                    }
                }
            }
        } else if (!dataf) {
            txt->hdr = (image4f){.w = imw, .h = imh, .pixels = calloc(imw * imh, sizeof(txt->hdr.pixels))};
            for (int j = 0; j < txt->hdr.h; j++) {
                for (int i = 0; i < txt->hdr.w; ++i) {
                    float* v = dataf + (imw * j + i) * ncomp;
                    switch (ncomp) {
                        case 1:
                            memcpy(&(txt->hdr.pixels[j * txt->hdr.w + i]), (float[]){v[0], v[0], v[0], 1}, sizeof(*txt->hdr.pixels));
                            break;
                        case 2:
                            memcpy(&(txt->hdr.pixels[j * txt->hdr.w + i]), (float[]){v[0], v[1], 0, 1}, sizeof(*txt->hdr.pixels));
                            break;
                        case 3:
                            memcpy(&(txt->hdr.pixels[j * txt->hdr.w + i]), (float[]){v[0], v[1], v[2], 1}, sizeof(*txt->hdr.pixels));
                            break;
                        case 4:
                            memcpy(&(txt->hdr.pixels[j * txt->hdr.w + i]), (float[]){v[0], v[1], v[2], v[3]}, sizeof(*txt->hdr.pixels));
                            break;
                        default:
                            assert(0);
                            break;
                    }
                }
            }
        }
        ++scn->num_textures;
        scn->textures = realloc(scn->textures, scn->num_textures * sizeof(*scn->textures));
        scn->textures[scn->num_textures - 1] = txt;
    }

    /* Convert Materials */
    for (size_t i = 0; i < gltf->num_materials; ++i) {
        struct gltf_material* gmat = &gltf->materials[i];
        struct material* mat = calloc(1, sizeof(*mat));
        material_init(mat);
        mat->name = strdup(gmat->name);
        memcpy(&mat->ke, gmat->emissive_factor, sizeof(mat->ke));
        if (gmat->emissive_texture) {
            mat->ke_txt_info = calloc(1, sizeof(*mat->ke_txt_info));
            scene_add_texture_helper(gltf, scn, gmat->emissive_texture, &mat->ke_txt, mat->ke_txt_info, 0, 0);
        }
        if (gmat->pbr_metallic_roughness) {
            struct gltf_material_pbr_metallic_roughness* gmr = gmat->pbr_metallic_roughness;
            mat->type = MATERIAL_TYPE_METALLIC_ROUGHNESS;
            mat->kd = *(vec3f*)&gmr->base_color_factor;
            mat->op = gmr->base_color_factor[3];
            mat->ks = (vec3f){gmr->metallic_factor, gmr->metallic_factor, gmr->metallic_factor};
            mat->rs = gmr->roughness_factor;
            if (gmr->base_color_texture)
                scene_add_texture_helper(gltf, scn, gmr->base_color_texture, &mat->kd_txt, mat->kd_txt_info, 0, 0);
            if (gmr->metallic_roughness_texture)
                scene_add_texture_helper(gltf, scn, gmr->metallic_roughness_texture, &mat->ks_txt, mat->ks_txt_info, 0, 0);
        }
        if (gmat->normal_texture) {
            mat->norm_txt_info = calloc(1, sizeof(*mat->norm_txt_info));
            scene_add_texture_helper(gltf, scn, (struct gltf_texture_info*)gmat->normal_texture, &mat->norm_txt, mat->norm_txt_info, 1, 0);
        }
        if (gmat->occlusion_texture) {
            mat->occ_txt_info = calloc(1, sizeof(*mat->occ_txt_info));
            scene_add_texture_helper(gltf, scn, (struct gltf_texture_info*)gmat->occlusion_texture, &mat->occ_txt, mat->occ_txt_info, 0, 1);
        }
        mat->double_sided = gmat->double_sided;
        ++scn->num_materials;
        scn->materials = realloc(scn->materials, scn->num_materials * sizeof(*scn->materials));
        scn->materials[scn->num_materials - 1] = mat;
    }

    /* Convert meshes */
    for (size_t k = 0; k < gltf->num_meshes; ++k) {
        struct gltf_mesh* gmesh = &gltf->meshes[k];
        struct mesh* msh = calloc(1, sizeof(*msh));
        _mesh_init(msh);
        msh->name = strdup(gmesh->name);
        /* Primitives */
        for (size_t l = 0; l < gmesh->num_primitives; ++l) {
            struct gltf_mesh_primitive* gprim = &gmesh->primitives[l];
            struct shape* prim = calloc(1, sizeof(*prim));
            shape_init(prim);
            if (gprim->material)
                prim->mat = scn->materials[gprim->material];
            /* Vertex data */
            for (size_t j = 0; j < gprim->num_attributes; ++j) {
                const char* semantic = gprim->attributes[j].semantic;
                struct gltf_accessor_view av;
                gltf_accessor_view_init(&av, gltf, &gltf->accessors[gprim->attributes[j].accessor_index]);
                if (strcmp(semantic, "POSITION") == 0) {
                    prim->num_pos = av.size;
                    prim->pos = calloc(prim->num_pos, sizeof(*prim->pos));
                    for (size_t i = 0; i < prim->num_pos; ++i)
                        prim->pos[i] = gltf_accessor_view_getv3f(&av, i);
                } else if (strcmp(semantic,"NORMAL") == 0) {
                    prim->num_norm = av.size;
                    prim->norm = calloc(prim->num_norm, sizeof(*prim->norm));
                    for (size_t i = 0; i < prim->num_norm; ++i)
                        prim->norm[i] = gltf_accessor_view_getv3f(&av, i);
                } else if ((strcmp(semantic, "TEXCOORD") == 0) || (strcmp(semantic, "TEXCOORD_0") == 0)) {
                    prim->num_texcoord = av.size;
                    prim->texcoord = calloc(prim->num_texcoord, sizeof(*prim->texcoord));
                    for (size_t i = 0; i < prim->num_texcoord; ++i)
                        prim->texcoord[i] = gltf_accessor_view_getv2f(&av, i);
                } else if (strcmp(semantic, "TEXCOORD_1") == 0) {
                    prim->num_texcoord1 = av.size;
                    prim->texcoord1 = calloc(prim->num_texcoord1, sizeof(*prim->texcoord1));
                    for (size_t i = 0; i < prim->num_texcoord1; ++i)
                        prim->texcoord1[i] = gltf_accessor_view_getv2f(&av, i);
                } else if ((strcmp(semantic, "COLOR") == 0) || (strcmp(semantic, "COLOR_0") == 0)) {
                    prim->num_color = av.size;
                    prim->color = calloc(prim->num_color, sizeof(*prim->color));
                    for (size_t i = 0; i < prim->num_color; ++i)
                        prim->color[i] = gltf_accessor_view_getv4f(&av, i);
                } else if (strcmp(semantic, "TANGENT") == 0) {
                    prim->num_tangsp = av.size;
                    prim->tangsp = calloc(prim->num_tangsp, sizeof(*prim->tangsp));
                    for (size_t i = 0; i < prim->num_tangsp; ++i)
                        prim->tangsp[i] = gltf_accessor_view_getv4f(&av, i);
                } else if (strcmp(semantic, "WEIGHTS_0") == 0) {
                    prim->num_skin_weights = av.size;
                    prim->skin_weights = calloc(prim->num_skin_weights, sizeof(*prim->skin_weights));
                    for (size_t i = 0; i < prim->num_skin_weights; ++i)
                        prim->skin_weights[i] = gltf_accessor_view_getv4f(&av, i);
                } else if (strcmp(semantic, "JOINTS_0") == 0) {
                    prim->num_skin_joints = av.size;
                    prim->skin_joints = calloc(prim->num_skin_joints, sizeof(*prim->skin_joints));
                    for (size_t i = 0; i < prim->num_skin_joints; ++i)
                        prim->skin_joints[i] = gltf_accessor_view_getv4i(&av, i);
                } else if (strcmp(semantic, "RADIUS") == 0) {
                    prim->num_radius = av.size;
                    prim->radius = calloc(prim->num_radius, sizeof(*prim->radius));
                    for (size_t i = 0; i < prim->num_radius; ++i)
                        prim->radius[i] = gltf_accessor_view_get(&av, i, 0);
                } else {
                    /* Ignore */
                }
            }
            /* Indices */
            if (!gprim->indices) {
                switch (gprim->mode) {
                    case GLTF_TRIANGLES: {
                        prim->num_triangles = prim->num_pos / 3;
                        prim->triangles = calloc(prim->num_triangles, sizeof(*prim->triangles));
                        for (size_t i = 0; i < prim->num_triangles; ++i)
                            prim->triangles[i] = (vec3i){i * 3 + 0, i * 3 + 1, i * 3 + 2};
                    } break;
                    case GLTF_TRIANGLE_FAN: {
                        prim->num_triangles = prim->num_pos - 2;
                        prim->triangles = calloc(prim->num_triangles, sizeof(*prim->triangles));
                        for (size_t i = 2; i < prim->num_pos; ++i)
                            prim->triangles[i] = (vec3i){0, i - 1, i};
                    } break;
                    case GLTF_TRIANGLE_STRIP: {
                        prim->num_triangles = prim->num_pos - 2;
                        prim->triangles = calloc(prim->num_triangles, sizeof(*prim->triangles));
                        for (size_t i = 2; i < prim->num_pos; ++i)
                            prim->triangles[i] = (vec3i){i - 2, i - 1, i};
                    } break;
                    case GLTF_LINES: {
                        prim->num_lines = prim->num_pos / 2;
                        prim->lines = calloc(prim->num_lines, sizeof(*prim->lines));
                        for (size_t i = 0; i < prim->num_pos / 2; ++i)
                            prim->lines[i] = (vec2i){i * 2 + 0, i * 2 + 1};
                    } break;
                    case GLTF_LINE_LOOP: {
                        prim->num_lines = prim->num_pos;
                        prim->lines = calloc(prim->num_lines, sizeof(*prim->lines));
                        for (size_t i = 1; i < prim->num_pos; ++i)
                            prim->lines[i] = (vec2i){i - 1, i};
                        prim->lines[prim->num_lines - 1] = (vec2i){(int)prim->num_pos - 1, 0};
                    } break;
                    case GLTF_LINE_STRIP: {
                        prim->num_lines = prim->num_pos - 1;
                        prim->lines = calloc(prim->num_lines, sizeof(*prim->lines));
                        for (size_t i = 1; i < prim->num_pos; ++i)
                            prim->lines[i] = (vec2i){i - 1, i};
                    } break;
                    case GLTF_POINTS: {
                        prim->num_points = prim->num_pos;
                        prim->points = calloc(prim->num_points, sizeof(*prim->points));
                        for (size_t i = 0; i < prim->num_pos; ++i)
                            prim->points[i] = i;
                    } break;
                }
            }  else {
                struct gltf_accessor_view av;
                gltf_accessor_view_init(&av, gltf, &gltf->accessors[gprim->indices]);
                switch (gprim->mode) {
                    case GLTF_TRIANGLES: {
                        prim->num_triangles = av.size;
                        prim->triangles = calloc(prim->num_triangles, sizeof(*prim->triangles));
                        for (size_t i = 0; i < av.size / 3; ++i) {
                            prim->triangles[i] = (vec3i){
                                gltf_accessor_view_geti(&av, i * 3 + 0, 0),
                                gltf_accessor_view_geti(&av, i * 3 + 1, 0),
                                gltf_accessor_view_geti(&av, i * 3 + 2, 0)};
                        }
                    } break;
                    case GLTF_TRIANGLE_FAN: {
                        prim->num_triangles = av.size - 2;
                        prim->triangles = calloc(prim->num_triangles, sizeof(*prim->triangles));
                        for (size_t i = 2; i < av.size; ++i) {
                            prim->triangles[i] = (vec3i){
                                gltf_accessor_view_geti(&av, 0, 0),
                                gltf_accessor_view_geti(&av, i - 1, 0),
                                gltf_accessor_view_geti(&av, i, 0)};
                        }
                    } break;
                    case GLTF_TRIANGLE_STRIP: {
                        prim->num_triangles = av.size - 2;
                        prim->triangles = calloc(prim->num_triangles, sizeof(*prim->triangles));
                        for (size_t i = 2; i < av.size; ++i) {
                            prim->triangles[i] = (vec3i){
                                gltf_accessor_view_geti(&av, i - 2, 0),
                                gltf_accessor_view_geti(&av, i - 1, 0),
                                gltf_accessor_view_geti(&av, i, 0)};
                        }
                    } break;
                    case GLTF_LINES: {
                        prim->num_lines = av.size / 2;
                        prim->lines = calloc(prim->num_lines, sizeof(*prim->lines));
                        for (size_t i = 0; i < av.size / 2; ++i) {
                            prim->lines[i] = (vec2i){
                                gltf_accessor_view_geti(&av, i * 2 + 0, 0),
                                gltf_accessor_view_geti(&av, i * 2 + 1, 0)};
                        }
                    } break;
                    case GLTF_LINE_LOOP: {
                        prim->num_lines = av.size;
                        prim->lines = calloc(prim->num_lines, sizeof(*prim->lines));
                        for (size_t i = 1; i < av.size; ++i) {
                            prim->lines[i] = (vec2i){
                                gltf_accessor_view_geti(&av, i - 1, 0),
                                gltf_accessor_view_geti(&av, i, 0)};
                        }
                        prim->lines[prim->num_lines - 1] = (vec2i){
                            gltf_accessor_view_geti(&av, av.size - 1, 0),
                            gltf_accessor_view_geti(&av, 0, 0)};
                    } break;
                    case GLTF_LINE_STRIP: {
                        prim->num_lines = av.size - 1;
                        prim->lines = calloc(prim->num_lines, sizeof(*prim->lines));
                        for (size_t i = 1; i < av.size; ++i) {
                            prim->lines[i] = (vec2i){
                                gltf_accessor_view_geti(&av, i - 1, 0),
                                gltf_accessor_view_geti(&av, i, 0)};
                        }
                    } break;
                    case GLTF_POINTS: {
                        prim->num_points = av.size;
                        prim->points = calloc(prim->num_points, sizeof(*prim->points));
                        for (size_t i = 0; i < av.size; ++i)
                            prim->points[i] = gltf_accessor_view_geti(&av, i, 0);
                    } break;
                }
            }

            /*
            // Morph targets
            int target_index = 0;
            for (auto& gtarget : gprim->targets) {
                auto target = new gltf_shape_morph();
                for (auto gattr : gtarget) {
                    auto semantic = gattr.first;
                    auto vals = accessor_view(gltf, gltf->get(gattr.second));
                    if (semantic == "POSITION") {
                        target->pos.reserve(vals.size());
                        for (auto i = 0; i < vals.size(); i++)
                            target->pos.push_back(vals.getv3f(i));
                    } else if (semantic == "NORMAL") {
                        target->norm.reserve(vals.size());
                        for (auto i = 0; i < vals.size(); i++)
                            target->norm.push_back(vals.getv3f(i));
                    } else if (semantic == "TANGENT") {
                        target->tangsp.reserve(vals.size());
                        for (auto i = 0; i < vals.size(); i++)
                            target->tangsp.push_back(vals.getv3f(i));
                    } else {
                        // ignore
                    }
                }
                if (target_index < (int)gmesh->weights.size() - 1)
                    target->weight = gmesh->weights[target_index];
                target_index++;
                prim->morph_targets.push_back(target);
            }
            */
            ++msh->num_shapes;
            msh->shapes = realloc(msh->shapes, msh->num_shapes * sizeof(*msh->shapes));
            msh->shapes[msh->num_shapes - 1] = prim;
        }
        ++scn->num_meshes;
        scn->meshes = realloc(scn->meshes, scn->num_meshes * sizeof(*scn->meshes));
        scn->meshes[scn->num_meshes - 1] = msh;
    }

    /* Convert cameras */
    for (size_t i = 0; i < gltf->num_cameras; ++i) {
        struct gltf_camera* gcam = &gltf->cameras[i];
        struct camera* cam = calloc(1, sizeof(*cam));
        cam->name = strdup(gcam->name);
        cam->ortho = strcmp(gcam->type, "orthographic") == 0;
        if (cam->ortho) {
            struct gltf_camera_orthographic* ortho = gcam->orthographic;
            cam->yfov = ortho->ymag;
            cam->aspect = ortho->xmag / ortho->ymag;
            cam->near = ortho->znear;
            cam->far = ortho->zfar;
        } else {
            struct gltf_camera_perspective* persp = gcam->perspective;
            cam->yfov = persp->yfov;
            cam->aspect = persp->aspect_ratio;
            if (!cam->aspect)
                cam->aspect = 16.0f / 9.0f;
            cam->near = persp->znear;
            cam->far = persp->zfar;
        }
        ++scn->num_cameras;
        scn->cameras = realloc(scn->cameras, scn->num_cameras * sizeof(*scn->cameras));
        scn->cameras[scn->num_cameras - 1] = cam;
    }

    /* Convert nodes */
    for (size_t i = 0; i < gltf->num_nodes; ++i) {
        struct gltf_node* gnode = &gltf->nodes[i];
        struct node* node = calloc(1, sizeof(*node));
        node->name = strdup(gnode->name);
        node->cam = (!gnode->camera) ? 0 : scn->cameras[gnode->camera];
        //TODO: Make mesh instance
        //node->msh = (!gnode->mesh) ? 0 : scn->meshes[gnode->mesh];
        node->translation = *(vec3f*)gnode->translation;
        node->rotation = *(quat4f*)gnode->rotation;
        node->scaling = *(vec3f*)gnode->scale;
        //TODO: Matrix to frame
        //node->matrix = *(mat4f*)gnode->matrix;
        ++scn->num_nodes;
        scn->nodes = realloc(scn->nodes, scn->num_nodes * sizeof(*scn->nodes));
        scn->nodes[scn->num_nodes - 1] = node;
    }

    /* Set up children pointers */
    for (size_t i = 0; i < gltf->num_nodes; ++i) {
        struct gltf_node* gnode = &gltf->nodes[i];
        struct node* node = scn->nodes[i];
        node->num_children = gnode->num_children;
        node->children = calloc(node->num_children, sizeof(*node->children));
        for (size_t j = 0; j < gnode->num_children; ++j)
            node->children[i] = scn->nodes[j];
    }

#if 0
    // Fix node morph weights
    for (auto node : scns->nodes) {
        if (!node->msh) continue;
        for (auto shp : node->msh->shapes) {
            if (node->morph_weights.size() < shp->morph_targets.size()) {
                node->morph_weights.resize(shp->morph_targets.size());
            }
        }
    }

    // Convert animations
    for (auto ganim : gltf->animations) {
        auto anim_group = new gltf_animation_group();
        anim_group->name = ganim->name;
        std::map<vec2i, gltf_animation*> sampler_map;
        for (auto gchannel : ganim->channels) {
            if (sampler_map.find({(int)gchannel->sampler,
                    (int)gchannel->target->path}) == sampler_map.end()) {
                auto gsampler = ganim->get(gchannel->sampler);
                auto keyframes = new gltf_animation();
                auto input_view =
                    accessor_view(gltf, gltf->get(gsampler->input));
                keyframes->time.resize(input_view.size());
                for (auto i = 0; i < input_view.size(); i++)
                    keyframes->time[i] = input_view.get(i);
                keyframes->interp =
                    (gltf_animation_interpolation)gsampler->interpolation;
                auto output_view =
                    accessor_view(gltf, gltf->get(gsampler->output));
                switch (gchannel->target->path) {
                    case glTFAnimationChannelTargetPath::Translation: {
                        keyframes->translation.reserve(output_view.size());
                        for (auto i = 0; i < output_view.size(); i++)
                            keyframes->translation.push_back(
                                output_view.getv3f(i));
                    } break;
                    case glTFAnimationChannelTargetPath::Rotation: {
                        keyframes->rotation.reserve(output_view.size());
                        for (auto i = 0; i < output_view.size(); i++)
                            keyframes->rotation.push_back(
                                (quat4f)output_view.getv4f(i));
                    } break;
                    case glTFAnimationChannelTargetPath::Scale: {
                        keyframes->scale.reserve(output_view.size());
                        for (auto i = 0; i < output_view.size(); i++)
                            keyframes->scale.push_back(output_view.getv3f(i));
                    } break;
                    case glTFAnimationChannelTargetPath::Weights: {
                        // get a node that it refers to
                        auto ncomp = 0;
                        auto gnode = gltf->get(gchannel->target->node);
                        auto gmesh = gltf->get(gnode->mesh);
                        if (gmesh) {
                            for (auto gshp : gmesh->primitives) {
                                ncomp = max((int)gshp->targets.size(), ncomp);
                            }
                        }
                        if (ncomp) {
                            auto values = std::vector<float>();
                            values.reserve(output_view.size());
                            for (auto i = 0; i < output_view.size(); i++)
                                values.push_back(output_view.get(i));
                            keyframes->morph_weights.resize(
                                values.size() / ncomp);
                            for (auto i = 0;
                                 i < keyframes->morph_weights.size(); i++) {
                                keyframes->morph_weights[i].resize(ncomp);
                                for (auto j = 0; j < ncomp; j++)
                                    keyframes->morph_weights[i][j] =
                                        values[i * ncomp + j];
                            }
                        }
                    } break;
                    default: {
                        // skip
                    }
                }
                sampler_map[{(int)gchannel->sampler,
                    (int)gchannel->target->path}] = keyframes;
                anim_group->animations.push_back(keyframes);
            }
            sampler_map
                .at({(int)gchannel->sampler, (int)gchannel->target->path})
                ->nodes.push_back(scns->nodes[(int)gchannel->target->node]);
        }
        scns->animations.push_back(anim_group);
    }

    // Convert skins
    for (auto gskin : gltf->skins) {
        auto skin = new gltf_skin();
        skin->name = gskin->name;
        for (auto gnode : gskin->joints)
            skin->joints.push_back(scns->nodes[(int)gnode]);
        skin->root = scns->nodes[(int)gskin->skeleton];
        if (!gskin->inverseBindMatrices) {
            skin->pose_matrices.assign(skin->joints.size(), identity_mat4f);
        } else {
            auto pose_matrix_view =
                accessor_view(gltf, gltf->get(gskin->inverseBindMatrices));
            skin->pose_matrices.resize(skin->joints.size());
            assert(pose_matrix_view.size() == skin->joints.size());
            assert(pose_matrix_view.ncomp() == 16);
            for (auto i = 0; i < pose_matrix_view.size(); i++) {
                skin->pose_matrices[i] = pose_matrix_view.getm4f(i);
            }
        }
        scns->skins.push_back(skin);
    }

    // Set skin pointers
    for (auto nid = 0; nid < gltf->nodes.size(); nid++) {
        if (!gltf->nodes[nid]->skin) continue;
        scns->nodes[nid]->skn = scns->skins[(int)gltf->nodes[nid]->skin];
    }

#endif
    /* Update transforms */
    //scene_update_transforms(scn);
    return scn;
}
