#version 330 core
out vec4 color;
uniform sampler2D tex;

const float gamma = 2.2;

void main()
{
    vec3 col = texelFetch(tex, ivec2(gl_FragCoord.xy), 0).rgb;
    vec3 mapped = pow(col, vec3(1.0 / gamma));
    color = vec4(mapped, 1.0);
}
