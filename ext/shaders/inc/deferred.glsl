//
// deferred.glsl
//
#extension GL_ARB_sample_shading : enable

struct gbuffer {
    sampler2DMS normal;
    sampler2DMS albedo;
    sampler2DMS depth;
};
uniform gbuffer gbuf;
uniform vec2 u_screen;
uniform mat4 u_inv_view_proj;

// Data extracted from gbuffer
struct {
    vec3 ws_pos;
    vec3 normal;
    vec3 albedo;
} d;

vec3 reconstruct_wpos_from_depth()
{
    vec2 st = gl_FragCoord.xy / u_screen;
    float z = texelFetch(gbuf.depth, ivec2(gl_FragCoord.xy), gl_SampleID).r;
    z = z * 2.0 - 1.0;
    vec4 spos = vec4(st * 2.0 - 1.0, z, 1.0);
    spos = u_inv_view_proj * spos;
    return spos.xyz / spos.w;
}

void fetch_gbuffer_data()
{
    ivec2 st = ivec2(gl_FragCoord.xy);
    d.ws_pos = reconstruct_wpos_from_depth();
    d.normal = texelFetch(gbuf.normal, st, gl_SampleID).rgb;
    d.albedo = texelFetch(gbuf.albedo, st, gl_SampleID).rgb;
}
