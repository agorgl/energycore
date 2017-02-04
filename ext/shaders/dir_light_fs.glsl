#version 330 core
#include "inc/deferred.glsl"
out vec4 color;

struct dir_light {
    vec3 direction;
    vec3 color;
};
uniform struct dir_light dir_l;

void main()
{
    // Prologue
    fetch_gbuffer_data();

    // Directional lighting
    vec3 light_dir = normalize(dir_l.direction);
    float Kd = max(dot(d.normal, light_dir), 0.0);
    vec3 result = Kd * d.albedo * dir_l.color;
    color = vec4(result, 1.0);
}
