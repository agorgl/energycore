#include <energycore/renderer.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <glad/glad.h>
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
    rs->proj = mat4_perspective(radians(45.0f), 0.1f, 300.0f, (800.0f / 600.0f));
    skybox_init(rs);
}

static void render_skybox(struct renderer_state* rs, mat4* view, mat4* proj, unsigned int cubemap)
{
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    glUseProgram(rs->skybox.shdr);

    /* Remove any translation component of the view matrix */
    mat4 nt_view = mat3_to_mat4(mat4_to_mat3(*view));
    glUniformMatrix4fv(glGetUniformLocation(rs->skybox.shdr, "proj"), 1, GL_FALSE, proj->m);
    glUniformMatrix4fv(glGetUniformLocation(rs->skybox.shdr, "view"), 1, GL_FALSE, nt_view.m);

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

static void render_probe(struct renderer_state* rs, GLuint sky, vec3 probe_pos)
{
    /* Create sphere data */
    struct sphere_gdata* vdat = uv_sphere_create(1.0f, 64, 64);
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

    /* Render setup */
    glUseProgram(rs->shdr_main);
    glUniform1i(glGetUniformLocation(rs->shdr_main, "sky"), 0);
    glUniform1i(glGetUniformLocation(rs->shdr_main, "render_mode"), 1);
    float scale = 0.2f;
    mat4 model = mat4_mul_mat4(
        mat4_translation(probe_pos),
        mat4_scale(vec3_new(scale, scale, scale)));
    glUniformMatrix4fv(glGetUniformLocation(rs->shdr_main, "model"), 1, GL_FALSE, model.m);
    /* Render */
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, sky);
    glDrawElements(GL_TRIANGLES, vdat->num_indices, GL_UNSIGNED_INT, (void*)0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glUseProgram(0);

    /* Free */
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDeleteBuffers(1, &sph_vbo);
    glDeleteVertexArrays(1, &sph_vao);
    uv_sphere_destroy(vdat);
}

static void render_scene(struct renderer_state* rs, struct renderer_input* ri, float view[16], float proj[16])
{
    /* Clear default buffers */
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* Render */
    glEnable(GL_DEPTH_TEST);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glUseProgram(rs->shdr_main);
    glUniform1i(glGetUniformLocation(rs->shdr_main, "render_mode"), 0);

    /* Setup matrices */
    GLuint proj_mat_loc = glGetUniformLocation(rs->shdr_main, "proj");
    GLuint view_mat_loc = glGetUniformLocation(rs->shdr_main, "view");
    GLuint modl_mat_loc = glGetUniformLocation(rs->shdr_main, "model");
    glUniformMatrix4fv(proj_mat_loc, 1, GL_FALSE, proj);
    glUniformMatrix4fv(view_mat_loc, 1, GL_FALSE, view);

    /* Loop through meshes */
    for (unsigned int i = 0; i < ri->num_meshes; ++i) {
        /* Setup mesh to be rendered */
        struct renderer_mesh* rm = ri->meshes + i;
        /* Upload model matrix */
        glUniformMatrix4fv(modl_mat_loc, 1, GL_FALSE, rm->model_mat);
        /* Render mesh */
        glBindVertexArray(rm->vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rm->ebo);
        glDrawElements(GL_TRIANGLES, rm->indice_count, GL_UNSIGNED_INT, (void*)0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    glUseProgram(0);

    /* Render skybox last */
    render_skybox(rs, (mat4*)view, (mat4*)proj, ri->skybox);
}

static unsigned int render_local_cubemap(struct renderer_state* rs, struct renderer_input* ri, vec3 pos)
{
    /* Temporary framebuffer */
    int fbwidth = 128, fbheight = 128;
    GLuint fb;
    glGenFramebuffers(1, &fb);
    glBindFramebuffer(GL_FRAMEBUFFER, fb);

    /* Store previous viewport and set the new one */
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    glViewport(0, 0, fbwidth, fbheight);

    /* Cubemap result */
    GLuint lcl_cubemap;
    glGenTextures(1, &lcl_cubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, lcl_cubemap);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    for (unsigned int i = 0; i < 6; ++i)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA8, fbwidth, fbheight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

    /* Temporary framebuffer's depth attachment */
    GLuint depth_rb;
    glGenRenderbuffers(1, &depth_rb);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_rb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, fbwidth, fbheight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rb);

    /* Vectors corresponding to cubemap faces */
    const float view_fronts[6][3] = {
        { 1.0f,  0.0f,  0.0f },
        {-1.0f,  0.0f,  0.0f },
        { 0.0f, -1.0f,  0.0f },
        { 0.0f,  1.0f,  0.0f },
        { 0.0f,  0.0f,  1.0f },
        { 0.0f,  0.0f, -1.0f }
    };
    const float view_ups[6][3] = {
        { 0.0f,  1.0f,  0.0f },
        { 0.0f,  1.0f,  0.0f },
        { 0.0f,  0.0f,  1.0f },
        { 0.0f,  0.0f, -1.0f },
        { 0.0f,  1.0f,  0.0f },
        { 0.0f,  1.0f,  0.0f }
    };

    /* Projection matrix */
    mat4 fproj = mat4_perspective(radians(90.0f), 0.1f, 300.0f, 1.0f);
    fproj = mat4_mul_mat4(fproj, mat4_scale(vec3_new(-1, 1, 1)));

    for (unsigned int i = 0; i < 6; ++i) {
        /* Create and set texture face */
        glBindTexture(GL_TEXTURE_CUBE_MAP, lcl_cubemap);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, lcl_cubemap, 0);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        /* Check framebuffer completeness */
        GLenum fb_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        assert(fb_status == GL_FRAMEBUFFER_COMPLETE);
        /* Construct view matrix towards current face */
        vec3 ffront = vec3_new(view_fronts[i][0], view_fronts[i][1], view_fronts[i][2]);
        vec3 fup = vec3_new(view_ups[i][0], view_ups[i][1], view_ups[i][2]);
        mat4 fview = mat4_view_look_at(pos, vec3_add(pos, ffront), fup);
        /* Render */
        render_scene(rs, ri, (float*)&fview, (float*)&fproj);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, 0);
    }

    /* Free temporary resources */
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteRenderbuffers(1, &depth_rb);
    glDeleteFramebuffers(1, &fb);

    /* Restore viewport */
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    return lcl_cubemap;
}

void renderer_render(struct renderer_state* rs, struct renderer_input* ri, float view[16])
{
    /* Calculate probe position */
    rs->prob_angle += 0.001f;
    vec3 probe_pos = vec3_new(cosf(rs->prob_angle), 0.0f, sinf(rs->prob_angle));

    /* Render local skybox */
    GLuint lcl_sky = render_local_cubemap(rs, ri, probe_pos);

    /* Render from camera view */
    render_scene(rs, ri, view, (float*)&rs->proj);

    /* Render sample probe */
    render_probe(rs, lcl_sky, probe_pos);
    glDeleteTextures(1, &lcl_sky);
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
