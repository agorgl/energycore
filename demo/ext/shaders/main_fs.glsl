#version 330 core
in vec2 UV;
out vec4 out_color;

void main()
{
    vec3 Color = vec3(1.0, 1.0, 1.0);
    out_color = vec4(Color, 1.0);
};
