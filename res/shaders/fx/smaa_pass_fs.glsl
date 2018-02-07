#version 330 core

uniform sampler2D input_tex;
vec2 tex_size = vec2(textureSize(input_tex, 0));

#define SMAA_RT_METRICS vec4(1.0/tex_size,tex_size)
#define SMAA_INCLUDE_VS 0
#define SMAA_GLSL_3 1
#define SMAA_PRESET_ULTRA 1
#include "../inc/smaa.glsl"

layout(location = 0) out vec4 color;

noperspective in vec2 texcoord;
noperspective in vec2 pixcoord;
noperspective in vec4 offset[3];

/*
 * ustep
 * 0: Edge Detection
 * 1: Weight Calculation
 * 2: Blending Weight
 */
uniform int ustep;
uniform sampler2D input_tex2;
uniform sampler2D input_tex3;

void main()
{
    if (ustep == 0)
        color = vec4(SMAAColorEdgeDetectionPS(texcoord, offset, input_tex), 0.0, 1.0);
    else if (ustep == 1)
        color = SMAABlendingWeightCalculationPS(texcoord, pixcoord, offset, input_tex, input_tex2, input_tex3, ivec4(0));
    else if (ustep == 2)
        color = SMAANeighborhoodBlendingPS(texcoord, offset[0], input_tex2, input_tex);
}
