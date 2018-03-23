#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec3 tangent;

out VS_OUT {
    vec2 uv;
    vec3 normal;
    vec3 frag_pos;
    mat3 TBN;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main()
{
    // Construct TBN Matrix
    vec3 T = normalize(vec3(model * vec4(tangent, 0.0)));
    vec3 N = normalize(vec3(model * vec4(normal,  0.0)));
    // Re-orthogonalize T with respect to N
    T = normalize(T - dot(T, N) * N);
    // Then retrieve perpendicular vector B with the cross product of T and N
    vec3 B = cross(N, T);
    // TBN must form a right handed coord system.
    // Some models have symmetric UVs. Check and fix.
    if (dot(cross(N, T), B) < 0.0)
        T = T * (-1.0);
    mat3 TBN = mat3(T, B, N);
    vs_out.uv = uv;
    vs_out.normal = mat3(transpose(inverse(model))) * normal;
    vs_out.frag_pos = vec3(model * vec4(position, 1.0));
    vs_out.TBN = TBN;
    gl_Position = proj * view * model * vec4(position, 1.0);
}
