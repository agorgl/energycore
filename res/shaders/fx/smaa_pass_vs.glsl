#version 330 core

uniform sampler2D input_tex;
vec2 tex_size = vec2(textureSize(input_tex, 0));

#define SMAA_RT_METRICS vec4(1.0/tex_size,tex_size)
#define SMAA_INCLUDE_PS 0
#define SMAA_GLSL_3 1
#define SMAA_PRESET_ULTRA 1
#include "../inc/smaa.glsl"

layout(location = 0) in vec3 position;

noperspective out vec2 texcoord;
noperspective out vec2 pixcoord;
noperspective out vec4 offset[3];

/*
 * ustep
 * 0: Edge Detection
 * 1: Weight Calculation
 * 2: Blending Weight
 */
uniform int ustep;

void main()
{
    texcoord = position.xy * 0.5 + 0.5;
    if (ustep == 0)
        SMAAEdgeDetectionVS(texcoord, offset);
    else if (ustep == 1)
        SMAABlendingWeightCalculationVS(texcoord, pixcoord, offset);
    else if (ustep == 2)
        SMAANeighborhoodBlendingVS(texcoord, offset[0]);
    gl_Position = vec4(position.xy, 0.0, 1.0);
}
