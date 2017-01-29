#include "probe_vis.h"
#include <string.h>
#include <glad/glad.h>
#include "glutils.h"

void probe_vis_init(struct probe_vis* pv)
{
    memset(pv, 0, sizeof(*pv));
    const unsigned int num_sectors = 64;
    /* Create sphere data */
    struct sphere_gdata* vdat = uv_sphere_create(1.0f, num_sectors, num_sectors);
    /* Create vao */
    GLuint sph_vao, sph_vbo, sph_ebo;
    glGenVertexArrays(1, &sph_vao);
    glBindVertexArray(sph_vao);
    /* Create vbo */
    glGenBuffers(1, &sph_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, sph_vbo);
    glBufferData(GL_ARRAY_BUFFER, vdat->num_verts * sizeof(struct sphere_vertice), vdat->vertices, GL_STATIC_DRAW);
    /* Setup vertex inputs */
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(struct sphere_vertice), (GLvoid*)offsetof(struct sphere_vertice, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(struct sphere_vertice), (GLvoid*)offsetof(struct sphere_vertice, uvs));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(struct sphere_vertice), (GLvoid*)offsetof(struct sphere_vertice, normal));
    /* Create ebo */
    glGenBuffers(1, &sph_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sph_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, vdat->num_indices * sizeof(uint32_t), vdat->indices, GL_STATIC_DRAW);
    /* Store gpu handles */
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    pv->vao = sph_vao;
    pv->vbo = sph_vbo;
    pv->ebo = sph_ebo;
    pv->num_indices = vdat->num_indices;
    uv_sphere_destroy(vdat);
}

void probe_vis_render(struct probe_vis* pv, unsigned int cubemap, vec3 probe_pos, mat4 view, mat4 proj, int mode)
{
    /* Calculate view position */
    mat4 inverse_view = mat4_inverse(view);
    vec3 view_pos = vec3_new(inverse_view.xw, inverse_view.yw, inverse_view.zw);

    /* Render setup */
    glUseProgram(pv->shdr);
    glUniformMatrix4fv(glGetUniformLocation(pv->shdr, "view"), 1, GL_FALSE, view.m);
    glUniformMatrix4fv(glGetUniformLocation(pv->shdr, "proj"), 1, GL_FALSE, proj.m);
    glUniform1i(glGetUniformLocation(pv->shdr, "u_mode"), mode);
    glUniform1i(glGetUniformLocation(pv->shdr, "u_envmap"), 0);
    glUniform3f(glGetUniformLocation(pv->shdr, "u_view_pos"), view_pos.x, view_pos.y, view_pos.z);
    float scale = 0.2f;
    mat4 model = mat4_mul_mat4(
        mat4_translation(probe_pos),
        mat4_scale(vec3_new(scale, scale, scale)));
    glUniformMatrix4fv(glGetUniformLocation(pv->shdr, "model"), 1, GL_FALSE, model.m);
    /* Render */
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
    glBindVertexArray(pv->vao);
    glDrawElements(GL_TRIANGLES, pv->num_indices, GL_UNSIGNED_INT, (void*)0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

void probe_vis_destroy(struct probe_vis* pv)
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDeleteBuffers(1, &pv->ebo);
    glDeleteBuffers(1, &pv->vbo);
    glDeleteVertexArrays(1, &pv->vao);
}
