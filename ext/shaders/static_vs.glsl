#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec3 normal;

out VS_OUT {
    vec2 uv;
    vec3 normal;
    vec3 frag_pos;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main()
{
    vs_out.uv = uv;
    vs_out.normal = mat3(transpose(inverse(model))) * normal;
    vs_out.frag_pos = vec3(model * vec4(position, 1.0));
    gl_Position = proj * view * model * vec4(position, 1.0);
}
