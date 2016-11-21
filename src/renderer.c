#include <energycore/renderer.h>
#include <glad/glad.h>
#include <linalgb.h>

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
}
