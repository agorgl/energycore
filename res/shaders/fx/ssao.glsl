#version 330 core
#include "../inc/deferred.glsl"
out float color;
in vec2 uv;

#define MAX_SAMPLES 64

uniform mat4 view;
uniform mat4 proj;
uniform vec3 samples[MAX_SAMPLES];
uniform int kernel_sz;
uniform sampler2D tex_noise;

const float radius = 0.5;
const float bias = 0.025;

void main()
{
    // Prologue
    fetch_gbuffer_data();

    // Get input for SSAO algorithm
    vec3 frag_pos = (view * vec4(d.ws_pos, 1.0)).xyz;
    vec3 normal = normalize(d.normal);
    vec2 noise_scale = textureSize(gbuf.depth, 0) / textureSize(tex_noise, 0);
    vec3 rand_vec = normalize(texture(tex_noise, uv * noise_scale).xyz);

    // Create TBN change-of-basis matrix: from tangent-space to view-space
    vec3 tangent = normalize(rand_vec - normal * dot(rand_vec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    // Iterate over the sample kernel and calculate occlusion factor
    float occlusion = 0.0;
    for (int i = 0; i < kernel_sz; ++i) {
        // Get sample position
        vec3 sample = TBN * samples[i]; // from tangent to view-space
        sample = frag_pos + sample * radius;

        // Project sample position (to sample texture) (to get position on screen/texture)
        vec4 offs = vec4(sample, 1.0);
        offs = proj * offs;               // From view to clip-space
        offs.xyz /= offs.w;               // Perspective divide
        offs.xyz  = offs.xyz * 0.5 + 0.5; // Transform to [0.0,1.0]

        // Get depth value of kernel sample
        vec3 spos = (view * vec4(gbuffer_position(offs.xy), 1.0)).xyz;
        float sample_depth = spos.z;

        // Range check & accumulate
        float range_check = smoothstep(0.0, 1.0, radius / abs(frag_pos.z - sample_depth));
        occlusion += (sample_depth >= sample.z + bias ? 1.0 : 0.0) * range_check;
    }
    occlusion = 1.0 - (occlusion / kernel_sz);

    color = occlusion;
}
