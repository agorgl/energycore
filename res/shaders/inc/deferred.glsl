//
// deferred.glsl
//
#extension GL_ARB_sample_shading : enable
#include "encoding.glsl"
#include "packing.glsl"

struct gbuffer {
    sampler2D normal;
    sampler2D albedo;
    sampler2D depth;
    sampler2D roughness_metallic;
};
uniform gbuffer gbuf;
uniform vec2 u_screen;
uniform mat4 u_inv_view_proj;

// Data extracted from gbuffer
struct {
    vec3 ws_pos;
    vec3 normal;
    vec3 albedo;
    float roughness;
    float metallic;
} d;

// Reconstruct world space position from depth at given texcoord
vec3 ws_pos_from_depth(vec2 st, float depth)
{
    float z = depth * 2.0 - 1.0;
    vec4 pos = vec4(st * 2.0 - 1.0, z, 1.0);
    pos = u_inv_view_proj * pos;
    return pos.xyz / pos.w;
}

// Depth at a given texcoord
float gbuffer_depth(vec2 coord)
{
    return textureLod(gbuf.depth, coord, 0).r;
}

vec3 gbuffer_position(vec2 coord)
{
    float depth = gbuffer_depth(coord);
    return ws_pos_from_depth(coord, depth);
}

vec3 reconstruct_wpos_from_depth()
{
    vec2 st = gl_FragCoord.xy / u_screen;
    float z = gbuffer_depth(st);
    return ws_pos_from_depth(st, z);
}

void fetch_gbuffer_data()
{
    ivec2 st = ivec2(gl_FragCoord.xy);
    d.ws_pos = reconstruct_wpos_from_depth();
    vec2 pckd_nm = texelFetch(gbuf.normal, st, 0).rg;
    vec3 albedo  = texelFetch(gbuf.albedo, st, 0).rgb;
    vec2 rgh_met = texelFetch(gbuf.roughness_metallic, st, 0).rg;
    d.normal    = unpack_normal_octahedron(pckd_nm);
    d.albedo    = albedo;
    d.roughness = rgh_met.r;
    d.metallic  = rgh_met.g;
}
