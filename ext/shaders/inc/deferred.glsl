//
// deferred.glsl
//

struct gbuffer {
    sampler2D normal;
    sampler2D albedo;
    sampler2D position;
};
uniform struct gbuffer gbuf;

// Data extracted from gbuffer
struct {
    vec3 ws_pos;
    vec3 normal;
    vec3 albedo;
} d;

void fetch_gbuffer_data()
{
    ivec2 st = ivec2(gl_FragCoord.xy);
    d.ws_pos = texelFetch(gbuf.position, st, 0).rgb;
    d.normal = texelFetch(gbuf.normal,   st, 0).rgb;
    d.albedo = texelFetch(gbuf.albedo,   st, 0).rgb;
}
