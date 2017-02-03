#version 330 core
#include "sh.glsl"
out vec4 color;

in VS_OUT {
    vec2 uv;
    vec3 normal;
    vec3 frag_pos;
} fs_in;

uniform vec3 sh_coeffs[SH_COEFF_NUM];
uniform vec3 probe_pos;

void main()
{
    vec3 norm = normalize(fs_in.normal);
    vec3 prob_dir = normalize(probe_pos - fs_in.frag_pos);
    float irradiance = max(dot(norm, prob_dir), 0.0);
    vec3 env_col = irradiance * sh_irradiance(norm, sh_coeffs);
    color = vec4(env_col, 1.0);
}
