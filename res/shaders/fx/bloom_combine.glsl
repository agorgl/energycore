#version 330 core
out vec4 color;
uniform sampler2D tex;
uniform sampler2D tex2;

void main()
{
    vec3 col = texelFetch(tex, ivec2(gl_FragCoord.xy), 0).rgb;
    vec3 col2 = texelFetch(tex2, ivec2(gl_FragCoord.xy), 0).rgb;
    color = vec4(col + col2, 1.0);
}
