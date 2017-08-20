#version 330 core
#include "inc/deferred.glsl"
#include "inc/sh.glsl"

out vec4 color;

uniform vec3 sh_coeffs[SH_COEFF_NUM];

void main()
{
    // Prologue
    fetch_gbuffer_data();

    // SH Lighting
    if (d.normal == vec3(0.0))
        discard;
    vec3 env_col = sh_irradiance(d.normal, sh_coeffs);
    color = vec4(env_col, 1.0);
}
