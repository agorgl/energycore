#version 330 core
#include "inc/sh.glsl"
#include "inc/deferred.glsl"

out vec4 color;

uniform vec3 sh_coeffs[SH_COEFF_NUM];
uniform vec3 probe_pos;

void main()
{
    // Prologue
    fetch_gbuffer_data();

    // SH Lighting
    vec3 prob_dir = normalize(probe_pos - d.ws_pos);
    float irradiance = max(dot(d.normal, prob_dir), 0.0);
    vec3 env_col = irradiance * sh_irradiance(d.normal, sh_coeffs);
    color = vec4(env_col, 1.0);
}
