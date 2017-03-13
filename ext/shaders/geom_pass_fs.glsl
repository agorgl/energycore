#version 330 core
#include "inc/encoding.glsl"
layout (location = 0) out vec3 g_normal;
layout (location = 1) out vec4 g_albedo;
layout (location = 2) out vec4 g_roughness_metallic;

in VS_OUT {
    vec2 uv;
    vec3 normal;
    vec3 frag_pos;
    mat3 TBN;
} fs_in;

struct material {
    sampler2D albedo_tex;
    vec2 albedo_scl;
    vec3 albedo_col;
    sampler2D normal_tex;
    vec2 normal_scl;
};
uniform material mat;
uniform int use_nm;

void main()
{
    vec3 tex_normal = texture(mat.normal_tex, fs_in.uv * mat.normal_scl).rgb;
    if (use_nm == 1)
        g_normal = normalize(fs_in.TBN * (tex_normal * 2.0 - 1.0));
    else
        g_normal = normalize(fs_in.normal);
    g_albedo = texture(mat.albedo_tex, fs_in.uv * mat.albedo_scl) + vec4(mat.albedo_col, 1.0);
    g_roughness_metallic = vec4(unpack3(0.8), 0.0);
}
