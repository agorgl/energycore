//
// convert.glsl
//
// Standard dielectric reflectivity coef at incident angle (= 4%)
const vec3 dielectric_specular = vec3(0.04, 0.04, 0.04);
const float conversion_epsilon = 1e-6f;

// All values are in linear space
struct metallic_roughness {
    vec3  basecolor;
    float metallic;
    float roughness;
};

// All values are in linear space
struct specular_glossiness {
    vec3 diffuse;
    vec3 specular;
    float glossiness;
};

/*-----------------------------------------------------------------
 * Helpers
 *-----------------------------------------------------------------*/
float max_component(vec3 v) { return max(max(v.r, v.g), v.b); }

float get_perceived_brightness(vec3 linear_color)
{
    float r = linear_color.r;
    float b = linear_color.b;
    float g = linear_color.g;
    return sqrt(0.299 * r * r + 0.587 * g * g + 0.114 * b * b);
}

float specular_strength(vec3 specular)
{
    #ifdef SPEC_STRENGTH_FROM_RED
        return specular.r; // Red channel - because most metals are either monochrome or with redish/yellowish tint
    #else
        return max_component(specular);
    #endif
}

float solve_metallic(float dielectric_specular, float diffuse, float specular, float one_minus_specular_strength)
{
    specular = max(specular, dielectric_specular);
    float a = dielectric_specular;
    float b = diffuse * one_minus_specular_strength / (1 - dielectric_specular) + specular - 2.0 * dielectric_specular;
    float c = dielectric_specular - specular;
    float D = b * b - 4.0 * a * c;
    return clamp((-b + sqrt(D)) / (2.0 * a), 0.0, 1.0);
}

/*-----------------------------------------------------------------
 * Converters
 *-----------------------------------------------------------------*/
specular_glossiness metalness_to_specular(metallic_roughness mr)
{
    vec3 basecolor  = mr.basecolor;
    float metallic  = mr.metallic;
    float roughness = mr.roughness;

    vec3 specular = mix(dielectric_specular, basecolor, metallic);
    float one_minus_specular_strength = 1.0 - specular_strength(specular);
    vec3 diffuse = (one_minus_specular_strength < conversion_epsilon) ? vec3(0.0) : basecolor * (1.0 - dielectric_specular.r) * (1.0 - metallic) / one_minus_specular_strength;

    specular_glossiness sg;
    sg.diffuse    = diffuse;
    sg.specular   = specular;
    sg.glossiness = 1.0 - roughness;
    return sg;
}

metallic_roughness specular_to_metalness(specular_glossiness sg)
{
    vec3 diffuse     = sg.diffuse;
    vec3 specular    = sg.specular;
    float glossiness = sg.glossiness;

    float one_minus_specular_strength = 1.0 - specular_strength(specular);
    float metallic = solve_metallic(dielectric_specular.r, get_perceived_brightness(diffuse), get_perceived_brightness(specular), one_minus_specular_strength);

    vec3 basecolor_from_diffuse = diffuse * one_minus_specular_strength / ((1.0 - dielectric_specular.r) * max(1.0 - metallic, conversion_epsilon));
    vec3 basecolor_from_specular = (specular - dielectric_specular * (1.0 - metallic)) / max(metallic, conversion_epsilon);
    vec3 basecolor = mix(basecolor_from_diffuse, basecolor_from_specular, metallic * metallic);

    metallic_roughness mr;
    mr.basecolor = basecolor;
    mr.metallic  = metallic;
    mr.roughness = 1.0 - glossiness;
    return mr;
}
