#include "gpures.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <prof.h>
#include <glad/glad.h>
#include <assets/assetload.h>
#include "glutil.h"

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

        /* Create vao */
        glGenVertexArrays(1, &mh->vao);
        glBindVertexArray(mh->vao);

        /* Create vertex data vbo */
        glGenBuffers(1, &mh->vbo);
        glBindBuffer(GL_ARRAY_BUFFER, mh->vbo);
        glBufferData(GL_ARRAY_BUFFER,
                mesh->num_verts * sizeof(struct vertex),
                mesh->vertices,
                GL_STATIC_DRAW);

        GLuint pos_attrib = 0;
        glEnableVertexAttribArray(pos_attrib);
        glVertexAttribPointer(pos_attrib, 3, GL_FLOAT, GL_FALSE, sizeof(struct vertex), (GLvoid*)offsetof(struct vertex, position));
        GLuint uv_attrib = 1;
        glEnableVertexAttribArray(uv_attrib);
        glVertexAttribPointer(uv_attrib, 2, GL_FLOAT, GL_FALSE, sizeof(struct vertex), (GLvoid*)offsetof(struct vertex, uvs));
        GLuint nm_attrib = 2;
        glEnableVertexAttribArray(nm_attrib);
        glVertexAttribPointer(nm_attrib, 3, GL_FLOAT, GL_FALSE, sizeof(struct vertex), (GLvoid*)offsetof(struct vertex, normal));

        /* Create vertex weight data vbo */
        if (mesh->weights) {
            glGenBuffers(1, &mh->wbo);
            glBindBuffer(GL_ARRAY_BUFFER, mh->wbo);
            glBufferData(GL_ARRAY_BUFFER,
                mesh->num_verts * sizeof(struct vertex_weight),
                mesh->weights,
                GL_STATIC_DRAW);

            GLuint bi_attrib = 3;
            glEnableVertexAttribArray(bi_attrib);
            glVertexAttribIPointer(bi_attrib, 4, GL_UNSIGNED_INT, sizeof(struct vertex_weight), (GLvoid*)offsetof(struct vertex_weight, bone_ids));
            GLuint bw_attrib = 4;
            glEnableVertexAttribArray(bw_attrib);
            glVertexAttribPointer(bw_attrib, 4, GL_FLOAT, GL_FALSE, sizeof(struct vertex_weight), (GLvoid*)offsetof(struct vertex_weight, bone_weights));
        }

        /* Create indice ebo */
        glGenBuffers(1, &mh->ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mh->ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                mesh->num_indices * sizeof(GLuint),
                mesh->indices,
                GL_STATIC_DRAW);
        mh->indice_count = mesh->num_indices;
        total_verts += mesh->num_verts;
        total_indices += mesh->num_indices;
    }

    /* Move skeleton and frameset */
    model->skel = m->skeleton;
    model->fset = m->frameset;
    m->skeleton = 0;
    m->frameset = 0;

    return model;
}

#define TIMED_LOAD_BEGIN(fname) \
    do { \
        printf("Loading: %-85s", fname); \
        timepoint_t t1 = millisecs();

#define TIMED_LOAD_END \
        timepoint_t t2 = millisecs(); \
        printf("[ %3lu ms ]\n", (t2 - t1)); \
    } while (0);

struct model_hndl* model_from_file_to_gpu(const char* filename)
{
    /* Load and parse model file */
    struct model* m = 0;
    TIMED_LOAD_BEGIN(filename)
    m = model_from_file(filename);
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
    free(mdlh->meshes);
    free(mdlh);
}

struct tex_hndl* tex_to_gpu(struct image* im)
{
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(
        GL_TEXTURE_2D, 0,
        im->channels == 4 ? GL_RGBA : GL_RGB,
        im->width, im->height, 0,
        im->channels == 4 ? GL_RGBA : GL_RGB,
        GL_UNSIGNED_BYTE,
        im->data);
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

struct tex_hndl* tex_env_to_gpu(struct image* im)
{
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, id);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    /* Set row read stride */
    unsigned int stride = im->width;
    unsigned int face_size = im->width / 4;
    glPixelStorei(GL_UNPACK_ROW_LENGTH, stride);

    /* Upload image data */
    for (int i = 0; i < 6; ++i) {
        int target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + i;
        glTexImage2D(
            target, 0,
            im->channels == 4 ? GL_RGBA : GL_RGB,
            face_size, face_size, 0,
            im->channels == 4 ? GL_RGBA : GL_RGB,
            GL_UNSIGNED_BYTE,
            im->data + hcross_face_offset(i, face_size) * im->channels);
    }
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    struct tex_hndl* th = calloc(1, sizeof(struct tex_hndl));
    th->id = id;
    return th;
}

struct tex_hndl* tex_env_from_file_to_gpu(const char* filename)
{
    struct image* im = image_from_file(filename);
    struct tex_hndl* th = tex_env_to_gpu(im);
    image_delete(im);
    return th;
}

void tex_free_from_gpu(struct tex_hndl* th)
{
    glDeleteTextures(1, &th->id);
    free(th);
}

unsigned int shader_from_sources(const char* vs_src, const char* gs_src, const char* fs_src)
{
    /* Vertex */
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vs_src, 0);
    glCompileShader(vs);
    gl_check_last_compile_error(vs);
    /* Geometry */
    GLuint gs = 0;
    if (gs_src) {
        gs = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(gs, 1, &gs_src, 0);
        glCompileShader(gs);
        gl_check_last_compile_error(gs);
    }
    /* Fragment */
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fs_src, 0);
    glCompileShader(fs);
    gl_check_last_compile_error(fs);
    /* Create program */
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    if (gs_src)
        glAttachShader(prog, gs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    gl_check_last_link_error(prog);
    /* Free unnecessary resources */
    glDeleteShader(vs);
    if (gs_src)
        glDeleteShader(gs);
    glDeleteShader(fs);
    return prog;
}

static void shader_load_err(void* userdata, const char* err)
{
    (void) userdata;
    fprintf(stderr, "%s\n", err);
}

unsigned int shader_from_files(const char* vsp, const char* gsp, const char* fsp)
{
    /* Load settings */
    struct shader_load_settings settings;
    memset(&settings, 0, sizeof(settings));
    settings.load_type = SHADER_LOAD_FILE;
    settings.error_cb = shader_load_err;
    /* Load sources */
    const char* vs_src = shader_load(vsp, &settings);
    const char* gs_src = 0;
    if (gsp)
        gs_src = shader_load(gsp, &settings);
    const char* fs_src = shader_load(fsp, &settings);
    /* Create shader */
    unsigned int shdr = shader_from_sources(vs_src, gs_src, fs_src);
    /* Free sources */
    if (gsp)
        free((void*)gs_src);
    free((void*)vs_src);
    free((void*)fs_src);
    return shdr;
}
