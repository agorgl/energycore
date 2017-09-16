#version 330 core
#include "inc/deferred.glsl"
#include "inc/light.glsl"
out vec4 color;

uniform vec3 view_pos;

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

    // View vector
    vec3 V = normalize(view_pos - d.ws_pos);

    // Ambient
    vec3 ao = vec3(1.0); // TODO
    vec3 env_col = vec3(0.05);
    vec3 environ = env_radiance(
        d.normal, V, d.albedo, d.metallic,
        d.roughness, env_col) * ao;

    color = vec4(environ, 1.0);
}
