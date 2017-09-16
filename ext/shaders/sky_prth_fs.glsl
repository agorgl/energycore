#version 330 core
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
