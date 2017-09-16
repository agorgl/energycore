#version 330 core
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
