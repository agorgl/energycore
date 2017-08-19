#include "sh_gi.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <glad/glad.h>
#include "lcl_cubemap.h"
#include "probe_vis.h"
#include "gbuffer.h"
#include "glutils.h"

void sh_gi_init(struct sh_gi_renderer* rs)
{
    /* Zero out mem */
    memset(rs, 0, sizeof(struct sh_gi_renderer));

    /* Initialize local cubemap renderer state */
    rs->lcr = malloc(sizeof(struct lc_renderer));
    lc_renderer_init(rs->lcr);
    rs->probe.cm = lc_create_cm(rs->lcr);
    rs->lcr_gbuf = malloc(sizeof(struct gbuffer));
    gbuffer_init(rs->lcr_gbuf, LCL_CM_SIZE, LCL_CM_SIZE);

    /* Initialize internal probe visualization state */
    rs->probe_vis = malloc(sizeof(struct probe_vis));
    probe_vis_init(rs->probe_vis);
}

void sh_gi_update_begin(struct sh_gi_renderer* shgi_rndr)
{
    /* Calculate probe position */
    shgi_rndr->angle += 0.01f;
    shgi_rndr->probe.pos = vec3_new(cosf(shgi_rndr->angle), 1.0f, sinf(shgi_rndr->angle));
    /* Start local cubemap rendering */
    shgi_rndr->rs.side = 0;
    lc_render_begin(shgi_rndr->lcr);
}

int sh_gi_update_pass_begin(struct sh_gi_renderer* shgi_rndr, mat4* view, mat4* proj)
{
    if (shgi_rndr->rs.side >= 6)
        return 0;
    lc_render_side_begin(shgi_rndr->lcr, shgi_rndr->rs.side, shgi_rndr->probe.cm, shgi_rndr->probe.pos, view, proj);
    return 1;
}

void sh_gi_update_pass_end(struct sh_gi_renderer* shgi_rndr)
{
    lc_render_side_end(shgi_rndr->lcr, shgi_rndr->rs.side);
    shgi_rndr->rs.side++;
}

void sh_gi_update_end(struct sh_gi_renderer* shgi_rndr)
{
    /* End local cubemap rendering */
    lc_render_end(shgi_rndr->lcr);
    /* Calculate SH coefficients from captured cubemap */
    lc_extract_sh_coeffs(shgi_rndr->lcr, shgi_rndr->probe.sh_coeffs, shgi_rndr->probe.cm);
}

static void upload_sh_coeffs(GLuint prog, double sh_coef[SH_COEFF_NUM][3])
{
    const char* uniform_name = "sh_coeffs";
    size_t uniform_name_len = strlen(uniform_name);
    glUseProgram(prog);
    for (unsigned int i = 0; i < SH_COEFF_NUM; ++i) {
        /* Construct uniform name ("sh_coeffs" + "[" + i + "]" + '\0') */
        size_t uname_sz = uniform_name_len + 1 + 2 + 1 + 1;
        char* uname = calloc(uname_sz, 1);
        strcat(uname, uniform_name);
        strcat(uname, "[");
        snprintf(uname + uniform_name_len + 1, 3, "%u", i);
        strcat(uname, "]");
        /* Upload */
        GLuint uloc = glGetUniformLocation(prog, uname);
        float c[3] = { (float)sh_coef[i][0], (float)sh_coef[i][1], (float)sh_coef[i][2] };
        glUniform3f(uloc, c[0], c[1], c[2]);
        free(uname);
    }
    glUseProgram(0);
}

void sh_gi_render(struct sh_gi_renderer* shgi_rndr)
{
    /* Disable depth writting */
    glDepthMask(GL_FALSE);
    /* Enable blend for additive lighting */
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE);

    /* Render environment light */
    upload_sh_coeffs(shgi_rndr->shdr, shgi_rndr->probe.sh_coeffs);
    glUseProgram(shgi_rndr->shdr);
    glUniform3fv(glGetUniformLocation(shgi_rndr->shdr, "probe_pos"), 1, shgi_rndr->probe.pos.xyz);

    /* Bind gbuffer textures and perform a full ndc quad render */
    glUseProgram(shgi_rndr->shdr);
    render_quad();
    glUseProgram(0);

    /* Restore gl values */
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
}

void sh_gi_vis_probes(struct sh_gi_renderer* shgi_rndr, float view[16], float proj[16], unsigned int mode)
{
    /* Visualize sample probe */
    upload_sh_coeffs(shgi_rndr->probe_vis->shdr, shgi_rndr->probe.sh_coeffs);
    probe_vis_render(shgi_rndr->probe_vis, shgi_rndr->probe.cm, shgi_rndr->probe.pos, *(mat4*)view, *(mat4*)proj, mode);
}

void sh_gi_destroy(struct sh_gi_renderer* rs)
{
    gbuffer_destroy(rs->lcr_gbuf);
    free(rs->lcr_gbuf);
    glDeleteTextures(1, &rs->probe.cm);

    lc_renderer_destroy(rs->lcr);
    free(rs->lcr);

    probe_vis_destroy(rs->probe_vis);
    free(rs->probe_vis);
}
