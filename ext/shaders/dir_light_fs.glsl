#version 330 core
#include "inc/deferred.glsl"
#include "inc/light.glsl"
#include "inc/shadow.glsl"
out vec4 color;

struct dir_light {
    vec3 direction;
    vec3 color;
};
uniform dir_light dir_l;
uniform vec3 view_pos;
uniform mat4 view;

uniform bool shadows_enabled;
uniform sampler2DArray shadowmap;
uniform shadow_cascade cascades[4];

void main()
{
    // Prologue
    fetch_gbuffer_data();

    // Light
    vec3 L = normalize(dir_l.direction);
    float light_intensity = 4.0;
    vec3 light_col = dir_l.color * light_intensity;
    float light_attenuation = 1.0;

    // Material
    vec3 albedo = d.albedo;
    float metallic = d.metallic;
    float roughness = d.roughness;

    // Directional lighting
    vec3 V = normalize(view_pos - d.ws_pos);
    vec3 Lo = radiance(d.normal, V, L,
                       light_col, light_attenuation,
                       albedo, metallic, roughness);

    // Shadow
    float shadow = shadows_enabled
        ? shadow_coef(shadowmap, cascades, d.ws_pos, d.normal, L, view)
        : 0.0;

    // Ambient
    vec3 ao = vec3(1.0);       // TODO
    vec3 amb_col = vec3(0.03); // TODO
    vec3 ambient = amb_col * albedo * ao;

    // Final
    vec3 result = ambient + Lo * (1.0 - shadow);

    color = vec4(result, 1.0);
}
