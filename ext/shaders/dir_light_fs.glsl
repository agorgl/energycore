#version 330 core
#include "deferred.glsl"
out vec4 color;

const vec3 light_pos = vec3(0.8, 1.0, 0.8);

void main()
{
    // Prologue
    fetch_gbuffer_data();

    // Directional lighting
    vec3 light_dir = normalize(light_pos);
    float Kd = max(dot(d.normal, light_dir), 0.0);
    vec3 result = Kd * d.albedo;
    color = vec4(result, 1.0);
}
