#version 330 core
layout (location = 0) out vec3 g_normal;
layout (location = 1) out vec4 g_albedo;

in VS_OUT {
    vec2 uv;
    vec3 normal;
    vec3 frag_pos;
} fs_in;

struct material {
    sampler2D albedo_tex;
    vec2 albedo_scl;
    vec3 albedo_col;
};
uniform struct material mat;

void main()
{
    g_normal = normalize(fs_in.normal);
    g_albedo = texture(mat.albedo_tex, fs_in.uv * mat.albedo_scl) + vec4(mat.albedo_col, 1.0);
}
