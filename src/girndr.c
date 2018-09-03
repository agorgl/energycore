#include "girndr.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "opengl.h"
#include "probe.h"
#include "gbuffer.h"
#include "glutils.h"

void gi_rndr_init(struct gi_rndr* r)
{
    memset(r, 0, sizeof(*r));
    /* Initialize subrenderers */
    r->probe_rndr = calloc(1, sizeof(struct probe_rndr));
    probe_rndr_init(r->probe_rndr);
    /* Fallback probe */
    struct probe* fp = calloc(1, sizeof(struct probe));
    probe_init(fp);
    r->fallback_probe.p = fp;
    /* Allocate probe array */
    r->pdata = malloc(0);
    /* Create mini gbuffer to update probes */
    r->probe_gbuf = malloc(sizeof(struct gbuffer));
    gbuffer_init(r->probe_gbuf, 128, 128);
}

void gi_rndr_destroy(struct gi_rndr* r)
{
    gbuffer_destroy(r->probe_gbuf);
    free(r->probe_gbuf);
    probe_destroy(r->fallback_probe.p);
    free(r->fallback_probe.p);
    for (unsigned int i = 0; i < r->num_probes; ++i) {
        struct probe* p = r->pdata[i].p;
        probe_destroy(p);
        free(p);
    }
    free(r->pdata);
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

void gi_preprocess(struct gi_rndr* r, unsigned int irr_conv_shdr, unsigned int prefilter_shdr)
{
    probe_preprocess(r->fallback_probe.p, irr_conv_shdr, prefilter_shdr);
    probe_extract_shcoeffs(r->fallback_probe.sh_coeffs, r->fallback_probe.p);
    for (unsigned int i = 0; i < r->num_probes; ++i) {
        probe_preprocess(r->pdata[i].p, irr_conv_shdr, prefilter_shdr);
        probe_extract_shcoeffs(r->pdata[i].sh_coeffs, r->pdata[i].p);
    }
}

void gi_upload_sh_coeffs(unsigned int shdr, double sh_coef[25][3])
{
    const char* uniform_name = "sh_coeffs";
    size_t uniform_name_len = strlen(uniform_name);
    glUseProgram(shdr);
    for (unsigned int i = 0; i < 25; ++i) {
        /* Construct uniform name ("sh_coeffs" + "[" + i + "]" + '\0') */
        size_t uname_sz = uniform_name_len + 1 + 2 + 1 + 1;
        char* uname = calloc(uname_sz, 1);
        strcat(uname, uniform_name);
        strcat(uname, "[");
        snprintf(uname + uniform_name_len + 1, 3, "%u", i);
        strcat(uname, "]");
        /* Upload */
        GLuint uloc = glGetUniformLocation(shdr, uname);
        float c[3] = { (float)sh_coef[i][0], (float)sh_coef[i][1], (float)sh_coef[i][2] };
        glUniform3f(uloc, c[0], c[1], c[2]);
        free(uname);
    }
    glUseProgram(0);
}

void gi_vis_probes(struct gi_rndr* r, unsigned int shdr, float view[16], float proj[16], unsigned int mode)
{
    struct gi_probe_data* pd = &r->fallback_probe;
    gi_upload_sh_coeffs(shdr, pd->sh_coeffs);
    probe_vis_render(pd->p, pd->pos, shdr, *(mat4*)view, *(mat4*)proj, mode);
    for (unsigned int i = 0; i < r->num_probes; ++i) {
        struct gi_probe_data* pd = r->pdata + i;
        /* Visualize sample probe */
        gi_upload_sh_coeffs(shdr, pd->sh_coeffs);
        probe_vis_render(pd->p, pd->pos, shdr, *(mat4*)view, *(mat4*)proj, mode);
    }
}
