#version 330 core
#include "inc/sh.glsl"
out vec4 color;

in VS_OUT {
    vec2 uv;
    vec3 normal;
    vec3 frag_pos;
    mat3 TBN;
} fs_in;

uniform int u_mode;
uniform vec3 u_view_pos;
uniform samplerCube u_envmap;
uniform vec3 sh_coeffs[SH_COEFF_NUM];

void main()
{
    vec3 norm = normalize(fs_in.normal);
    if (u_mode == 0) {
        // Reflective mode
        vec3 view_dir = normalize(fs_in.frag_pos - u_view_pos);
        vec3 refl_dir = reflect(view_dir, norm).xyz;
        vec3 refl_col = texture(u_envmap, refl_dir).xyz;
        color = vec4(refl_col, 1.0);
    } else if (u_mode == 1) {
        // SH irradiance mode
        vec3 env_col = sh_irradiance(norm, sh_coeffs);
        color = vec4(env_col, 1.0);
    }
}
