#version 330 core
out vec4 color;

in VS_OUT {
    vec2 uv;
    vec3 normal;
    vec3 frag_pos;
} fs_in;

uniform samplerCube sky;
uniform vec3 view_pos;
uniform int render_mode;
const vec3 light_pos = vec3(0.8, 1.0, 0.8);

void main()
{
    vec3 norm = normalize(fs_in.normal);
    if (render_mode == 0) {
        // Normal mode
        vec3 light_dir = normalize(light_pos - fs_in.frag_pos);
        float diffuse = max(dot(norm, light_dir), 0.0);
        vec3 result = vec3(1.0, 1.0, 1.0) * diffuse;
        color = vec4(result, 1.0);
    } else if (render_mode == 1) {
        // Skybox mode
        vec3 norm_y_inv = vec3(norm.x, -norm.y, norm.z);
        vec3 skycol = texture(sky, norm_y_inv).xyz;
        color = vec4(skycol, 1.0);
    } else if (render_mode == 2) {
        // Probe mode
        vec3 view_dir = normalize(fs_in.frag_pos - view_pos);
        vec3 refl_dir = reflect(view_dir, norm).xyz;
        vec3 refl_col = texture(sky, refl_dir).xyz;
        color = vec4(refl_col, 1.0);
    }
};
