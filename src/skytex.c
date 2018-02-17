#include "skytex.h"
#include "opengl.h"
#include "glutils.h"

static const char* skybox_vs_src = GLSRC(
layout (location = 0) in vec3 position;

out VS_OUT {
    vec3 tex_coords;
} vs_out;

uniform mat4 proj;
uniform mat4 view;

void main()
{
    vs_out.tex_coords = vec3(position.x, -position.y, position.z);
    vec4 pos = proj * view * vec4(position, 1.0);
    gl_Position = pos.xyww;
}
);

static const char* skybox_fs_src = GLSRC(
out vec4 fcolor;

in VS_OUT {
    vec3 tex_coords;
} fs_in;

uniform samplerCube sky_tex;

void main()
{
    vec3 color = texture(sky_tex, fs_in.tex_coords).rgb;
    fcolor = vec4(color, 1.0);
}
);

void sky_texture_init(struct sky_texture* st)
{
    /* Build shader */
    st->shdr = shader_from_srcs(skybox_vs_src, 0, skybox_fs_src);
}

void sky_texture_render(struct sky_texture* st, mat4* view, mat4* proj, unsigned int cubemap)
{
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    glUseProgram(st->shdr);

    /* Remove any translation component of the view matrix */
    mat4 nt_view = mat3_to_mat4(mat4_to_mat3(*view));
    glUniformMatrix4fv(glGetUniformLocation(st->shdr, "proj"), 1, GL_FALSE, proj->m);
    glUniformMatrix4fv(glGetUniformLocation(st->shdr, "view"), 1, GL_FALSE, nt_view.m);
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(glGetUniformLocation(st->shdr, "sky_tex"), 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
    render_cube();
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    glUseProgram(0);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
}

void sky_texture_destroy(struct sky_texture* st)
{
    glDeleteProgram(st->shdr);
}
