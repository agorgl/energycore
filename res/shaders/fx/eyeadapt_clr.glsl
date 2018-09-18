#version 430 core
layout(local_size_x = 16, local_size_y = 1, local_size_z = 1) in;
#include "eyeadapt_buf.glsl"

void main()
{
    if (gl_GlobalInvocationID.x < NUM_HISTOGRAM_BINS)
        histogram[gl_GlobalInvocationID.x] = 0;
}
