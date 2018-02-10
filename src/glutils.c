#include "glutils.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define _USE_MATH_CONSTANTS
#include <math.h>
#define pi M_PI
#include "opengl.h"

struct {
    struct {
        GLuint vao, vbo;
    } quad, cube;
    struct {
        GLuint vao, vbo, ebo;
        GLuint num_indices;
    } sphr;
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

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    *vao = tvao;
    *vbo = tvbo;
}

static void mesh_destroy(GLuint* vao, GLuint* vbo)
{
    glDeleteBuffers(1, vbo);
    glDeleteVertexArrays(1, vao);
    *vao = 0;
    *vbo = 0;
}

static void sphere_create()
{
    const unsigned int num_sectors = 32;
    struct sphere_gdata* vdat = uv_sphere_create(1.0f, num_sectors, num_sectors);

    GLuint sph_vao, sph_vbo, sph_ebo;
    glGenVertexArrays(1, &sph_vao);
    glBindVertexArray(sph_vao);

    glGenBuffers(1, &sph_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, sph_vbo);
    glBufferData(GL_ARRAY_BUFFER, vdat->num_verts * sizeof(struct sphere_vertice), vdat->vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(struct sphere_vertice), (GLvoid*)offsetof(struct sphere_vertice, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(struct sphere_vertice), (GLvoid*)offsetof(struct sphere_vertice, uvs));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(struct sphere_vertice), (GLvoid*)offsetof(struct sphere_vertice, normal));

    glGenBuffers(1, &sph_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sph_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, vdat->num_indices * sizeof(uint32_t), vdat->indices, GL_STATIC_DRAW);

    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    g_glutils_state.sphr.vao = sph_vao;
    g_glutils_state.sphr.vbo = sph_vbo;
    g_glutils_state.sphr.ebo = sph_ebo;
    g_glutils_state.sphr.num_indices = vdat->num_indices;
    uv_sphere_destroy(vdat);
}

static void sphere_destroy()
{
    glDeleteBuffers(1, &g_glutils_state.sphr.ebo);
    glDeleteBuffers(1, &g_glutils_state.sphr.vbo);
    glDeleteVertexArrays(1, &g_glutils_state.sphr.vao);
}

void glutils_init()
{
    memset(&g_glutils_state, 0, sizeof(g_glutils_state));
    mesh_create(&g_glutils_state.quad.vao, &g_glutils_state.quad.vbo, (GLfloat*)quad_verts, quad_verts_sz);
    mesh_create(&g_glutils_state.cube.vao, &g_glutils_state.cube.vbo, (GLfloat*)cube_verts, cube_verts_sz);
    sphere_create();
}

void glutils_deinit()
{
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    sphere_destroy();
    mesh_destroy(&g_glutils_state.cube.vao, &g_glutils_state.cube.vbo);
    mesh_destroy(&g_glutils_state.quad.vao, &g_glutils_state.quad.vbo);
}

void render_quad()
{
    glBindVertexArray(g_glutils_state.quad.vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void render_cube()
{
    glBindVertexArray(g_glutils_state.cube.vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void render_sphere()
{
    glBindVertexArray(g_glutils_state.sphr.vao);
    glDrawElements(GL_TRIANGLES, g_glutils_state.sphr.num_indices, GL_UNSIGNED_INT, 0);
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
