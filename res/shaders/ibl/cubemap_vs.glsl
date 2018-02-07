#version 330 core
layout (location = 0) in vec3 apos;
out vec3 ws_pos;

uniform mat4 proj;
uniform mat4 view;

void main()
{
    ws_pos = apos;
    gl_Position =  proj * view * vec4(ws_pos, 1.0);
}
