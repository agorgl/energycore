#version 330 core
out vec4 color;
uniform sampler2D tex;
uniform float bloom_threshold;

void main()
{
    vec3 col = texelFetch(tex, ivec2(gl_FragCoord.xy), 0).rgb;
    // Check whether fragment output is higher than threshold, if so output as brightness color
    float brightness = dot(col, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > bloom_threshold)
        color = vec4(col, 1.0);
    else
        color = vec4(vec3(0.0), 1.0);
}
