#include "shdwmap.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <glad/glad.h>
#include <math.h>
#include "glutils.h"

#define GLSRCEXT(src) "#version 330 core\n" \
                      "#extension GL_ARB_gpu_shader5 : enable\n" \
                      #src

static const char* vs_src = GLSRCEXT(
layout (location = 0) in vec3 position;
uniform mat4 model;
void main()
{
    gl_Position = model * vec4(position, 1.0f);
}
);

static const char* gs_src = GLSRCEXT(
layout(triangles,       invocations = 4) in;
layout(triangle_strip, max_vertices = 3) out;

out float layer;
out vec3 vs_pos;

uniform mat4 cascades_view_mats[4];
uniform mat4 cascades_proj_mats[4];

void main()
{
    for (int i = 0; i < gl_in.length(); ++i) {
        vec4 pos    = cascades_view_mats[gl_InvocationID] * gl_in[i].gl_Position;
        vs_pos      = pos.xyz;
        layer       = float(gl_InvocationID);
        gl_Position = cascades_proj_mats[gl_InvocationID] * pos;
        gl_Layer    = gl_InvocationID;
        EmitVertex();
    }
    EndPrimitive();
}
);

static const char* fs_src = GLSRCEXT(
in float layer;
in vec3 vs_pos;

uniform float cascades_near[4];
uniform float cascades_far[4];

void main()
{
    int layeri = int(layer);
    float near = cascades_near[layeri];
    float far  = cascades_far[layeri];
    float linear_depth = (-vs_pos.z - near) / (far - near);
    gl_FragDepth = linear_depth;
}
);

void shadowmap_init(struct shadowmap* sm, int width, int height)
{
    memset(sm, 0, sizeof(*sm));
    sm->width = width;
    sm->height = height;
    sm->glh.shdr = shader_from_srcs(vs_src, gs_src, fs_src);

    /* Create texture array that will hold the shadow maps */
    GLuint depth_tex;
    glGenTextures(1, &depth_tex);
    glBindTexture(GL_TEXTURE_2D_ARRAY, depth_tex);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT, width, height, SHADOWMAP_NSPLITS, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    GLfloat border_col[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, border_col);

    /* Prepare framebuffer */
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth_tex, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    /* Store handles */
    sm->glh.tex_id = depth_tex;
    sm->glh.fbo_id = fbo;
}

/* From the source code it is:
 * const float tan_half_fovy = tan(fovy / 2.0f);
 * result[1][1] = 1.0f / tan_half_fovy; */
static float extract_fov_from_projection(float proj[4][4])
{
    float t = proj[1][1];
    float fov = atan((1.0f / t)) * 2.0f;
    return fov;
}

/* From the source code it is:
 * result[0][0] = 1.0f / (aspect * tan_half_fovy);
 * result[1][1] = 1.0f / (tan_half_fovy); */
static float extract_aspect_from_projection(float proj[4][4])
{
    float tanHalfFovy = 1.0f / proj[1][1];
    float t = proj[0][0];
    float aspect = 1.0f / (t * tanHalfFovy);
    /* float aspect = proj[1][1] / proj[0][0] */
    return aspect;
}

/* Transform 1 front and 1 back corner of the NDC box
 * using the inverse projection matrix, and perform perspective division.
 * Their z values will now contain the near and far values of the frustum */
static void extract_near_far_from_projection(float proj[4][4], float* near, float* far)
{
    mat4 inv_proj = mat4_inverse(*(mat4*)proj);
    vec4 points[] = { vec4_new(-1, 1, -1, 1), vec4_new(-1, 1, 1, 1) };
    for (int i = 0; i < 2; ++i) {
        points[i] = mat4_mul_vec4(inv_proj, points[i]);
        points[i] = vec4_div(points[i], points[i].w);
    }
    *near = -points[0].z;
    *far = -points[1].z;
}

/* Lerp */
static float mix(float x, float y, float a) { return x * (1.0 - a) + y * a; }

static void shadowmap_setup_splits(struct shadowmap* sm, float light_pos[3], float view[16], float proj[16])
{
    /* Get projection properties */
    const float camera_fov    = extract_fov_from_projection((float(*)[4])proj);
    const float camera_aspect = extract_aspect_from_projection((float(*)[4])proj);
    float camera_near = 0.0f, camera_far = 0.0f;
    extract_near_far_from_projection((float(*)[4])proj, &camera_near, &camera_far);
    /* Calculate inverse view matrix */
    mat4 inv_view = mat4_inverse(*(mat4*)view);
    /* Override camera far value */
    camera_far = 100.0f; /* TODO */

    /* Clear previous split values */
    memset(&sm->sd, 0, sizeof(sm->sd));
    /* Calculate split data */
    const float split_lambda = 0.9f;
    const unsigned int split_num = SHADOWMAP_NSPLITS;
    for (unsigned int i = 0; i < split_num; ++i) {
        /* Find the split planes using GPU Gem 3. Chap 10 "Practical Split Scheme". */
        float split_near = i > 0
            ? mix(
                camera_near + ((float)(i) / split_num) * (camera_far - camera_near),
                camera_near * powf(camera_far / camera_near, (float)(i) / split_num),
                split_lambda)
            : camera_near;
        float split_far = i < split_num - 1
            ? mix(
                camera_near + ((float)(i + 1) / split_num) * (camera_far - camera_near),
                camera_near * powf(camera_far / camera_near, (float)(i + 1) / split_num),
                split_lambda)
            : camera_far;

        /* Create the projection for this split */
        mat4 split_proj = mat4_perspective(camera_fov, split_near, split_far, camera_aspect);

        /* Extract the frustum points in world space coordinates */
        vec4 split_vertices[8] = {
            /* Near plane */
            vec4_new(-1.f, -1.f, -1.f, 1.f),
            vec4_new(-1.f,  1.f, -1.f, 1.f),
            vec4_new( 1.f,  1.f, -1.f, 1.f),
            vec4_new( 1.f, -1.f, -1.f, 1.f),
            /* Far plane */
            vec4_new(-1.f, -1.f,  1.f, 1.f),
            vec4_new(-1.f,  1.f,  1.f, 1.f),
            vec4_new( 1.f,  1.f,  1.f, 1.f),
            vec4_new( 1.f, -1.f,  1.f, 1.f)
        };
        for (unsigned int j = 0; j < 8; ++j) {
            /* NDC -> View space */
            split_vertices[j] = mat4_mul_vec4(mat4_inverse(split_proj), split_vertices[j]);
            split_vertices[j] = vec4_div(split_vertices[j], split_vertices[j].w);
            /* View space -> World space */
            split_vertices[j] = mat4_mul_vec4(inv_view, split_vertices[j]);
        }

        /* Find the centroid in order to construct the light view matrix eye */
        vec4 split_centroid = vec4_zero();
        for(unsigned int j = 0; j < 8; ++j)
            split_centroid = vec4_add(split_centroid, split_vertices[j]);
        split_centroid = vec4_div(split_centroid, 8.0f);

        /* Construct the light view matrix */
        float dist = max(split_far - split_near, vec4_dist(split_vertices[5], split_vertices[6]));
        vec3 light_dir = vec3_normalize(*(vec3*)light_pos);
        vec3 split_centr = vec3_new(split_centroid.x, split_centroid.y, split_centroid.z);
        vec3 view_pos = vec3_add(split_centr, vec3_mul(light_dir, dist));
        mat4 view_mat = mat4_view_look_at(view_pos, split_centr, vec3_new(0.0f, 1.0f, 0.0f));

        /* Transform split vertices to the light view space */
        vec4 split_vertices_ls[8];
        memset(split_vertices_ls, 0, sizeof(split_vertices_ls));
        for(unsigned int j = 0; j < 8; ++j)
            split_vertices_ls[j] = mat4_mul_vec4(view_mat, split_vertices[j]);

        /* Find the frustum bounding box in view space */
        vec4 min = split_vertices_ls[0];
        vec4 max = split_vertices_ls[0];
        for(unsigned int j = 0; j < 8; ++j) {
            min = vec4_min(min, split_vertices_ls[j]);
            max = vec4_max(max, split_vertices_ls[j]);
        }

        /* Create an orthogonal projection matrix with the corners */
        float near_offset = 10.0f;
        float far_offset = 20.0f;
        mat4 proj_mat = mat4_orthographic(min.x, max.x, min.y, max.y, -max.z - near_offset, -min.z + far_offset);

        /* Save matrices and planes */
        sm->sd[i].proj_mat = proj_mat;
        sm->sd[i].view_mat = view_mat;
        sm->sd[i].shdw_mat = mat4_mul_mat4(proj_mat, view_mat);
        sm->sd[i].plane = vec2_new(split_near, split_far);
        sm->sd[i].near_plane = -max.z - near_offset;
        sm->sd[i].far_plane = -min.z + far_offset;
    }
}

void shadowmap_render(struct shadowmap* sm, float light_pos[3], float view[16], float proj[16], render_sm_scene_fn render_scene, void* userdata)
{
    /* Calculate split data */
    shadowmap_setup_splits(sm, light_pos, view, proj);

    /* Enable depth testing (duh!) */
    glEnable(GL_DEPTH_TEST);

    /* Store previous viewport and set the new one */
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    glViewport(0, 0, sm->width, sm->height);

    /* Bind shadow map fbo */
    glBindFramebuffer(GL_FRAMEBUFFER, sm->glh.fbo_id);
    glClear(GL_DEPTH_BUFFER_BIT);

    /* Setup uniforms */
    GLuint shdr = sm->glh.shdr;
    glUseProgram(shdr);
    for (int i = 0; i < SHADOWMAP_NSPLITS; ++i) {
        GLint uloc = 0;
        char* uname_buf[64];
        /* Proj matrix */
        snprintf((char*)uname_buf, sizeof(uname_buf), "cascades_proj_mats[%u]", i);
        uloc = glGetUniformLocation(shdr, (const char*)uname_buf);
        glUniformMatrix4fv(uloc, 1, GL_FALSE, sm->sd[i].proj_mat.m);
        /* View matrix */
        snprintf((char*)uname_buf, sizeof(uname_buf), "cascades_view_mats[%u]", i);
        uloc = glGetUniformLocation(shdr, (const char*)uname_buf);
        glUniformMatrix4fv(uloc, 1, GL_FALSE, sm->sd[i].view_mat.m);
        /* Near value */
        snprintf((char*)uname_buf, sizeof(uname_buf), "cascades_near[%u]", i);
        uloc = glGetUniformLocation(shdr, (const char*)uname_buf);
        glUniform1fv(uloc, 1, &sm->sd[i].near_plane);
        /* Far value */
        snprintf((char*)uname_buf, sizeof(uname_buf), "cascades_far[%u]", i);
        uloc = glGetUniformLocation(shdr, (const char*)uname_buf);
        glUniform1fv(uloc, 1, &sm->sd[i].far_plane);
    }

    /* Set cull face mode to front
     * in order to improve peter panning issues on solid objects */
    //glCullFace(GL_FRONT);
    /* Invoke callback to render scene */
    render_scene(shdr, userdata);
    /* Revert cull face mode */
    glCullFace(GL_BACK);

    /* Unbind shadowmap fbo and shader */
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(0);

    /* Restore viewport */
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
}

void shadowmap_bind(struct shadowmap* sm, unsigned int shdr)
{
    for (int i = 0; i < SHADOWMAP_NSPLITS; ++i) {
        GLint uloc = 0;
        char* uname_buf[64];
        /* Pair of split's near and far values */
        snprintf((char*)uname_buf, sizeof(uname_buf), "cascades[%u].plane", i);
        uloc = glGetUniformLocation(shdr, (const char*)uname_buf);
        glUniform2fv(uloc, 1, sm->sd[i].plane.xy);
        /* Light view * proj matrix */
        snprintf((char*)uname_buf, sizeof(uname_buf), "cascades[%u].vp_mat", i);
        uloc = glGetUniformLocation(shdr, (const char*)uname_buf);
        glUniformMatrix4fv(uloc, 1, GL_FALSE, sm->sd[i].shdw_mat.m);
    }
}

void shadowmap_destroy(struct shadowmap* sm)
{
    glDeleteTextures(1, &sm->glh.tex_id);
    glDeleteFramebuffers(1, &sm->glh.fbo_id);
    glDeleteProgram(sm->glh.shdr);
}
