//
// swadow.glsl
//
#include "math.glsl"
struct shadow_cascade {
    vec2 plane;
    mat4 vp_mat;
};

vec4 shadow_cascade_weights(float depth, vec4 split_nears, vec4 split_fars)
{
    vec4 near = step(split_nears, vec4(depth));
    vec4 far = step(depth, split_fars);
    return near * far;
}

float weighted_array_value(vec4 weights, float v[4])
{
    return v[0] * weights.x
         + v[1] * weights.y
         + v[2] * weights.z
         + v[3] * weights.w;
}

mat4 shadow_cascade_view_projection(vec4 weights, mat4 view_proj[4])
{
    return view_proj[0] * weights.x
         + view_proj[1] * weights.y
         + view_proj[2] * weights.z
         + view_proj[3] * weights.w;
}

vec3 shadow_cascade_color(vec4 weights)
{
    return vec3(1,0,0) * weights.x
         + vec3(0,1,0) * weights.y
         + vec3(0,0,1) * weights.z
         + vec3(1,0,1) * weights.w;
}

bool is_vertex_in_shadow_map(vec3 coord)
{
    return coord.z > 0.0
        && coord.x > 0.0
        && coord.y > 0.0
        && coord.x <= 1.0
        && coord.y <= 1.0;
}

float calc_shadow_coef(
    sampler2DArray shadow_map,
    vec3 vws_pos,
    vec3 vvs_pos,
    mat4 lview_projs[4],
    vec2 split_planes[4],
    float bias
)
{
    // Find frustum section
    vec4 cascade_weights = shadow_cascade_weights(-vvs_pos.z,
        vec4(split_planes[0].x, split_planes[1].x, split_planes[2].x, split_planes[3].x),
        vec4(split_planes[0].y, split_planes[1].y, split_planes[2].y, split_planes[3].y)
    );

    // Get vertex position in light space
    mat4 view_proj = shadow_cascade_view_projection(cascade_weights, lview_projs);
    vec4 vertex_light_pos = view_proj * vec4(vws_pos, 1.0);
    // Perform perspective divide
    vec3 coords = vertex_light_pos.xyz / vertex_light_pos.w;
    // Transform to [0,1] range
    coords = coords * 0.5 + 0.5;

    // Check if vertex is captured by the shadow map
    if(!is_vertex_in_shadow_map(coords))
        return 0.0;

    // Fetch the layer index of the current used cascade
    float layer = weighted_array_value(cascade_weights, float[](0.0, 1.0, 2.0, 3.0));
    // Get closest depth value from light's perspective
    float closest_depth = texture(shadow_map, vec3(coords.xy, layer)).r;
    // Get depth of current fragment from light's perspective
    float current_depth = coords.z;

    // Calculate shadow factor
    float shadow = 0.0;
    // No PCF
    //shadow = current_depth - bias > closest_depth ? 1.0 : 0.0;

    // PCF
    vec2 texel_size = 1.0 / textureSize(shadow_map, 0).xy;
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcf_depth = texture(shadow_map, vec3(coords.xy + vec2(x, y) * texel_size, layer)).r;
            shadow += current_depth - bias > pcf_depth  ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    return shadow;
}

// http://the-witness.net/news/2013/09/shadow-mapping-summary-part-1/
// Returns the normal and light dependent bias
vec2 get_shadow_offsets(vec3 n, vec3 l)
{
    float cos_alpha = saturate(dot(n, l));
    float offset_scale_n = sqrt(1 - cos_alpha * cos_alpha); // sin(acos(L·N))
    float offset_scale_l = offset_scale_n / cos_alpha;      // tan(acos(L·N))
    return vec2(offset_scale_n, min(2, offset_scale_l));
}

// Offsets a position based on slope and normal
vec3 get_biased_position(vec3 pos, float slope_bias, float normal_bias, vec3 normal, vec3 light)
{
    vec2 offsets = get_shadow_offsets(normal, light);
    pos += normal * offsets.x * normal_bias;
    pos += light  * offsets.y * slope_bias;
    return pos;
}

float shadow_coef(sampler2DArray shadowmap, shadow_cascade cascades[4], vec3 ws_pos, vec3 normal, vec3 light_dir, mat4 view)
{
    mat4 vp_mats[4]   = mat4[](cascades[0].vp_mat, cascades[1].vp_mat,
                               cascades[2].vp_mat, cascades[3].vp_mat);
    vec2 sp_planes[4] = vec2[](cascades[0].plane, cascades[1].plane,
                               cascades[2].plane, cascades[3].plane);
    // TODO: make this configurable
    // XXX: Scale by resolution (higher resolution needs smaller bias)
    const float slope_bias  = 0.001;
    const float normal_bias = 0.0001;
    const float const_bias  = 0.001;
    vec3 biased_pos = get_biased_position(ws_pos, slope_bias, normal_bias, normal, light_dir);
    vec3 vs_pos = (view * vec4(biased_pos, 1.0)).xyz;
    float shadow = calc_shadow_coef(shadowmap, biased_pos, vs_pos, vp_mats, sp_planes, const_bias);
    return shadow;
}

vec3 cascades_vis(shadow_cascade cascades[4], mat4 view, vec3 ws_pos)
{
    vec3 vvs_pos = (view * vec4(ws_pos, 1.0)).xyz;
    vec2 split_planes[4] = vec2[](cascades[0].plane, cascades[1].plane,
                                  cascades[2].plane, cascades[3].plane);
    vec4 cascade_weights = shadow_cascade_weights(-vvs_pos.z,
        vec4(split_planes[0].x, split_planes[1].x, split_planes[2].x, split_planes[3].x),
        vec4(split_planes[0].y, split_planes[1].y, split_planes[2].y, split_planes[3].y)
    );
    return shadow_cascade_color(cascade_weights);
}
