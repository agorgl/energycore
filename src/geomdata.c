#include "geomdata.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Triangle declarations */
vec3 triangle_tangent(vertex v1, vertex v2, vertex v3);
vec3 triangle_binormal(vertex v1, vertex v2, vertex v3);
vec3 triangle_normal(vertex v1, vertex v2, vertex v3);
vec3 triangle_random_position(vertex v1, vertex v2, vertex v3);
float triangle_area(vertex v1, vertex v2, vertex v3);
float triangle_difference_u(vertex v1, vertex v2, vertex v3);
float triangle_difference_v(vertex v1, vertex v2, vertex v3);
vertex triangle_random_position_interpolation(vertex v1, vertex v2, vertex v3);

vec3 triangle_tangent(vertex vert1, vertex vert2, vertex vert3)
{
    vec3 pos1 = vert1.position;
    vec3 pos2 = vert2.position;
    vec3 pos3 = vert3.position;

    vec2 uv1 = vert1.uvs;
    vec2 uv2 = vert2.uvs;
    vec2 uv3 = vert3.uvs;

    /* Get component vectors */
    float x1 = pos2.x - pos1.x;
    float x2 = pos3.x - pos1.x;

    float y1 = pos2.y - pos1.y;
    float y2 = pos3.y - pos1.y;

    float z1 = pos2.z - pos1.z;
    float z2 = pos3.z - pos1.z;

    /* Generate uv space vectors */
    float s1 = uv2.x - uv1.x;
    float s2 = uv3.x - uv1.x;

    float t1 = uv2.y - uv1.y;
    float t2 = uv3.y - uv1.y;

    float r = 1.0f / ((s1 * t2) - (s2 * t1));

    vec3 tdir = vec3_new(
        (s1 * x2 - s2 * x1) * r,
        (s1 * y2 - s2 * y1) * r,
        (s1 * z2 - s2 * z1) * r);

    return vec3_normalize(tdir);
}

vec3 triangle_binormal(vertex vert1, vertex vert2, vertex vert3)
{
    vec3 pos1 = vert1.position;
    vec3 pos2 = vert2.position;
    vec3 pos3 = vert3.position;

    vec2 uv1 = vert1.uvs;
    vec2 uv2 = vert2.uvs;
    vec2 uv3 = vert3.uvs;

    /* Get component Vectors */
    float x1 = pos2.x - pos1.x;
    float x2 = pos3.x - pos1.x;

    float y1 = pos2.y - pos1.y;
    float y2 = pos3.y - pos1.y;

    float z1 = pos2.z - pos1.z;
    float z2 = pos3.z - pos1.z;

    /* Generate uv space vectors */
    float s1 = uv2.x - uv1.x;
    float s2 = uv3.x - uv1.x;

    float t1 = uv2.y - uv1.y;
    float t2 = uv3.y - uv1.y;

    float r = 1.0f / ((s1 * t2) - (s2 * t1));

    vec3 sdir = vec3_new(
        (t2 * x1 - t1 * x2) * r,
        (t2 * y1 - t1 * y2) * r,
        (t2 * z1 - t1 * z2) * r);

    return vec3_normalize(sdir);
}

vec3 triangle_normal(vertex v1, vertex v2, vertex v3)
{
    vec3 edge1 = vec3_sub(v2.position, v1.position);
    vec3 edge2 = vec3_sub(v3.position, v1.position);
    vec3 normal = vec3_cross(edge1, edge2);
    return vec3_normalize(normal);
}

float triangle_area(vertex v1, vertex v2, vertex v3)
{
    vec3 ab = vec3_sub(v1.position, v2.position);
    vec3 ac = vec3_sub(v1.position, v3.position);
    float area = 0.5 * vec3_length(vec3_cross(ab, ac));
    return area;
}

vec3 triangle_random_position(vertex v1, vertex v2, vertex v3)
{
    float r1 = (float)rand() / (float)RAND_MAX;
    float r2 = (float)rand() / (float)RAND_MAX;

    if (r1 + r2 >= 1) {
        r1 = 1 - r1;
        r2 = 1 - r2;
    }

    vec3 ab = vec3_sub(v1.position, v2.position);
    vec3 ac = vec3_sub(v1.position, v3.position);

    vec3 a = v1.position;
    a = vec3_sub(a, vec3_mul(ab, r1));
    a = vec3_sub(a, vec3_mul(ac, r2));
    return a;
}

vertex triangle_random_position_interpolation(vertex v1, vertex v2, vertex v3)
{
    float r1 = (float)rand() / (float)RAND_MAX;
    float r2 = (float)rand() / (float)RAND_MAX;

    if (r1 + r2 >= 1) {
        r1 = 1 - r1;
        r2 = 1 - r2;
    }

    vertex v;

    vec3 v_pos, v_norm, v_tang, v_binorm;
    vec4 v_col;
    vec2 v_uv;

    v_pos = v1.position;
    v_pos = vec3_sub(v_pos, vec3_mul(vec3_sub(v1.position, v2.position), r1));
    v_pos = vec3_sub(v_pos, vec3_mul(vec3_sub(v1.position, v3.position), r2));

    v_norm = v1.normal;
    v_norm = vec3_sub(v_norm, vec3_mul(vec3_sub(v1.normal, v2.normal), r1));
    v_norm = vec3_sub(v_norm, vec3_mul(vec3_sub(v1.normal, v3.normal), r2));

    v_tang = v1.tangent;
    v_tang = vec3_sub(v_tang, vec3_mul(vec3_sub(v1.tangent, v2.tangent), r1));
    v_tang = vec3_sub(v_tang, vec3_mul(vec3_sub(v1.tangent, v3.tangent), r2));

    v_binorm = v1.binormal;
    v_binorm = vec3_sub(v_binorm, vec3_mul(vec3_sub(v1.binormal, v2.binormal), r1));
    v_binorm = vec3_sub(v_binorm, vec3_mul(vec3_sub(v1.binormal, v3.binormal), r2));

    v_col = v1.color;
    v_col = vec4_sub(v_col, vec4_mul(vec4_sub(v1.color, v2.color), r1));
    v_col = vec4_sub(v_col, vec4_mul(vec4_sub(v1.color, v3.color), r2));

    v_uv = v1.uvs;
    v_uv = vec2_sub(v_uv, vec2_mul(vec2_sub(v1.uvs, v2.uvs), r1));
    v_uv = vec2_sub(v_uv, vec2_mul(vec2_sub(v1.uvs, v3.uvs), r2));

    v.position = v_pos;
    v.normal = v_norm;
    v.tangent = v_tang;
    v.binormal = v_binorm;
    v.color = v_col;
    v.uvs = v_uv;

    return v;
}

float triangle_difference_u(vertex v1, vertex v2, vertex v3)
{
    float max = v1.uvs.x;
    max = v2.uvs.x > max ? v2.uvs.x : max;
    max = v3.uvs.x > max ? v3.uvs.x : max;

    float min = v1.uvs.x;
    min = v2.uvs.x < min ? v2.uvs.x : min;
    min = v3.uvs.x < min ? v3.uvs.x : min;

    return max - min;
}

float triangle_difference_v(vertex v1, vertex v2, vertex v3)
{
    float max = v1.uvs.y;
    max = v2.uvs.x > max ? v2.uvs.y : max;
    max = v3.uvs.x > max ? v3.uvs.y : max;

    float min = v1.uvs.y;
    min = v2.uvs.y < min ? v2.uvs.y : min;
    min = v3.uvs.y < min ? v3.uvs.y : min;

    return max - min;
}

vertex vertex_new()
{
    vertex v;
    memset(&v, 0, sizeof(vertex));
    return v;
}

bool vertex_equal(vertex v1, vertex v2)
{
    if (!vec3_equ(v1.position, v2.position))
        return false;
    if (!vec3_equ(v1.normal, v2.normal))
        return false;
    if (!vec2_equ(v1.uvs, v2.uvs))
        return false;
    return true;
}

void vertex_print(vertex v)
{
    printf("V(Position: ");
    vec3_print(v.position);
    printf(", Normal: ");
    vec3_print(v.normal);
    printf(", Tangent: ");
    vec3_print(v.tangent);
    printf(", Binormal: ");
    vec3_print(v.binormal);
    printf(", Color: ");
    vec4_print(v.color);
    printf(", Uvs: ");
    vec2_print(v.uvs);
    printf(")");
}

void mesh_print(mesh* m)
{
    printf("Num Verts: %i\n", m->num_verts);
    printf("Num Tris: %i\n", m->num_triangles);
    for (int i = 0; i < m->num_verts; i++) {
        vertex_print(m->verticies[i]);
        printf("\n");
    }
    printf("Triangle Indicies:");
    for (int i = 0; i < m->num_triangles * 3; i++) {
        printf("%i ", m->triangles[i]);
    }
    printf("\n");
}

mesh* mesh_new()
{
    mesh* m = malloc(sizeof(mesh));
    m->num_verts = 0;
    m->num_triangles = 0;
    m->verticies = malloc(sizeof(vertex) * m->num_verts);
    m->triangles = malloc(sizeof(int) * m->num_triangles * 3);
    return m;
}

void mesh_delete(mesh* m)
{
    free(m->verticies);
    free(m->triangles);
    free(m);
}

void mesh_generate_tangents(mesh* m)
{
    /* Clear all tangents to 0,0,0 */
    for (int i = 0; i < m->num_verts; i++) {
        m->verticies[i].tangent = vec3_zero();
        m->verticies[i].binormal = vec3_zero();
    }

    /* Loop over faces, calculate tangent and append to verticies of that face */
    int i = 0;
    while (i < m->num_triangles * 3) {
        int t_i1 = m->triangles[i];
        int t_i2 = m->triangles[i + 1];
        int t_i3 = m->triangles[i + 2];

        vertex v1 = m->verticies[t_i1];
        vertex v2 = m->verticies[t_i2];
        vertex v3 = m->verticies[t_i3];

        vec3 face_tangent = triangle_tangent(v1, v2, v3);
        vec3 face_binormal = triangle_binormal(v1, v2, v3);

        v1.tangent = vec3_add(face_tangent, v1.tangent);
        v2.tangent = vec3_add(face_tangent, v2.tangent);
        v3.tangent = vec3_add(face_tangent, v3.tangent);

        v1.binormal = vec3_add(face_binormal, v1.binormal);
        v2.binormal = vec3_add(face_binormal, v2.binormal);
        v3.binormal = vec3_add(face_binormal, v3.binormal);

        m->verticies[t_i1] = v1;
        m->verticies[t_i2] = v2;
        m->verticies[t_i3] = v3;

        i = i + 3;
    }

    /* normalize all tangents */
    for (int i = 0; i < m->num_verts; i++) {
        m->verticies[i].tangent = vec3_normalize(m->verticies[i].tangent);
        m->verticies[i].binormal = vec3_normalize(m->verticies[i].binormal);
    }
}

void mesh_generate_normals(mesh* m)
{
    /* Clear all normals to 0,0,0 */
    for (int i = 0; i < m->num_verts; i++) {
        m->verticies[i].normal = vec3_zero();
    }

    /* Loop over faces, calculate normals and append to verticies of that face */
    int i = 0;
    while (i < m->num_triangles * 3) {
        int t_i1 = m->triangles[i];
        int t_i2 = m->triangles[i + 1];
        int t_i3 = m->triangles[i + 2];

        vertex v1 = m->verticies[t_i1];
        vertex v2 = m->verticies[t_i2];
        vertex v3 = m->verticies[t_i3];

        vec3 face_normal = triangle_normal(v1, v2, v3);

        v1.normal = vec3_add(face_normal, v1.normal);
        v2.normal = vec3_add(face_normal, v2.normal);
        v3.normal = vec3_add(face_normal, v3.normal);

        m->verticies[t_i1] = v1;
        m->verticies[t_i2] = v2;
        m->verticies[t_i3] = v3;

        i = i + 3;
    }

    /* normalize all normals */
    for (int i = 0; i < m->num_verts; i++) {
        m->verticies[i].normal = vec3_normalize(m->verticies[i].normal);
    }
}

void mesh_generate_orthagonal_tangents(mesh* m)
{
    /* Clear all tangents to 0,0,0 */
    for (int i = 0; i < m->num_verts; i++) {
        m->verticies[i].tangent = vec3_zero();
        m->verticies[i].binormal = vec3_zero();
    }

    /* Loop over faces, calculate tangent and append to verticies of that face */
    int i = 0;
    while (i < m->num_triangles * 3) {
        int t_i1 = m->triangles[i];
        int t_i2 = m->triangles[i + 1];
        int t_i3 = m->triangles[i + 2];

        vertex v1 = m->verticies[t_i1];
        vertex v2 = m->verticies[t_i2];
        vertex v3 = m->verticies[t_i3];

        vec3 face_normal = triangle_normal(v1, v2, v3);
        vec3 face_binormal_temp = triangle_binormal(v1, v2, v3);

        vec3 face_tangent = vec3_normalize(vec3_cross(face_binormal_temp, face_normal));
        vec3 face_binormal = vec3_normalize(vec3_cross(face_tangent, face_normal));

        v1.tangent = vec3_add(face_tangent, v1.tangent);
        v2.tangent = vec3_add(face_tangent, v2.tangent);
        v3.tangent = vec3_add(face_tangent, v3.tangent);

        v1.binormal = vec3_add(face_binormal, v1.binormal);
        v2.binormal = vec3_add(face_binormal, v2.binormal);
        v3.binormal = vec3_add(face_binormal, v3.binormal);

        m->verticies[t_i1] = v1;
        m->verticies[t_i2] = v2;
        m->verticies[t_i3] = v3;

        i = i + 3;
    }

    /* normalize all tangents */
    for (int i = 0; i < m->num_verts; i++) {
        m->verticies[i].tangent = vec3_normalize(m->verticies[i].tangent);
        m->verticies[i].binormal = vec3_normalize(m->verticies[i].binormal);
    }
}

void mesh_generate_texcoords_cylinder(mesh* m)
{
    vec2 unwrap_vector = vec2_new(1, 0);

    float max_height = -99999999;
    float min_height = 99999999;

    for (int i = 0; i < m->num_verts; i++) {
        float v = m->verticies[i].position.y;
        max_height = max(max_height, v);
        min_height = min(min_height, v);

        vec2 proj_position = vec2_new(m->verticies[i].position.x, m->verticies[i].position.z);
        vec2 from_center = vec2_normalize(proj_position);
        float u = (vec2_dot(from_center, unwrap_vector) + 1) / 8;

        m->verticies[i].uvs = vec2_new(u, v);
    }

    float scale = (max_height - min_height);

    for (int i = 0; i < m->num_verts; i++) {
        m->verticies[i].uvs = vec2_new(m->verticies[i].uvs.x, m->verticies[i].uvs.y / scale);
    }
}

float mesh_surface_area(mesh* m)
{
    float total = 0.0;
    int i = 0;
    while (i < m->num_triangles * 3) {
        int t_i1 = m->triangles[i];
        int t_i2 = m->triangles[i + 1];
        int t_i3 = m->triangles[i + 2];

        vertex v1 = m->verticies[t_i1];
        vertex v2 = m->verticies[t_i2];
        vertex v3 = m->verticies[t_i3];

        total += triangle_area(v1, v2, v3);

        i = i + 3;
    }
    return total;
}

void mesh_translate(mesh* m, vec3 translation)
{
    int i = 0;
    while (i < m->num_triangles * 3) {
        int t_i1 = m->triangles[i];
        int t_i2 = m->triangles[i + 1];
        int t_i3 = m->triangles[i + 2];

        m->verticies[t_i1].position = vec3_add(m->verticies[t_i1].position, translation);
        m->verticies[t_i2].position = vec3_add(m->verticies[t_i2].position, translation);
        m->verticies[t_i3].position = vec3_add(m->verticies[t_i3].position, translation);

        i = i + 3;
    }
}

void mesh_scale(mesh* m, float scale)
{
    int i = 0;
    while (i < m->num_triangles * 3) {
        int t_i1 = m->triangles[i];
        int t_i2 = m->triangles[i + 1];
        int t_i3 = m->triangles[i + 2];

        m->verticies[t_i1].position = vec3_mul(m->verticies[t_i1].position, scale);
        m->verticies[t_i2].position = vec3_mul(m->verticies[t_i2].position, scale);
        m->verticies[t_i3].position = vec3_mul(m->verticies[t_i3].position, scale);

        i = i + 3;
    }
}

void mesh_transform(mesh* m, mat4 transform)
{

    int i = 0;
    while (i < m->num_triangles * 3) {
        int t_i1 = m->triangles[i];
        int t_i2 = m->triangles[i + 1];
        int t_i3 = m->triangles[i + 2];

        m->verticies[t_i1].position = mat4_mul_vec3(transform, m->verticies[t_i1].position);
        m->verticies[t_i2].position = mat4_mul_vec3(transform, m->verticies[t_i2].position);
        m->verticies[t_i3].position = mat4_mul_vec3(transform, m->verticies[t_i3].position);

        i = i + 3;
    }
}

sphere mesh_bounding_sphere(mesh* m)
{
    sphere s = sphere_new(vec3_zero(), 0);

    for (int i = 0; i < m->num_verts; i++)
        s.center = vec3_add(s.center, m->verticies[i].position);
    s.center = vec3_div(s.center, m->num_verts);

    for (int i = 0; i < m->num_verts; i++)
        s.radius = max(s.radius, vec3_dist(s.center, m->verticies[i].position));

    return s;
}

void model_print(model* m)
{
    for (int i = 0; i < m->num_meshes; i++) {
        mesh_print(m->meshes[i]);
    }
}

model* model_new()
{
    model* m = malloc(sizeof(model));
    m->num_meshes = 0;
    m->meshes = malloc(sizeof(mesh*) * m->num_meshes);
    return m;
}

void model_delete(model* m)
{
    for (int i = 0; i < m->num_meshes; i++)
        mesh_delete(m->meshes[i]);
    free(m);
}

void model_generate_normals(model* m)
{
    for (int i = 0; i < m->num_meshes; i++)
        mesh_generate_normals(m->meshes[i]);
}

void model_generate_tangents(model* m)
{
    for (int i = 0; i < m->num_meshes; i++)
        mesh_generate_tangents(m->meshes[i]);
}

void model_generate_orthagonal_tangents(model* m)
{
    for (int i = 0; i < m->num_meshes; i++)
        mesh_generate_orthagonal_tangents(m->meshes[i]);
}

void model_generate_texcoords_cylinder(model* m)
{
    for (int i = 0; i < m->num_meshes; i++)
        mesh_generate_texcoords_cylinder(m->meshes[i]);
}

float model_surface_area(model* m)
{
    float total = 0.0f;
    for (int i = 0; i < m->num_meshes; i++)
        total += mesh_surface_area(m->meshes[i]);
    return total;
}

void model_translate(model* m, vec3 translation)
{
    for (int i = 0; i < m->num_meshes; i++)
        mesh_translate(m->meshes[i], translation);
}

void model_scale(model* m, float scale)
{
    for (int i = 0; i < m->num_meshes; i++)
        mesh_scale(m->meshes[i], scale);
}

void model_transform(model* m, mat4 transform)
{
    for (int i = 0; i < m->num_meshes; i++)
        mesh_transform(m->meshes[i], transform);
}
