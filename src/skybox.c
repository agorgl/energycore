#include "skybox.h"
#include <glad/glad.h>
#include "glutils.h"

static const char* skybox_vs_src = GLSRC(
layout (location = 0) in vec3 position;

out VS_OUT {
    vec3 tex_coords;
} vs_out;

uniform mat4 proj;
uniform mat4 view;

void main()
{
    vs_out.tex_coords = vec3(position.x, -position.y, position.z);
    vec4 pos = proj * view * vec4(position, 1.0);
    gl_Position = pos.xyww;
}
);

static const char* skybox_fs_src = GLSRC(
out vec4 color;

in VS_OUT {
    vec3 tex_coords;
} fs_in;

uniform samplerCube skybox;

void main()
{
    color = texture(skybox, fs_in.tex_coords);
}
);

/* The skybox cube vertex data */
static const float skybox_vertices[] = {
    /* Positions */
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

static const size_t skybox_vertices_sz = sizeof(skybox_vertices);

void skybox_init(struct skybox* sb)
{
    /* Build shader */
    sb->shdr = shader_from_srcs(skybox_vs_src, 0, skybox_fs_src);
    /* Create cube */
    glGenVertexArrays(1, &sb->vao);
    glBindVertexArray(sb->vao);
    glGenBuffers(1, &sb->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, sb->vbo);
    glBufferData(GL_ARRAY_BUFFER, skybox_vertices_sz, &skybox_vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void skybox_render(struct skybox* sb, mat4* view, mat4* proj, unsigned int cubemap)
{
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    glUseProgram(sb->shdr);

    /* Remove any translation component of the view matrix */
    mat4 nt_view = mat3_to_mat4(mat4_to_mat3(*view));
    glUniformMatrix4fv(glGetUniformLocation(sb->shdr, "proj"), 1, GL_FALSE, proj->m);
    glUniformMatrix4fv(glGetUniformLocation(sb->shdr, "view"), 1, GL_FALSE, nt_view.m);

    glActiveTexture(GL_TEXTURE0);
    glUniform1i(glGetUniformLocation(sb->shdr, "skybox"), 0);

    glBindVertexArray(sb->vao);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glBindVertexArray(0);
    glUseProgram(0);

    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
}

void skybox_destroy(struct skybox* sb)
{
    glDeleteBuffers(1, &sb->vbo);
    glDeleteVertexArrays(1, &sb->vao);
    glDeleteProgram(sb->shdr);
}
