#version 330 core
out vec4 color;

in VS_OUT {
    vec2 uv;
    vec3 normal;
    vec3 frag_pos;
} fs_in;

uniform sampler2D albedo_tex;
uniform vec3 albedo_col;

const vec3 light_pos = vec3(0.8, 1.0, 0.8);

void main()
{
    // Inputs
    vec3 diff_col = texture2D(albedo_tex, fs_in.uv).rgb + albedo_col;
    vec3 norm = normalize(fs_in.normal);
    vec3 light_dir = normalize(light_pos);

    // Directional lighting
    float Kd = max(dot(norm, light_dir), 0.0);
    vec3 result = Kd * diff_col;
    color = vec4(result, 1.0);
}
