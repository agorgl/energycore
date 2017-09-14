#version 330 core
#include "inc/deferred.glsl"
#include "inc/sh.glsl"
#include "inc/light.glsl"

out vec4 color;

uniform vec3 view_pos;
uniform vec3 sh_coeffs[SH_COEFF_NUM];

void main()
{
    // Prologue
    fetch_gbuffer_data();

    // SH Lighting
    if (d.normal == vec3(0.0))
        discard;
    vec3 env_col = sh_irradiance(d.normal, sh_coeffs);

    vec3 V = normalize(view_pos - d.ws_pos);
    vec3 amb = env_radiance(d.normal, V,
                            d.albedo, d.metallic, d.roughness,
                            env_col);
    color = vec4(amb, 1.0);
}
