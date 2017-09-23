#include "girndr.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <glad/glad.h>
#include <emproc/sh.h>
#include "probe.h"
#include "gbuffer.h"
#include "glutils.h"

void gi_rndr_init(struct gi_rndr* r)
{
    memset(r, 0, sizeof(*r));
    /* Initialize subrenderers */
    r->probe_rndr = calloc(1, sizeof(struct probe_rndr));
    probe_rndr_init(r->probe_rndr);
    r->probe_vis = calloc(1, sizeof(struct probe_vis));
    probe_vis_init(r->probe_vis);
    /* Initialize probe processing */
    r->probe_proc = calloc(1, sizeof(struct probe_proc));
    probe_proc_init(r->probe_proc);
    /* Allocate probe array */
    r->pdata = calloc(1, sizeof(*(r->pdata)));
    /* Create mini gbuffer to update probes */
    r->probe_gbuf = malloc(sizeof(struct gbuffer));
    gbuffer_init(r->probe_gbuf, 128, 128);
}

void gi_rndr_destroy(struct gi_rndr* r)
{
    gbuffer_destroy(r->probe_gbuf);
    free(r->probe_gbuf);
    for (unsigned int i = 0; i < r->num_probes; ++i) {
        struct probe* p = r->pdata[i].p;
        probe_destroy(p);
        free(p);
    }
    free(r->pdata);
    probe_proc_destroy(r->probe_proc);
    free(r->probe_proc);
    probe_vis_destroy(r->probe_vis);
    free(r->probe_vis);
    probe_rndr_destroy(r->probe_rndr);
    free(r->probe_rndr);
}

void gi_add_probe(struct gi_rndr* r, vec3 pos)
{
    /* Expand array */
    r->num_probes++;
    r->pdata = realloc(r->pdata, r->num_probes * sizeof(*(r->pdata)));
    /* Create and append new probe */
    struct probe* np = calloc(1, sizeof(struct probe));
    probe_init(np);
    r->pdata[r->num_probes - 1].p = np;
    r->pdata[r->num_probes - 1].pos = pos;
}

void gi_update_begin(struct gi_rndr* r)
{
    r->rs.pidx = 0;
    r->rs.side = 0;
    probe_render_begin(r->probe_rndr);
}

int gi_update_pass_begin(struct gi_rndr* r, mat4* view, mat4* proj)
{
    /* No more probes */
    if (r->rs.pidx >= r->num_probes)
        return 0;
    struct gi_probe_data* pd = r->pdata + r->rs.pidx;
    probe_render_side_begin(r->probe_rndr, r->rs.side, pd->p, pd->pos, view, proj);
    return 1;
}

void gi_update_pass_end(struct gi_rndr* r)
{
    probe_render_side_end(r->probe_rndr, r->rs.side);
    r->rs.side++;
    if (r->rs.side >= 6) {
        /* Calculate SH coefficients from captured cubemap */
        struct gi_probe_data* pd = r->pdata + r->rs.pidx;
        probe_extract_shcoeffs(r->probe_proc, pd->sh_coeffs, pd->p);
        /* Advance probe */
        r->rs.pidx++;
        r->rs.side = 0;
    }
}

void gi_update_end(struct gi_rndr* r)
{
    /* End local cubemap rendering */
    probe_render_end(r->probe_rndr);
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

void gi_render(struct gi_rndr* r)
{
    struct gi_probe_data* pd = r->pdata + 0; // TODO
    /* Upload coeffs */
    upload_sh_coeffs(r->shdr, pd->sh_coeffs);
    /* Perform a full ndc quad render */
    glUseProgram(r->shdr);
    render_quad();
    glUseProgram(0);
}

void gi_vis_probes(struct gi_rndr* r, float view[16], float proj[16], unsigned int mode)
{
    for (unsigned int i = 0; i < r->num_probes; ++i) {
        struct gi_probe_data* pd = r->pdata + i;
        /* Visualize sample probe */
        upload_sh_coeffs(r->probe_vis->shdr, pd->sh_coeffs);
        probe_vis_render(r->probe_vis, pd->p, pd->pos, *(mat4*)view, *(mat4*)proj, mode);
    }
}
