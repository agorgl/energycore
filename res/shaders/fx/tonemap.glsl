#version 430 core
out vec4 color;
uniform sampler2D tex;
#include "eyeadapt_buf.glsl"

//------------------------------------------------------------
// Helpers
//------------------------------------------------------------
float luminance(vec3 c)
{
    float lum = dot(c, vec3(0.2126, 0.7152, 0.0722));
    return lum;
}

//------------------------------------------------------------
// Linear
//------------------------------------------------------------
vec3 linear(vec3 hdr_color, float exposure)
{
    vec3 c = clamp(exposure * hdr_color, 0.0, 1.0);
    return c;
}

//------------------------------------------------------------
// Reinhard
//------------------------------------------------------------
vec3 reinhard_simple(vec3 hdr_color, float exposure)
{
    vec3 c = vec3(exposure) * hdr_color / (vec3(1.0) + hdr_color / vec3(exposure));
    return c;
}

vec3 reinhard_luma(vec3 hdr_color)
{
    float luma = luminance(hdr_color);
    float tonemapped_luma = luma / (1.0 + luma);
    vec3 c = hdr_color * (tonemapped_luma / luma);
    return c;
}

vec3 reinhard_white_preserving_luma(vec3 hdr_color)
{
    float white = 2.0;
    float luma = luminance(hdr_color);
    float tonemapped_luma = luma * (1.0 + luma / (white * white)) / (1.0 + luma);
    vec3 c = hdr_color * (tonemapped_luma / luma);
    return c;
}

//------------------------------------------------------------
// Exposure
//------------------------------------------------------------
vec3 exposure(vec3 hdr_color, float exposure)
{
    return vec3(1.0) - exp(-hdr_color * exposure);
}

//------------------------------------------------------------
// Filmic
//------------------------------------------------------------
const float A = 0.15;
const float B = 0.50;
const float C = 0.10;
const float D = 0.20;
const float E = 0.02;
const float F = 0.30;
const float W = 11.2;

vec3 uncharted2tonemap(vec3 x)
{
    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

vec3 filmic(vec3 hdr_color)
{
    // Filmic tone mapping http://filmicworlds.com/blog/filmic-tonemapping-operators/
    //hdr_color *= 16; // Hardcoded exposure adjustment
    float exposure_bias = 2.0f;
    vec3 curr = uncharted2tonemap(exposure_bias * hdr_color);
    vec3 white_scale = 1.0f / uncharted2tonemap(vec3(W));
    vec3 color = curr * white_scale;
    return color;
}

//------------------------------------------------------------
// ACES
//------------------------------------------------------------
vec3 aces_film(vec3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

//------------------------------------------------------------
// Entrypoint
//------------------------------------------------------------
void main()
{
    vec3 hdr_color = texelFetch(tex, ivec2(gl_FragCoord.xy), 0).rgb;
    float exposure_value = exposure_cur;
    hdr_color *= exposure_value;

    // Tone mapping types
    const int tonemap_type = 3;
    vec3 mapped = hdr_color;
    if (tonemap_type == 0)
        mapped = reinhard_white_preserving_luma(hdr_color);
    else if (tonemap_type == 1)
        mapped = exposure(hdr_color, 1.0);
    else if (tonemap_type == 2)
        mapped = filmic(hdr_color);
    else if (tonemap_type == 3)
        mapped = aces_film(hdr_color);

    color = vec4(mapped, 1.0);
}
