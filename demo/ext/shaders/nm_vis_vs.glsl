#version 330 core
layout (location = 0) in vec3 position;
layout (location = 2) in vec3 normal;

uniform mat4 proj;
uniform mat4 view;
uniform mat4 model;

out VS_OUT {
    vec4 normal;
} vs_out;

void main()
{
    mat3 normal_mat = mat3(transpose(inverse(view * model)));
    vs_out.normal = normalize(proj * vec4(normal_mat * normal, 0.0));
    gl_Position = proj * view * model * vec4(position, 1.0);
}
