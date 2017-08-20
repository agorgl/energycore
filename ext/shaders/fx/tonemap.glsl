#version 330 core
out vec4 color;
uniform sampler2D tex;

//------------------------------------------------------------
// Reinhard
//------------------------------------------------------------
vec3 reinhard(vec3 hdr_color)
{
    return hdr_color / (hdr_color + vec3(1.0));
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
    hdr_color *= 16; // Hardcoded exposure adjustment
    float exposure_bias = 2.0f;
    vec3 curr = uncharted2tonemap(exposure_bias * hdr_color);
    vec3 white_scale = 1.0f / uncharted2tonemap(vec3(W));
    vec3 color = curr * white_scale;
    return color;
}

//------------------------------------------------------------
// Entrypoint
//------------------------------------------------------------
void main()
{
    vec3 hdr_color = texelFetch(tex, ivec2(gl_FragCoord.xy), 0).rgb;

    // Tone mapping types
    const int tonemap_type = 0;
    vec3 mapped = hdr_color;
    if (tonemap_type == 0)
        mapped = reinhard(hdr_color);
    else if (tonemap_type == 1)
        mapped = exposure(hdr_color, 1.0);
    else if (tonemap_type == 2)
        mapped = filmic(hdr_color);

    color = vec4(mapped, 1.0);
}
