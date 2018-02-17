#include "gpures.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <prof.h>
#include <glad/glad.h>
#include <assets/assetload.h>
#include <assets/model/postprocess.h>
#include <energycore/asset.h>

static void mesh_calc_aabb(struct mesh* m, float min[3], float max[3])
{
    memset(min, 0, 3 * sizeof(float));
    memset(max, 0, 3 * sizeof(float));
    for (size_t i = 0; i < m->num_verts; ++i) {
        struct vertex* v = m->vertices + i;
        for (unsigned int j = 0; j < 3; ++j) {
            if (v->position[j] < min[j])
                min[j] = v->position[j];
            else if (v->position[j] > max[j])
                max[j] = v->position[j];
        }
    }
}

struct model_hndl* model_to_gpu(struct model* m)
{
    struct model_hndl* model = calloc(1, sizeof(struct model_hndl));
    /* Allocate handle memory */
    model->num_meshes = m->num_meshes;
    model->meshes = malloc(m->num_meshes * sizeof(struct mesh_hndl));
    memset(model->meshes, 0, model->num_meshes * sizeof(struct mesh_hndl));

    unsigned int total_verts = 0;
    unsigned int total_indices = 0;
    for (unsigned int i = 0; i < model->num_meshes; ++i) {
        struct mesh* mesh = m->meshes[i];
        struct mesh_hndl* mh = model->meshes + i;
        mh->mat_idx = mesh->mat_index;
        mh->mgroup_idx = mesh->mgroup_idx;

        /* Create vao */
        glGenVertexArrays(1, &mh->vao);
        glBindVertexArray(mh->vao);

        /* Create vertex data vbo */
        size_t vsz = \
            3 * sizeof(float)  /* Position */
          + 2 * sizeof(float)  /* UV */
          + 3 * sizeof(float)  /* Normal */
          + 3 * sizeof(float)  /* Tangent */
          + 3 * sizeof(float); /* Bitangent */
        glGenBuffers(1, &mh->vbo);
        glBindBuffer(GL_ARRAY_BUFFER, mh->vbo);
        glBufferData(GL_ARRAY_BUFFER, vsz * mesh->num_verts, 0, GL_STATIC_DRAW);

        void* vbuf = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        for (size_t j = 0; j < mesh->num_verts; ++j) {
            struct vertex* v = mesh->vertices + j;
            memcpy(vbuf, v->position, 3 * sizeof(float));
            vbuf += 3 * sizeof(float);
        }
        for (size_t j = 0; j < mesh->num_verts; ++j) {
            struct vertex* v = mesh->vertices + j;
            memcpy(vbuf, v->uvs, 2 * sizeof(float));
            vbuf += 2 * sizeof(float);
        }
        for (size_t j = 0; j < mesh->num_verts; ++j) {
            struct vertex* v = mesh->vertices + j;
            memcpy(vbuf, v->normal, 3 * sizeof(float));
            vbuf += 3 * sizeof(float);
        }
        for (size_t j = 0; j < mesh->num_verts; ++j) {
            struct vertex* v = mesh->vertices + j;
            memcpy(vbuf, v->tangent, 3 * sizeof(float));
            vbuf += 3 * sizeof(float);
        }
        for (size_t j = 0; j < mesh->num_verts; ++j) {
            struct vertex* v = mesh->vertices + j;
            memcpy(vbuf, v->binormal, 3 * sizeof(float));
            vbuf += 3 * sizeof(float);
        }
        glUnmapBuffer(GL_ARRAY_BUFFER);

        GLvoid* offset = 0;
        GLuint pos_attrib = 0;
        glEnableVertexAttribArray(pos_attrib);
        glVertexAttribPointer(pos_attrib, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), offset);
        offset += mesh->num_verts * 3 * sizeof(float);

        GLuint uv_attrib = 1;
        glEnableVertexAttribArray(uv_attrib);
        glVertexAttribPointer(uv_attrib, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), offset);
        offset += mesh->num_verts * 2 * sizeof(float);

        GLuint nm_attrib = 2;
        glEnableVertexAttribArray(nm_attrib);
        glVertexAttribPointer(nm_attrib, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), offset);
        offset += mesh->num_verts * 3 * sizeof(float);

        GLuint tn_attrib = 3;
        glEnableVertexAttribArray(tn_attrib);
        glVertexAttribPointer(tn_attrib, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), offset);
        offset += mesh->num_verts * 3 * sizeof(float);

        GLuint btn_attrib = 4;
        glEnableVertexAttribArray(btn_attrib);
        glVertexAttribPointer(btn_attrib, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), offset);

        /* Create indice ebo */
        glGenBuffers(1, &mh->ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mh->ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                mesh->num_indices * sizeof(GLuint),
                mesh->indices,
                GL_STATIC_DRAW);
        mh->indice_count = mesh->num_indices;

        /* Compute local AABB */
        mesh_calc_aabb(mesh, mh->aabb_min, mh->aabb_max);

        total_verts += mesh->num_verts;
        total_indices += mesh->num_indices;
    }

    /* Move skeleton and frameset */
    model->skel = m->skeleton;
    model->fset = m->frameset;
    model->mesh_groups = m->mesh_groups;
    model->num_mesh_groups = m->num_mesh_groups;
    m->skeleton = 0;
    m->frameset = 0;
    m->mesh_groups = 0;

    return model;
}

#define TIMED_LOAD_BEGIN(fname) \
    do { \
        printf("Loading: %-85s", fname); \
        timepoint_t t1 = millisecs();

#define TIMED_LOAD_END \
        timepoint_t t2 = millisecs(); \
        printf("[ %3lu ms ]\n", (unsigned long)(t2 - t1)); \
    } while (0);

struct model_hndl* model_from_file_to_gpu(const char* filename)
{
    /* Load and parse model file */
    struct model* m = 0;
    TIMED_LOAD_BEGIN(filename)
    m = model_from_file(filename);
    model_generate_tangents(m);
    TIMED_LOAD_END
    /* Transfer data to gpu */
    struct model_hndl* h = model_to_gpu(m);
    /* Free memory data */
    model_delete(m);
    return h;
}

void model_free_from_gpu(struct model_hndl* mdlh)
{
    /* Free geometry */
    for (unsigned int i = 0; i < mdlh->num_meshes; ++i) {
        struct mesh_hndl* mh = mdlh->meshes + i;
        glDeleteBuffers(1, &mh->ebo);
        glDeleteBuffers(1, &mh->vbo);
        if (mh->wbo)
            glDeleteBuffers(1, &mh->wbo);
        glDeleteVertexArrays(1, &mh->vao);
    }
    /* Free skeleton if exists */
    if (mdlh->skel)
        skeleton_delete(mdlh->skel);
    /* Free frameset if exists */
    if (mdlh->fset)
        frameset_delete(mdlh->fset);
    for (unsigned int i = 0; i < mdlh->num_mesh_groups; ++i)
        mesh_group_delete(mdlh->mesh_groups[i]);
    free(mdlh->mesh_groups);
    free(mdlh->meshes);
    free(mdlh);
}

struct tex_hndl* tex_to_gpu(struct image* im)
{
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4.0f);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    if (im->compression_type == 0) {
        glTexImage2D(
            GL_TEXTURE_2D, 0,
            im->channels == 4 ? GL_RGBA : GL_RGB,
            im->width, im->height, 0,
            im->channels == 4 ? GL_RGBA : GL_RGB,
            GL_UNSIGNED_BYTE,
            im->data);
    } else {
        glCompressedTexImage2D(
            GL_TEXTURE_2D, 0,
            im->compression_type,
            im->width,
            im->height,
            0, im->data_sz, im->data);
    }
    glGenerateMipmap(GL_TEXTURE_2D);

    struct tex_hndl* th = calloc(1, sizeof(struct tex_hndl));
    th->id = id;
    return th;
}

struct tex_hndl* tex_from_file_to_gpu(const char* filename)
{
    struct image* im = 0;
    TIMED_LOAD_BEGIN(filename)
    im = image_from_file(filename);
    TIMED_LOAD_END
    struct tex_hndl* th = tex_to_gpu(im);
    image_delete(im);
    return th;
}

static int hcross_face_map[6][2] = {
    {2, 1}, /* Pos X */
    {0, 1}, /* Neg X */
    {1, 0}, /* Pos Y */
    {1, 2}, /* Neg Y */
    {1, 1}, /* Pos Z */
    {3, 1}  /* Neg Z */
};

/* In pixels */
static size_t hcross_face_offset(int face, size_t face_size)
{
    size_t stride = 4 * face_size;
    return hcross_face_map[face][1] * face_size * stride + hcross_face_map[face][0] * face_size;
}

static unsigned int tex_env_from_hcross(void* data, unsigned int width, unsigned int height, unsigned int channels)
{
    (void) height;
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, id);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    /* Set row read stride */
    unsigned int stride = width;
    unsigned int face_size = width / 4;
    glPixelStorei(GL_UNPACK_ROW_LENGTH, stride);

    /* Upload image data */
    for (int i = 0; i < 6; ++i) {
        int target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + i;
        glTexImage2D(
            target, 0,
            channels == 4 ? GL_SRGB8_ALPHA8 : GL_SRGB8,
            face_size, face_size, 0,
            channels == 4 ? GL_RGBA : GL_RGB,
            GL_UNSIGNED_BYTE,
            data + hcross_face_offset(i, face_size) * channels);
    }
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    return id;
}

struct tex_hndl* tex_env_from_file_to_gpu(const char* filename)
{
    GLuint id = 0;
    if (strcmp(strrchr(filename, '.'), ".hdr") == 0) {
        id = texture_cubemap_from_hdr(filename);
    } else {
        struct image* im = image_from_file(filename);
        id = tex_env_from_hcross(im->data, im->width, im->height, im->channels);
        image_delete(im);
    }
    struct tex_hndl* th = calloc(1, sizeof(struct tex_hndl));
    th->id = id;
    return th;
}

void tex_free_from_gpu(struct tex_hndl* th)
{
    glDeleteTextures(1, &th->id);
    free(th);
}

struct res_mngr* res_mngr_create()
{
    struct res_mngr* rmgr = calloc(1, sizeof(struct res_mngr));
    hashmap_init(&rmgr->mdl_store, hm_str_hash, hm_str_eql);
    hashmap_init(&rmgr->tex_store, hm_str_hash, hm_str_eql);
    hashmap_init(&rmgr->mat_store, hm_str_hash, hm_str_eql);
    return rmgr;
}

struct model_hndl* res_mngr_mdl_get(struct res_mngr* rmgr, const char* mdl_name)
{
    hm_ptr* p = hashmap_get(&rmgr->mdl_store, hm_cast(mdl_name));
    return p == 0 ? 0 : hm_pcast(*p);
}

struct tex_hndl* res_mngr_tex_get(struct res_mngr* rmgr, const char* tex_name)
{
    hm_ptr* p = hashmap_get(&rmgr->tex_store, hm_cast(tex_name));
    return p == 0 ? 0 : hm_pcast(*p);
}

struct material* res_mngr_mat_get(struct res_mngr* rmgr, const char* mat_name)
{
    hm_ptr* p = hashmap_get(&rmgr->mat_store, hm_cast(mat_name));
    return p == 0 ? 0 : hm_pcast(*p);
}

void res_mngr_mdl_put(struct res_mngr* rmgr, const char* mdl_name, struct model_hndl* m)
{
    hashmap_put(&rmgr->mdl_store, hm_cast(strdup(mdl_name)), hm_cast(m));
}

void res_mngr_tex_put(struct res_mngr* rmgr, const char* tex_name, struct tex_hndl* t)
{
    hashmap_put(&rmgr->tex_store, hm_cast(strdup(tex_name)), hm_cast(t));
}

void res_mngr_mat_put(struct res_mngr* rmgr, const char* mat_name, struct material* mat)
{
    hashmap_put(&rmgr->mat_store, hm_cast(strdup(mat_name)), hm_cast(mat));
}

void res_mngr_destroy(struct res_mngr* rmgr)
{
    struct hashmap_iter it;
    /* Free model store data */
    hashmap_for(rmgr->mdl_store, it) {
        free((void*)hm_pcast(it.p->key));
        struct model_hndl* mh = hm_pcast(it.p->value);
        model_free_from_gpu(mh);
    }
    /* Free texture store data */
    hashmap_for(rmgr->tex_store, it) {
        free((void*)hm_pcast(it.p->key));
        struct tex_hndl* th = hm_pcast(it.p->value);
        tex_free_from_gpu(th);
    }
    /* Free material store data */
    hashmap_for(rmgr->mat_store, it) {
        free((void*)hm_pcast(it.p->key));
        free((void*)hm_pcast(it.p->value));
    }
    /* Free stores */
    hashmap_destroy(&rmgr->mdl_store);
    hashmap_destroy(&rmgr->mat_store);
    hashmap_destroy(&rmgr->tex_store);
    free(rmgr);
}
