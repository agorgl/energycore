//
// sampling.glsl
//

// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
// Efficient VanDerCorput calculation.
float radical_inverse_vdc(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

float van_der_corput(uint n, uint base)
{
    float inv_base = 1.0 / float(base);
    float denom   = 1.0;
    float result  = 0.0;
    for(uint i = 0u; i < 32u; ++i) {
        if(n > 0u) {
            denom    = mod(float(n), 2.0);
            result  += denom * inv_base;
            inv_base = inv_base / 2.0;
            n        = uint(float(n) / 2.0);
        }
    }
    return result;
}

vec2 hammersley(uint i, uint N)
{
    return vec2(float(i) / float(N), radical_inverse_vdc(i));
}

vec2 hammersley_nobitops(uint i, uint N)
{
    return vec2(float(i) / float(N), van_der_corput(i, 2u));
}

// Create a uniform distributed direction (z-up) from the hammersley point
vec3 hemisphere_sample_uniform(float u, float v)
{
    float phi = v * 2.0 * PI;
    float cos_theta = 1.0 - u;
    float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
    return vec3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
}

// Create a cosinus distributed direction (z-up) from the hammersley point
vec3 hemisphere_sample_cos(float u, float v)
{
    float phi = v * 2.0 * PI;
    float cos_theta = sqrt(1.0 - u);
    float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
    return vec3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
}
// Note that the `1 - u` in the code is necessary to map the first point
// in the sequence to the up direction instead of the tangent plane

vec3 importance_sample_ggx(vec2 Xi, vec3 N, float roughness)
{
    float a = roughness * roughness;

    float phi = 2.0 * PI * Xi.x;
    float cos_theta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sin_theta = sqrt(1.0 - cos_theta * cos_theta);

    // From spherical coordinates to cartesian coordinates - Halfway vector
    vec3 H;
    H.x = cos(phi) * sin_theta;
    H.y = sin(phi) * sin_theta;
    H.z = cos_theta;

    // From tangent-space H vector to world-space sample vector
    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);

    vec3 sample_vec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sample_vec);
}
