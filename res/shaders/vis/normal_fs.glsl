#version 330 core
out vec4 out_color;

in GS_OUT {
    vec3 nm_col;
} fs_in;

void main()
{
    out_color = vec4(fs_in.nm_col, 1.0);
}
