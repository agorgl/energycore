#version 330 core
out float color;
in vec2 uv;
uniform int blur_sz;
uniform sampler2D tex;

void main()
{
    vec2 texel_sz = 1.0 / textureSize(tex, 0);
    float result = 0.0;
    for (int x = -blur_sz / 2; x < blur_sz / 2; ++x) {
        for (int y = -blur_sz / 2; y < blur_sz / 2; ++y) {
            vec2 offs = vec2(float(x), float(y)) * texel_sz;
            result += texture(tex, uv + offs).r;
        }
    }
    color = result / (blur_sz * blur_sz);
}
