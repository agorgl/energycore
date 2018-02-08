#include "bbrndr.h"
#include <string.h>
#include "opengl.h"
#include "glutils.h"

static const char* vs_src = GLSRC(
layout (location = 0) in vec3 position;

uniform mat4 proj;
uniform mat4 view;
uniform mat4 model;

void main()
{
    vec4 pos = proj * view * model * vec4(position, 1.0);
    gl_Position = pos;
}
);

static const char* fs_src = GLSRC(
out vec4 color;

void main()
{
    color = vec4(0.0, 0.0, 1.0, 1.0);
}
);

static const GLuint box_indices[] = {
    0, 1, 2,
    1, 3, 2,
    4, 6, 7,
    7, 5, 4,
    1, 5 ,7,
    7, 3, 1,
    0, 2, 6,
    6, 4, 0,
    5, 1, 0,
    0, 4, 5,
    7, 6, 2,
    2, 3, 7
};

void bbox_rndr_init(struct bbox_rndr* st)
{
    /* Build visualization shader */
    st->vis_shdr = shader_from_srcs(vs_src, 0, fs_src);

    /* Create cube vao and empty vbo */
    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(float), 0, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);

    /* Create common indice buffer */
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(box_indices), box_indices, GL_STATIC_DRAW);
    unsigned int indice_count = sizeof(box_indices) / sizeof(box_indices[0]);
    glBindVertexArray(0);

    /* Store state */
    st->vao = vao;
    st->vbo = vbo;
    st->ebo = ebo;
    st->indice_count = indice_count;
}

void bbox_rndr_vis(struct bbox_rndr* st, float model[16], float view[16], float proj[16], float aabb_min[3], float aabb_max[3])
{
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    GLuint shdr = st->vis_shdr;
    glUseProgram(shdr);
    GLuint proj_mat_loc = glGetUniformLocation(shdr, "proj");
    GLuint view_mat_loc = glGetUniformLocation(shdr, "view");
    GLuint modl_mat_loc = glGetUniformLocation(shdr, "model");
    glUniformMatrix4fv(proj_mat_loc, 1, GL_FALSE, proj);
    glUniformMatrix4fv(view_mat_loc, 1, GL_FALSE, view);
    glUniformMatrix4fv(modl_mat_loc, 1, GL_FALSE, model);

    bbox_rndr_render(st, aabb_min, aabb_max);
    glUseProgram(0);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void bbox_rndr_render(struct bbox_rndr* st, float aabb_min[3], float aabb_max[3])
{
    float verts[24] = {
        aabb_max[0], aabb_max[1], aabb_max[2],
        aabb_min[0], aabb_max[1], aabb_max[2],
        aabb_max[0], aabb_min[1], aabb_max[2],
        aabb_min[0], aabb_min[1], aabb_max[2],
        aabb_max[0], aabb_max[1], aabb_min[2],
        aabb_min[0], aabb_max[1], aabb_min[2],
        aabb_max[0], aabb_min[1], aabb_min[2],
        aabb_min[0], aabb_min[1], aabb_min[2],
    };
    glBindBuffer(GL_ARRAY_BUFFER, st->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    glBindVertexArray(st->vao);
    glDrawElements(GL_TRIANGLES, st->indice_count, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void bbox_rndr_destroy(struct bbox_rndr* st)
{
    glDeleteBuffers(1, &st->ebo);
    glDeleteBuffers(1, &st->vbo);
    glDeleteVertexArrays(1, &st->vao);
    glDeleteProgram(st->vis_shdr);
}
