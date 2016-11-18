#version 330 core
in vec2 UV;
in vec3 Normal;
in vec3 FragPos;
out vec4 out_color;

const vec3 lightPos = vec3(-0.8, 1.0, -0.8);

void main()
{
    vec3 lightDir = normalize(lightPos - FragPos);
    float diffuse = max(dot(Normal, lightDir), 0.0);
    vec3 result = vec3(1.0, 1.0, 1.0) * diffuse;
    out_color = vec4(result, 1.0);
};
