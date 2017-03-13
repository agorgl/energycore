//
// encoding.glsl
//

float pack3(vec3 val)
{
    return val.r + val.g * 256.0 + val.b * 256.0 * 256.0;
}

vec3 unpack3(float f)
{
    vec3 val;
    val.b = floor(f / 256.0 / 256.0);
    val.g = floor((f - val.b * 256.0 * 256.0) / 256.0);
    val.r = floor(f - val.b * 256.0 * 256.0 - val.g * 256.0);
    return val / 255.0;
}
