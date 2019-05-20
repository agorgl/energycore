#version 430 core
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
layout(binding = 0) uniform sampler2D img;
#include "eyeadapt_buf.glsl"

const  bool vignette_weighting = false;
shared uint group_histogram[NUM_HISTOGRAM_BINS];

// Returns position in [0,1] range
float histogram_bin_from_luminance(float lum, float scale, float offs)
{
    // (lum - lum_min) / (lum_max - lum_min)
    float v = log2(lum) * scale + offs;
    return clamp(v, 0.0, 1.0);
}

void main()
{
    // Initialise the contents of the group shared memory.
    if (gl_LocalInvocationIndex < NUM_HISTOGRAM_BINS)
        group_histogram[gl_LocalInvocationIndex] = 0;
    memoryBarrierShared();
    barrier();

    // Precompute some parameters
    float one_over_preexposure = 1.0 / exposure_val;
    float scale = scale_offst.x;
    float offst = scale_offst.y;

    // It is possible that we emit more threads than we have pixels.
    // This is caused due to rounding up an image to a multiple of the tile size
    uvec2 ipos = gl_GlobalInvocationID.xy * 2;
    uvec2 img_sz = textureSize(img, 0);
    if (ipos.x < img_sz.x && ipos.y < img_sz.y) {
        uint weight = 1;
        vec2 sspos = ipos / vec2(img_sz);
        if (vignette_weighting) {
            vec2 d = abs(sspos - vec2(0.5).xx);
            float vfactor = clamp(1.0 - dot(d, d), 0.0, 1.0);
            vfactor *= vfactor;
            weight = uint(64.0 * vfactor);
        }
        vec3 c = texture(img, sspos).rgb;
        c *= one_over_preexposure;
        //float lum = max(c.x, max(c.y, c.z));
        float lum = dot(c, vec3(0.2126729, 0.7151522, 0.0721750));
        uint bin = uint(histogram_bin_from_luminance(lum, scale, offst) * float(NUM_HISTOGRAM_BINS - 1));
        atomicAdd(group_histogram[bin], weight);
    }
    groupMemoryBarrier();
    barrier();

    // Adds this group's histogram to the global buffer
    if (gl_LocalInvocationIndex < NUM_HISTOGRAM_BINS)
        atomicAdd(histogram[gl_LocalInvocationIndex], group_histogram[gl_LocalInvocationIndex]);
}
