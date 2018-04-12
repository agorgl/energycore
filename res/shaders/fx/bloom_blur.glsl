#version 330 core
out vec4 color;
uniform sampler2D tex;
uniform vec2 src_sz;
uniform vec2 dst_sz;
uniform bool downmode;

vec4 downsample(vec2 uv, vec2 halfpixel)
{
    vec4 sum = texture(tex, uv) * 4.0;
    sum += texture(tex, uv - halfpixel.xy);
    sum += texture(tex, uv + halfpixel.xy);
    sum += texture(tex, uv + vec2(halfpixel.x, -halfpixel.y));
    sum += texture(tex, uv - vec2(halfpixel.x, -halfpixel.y));
    return sum / 8.0;
}

vec4 upsample(vec2 uv, vec2 halfpixel)
{
    vec4 sum = texture(tex, uv + vec2(-halfpixel.x * 2.0, 0.0));
    sum += texture(tex, uv + vec2(-halfpixel.x, halfpixel.y)) * 2.0;
    sum += texture(tex, uv + vec2(0.0, halfpixel.y * 2.0));
    sum += texture(tex, uv + vec2(halfpixel.x, halfpixel.y)) * 2.0;
    sum += texture(tex, uv + vec2(halfpixel.x * 2.0, 0.0));
    sum += texture(tex, uv + vec2(halfpixel.x, -halfpixel.y)) * 2.0;
    sum += texture(tex, uv + vec2(0.0, -halfpixel.y * 2.0));
    sum += texture(tex, uv + vec2(-halfpixel.x, -halfpixel.y)) * 2.0;
    return sum / 12.0;
}

void main()
{
    // Real src texture size
    vec2 tex_sz = textureSize(tex, 0);
    // Real src texel size
    vec2 texel_size = 1.0 / tex_sz;
    // Target uv value
    vec2 coord = gl_FragCoord.xy / dst_sz;
    // Real uv value (src data use subarea of the real texture)
    coord = coord * (src_sz / tex_sz);

    vec4 result;
    if (downmode)
        result = downsample(coord, texel_size * 0.5);
    else
        result = upsample(coord, texel_size * 0.5);
    color = result;
}
