#version 330 core
#include "sh.glsl"
out vec4 color;

in VS_OUT {
    vec2 uv;
    vec3 normal;
    vec3 frag_pos;
} fs_in;

uniform sampler2D albedo_tex;
uniform vec3 albedo_col;
uniform vec3 sh_coeffs[SH_COEFF_NUM];
uniform vec3 probe_pos;
uniform int render_mode;
const vec3 light_pos = vec3(0.8, 1.0, 0.8);

void main()
{
    vec3 diff_col = texture2D(albedo_tex, fs_in.uv).rgb + albedo_col;
    vec3 norm = normalize(fs_in.normal);
    if (render_mode == 0) {
        // Normal mode
        vec3 light_dir = normalize(light_pos);
        float Kd = max(dot(norm, light_dir), 0.0);
        vec3 result = Kd * diff_col;
        color = vec4(result, 1.0);
    } else if (render_mode == 1) {
        // Direct light
        vec3 light_dir = normalize(light_pos);
        float Kd = max(dot(norm, light_dir), 0.0);
        vec3 dir_col = Kd * diff_col;
        // Indirect light
        vec3 prob_dir = normalize(probe_pos - fs_in.frag_pos);
        float irradiance = max(dot(norm, prob_dir), 0.0);
        vec3 env_col = irradiance * sh_irradiance(norm, sh_coeffs);
        color = vec4(dir_col + env_col, 1.0);
    }
}
