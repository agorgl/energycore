#include "skyprth.h"
#include <string.h>
#include <math.h>
#include "opengl.h"
#include "glutils.h"

static vec3 view_target(mat4* view)
{
    vec3 f = vec3_new(-(*view).zx, -(*view).zy, -(*view).zz);
    return f;
}

void sky_preetham_init(struct sky_preetham* sp)
{
    memset(sp, 0, sizeof(*sp));
}

void sky_preetham_default_params(struct sky_preetham_params* params)
{
    params->luminance = 1.0f;
    params->turbidity = 2.0f;
    params->rayleigh = 1.0f;
    params->mie_coef = 0.005;
    params->mie_dirg = 0.8;
    params->inclination = 0.76;
    params->azimuth = 0.625;
}

void sky_preetham_render(struct sky_preetham* sp, struct sky_preetham_params* params, mat4* proj, mat4* view)
{
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);

    /* Render setup */
    glUseProgram(sp->shdr);

    vec3 vlook = view_target(view);
    glUniform3f(glGetUniformLocation(sp->shdr, "cam_dir"), vlook.x, vlook.y, vlook.z);
    glUniformMatrix4fv(glGetUniformLocation(sp->shdr, "proj"), 1, GL_FALSE, proj->m);

    /* Preetham sky model params */
    glUniform1f(glGetUniformLocation(sp->shdr, "luminance"), params->luminance);
    glUniform1f(glGetUniformLocation(sp->shdr, "turbidity"), params->turbidity);
    glUniform1f(glGetUniformLocation(sp->shdr, "rayleigh"), params->rayleigh);
    glUniform1f(glGetUniformLocation(sp->shdr, "mie_coef"), params->mie_coef);
    glUniform1f(glGetUniformLocation(sp->shdr, "mie_directional_g"), params->mie_dirg);

    /* Calculate sun position according to inclination and azimuth params */
    const unsigned int distance = 400000;
    const float theta = 2.0f * M_PI * (params->azimuth - 0.5);
    const float phi = M_PI * (params->inclination - 0.5);
    vec3 sun_pos = vec3_new(
        distance * sin(phi) * sin(theta),
        distance * cos(phi),
        distance * sin(phi) * cos(theta)
    );
    glUniform3f(glGetUniformLocation(sp->shdr, "sun_pos"), sun_pos.x, sun_pos.y, sun_pos.z);

    render_quad();
    glUseProgram(0);

    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
}

void sky_preetham_destroy(struct sky_preetham* sp)
{
    (void) sp;
}
