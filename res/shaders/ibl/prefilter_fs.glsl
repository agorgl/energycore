#version 330 core
#include "../inc/light.glsl"
#include "../inc/sampling.glsl"
out vec4 fcolor;
in vec3 ws_pos;

uniform samplerCube env_map;
uniform float roughness;
uniform float map_res;

// ----------------------------------------------------------------------------
void main()
{
    vec3 N = normalize(ws_pos);

    // Make the simplyfying assumption that V equals R equals the normal
    vec3 R = N;
    vec3 V = R;

    const uint SAMPLE_COUNT = 1024u;
    vec3 prefiltered_color = vec3(0.0);
    float tot_weight = 0.0;
    for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
        // Generates a sample vector that's biased towards the preferred alignment direction (importance sampling).
        vec2 Xi = hammersley(i, SAMPLE_COUNT);
        vec3 H = importance_sample_ggx(Xi, N, roughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        if (NdotL > 0.0) {
            // Sample from the environment's mip level based on roughness/pdf
            float D = distribution_ggx(N, H, roughness);
            float NdotH = max(dot(N, H), 0.0);
            float HdotV = max(dot(H, V), 0.0);
            float pdf = D * NdotH / (4.0 * HdotV) + 0.0001;

            float sa_texel = 4.0 * PI / (6.0 * map_res * map_res);
            float sa_sample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);

            float mip_level = roughness == 0.0 ? 0.0 : 0.5 * log2(sa_sample / sa_texel);
            prefiltered_color += textureLod(env_map, L, mip_level).rgb * NdotL;
            tot_weight += NdotL;
        }
    }

    prefiltered_color = prefiltered_color / tot_weight;
    fcolor = vec4(prefiltered_color, 1.0);
}
