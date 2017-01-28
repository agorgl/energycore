#include "sky_preetham.h"
#include <string.h>
#include <math.h>
#include <glad/glad.h>
#include "glutils.h"

static const char* vs_src = GLSRC(
    layout (location = 0) in vec3 position;

    out VS_OUT {
        vec3  sun_direction;
        float sun_fade;
        vec3  beta_r;
        vec3  beta_m;
        float sun_e;
        vec2  uv;
    } vs_out;

    // Parameters
    uniform vec3 sun_position;
    uniform float rayleigh;
    uniform float turbidity;
    uniform float mie_coef;

    // Math constants
    const float e = 2.71828182845904523536028747135266249775724709369995957;
    const float pi = 3.141592653589793238462643383279502884197169;

    // Constants for atmospheric scattering
    // const float n = 1.0003;   // refractive index of air
    // const float N = 2.545E25; // number of molecules per unit volume for air at 288.15K and 1013mb (sea level -45 celsius)

    // Wavelength of used primaries, according to preetham
    // const vec3 lambda = vec3(680E-9, 550E-9, 450E-9);

    // This pre-calcuation replaces older total_rayleigh(vec3 lambda) function:
    // (8.0 * pow(pi, 3.0) * pow(pow(n, 2.0) - 1.0, 2.0) * (6.0 + 3.0 * pn)) / (3.0 * N * pow(lambda, vec3(4.0)) * (6.0 - 7.0 * pn))
    const vec3 total_rayleigh = vec3(5.804542996261093E-6, 1.3562911419845635E-5, 3.0265902468824876E-5);

    // Mie stuff
    // K coefficient for the primaries
    // const vec3 K = vec3(0.686, 0.678, 0.666);
    // const float v = 4.0;

    // Earth shadow hack
    const float cutoff_angle = 1.6110731556870734; // pi / 1.95;
    const float steepness = 1.5;
    // Sun intensity
    const float EE = 1000.0;

    float sun_intensity(float zenith_angle_cos)
    {
        zenith_angle_cos = clamp(zenith_angle_cos, -1.0, 1.0);
        return EE * max(0.0, 1.0 - pow(e, -((cutoff_angle - acos(zenith_angle_cos)) / steepness)));
    }

    vec3 total_mie(float T)
    {
        // Precalculated
        // mie_const = pi * pow((2.0 * pi) / lambda, vec3(v - 2.0)) * K
        const vec3 mie_const = vec3(1.8399918514433978E14, 2.7798023919660528E14, 4.0790479543861094E14);
        float c = (0.2 * T) * 10E-18;
        return 0.434 * c * mie_const;
    }

    void main()
    {
        vs_out.sun_direction = normalize(sun_position);
        // Sun intensity based on how high the sun is
        const vec3 up = vec3(0.0, 1.0, 0.0);
        vs_out.sun_e = sun_intensity(dot(vs_out.sun_direction, up));
        vs_out.sun_fade = 1.0 - clamp(1.0 - exp((sun_position.y / 450000.0)), 0.0, 1.0);

        // Extinction (absorbtion + out scattering)
        // Rayleigh coefficients
        float rayleigh_coef = rayleigh - (1.0 * (1.0 - vs_out.sun_fade));
        vs_out.beta_r = total_rayleigh * rayleigh_coef;
        // Mie coefficients
        vs_out.beta_m = total_mie(turbidity) * mie_coef;

        // Screen space quad positions and uvs
        vs_out.uv = position.xy;
        vec4 pos = vec4(position, 1.0);
        gl_Position = pos.xyww;
    }
);

static const char* fs_src = GLSRC(
    out vec4 color;

    in VS_OUT {
        vec3  sun_direction;
        float sun_fade;
        vec3  beta_r;
        vec3  beta_m;
        float sun_e;
        vec2  uv;
    } fs_in;

    uniform float luminance;
    uniform float mie_dirg;

    uniform mat4 proj;
    uniform vec3 cam_dir;

    // Math constants
    const float pi = 3.141592653589793238462643383279502884197169;
    const float THREE_OVER_SIXTEENPI = 0.05968310365946075; // 3.0 / (16.0 * pi)
    const float ONE_OVER_FOURPI = 0.07957747154594767;      // 1.0 / (4.0 * pi)

    // Optical length at zenith for molecules
    const float rayleigh_zenith_length = 8.4E3;
    const float mie_zenigth_length = 1.25E3;
    // 66 arc seconds -> degrees, and the cosine of that
    const float sun_angular_diameter_cos = 0.999956676946448443553574619906976478926848692873900859324;

    float rayleigh_phase(float cos_theta)
    {
        return THREE_OVER_SIXTEENPI * (1.0 + pow(cos_theta, 2.0));
    }

    float hg_phase(float cos_theta, float g)
    {
        float g2 = pow(g, 2.0);
        float inverse = 1.0 / pow(1.0 - 2.0 * g * cos_theta + g2, 1.5);
        return ONE_OVER_FOURPI * ((1.0 - g2) * inverse);
    }

    // Filmic ToneMapping http://filmicgames.com/archives/75
    const float A = 0.15;
    const float B = 0.50;
    const float C = 0.10;
    const float D = 0.20;
    const float E = 0.02;
    const float F = 0.30;
    // 1.0 / uncharted2_tonemap(1000.0)
    // (EE (Sun intensity) = 1000.0 in vs)
    const float whitescale = 1.0748724675633854;

    vec3 uncharted2_tonemap(vec3 x)
    {
        return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
    }

    /* Direction from projection, view direction and uv ([-1, 1]) */
    vec3 dir_from_pvdr(mat4 proj, vec3 view_dir, vec2 st)
    {
        vec2 persp = vec2(1 / proj[0][0], 1 / proj[1][1]);
        vec3 cforward = normalize(view_dir);
        vec3 nup;
        const float eps = 10e-6;
        if(abs(cforward.x) < eps && abs(cforward.z) < eps) {
            if(cforward.y > 0)
                nup = vec3(0, 0, -1);
            else
                nup = vec3(0, 0, 1);
        } else {
            nup = vec3(0, 1, 0);
        }
        vec3 cright = normalize(cross(cforward, nup));
        vec3 cup = normalize(cross(cright, cforward));
        vec3 direction = normalize(persp.x * cright * st.x + persp.y * cup * st.y + cforward);
        return direction;
    }

    void main()
    {
        // Calc look direction
        vec2 st = vec2(fs_in.uv.x, fs_in.uv.y);
        vec3 direction = dir_from_pvdr(proj, cam_dir, st);

        // Common variables
        const vec3 up = vec3(0.0, 1.0, 0.0);
        float cos_theta = dot(direction, fs_in.sun_direction);

        // Optical length
        // Cutoff angle at 90 to avoid singularity in next formula.
        float zenith_angle = acos(max(0.0, dot(up, direction)));
        float inverse = 1.0 / (cos(zenith_angle) + 0.15 * pow(93.885 - ((zenith_angle * 180.0) / pi), -1.253));
        float sr = rayleigh_zenith_length * inverse;
        float sm = mie_zenigth_length * inverse;

        // Combined extinction factor
        vec3 fex = exp(-(fs_in.beta_r * sr + fs_in.beta_m * sm));

        // In scattering
        float r_phase = rayleigh_phase(cos_theta * 0.5 + 0.5);
        vec3 beta_r_theta = fs_in.beta_r * r_phase;
        float m_phase = hg_phase(cos_theta, mie_dirg);
        vec3 beta_m_theta = fs_in.beta_m * m_phase;

        vec3 lin = pow(fs_in.sun_e * ((beta_r_theta + beta_m_theta) / (fs_in.beta_r + fs_in.beta_m)) * (1.0 - fex), vec3(1.5));
        lin *= mix(vec3(1.0), pow(fs_in.sun_e * ((beta_r_theta + beta_m_theta) / (fs_in.beta_r + fs_in.beta_m)) * fex, vec3(1.0 / 2.0)),
                   clamp(pow(1.0 - dot(up, fs_in.sun_direction), 5.0), 0.0, 1.0));

        // Nightsky
        float theta = acos(direction.y);            // elevation --> y-axis [-pi/2, pi/2]
        float phi = atan(direction.z, direction.x); // azimuth   --> x-axis [-pi/2, pi/2]
        vec2 uv = vec2(phi, theta) / vec2(2.0 * pi, pi) + vec2(0.5, 0.0);
        vec3 L0 = vec3(0.1) * fex;

        // Composition + Solar disc
        float sundisk = smoothstep(sun_angular_diameter_cos, sun_angular_diameter_cos + 0.00002, cos_theta);
        L0 += (fs_in.sun_e * 19000.0 * fex) * sundisk;
        vec3 tex_col = (lin + L0) * 0.04 + vec3(0.0, 0.0003, 0.00075);

        // Tonemap
        vec3 curr = uncharted2_tonemap((log2(2.0 / pow(luminance, 4.0))) * tex_col);
        vec3 mcolor = curr * whitescale;

        vec3 ret_col = pow(mcolor, vec3(1.0 / (1.2 + (1.2 * fs_in.sun_fade))));
        color = vec4(ret_col, 1.0);
    }
);

static vec3 view_target(mat4* view)
{
    vec3 f = vec3_new(-(*view).zx, -(*view).zy, -(*view).zz);
    return f;
}

void sky_preetham_init(struct sky_preetham* sp)
{
    memset(sp, 0, sizeof(*sp));
    /* Build shader */
    sp->shdr = shader_from_srcs(vs_src, 0, fs_src);
}

void sky_preetham_default_params(struct sky_preetham_params* params)
{
    params->luminance = 1.0f;
    params->turbidity = 2.0f;
    params->rayleigh = 1.0f;
    params->mie_coef = 0.005;
    params->mie_dirg = 0.8;
    params->inclination = 0.49;
    params->azimuth = 0.25;
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
    glUniform1f(glGetUniformLocation(sp->shdr, "mie_dirg"), params->mie_dirg);

    /* Calculate sun position according to inclination and azimuth params */
    const unsigned int distance = 400000;
    const float theta = M_PI * (params->inclination - 0.5);
    const float phi = 2.0f * M_PI * (params->azimuth - 0.5);
    vec3 sun_pos = vec3_new(
        distance * cos(phi),
        distance * sin(phi) * sin(theta),
        distance * sin(phi) * cos(theta)
    );
    glUniform3f(glGetUniformLocation(sp->shdr, "sun_position"), sun_pos.x, sun_pos.y, sun_pos.z);

    render_quad();
    glUseProgram(0);

    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
}

void sky_preetham_destroy(struct sky_preetham* sp)
{
    glDeleteProgram(sp->shdr);
}
