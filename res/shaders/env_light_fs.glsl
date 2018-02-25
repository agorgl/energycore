#version 330 core
#include "inc/deferred.glsl"
#include "inc/light.glsl"
#ifdef SH
#include "inc/sh.glsl"
#endif
out vec4 color;

uniform vec3 view_pos;

#ifndef SH
uniform samplerCube irr_map;
uniform samplerCube pf_map;
#else
uniform vec3 sh_coeffs[SH_COEFF_NUM];
#endif
uniform sampler2D brdf_lut;


void main()
{
    // Prologue
    fetch_gbuffer_data();

    // Material
    vec3 albedo = d.albedo;
    float metallic = d.metallic;
    float roughness = d.roughness;

    // Unset gbuf fragments
    if (d.normal == vec3(0.0))
        discard;

#ifdef SH
    // Diffuse irradiance from SH coeffs
    vec3 env_col = sh_irradiance(d.normal, sh_coeffs);
#endif

    // View vector
    vec3 V = normalize(view_pos - d.ws_pos);

    // Ambient
    vec3 ao = vec3(1.0); // TODO
    float intensity = 0.2;
    vec3 environ = env_radiance(
        d.normal, V, d.albedo, d.metallic,
        d.roughness, irr_map, pf_map, brdf_lut) * ao;

    color = vec4(intensity * environ, 1.0);
}
