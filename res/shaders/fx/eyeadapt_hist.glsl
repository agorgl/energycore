#version 430 core
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
layout(rgba16f, binding = 0) uniform image2D img;
#include "eyeadapt_buf.glsl"

shared uint group_histogram[NUM_HISTOGRAM_BINS];

// Returns position in [0,1] range
float histogram_position(float lum)
{
    float lum_min = lum_range.x;
    float lum_max = lum_range.y;
    // (lum - lum_min) / (lum_max - lum_min)
    float multiply = 1.0 / (lum_max - lum_min);
    float add = -lum_min * multiply;
    return clamp(log2(lum) * multiply + add, 0.0, 1.0);
}

void main()
{
    // Initialise the contents of the group shared memory.
    if (gl_LocalInvocationIndex < NUM_HISTOGRAM_BINS)
        group_histogram[gl_LocalInvocationIndex] = 0;
    memoryBarrierShared();
    barrier();

    // It is possible that we emit more threads than we have pixels.
    // This is caused due to rounding up an image to a multiple of the tile size
    vec2 img_sz = imageSize(img);
    const uint num_fetches = 16;
    for (uint i = 0; i < num_fetches; ++i) {
        uint x = gl_WorkGroupID.x * gl_WorkGroupSize.x * num_fetches + gl_LocalInvocationID.x + i * num_fetches;
        uint y = gl_WorkGroupID.y * gl_WorkGroupSize.y + gl_LocalInvocationID.y;
        ivec2 sample_point = ivec2(x, y);
        if (sample_point.x < img_sz.x && sample_point.y < img_sz.y) {
            vec3 c = imageLoad(img, sample_point).rgb;
            //float lum = dot(c, vec3(0.2126, 0.7152, 0.0722));
            float lum = max(max(c.x, c.y), c.z);
            uint bin = uint(histogram_position(lum) * float(NUM_HISTOGRAM_BINS - 1));
            atomicAdd(group_histogram[bin], 1);
        }
    }
    groupMemoryBarrier();
    barrier();

    // Adds this group's histogram to the global buffer
    if (gl_LocalInvocationIndex < NUM_HISTOGRAM_BINS)
        atomicAdd(histogram[gl_LocalInvocationIndex], group_histogram[gl_LocalInvocationIndex]);
}
