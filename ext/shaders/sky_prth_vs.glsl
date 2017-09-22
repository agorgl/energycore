#version 330 core
layout (location = 0) in vec3 position;
out vec2 uv;

void main()
{
    // Screen space quad positions and uvs
    uv = position.xy;
    vec4 pos = vec4(position, 1.0);
    gl_Position = pos.xyww;
}
