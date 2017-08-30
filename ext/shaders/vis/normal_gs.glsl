#version 330 core
layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

in VS_OUT {
    vec4 normal;
} gs_in[];

out GS_OUT {
    vec3 nm_col;
} gs_out;

const float MAGNITUDE = 0.15;
const vec3 vnm_colors[2] = vec3[](vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0));

void main()
{
    for (int i = 0; i < gl_in.length(); ++i) {
        gl_Position = gl_in[i].gl_Position;
        gs_out.nm_col = vnm_colors[0];
        EmitVertex();
        gl_Position = gl_in[i].gl_Position + gs_in[i].normal * MAGNITUDE;
        gs_out.nm_col = vnm_colors[1];
        EmitVertex();
        EndPrimitive();
    }
}
