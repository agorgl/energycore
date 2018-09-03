#include "asset_data.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <prof.h>
#include <stb_image.h>
#include <assets/assetload.h>
#include <assets/model/postprocess.h>

#define timed_load(fname) \
    for (timepoint_t _start_ms = millisecs(), _break = (printf("Loading: %-85s", fname), 1); \
         _break; _break = 0, printf("[ %3lu ms ]\n", (unsigned long)(millisecs() - _start_ms)))

void model_data_from_file(struct model_data* mdl, const char* filepath)
{
    struct model* m = 0;
    timed_load(filepath) {
        m = model_from_file(filepath);
        model_generate_tangents(m);
    }

    mdl->num_meshes = m->num_meshes;
    mdl->meshes = calloc(mdl->num_meshes, sizeof(*mdl->meshes));
    for (size_t i = 0; i < mdl->num_meshes; ++i) {
        struct mesh* mesh = m->meshes[i];
        struct mesh_data* md = mdl->meshes + i;
        md->num_verts = mesh->num_verts;

        md->positions = calloc(md->num_verts, sizeof(*md->positions));
        md->normals   = calloc(md->num_verts, sizeof(*md->normals));
        md->texcoords = calloc(md->num_verts, sizeof(*md->texcoords));
        md->tangents  = calloc(md->num_verts, sizeof(*md->tangents));
        for (size_t j = 0; j < mesh->num_verts; ++j) {
            memcpy(md->positions + j, (mesh->vertices + j)->position, sizeof(*md->positions));
            memcpy(md->normals + j,   (mesh->vertices + j)->normal,   sizeof(*md->normals));
            memcpy(md->texcoords + j, (mesh->vertices + j)->uvs,      sizeof(*md->texcoords));
            memcpy(md->tangents + j,  (mesh->vertices + j)->tangent,  sizeof(*md->tangents));
        }

        md->num_triangles = mesh->num_indices / 3;
        md->triangles = calloc(md->num_triangles, sizeof(*md->triangles));
        memcpy(md->triangles, mesh->indices, mesh->num_indices * sizeof(*mesh->indices));

        md->group_idx = mesh->mgroup_idx;
        md->mat_idx = mesh->mat_index;
    }

    mdl->num_group_names = m->num_mesh_groups;
    mdl->group_names = calloc(mdl->num_group_names, sizeof(*mdl->group_names));
    for (size_t i = 0; i < mdl->num_group_names; ++i)
        mdl->group_names[i] = strdup(m->mesh_groups[i]->name);

    model_delete(m);
}

void model_data_free(struct model_data* mdl)
{
    for (size_t i = 0; i < mdl->num_meshes; ++i) {
        struct mesh_data* md = &mdl->meshes[i];
        free(md->positions);
        free(md->normals);
        free(md->texcoords);
        free(md->tangents);
        free(md->triangles);
    }
    free(mdl->meshes);

    for (size_t i = 0; i < mdl->num_group_names; ++i)
        free((void*)mdl->group_names[i]);
    free(mdl->group_names);
}

void image_data_from_file(struct image_data* img, const char* filepath)
{
    int width = 0, height = 0, channels = 0;
    void* data = 0;
    stbi_set_flip_vertically_on_load(1);
    timed_load(filepath)
        data = stbi_load(filepath, &width, &height, &channels, 0);
    img->data      = data;
    img->width     = width;
    img->height    = height;
    img->channels  = channels;
    img->bit_depth = 8;
    img->data_sz   = 0;
    img->flags.hdr = 0;
    img->flags.compressed = 0;
    img->compression_type = 0;
}
