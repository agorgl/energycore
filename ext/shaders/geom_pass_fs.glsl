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
    float roughness;
    sampler2D rough_tex;
    vec2 rough_scl;
    float metallic;
    sampler2D metal_tex;
    vec2 metal_scl;
};
uniform material mat;
uniform int use_nm;

void main()
{
    // Normal output
    vec3 tex_normal = texture(mat.normal_tex, fs_in.uv * mat.normal_scl).rgb;
    if (use_nm == 1)
        g_normal = normalize(fs_in.TBN * (tex_normal * 2.0 - 1.0));
    else
        g_normal = normalize(fs_in.normal);
    // Albedo output
    g_albedo = texture(mat.albedo_tex, fs_in.uv * mat.albedo_scl) + vec4(mat.albedo_col, 1.0);
    // Roughness / Metallic output
    vec4 tex_roughn = texture(mat.rough_tex, fs_in.uv * mat.rough_scl);
    float roughness = (1.0 - tex_roughn.a) + mat.roughness;
    vec4 tex_metal = texture(mat.metal_tex, fs_in.uv * mat.metal_scl);
    float metallic = tex_metal.r + mat.metallic;
    g_roughness_metallic = vec4(vec3(roughness), metallic);
}
