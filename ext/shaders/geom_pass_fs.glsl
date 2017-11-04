#version 330 core
#include "inc/encoding.glsl"
#include "inc/blending.glsl"
#include "inc/packing.glsl"
#include "inc/convert.glsl"

layout (location = 0) out vec2 g_normal;
layout (location = 1) out vec4 g_albedo;
layout (location = 2) out vec2 g_roughness_metallic;

in VS_OUT {
    vec2 uv;
    vec3 normal;
    vec3 frag_pos;
    mat3 TBN;
} fs_in;

struct material_tex {
    sampler2D tex;
    vec2 scl;
    int use;
};

struct material {
    /* Albedo / Diffuse */
    material_tex albedo_map;
    vec3 albedo_col;
    /* Normal */
    material_tex normal_map;
    /* Roughness / Glossiness */
    material_tex roughness_map;
    int glossiness_mode;
    float roughness;
    /* Metallic / Specular */
    material_tex metallic_map;
    int specular_mode;
    float metallic;
    /* Detail Albedo */
    material_tex detail_albedo_map;
    /* Detail Normal */
    material_tex detail_normal_map;
};
uniform material mat;

vec3 unpack_tex_normal(vec2 tex_normal)
{
    vec3 n;
    n.xy = tex_normal * 2.0 - 1.0;
    n.z = sqrt(1.0 - (n.x * n.x + n.y * n.y));
    return n;
}

void main()
{
    // Gather texture data
    vec2 uv = fs_in.uv;
    vec4 tex_albedo = texture(mat.albedo_map.tex,    uv * mat.albedo_map.scl);
    vec2 tex_normal = texture(mat.normal_map.tex,    uv * mat.normal_map.scl).rg;
    vec4 tex_roughn = texture(mat.roughness_map.tex, uv * mat.roughness_map.scl);
    vec4 tex_metal  = texture(mat.metallic_map.tex,  uv * mat.metallic_map.scl);
    vec4 tex_dalbd  = texture(mat.detail_albedo_map.tex, uv * mat.detail_albedo_map.scl);
    vec2 tex_dnorm  = texture(mat.detail_normal_map.tex, uv * mat.detail_normal_map.scl).rg;

    // Convert between workflows
    metallic_roughness mr;
    if (mat.specular_mode == 1) {
        specular_glossiness sg;
        sg.diffuse    = tex_albedo.rgb;
        sg.specular   = tex_metal.rgb;
        sg.glossiness = tex_roughn.a;
        mr = specular_to_metalness(sg);
    } else {
        mr.basecolor = tex_albedo.rgb;
        mr.metallic  = tex_metal.r;
        mr.roughness = tex_roughn.a;
    }
    float rn_const  = mix(mat.roughness, 1.0 - mat.roughness, mat.glossiness_mode);

    // Extract values
    vec3 albedo     = mix(mat.albedo_col, mr.basecolor, mat.albedo_map.use);
    float metallic  = mix(mat.metallic, mr.metallic, mat.metallic_map.use);
    float roughness = mix(rn_const, mr.roughness, mat.roughness_map.use);

    // Perceived to actual roughness
    roughness = roughness * roughness;

    // Combine albedo maps
    if (mat.detail_albedo_map.use == 1)
        albedo = albedo * tex_dalbd.rgb * 2.0;

    // Gamma
    const float gamma = 2.2;
    albedo.rgb = pow(albedo.rgb, vec3(gamma));

    // Normal output
    vec3 n = normalize(fs_in.normal);
    if (mat.normal_map.use == 1) {
        n = unpack_tex_normal(tex_normal);
        if (mat.detail_normal_map.use == 1) {
            vec3 dn = unpack_tex_normal(tex_dnorm);
            n = blend_normals(n, dn);
        }
        n = normalize(fs_in.TBN * n);
    }

    // GBuffer outputs
    g_normal.rg = pack_normal_octahedron(n);
    g_albedo = vec4(albedo, 1.0);
    g_roughness_metallic.rg = vec2(roughness, metallic);
}
