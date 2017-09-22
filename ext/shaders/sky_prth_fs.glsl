#version 330 core
out vec4 fcolor;
in vec2 uv;

// Parameters
uniform vec3  sun_pos;
uniform float rayleigh;
uniform float turbidity;
uniform float mie_coef;
uniform float luminance;
uniform float mie_directional_g;

uniform mat4 proj;
uniform vec3 cam_dir;

// Math constants
const float e = 2.71828182845904523536028747135266249775724709369995957;
const float pi = 3.141592653589793238462643383279502884197169;
const vec3  up = vec3(0.0, 1.0, 0.0);

// Atmospheric scattering constants (SI units unless otherwise noted)
// const float n = 1.0003;   // Refractive index of air
// const float N = 2.545E25; // Number of molecules per unit volume for air at 288.15K and 1013mb (sea level -45 celsius)
// const float pn = 0.035;   // Depolatization factor for standard air

// Wavelength of used primaries, according to preetham
// const vec3 lambda = vec3(680E-9, 550E-9, 450E-9);

// This pre-calcuation replaces older total_rayleigh(vec3 lambda) function:
// (8.0 * pow(pi, 3.0) * pow(pow(n, 2.0) - 1.0, 2.0) * (6.0 + 3.0 * pn)) / (3.0 * N * pow(lambda, vec3(4.0)) * (6.0 - 7.0 * pn))
const vec3 total_rayleigh = vec3(5.804542996261093E-6, 1.3562911419845635E-5, 3.0265902468824876E-5);

// Optical length at zenith for molecules
const float rayleigh_zenith_length = 8.4E3;
const float mie_zenith_length = 1.25E3;

// Sun intensity
const float EE = 1000.0;
// 66 arc seconds -> degrees, and the cosine of that
const float sun_angular_diameter_cos = 0.999956676946448443553574619906976478926848692873900859324;

// Earth shadow hack
const float cutoff_angle = 1.6110731556870734; // pi / 1.95; (original: pi / 2.0)
const float steepness = 1.5; // (original 0.5)

float sun_intensity(float zenith_angle_cos)
{
    zenith_angle_cos = clamp(zenith_angle_cos, -1.0, 1.0);
    return EE * max(0.0, 1.0 - pow(e, -((cutoff_angle - acos(zenith_angle_cos)) / steepness)));
}

vec3 total_mie(float T)
{
    // K coefficient for the primaries
    // const float v = 4.0;
    // const vec3 K = vec3(0.686, 0.678, 0.666);
    // mie_const = pi * pow((2.0 * pi) / lambda, vec3(v - 2.0)) * K
    const vec3 mie_const = vec3(1.8399918514433978E14, 2.7798023919660528E14, 4.0790479543861094E14);
    float c = (0.2 * T) * 10E-18;
    return 0.434 * c * mie_const;
}

/* Rayleigh phase function as a function of cos(theta) */
float rayleigh_phase(float cos_theta)
{
    const float THREE_OVER_SIXTEENPI = 0.05968310365946075; // 3.0 / (16.0 * pi)
    return THREE_OVER_SIXTEENPI * (1.0 + pow(cos_theta, 2.0));
}

/* Henyey-Greenstein approximation as a function of cos(theta)
   - g goemetric constant that defines the shape of the ellipse. */
float hg_phase(float cos_theta, float g)
{
    const float ONE_OVER_FOURPI = 0.07957747154594767; // 1.0 / (4.0 * pi)
    float g2 = pow(g, 2.0);
    float inv = 1.0 / pow(1.0 - 2.0 * g * cos_theta + g2, 1.5);
    return ONE_OVER_FOURPI * ((1.0 - g2) * inv);
}

// Filmic ToneMapping http://filmicgames.com/archives/75
const float A = 0.15;
const float B = 0.50;
const float C = 0.10;
const float D = 0.20;
const float E = 0.02;
const float F = 0.30;
const float whitescale = 1.0748724675633854; // 1.0 / uncharted2tonemap(1000.0)

vec3 uncharted2tonemap(vec3 x)
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
    // Look direction
    vec2 st = uv.xy;
    vec3 ldir = dir_from_pvdr(proj, cam_dir, st); // normalize(ws_pos - cam_pos)

    // Sun intensity with earth shadow hack and based on how high the sun is
    vec3 sun_dir = normalize(sun_pos);
    float sun_e = sun_intensity(dot(sun_dir, up));
    float sun_fade = 1.0 - clamp(1.0 - exp((sun_pos.y / 450000.0)), 0.0, 1.0);

    // Extinction (absorption + out scattering)
    // Rayleigh coefficients
    float rayleigh_coef = rayleigh - (1.0 * (1.0 - sun_fade));
    vec3 beta_r = total_rayleigh * rayleigh_coef;
    // Mie coefficients
    vec3 beta_m = total_mie(turbidity) * mie_coef;

    // Optical length
    // Cutoff angle at 90 to avoid singularity in next formula.
    float zenith_angle = acos(max(0.0, dot(up, ldir)));
    float inv = 1.0 / (cos(zenith_angle) + 0.15 * pow(93.885 - ((zenith_angle * 180.0) / pi), -1.253));
    float s_r = rayleigh_zenith_length * inv;
    float s_m = mie_zenith_length * inv;

    // Combined extinction factor
    vec3 fex = exp(-(beta_r * s_r + beta_m * s_m));

    // In scattering
    float cos_theta = dot(ldir, sun_dir);
    float r_phase = rayleigh_phase(cos_theta * 0.5 + 0.5);
    vec3 beta_r_theta = beta_r * r_phase;
    float m_phase = hg_phase(cos_theta, mie_directional_g);
    vec3 beta_m_theta = beta_m * m_phase;

    vec3 Lin = pow(sun_e * ((beta_r_theta + beta_m_theta) / (beta_r + beta_m)) * (1.0 - fex), vec3(1.5));
    Lin *= mix(vec3(1.0), pow(sun_e * ((beta_r_theta + beta_m_theta) / (beta_r + beta_m)) * fex, vec3(1.0 / 2.0)),
               clamp(pow(1.0 - dot(up, sun_dir), 5.0), 0.0, 1.0));

    // Nightsky
    float theta = acos(ldir.y);       // Elevation --> y-axis [-pi/2, pi/2]
    float phi = atan(ldir.z, ldir.x); // Azimuth   --> x-axis [-pi/2, pi/2]
    vec2 uv = vec2(phi, theta) / vec2(2.0 * pi, pi) + vec2(0.5, 0.0);
    vec3 L0 = vec3(0.1) * fex;

    // Composition + Solar Disc
    float sundisk = smoothstep(sun_angular_diameter_cos, sun_angular_diameter_cos + 0.00002, cos_theta);
    L0 += (sun_e * 19000.0 * fex) * sundisk;
    vec3 tex_color = (Lin + L0) * 0.04 + vec3(0.0, 0.0003, 0.00075);

    // Tonemapping
    vec3 curr = uncharted2tonemap((log2(2.0 / pow(luminance, 4.0))) * tex_color);
    vec3 color = curr * whitescale;

    vec3 ret_color = pow(color, vec3(1.0 / (1.2 + (1.2 * sun_fade))));
    fcolor = vec4(ret_color, 1.0);
}
