#include "glutils.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <glad/glad.h>
#define _USE_MATH_CONSTANTS
#include <math.h>
#define pi M_PI

struct {
    GLuint quad_vao;
    GLuint quad_vbo;
    GLuint cube_vao;
    GLuint cube_vbo;
} g_glutils_state;

static const GLfloat quad_verts[] =
{
   -1.0f,  1.0f, 0.0f,
   -1.0f, -1.0f, 0.0f,
    1.0f,  1.0f, 0.0f,
    1.0f, -1.0f, 0.0f
};
static const size_t quad_verts_sz = sizeof(quad_verts);

static const GLfloat cube_verts[] =
{
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
};
static const size_t cube_verts_sz = sizeof(cube_verts);

static void mesh_create(GLuint* vao, GLuint *vbo, GLfloat* verts, GLuint verts_sz)
{
    GLuint tvao;
    glGenVertexArrays(1, &tvao);
    glBindVertexArray(tvao);

    GLuint tvbo;
    glGenBuffers(1, &tvbo);
    glBindBuffer(GL_ARRAY_BUFFER, tvbo);
    glBufferData(GL_ARRAY_BUFFER, verts_sz, verts, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    *vao = tvao;
    *vbo = tvbo;
}

static void mesh_destroy(GLuint* vao, GLuint* vbo)
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDeleteBuffers(1, vbo);
    glDeleteVertexArrays(1, vao);
    *vao = 0;
    *vbo = 0;
}

void glutils_init()
{
    memset(&g_glutils_state, 0, sizeof(g_glutils_state));
    mesh_create(&g_glutils_state.quad_vao, &g_glutils_state.quad_vbo, (GLfloat*)quad_verts, quad_verts_sz);
    mesh_create(&g_glutils_state.cube_vao, &g_glutils_state.cube_vbo, (GLfloat*)cube_verts, cube_verts_sz);
}

void glutils_deinit()
{
    mesh_destroy(&g_glutils_state.cube_vao, &g_glutils_state.cube_vbo);
    mesh_destroy(&g_glutils_state.quad_vao, &g_glutils_state.quad_vbo);
}

void render_quad()
{
    glBindVertexArray(g_glutils_state.quad_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void render_cube()
{
    glBindVertexArray(g_glutils_state.cube_vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

struct sphere_gdata* uv_sphere_create(float radius, unsigned int rings, unsigned int sectors)
{
    struct sphere_gdata* vdat = malloc(sizeof(struct sphere_gdata));
    memset(vdat, 0, sizeof(struct sphere_gdata));
    /* Allocate space */
    vdat->num_verts = rings * sectors;
    vdat->num_indices = rings * sectors * 6;
    vdat->vertices = calloc(vdat->num_verts, sizeof(*vdat->vertices));
    vdat->indices = calloc(vdat->num_indices, sizeof(*vdat->indices));

    /* */
    const float R = 1.0f / (float)(rings - 1);
    const float S = 1.0f / (float)(sectors - 1);
    /* Calc the vertices */
    struct sphere_vertice* sv = vdat->vertices;
    for (unsigned int r = 0; r < rings; ++r) {
        for (unsigned int s = 0; s < sectors; ++s) {
            float x = cosf(2 * pi * s * S) * sinf(pi * r * R);
            float z = sinf(2 * pi * s * S) * sinf(pi * r * R);
            float y = sinf(-pi / 2 + pi * r * R);
            /* Push back vertex data */
            sv->position[0] = x * radius;
            sv->position[1] = y * radius;
            sv->position[2] = z * radius;
            /* Push back normal */
            sv->normal[0] = x;
            sv->normal[1] = y;
            sv->normal[2] = z;
            /* Push back texture coordinates */
            sv->uvs[0] = s * S;
            sv->uvs[1] = (rings - 1 - r) * R;
            /* Increment iterator */
            ++sv;
        }
    }

    /* Calc indices */
    uint32_t* it = vdat->indices;
    for (unsigned int r = 0; r < rings - 1; ++r) {
        for (unsigned int s = 0; s < sectors - 1; ++s) {
            unsigned int cur_row = r * sectors;
            unsigned int next_row = (r + 1) * sectors;

            *it++ = (cur_row + s);
            *it++ = (next_row + s);
            *it++ = (next_row + (s+1));

            *it++ = (cur_row + s);
            *it++ = (next_row + (s+1));
            *it++ = (cur_row + (s+1));
        }
    }
    return vdat;
}

void uv_sphere_destroy(struct sphere_gdata* vdat)
{
    free(vdat->vertices);
    free(vdat->indices);
    free(vdat);
}

unsigned int shader_from_srcs(const char* vs_src, const char* gs_src, const char* fs_src)
{
    /* Vertex */
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vs_src, 0);
    glCompileShader(vs);
    /* Geometry */
    GLuint gs = 0;
    if (gs_src) {
        gs = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(gs, 1, &gs_src, 0);
        glCompileShader(gs);
    }
    /* Fragment */
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fs_src, 0);
    glCompileShader(fs);
    /* Create program */
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    if (gs_src)
        glAttachShader(prog, gs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    /* Free unnecessary resources */
    glDeleteShader(vs);
    if (gs_src)
        glDeleteShader(gs);
    glDeleteShader(fs);
    return prog;
}
