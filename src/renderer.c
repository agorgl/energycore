#include <energycore/renderer.h>
#include <string.h>
#include <glad/glad.h>
#include <linalgb.h>
#include "static_data.h"
#include "glutils.h"

static void skybox_init(struct renderer_state* rs)
{
    /* Build shader */
    rs->skybox.shdr = shader_from_srcs(skybox_vs_src, 0, skybox_fs_src);
    /* Create cube */
    glGenVertexArrays(1, &rs->skybox.vao);
    glBindVertexArray(rs->skybox.vao);
    glGenBuffers(1, &rs->skybox.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, rs->skybox.vbo);
    glBufferData(GL_ARRAY_BUFFER, skybox_vertices_sz, &skybox_vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void renderer_init(struct renderer_state* rs)
{
    memset(rs, 0, sizeof(*rs));
    skybox_init(rs);
}

static void render_skybox(struct renderer_state* rs, mat4* view, mat4* proj, unsigned int cubemap)
{
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    glUseProgram(rs->skybox.shdr);

    /* Remove any translation component of the view matrix */
    mat4 nt_view = mat3_to_mat4(mat4_to_mat3(*view));
    glUniformMatrix4fv(glGetUniformLocation(rs->skybox.shdr, "proj"), 1, GL_TRUE, (GLfloat*)proj);
    glUniformMatrix4fv(glGetUniformLocation(rs->skybox.shdr, "view"), 1, GL_TRUE, (GLfloat*)&nt_view);

    glActiveTexture(GL_TEXTURE0);
    glUniform1i(glGetUniformLocation(rs->skybox.shdr, "skybox"), 0);

    glBindVertexArray(rs->skybox.vao);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glBindVertexArray(0);
    glUseProgram(0);

    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
}

void renderer_render(struct renderer_state* rs, struct renderer_input* ri, float view[16])
{
    /* Clear default buffers */
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* Create view and projection matrices */
    mat4 proj = mat4_perspective(radians(45.0f), 0.1f, 300.0f, 1.0f / (800.0f / 600.0f));

    /* Render */
    glEnable(GL_DEPTH_TEST);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glUseProgram(rs->shdr_main);

    /* Setup matrices */
    GLuint proj_mat_loc = glGetUniformLocation(rs->shdr_main, "proj");
    GLuint view_mat_loc = glGetUniformLocation(rs->shdr_main, "view");
    GLuint modl_mat_loc = glGetUniformLocation(rs->shdr_main, "model");
    glUniformMatrix4fv(proj_mat_loc, 1, GL_TRUE, (GLfloat*)&proj);
    glUniformMatrix4fv(view_mat_loc, 1, GL_TRUE, (GLfloat*)view);

    /* Loop through meshes */
    for (unsigned int i = 0; i < ri->num_meshes; ++i) {
        /* Setup mesh to be rendered */
        struct renderer_mesh* rm = ri->meshes + i;
        /* Upload model matrix */
        glUniformMatrix4fv(modl_mat_loc, 1, GL_TRUE, (GLfloat*)rm->model_mat);
        /* Render mesh */
        glBindVertexArray(rm->vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rm->ebo);
        glDrawElements(GL_TRIANGLES, rm->indice_count, GL_UNSIGNED_INT, (void*)0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    glUseProgram(0);

    /* Render skybox last */
    render_skybox(rs, (mat4*)view, &proj, ri->skybox);
}

static void skybox_destroy(struct renderer_state* rs)
{
    glDeleteBuffers(1, &rs->skybox.vbo);
    glDeleteVertexArrays(1, &rs->skybox.vao);
    glDeleteProgram(rs->skybox.shdr);
}

void renderer_destroy(struct renderer_state* rs)
{
    skybox_destroy(rs);
}
