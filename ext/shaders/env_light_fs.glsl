#version 330 core
#include "inc/deferred.glsl"
#include "inc/light.glsl"
out vec4 color;

void main()
{
    // Prologue
    fetch_gbuffer_data();

    // Material
    vec3 albedo = d.albedo;
    float metallic = d.metallic;
    float roughness = d.roughness;

    // Ambient
    vec3 ao = vec3(1.0);       // TODO
    vec3 amb_col = vec3(0.03); // TODO
    vec3 ambient = amb_col * albedo * ao;

    color = vec4(ambient, 1.0);
}
