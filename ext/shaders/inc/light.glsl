//
// light.glsl
//
const float PI = 3.14159265359;

float distribution_ggx(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float geometry_schlick_ggx(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float geometry_schlick_ggx_ibl(float NdotV, float roughness)
{
    // Note that we use a different k for IBL
    float a = roughness;
    float k = (a*a) / 2.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float geometry_smith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometry_schlick_ggx(NdotV, roughness);
    float ggx1 = geometry_schlick_ggx(NdotL, roughness);
    return ggx1 * ggx2;
}

float geometry_smith_ibl(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometry_schlick_ggx_ibl(NdotV, roughness);
    float ggx1 = geometry_schlick_ggx_ibl(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 fresnel_schlick(float cos_theta, vec3 f0)
{
    return f0 + (1.0 - f0) * pow(1.0 - cos_theta, 5.0);
}

// As the ambient light comes from all directions within the hemisphere oriented around
// the normal N there's no single halfway vector to determine the Fresnel response.
// To still simulate Fresnel, we calculate the Fresnel from the angle between the normal
// and view vector. However, earlier we used the micro-surface halfway vector, influenced
// by the roughness of the surface, as input to the Fresnel equation.
// As we currently don't take any roughness into account, the surface's reflective ratio
// will always end up relatively high. Indirect light follows the same properties of direct
// light so we expect rougher surfaces to reflect less strongly on the surface edges.
// As we don't take the surface's roughness into account, the indirect Fresnel reflection
// strength looks off on rough non-metal surfaces (slightly exaggerated for demonstration purposes)
// We can alleviate the issue by injecting a roughness term in the Fresnel-Schlick equation
// as described by Sébastien Lagarde (https://seblagarde.wordpress.com/2011/08/17/hello-world/):
vec3 fresnel_schlick_roughness(float cos_theta, vec3 f0, float roughness)
{
    return f0 + (max(vec3(1.0 - roughness), f0) - f0) * pow(1.0 - cos_theta, 5.0);
}

float point_light_attenuation(vec3 light_pos, vec3 frag_wpos, vec3 view_pos)
{
    float distance = length(light_pos - frag_wpos);
    float attenuation = 1.0 / (distance * distance);
    return attenuation;
}

vec3 radiance(vec3 N, vec3 V, vec3 L, vec3 light_col, float attenuation, vec3 albedo, float metallic, float roughness)
{
    // Calculate reflectance at normal incidence; if dia-electric (like plastic) use f0
    // of 0.04 and if it's a metal, use their albedo color as f0 (metallic workflow)
    vec3 f0 = vec3(0.04);
    f0 = mix(f0, albedo, metallic);

    // Calculate per-light radiance
    vec3 radiance = light_col * attenuation;

    // Halfway vector
    vec3 H = normalize(V + L);

    // Cook-Torrance BRDF
    float NDF = distribution_ggx(N, H, roughness);
    float G   = geometry_smith(N, V, L, roughness);
    vec3  F   = fresnel_schlick(max(dot(H, V), 0.0), f0);

    // kS is equal to Fresnel
    vec3 kS = F;
    // For energy conservation, the diffuse and specular light can't
    // be above 1.0 (unless the surface emits light); to preserve this
    // relationship the diffuse component (kD) should equal 1.0 - kS.
    vec3 kD = vec3(1.0) - kS;
    // Multiply kD by the inverse metalness such that only non-metals
    // have diffuse lighting, or a linear blend if partly metal (pure metals
    // have no diffuse light).
    kD *= 1.0 - metallic;

    // Cook-Torrance
    vec3  nominator   = NDF * G * F;
    float denominator = 4 * max(dot(V, N), 0.0) * max(dot(L, N), 0.0) + 0.001; // 0.001 to prevent divide by zero.
    vec3 brdf = nominator / denominator;

    // Scale light by NdotL
    float NdotL = max(dot(N, L), 0.0);

    // Outgoing radiance Lo
    // Note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    vec3 Lo = (kD * albedo / PI + brdf) * radiance * NdotL;
    return Lo;
}

vec3 env_radiance(vec3 N, vec3 V, vec3 albedo, float metallic, float roughness, samplerCube irr_map, samplerCube pf_map, sampler2D brdf_lut)
{
    // Calculate reflectance at normal incidence; if dia-electric (like plastic) use f0
    // of 0.04 and if it's a metal, use their albedo color as f0 (metallic workflow)
    vec3 f0 = vec3(0.04);
    f0 = mix(f0, albedo, metallic);

    // Reflect vector
    vec3 R = reflect(-V, N);

    // Ambient lighting (we now use IBL as the ambient term)
    vec3 F = fresnel_schlick_roughness(max(dot(N, V), 0.0), f0, roughness);

    // kS is equal to Fresnel
    vec3 kS = F;
    // For energy conservation, the diffuse and specular light can't
    // be above 1.0 (unless the surface emits light); to preserve this
    // relationship the diffuse component (kD) should equal 1.0 - kS.
    vec3 kD = vec3(1.0) - kS;
    // Multiply kD by the inverse metalness such that only non-metals
    // have diffuse lighting, or a linear blend if partly metal (pure metals
    // have no diffuse light).
    kD *= 1.0 - metallic;

    // Calculate diffuse ambient component
    vec3 irradiance = texture(irr_map, N).rgb;
    vec3 diffuse = irradiance * albedo;

    // Calculate specular ambient component
    // Sample both the pre-filter map and the BRDF lut and combine them together
    // as per the Split-Sum approximation to get the IBL specular part.
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefiltered_color = textureLod(pf_map, R, roughness * MAX_REFLECTION_LOD).rgb;
    vec2 brdf = texture(brdf_lut, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefiltered_color * (F * brdf.x + brdf.y);

    // Ambient light sum
    vec3 ambient = (kD * diffuse + specular);
    return ambient;
}
