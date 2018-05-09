/*********************************************************************************************************************/
/*                                                  /===-_---~~~~~~~~~------____                                     */
/*                                                 |===-~___                _,-'                                     */
/*                  -==\\                         `//~\\   ~~~~`---.___.-~~                                          */
/*              ______-==|                         | |  \\           _-~`                                            */
/*        __--~~~  ,-/-==\\                        | |   `\        ,'                                                */
/*     _-~       /'    |  \\                      / /      \      /                                                  */
/*   .'        /       |   \\                   /' /        \   /'                                                   */
/*  /  ____  /         |    \`\.__/-~~ ~ \ _ _/'  /          \/'                                                     */
/* /-'~    ~~~~~---__  |     ~-/~         ( )   /'        _--~`                                                      */
/*                   \_|      /        _)   ;  ),   __--~~                                                           */
/*                     '~~--_/      _-~/-  / \   '-~ \                                                               */
/*                    {\__--_/}    / \\_>- )<__\      \                                                              */
/*                    /'   (_/  _-~  | |__>--<__|      |                                                             */
/*                   |0  0 _/) )-~     | |__>--<__|     |                                                            */
/*                   / /~ ,_/       / /__>---<__/      |                                                             */
/*                  o o _//        /-~_>---<__-~      /                                                              */
/*                  (^(~          /~_>---<__-      _-~                                                               */
/*                 ,/|           /__>--<__/     _-~                                                                  */
/*              ,//('(          |__>--<__|     /                  .----_                                             */
/*             ( ( '))          |__>--<__|    |                 /' _---_~\                                           */
/*          `-)) )) (           |__>--<__|    |               /'  /     ~\`\                                         */
/*         ,/,'//( (             \__>--<__\    \            /'  //        ||                                         */
/*       ,( ( ((, ))              ~-__>--<_~-_  ~--____---~' _/'/        /'                                          */
/*     `~/  )` ) ,/|                 ~-_~>--<_/-__       __-~ _/                                                     */
/*   ._-~//( )/ )) `                    ~~-'_/_/ /~~~~~~~__--~                                                       */
/*    ;'( ')/ ,)(                              ~~~~~~~~~~                                                            */
/*   ' ') '( (/                                                                                                      */
/*     '   '  `                                                                                                      */
/*********************************************************************************************************************/
#ifndef _SCENE_ASSET_H_
#define _SCENE_ASSET_H_

#include <stdlib.h>

/* Math primitives */
typedef struct vec2f { float x, y; } vec2f;
typedef struct vec3f { float x, y, z; } vec3f;
typedef struct vec4f { float x, y, z, w; } vec4f;
typedef struct vec2i { int x, y; } vec2i;
typedef struct vec3i { int x, y, z; } vec3i;
typedef struct vec4i { int x, y, z, w; } vec4i;
typedef struct quat4f { float x, y, z, w; } quat4f;
typedef struct bbox2f { vec2f min, max; } bbox2f;
typedef struct bbox3f { vec3f min, max; } bbox3f;
typedef struct frame3f { vec3f x, y, z, o; } frame3f;
typedef struct mat4f { vec4f x, y, z, w; } mat4f;
typedef struct image { unsigned int w, h; unsigned short channels, bit_depth; void* data; size_t sz; int compression_type; } image;

/* Math primitive constants */
extern const quat4f identity_quat4f;
extern const frame3f identity_frame3f;
extern const mat4f identity_mat4f;

/* Camera */
struct camera {
    /* Name (default: "") */
    const char* name;
    /* Transform frame (default: identity_frame3f) */
    frame3f frame;
    /* Orthographic (default: 0) */
    int ortho;
    /* Vertical field of view (perspective) or size (orthographic), (default: 2 * atan(0.5f)) */
    float yfov;
    /* Aspect ratio (default: 16.0f / 9.0f) */
    float aspect;
    /* Focus distance (default: 1, range: [0.01,1000]) */
    float focus;
    /* Lens aperture (default: 0, range: [0,5]) */
    float aperture;
    /* Near plane distance (default: 0.01f, range: [0.01,10]) */
    float near;
    /* Far plane distance (default: 10000, range: [10,10000]) */
    float far;
};

/* Texture containing either an LDR or HDR image. */
struct texture {
    /* Name (default: "") */
    const char* name;
    /* Path (default: "") */
    const char* path;
    /* Image data */
    image img;
};

/* Texture wrap mode */
enum texture_wrap {
    /* Repeat */
    TEXTURE_WRAP_REPEAT = 1,
    /* Clamp */
    TEXTURE_WRAP_CLAMP  = 2,
    /* Mirror */
    TEXTURE_WRAP_MIRROR = 3,
};

/* Texture filter mode */
enum texture_filter {
    /* Linear */
    TEXTURE_FILTER_LINEAR                 = 1,
    /* Nearest */
    TEXTURE_FILTER_NEAREST                = 2,
    /* Linear mipmap linear */
    TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR   = 3,
    /* Nearest mipmap nearest */
    TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST = 4,
    /* Linear mipmap nearest */
    TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST  = 5,
    /* Nearest mipmap linear */
    TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR  = 6,
};

/* Texture information to use for lookup. */
struct texture_info {
    /* Wrap mode for s coordinate (default: TEXTURE_WRAP_REPEAT) */
    enum texture_wrap wrap_s;
    /* Wrap mdoe for t coordinate (default: TEXTURE_WRAP_REPEAT) */
    enum texture_wrap wrap_t;
    /* Filter mode (default: TEXTURE_FILTER_LINEAR) */
    enum texture_filter filter_mag;
    /* Filter mode (default: TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR) */
    enum texture_filter filter_min;
    /* Texture strength (occlusion and normal) (default: 1, range: [0,10]) */
    float scale;
};

/* Material type. */
enum material_type {
    /* Microfacet material type (OBJ). */
    MATERIAL_TYPE_SPECULAR_ROUGHNESS  = 0,
    /* Base and metallic material (metallic-roughness in glTF). */
    MATERIAL_TYPE_METALLIC_ROUGHNESS  = 1,
    /* Diffuse and specular material (specular-glossness in glTF). */
    MATERIAL_TYPE_SPECULAR_GLOSSINESS = 2,
};

/* Material for surfaces, lines and triangles. */
struct material {
    /* Name (default: "") */
    const char* name;
    /* Double-sided rendering (default: 0) */
    int double_sided;
    /* Material type (default: MATERIAL_TYPE_METALLIC_ROUGHNESS) */
    enum material_type type;

    /* Emission color (default: {0, 0, 0}) */
    vec3f ke;
    /* Diffuse color / base color (default: {0, 0, 0}) */
    vec3f kd;
    /* Specular color / metallic factor (default: {0, 0, 0}) */
    vec3f ks;
    /* Clear coat reflection (default: {0, 0, 0}) */
    vec3f kr;
    /* Transmission color (default: {0, 0, 0}) */
    vec3f kt;
    /* Roughness (default: 0.0001) */
    float rs;
    /* Opacity (default: 1) */
    float op;

    /* Emission texture ref (default: null) */
    struct texture* ke_txt;
    /* Diffuse / base texture ref (default: null) */
    struct texture* kd_txt;
    /* Specular / metallic texture ref (default: null) */
    struct texture* ks_txt;
    /* Clear coat reflection texture ref (default: null) */
    struct texture* kr_txt;
    /* Transmission texture ref (default: null) */
    struct texture* kt_txt;
    /* Roughness texture ref (default: null) */
    struct texture* rs_txt;
    /* Bump map texture (heighfield) ref (default: null) */
    struct texture* bump_txt;
    /* Displacement map texture (heighfield) ref (default: null) */
    struct texture* disp_txt;
    /* Normal texture ref (default: null) */
    struct texture* norm_txt;
    /* Occlusion texture ref (default: null) */
    struct texture* occ_txt;
    /* Detail diffuse / base texture ref (default: null) */
    struct texture* kdd_txt;
    /* Detail normal texture ref (default: null) */
    struct texture* normd_txt;

    /* Emission texture info (default: null) */
    struct texture_info* ke_txt_info;
    /* Diffuse texture info (default: null) */
    struct texture_info* kd_txt_info;
    /* Specular texture info (default: null) */
    struct texture_info* ks_txt_info;
    /* Clear coat reflection texture info (default: null) */
    struct texture_info* kr_txt_info;
    /* Transmission texture info (default: null) */
    struct texture_info* kt_txt_info;
    /* Roughness texture info (default: null) */
    struct texture_info* rs_txt_info;
    /* Bump map texture (heighfield) info (default: null) */
    struct texture_info* bump_txt_info;
    /* Displacement map texture (heighfield) info (default: null) */
    struct texture_info* disp_txt_info;
    /* Normal texture info (default: null) */
    struct texture_info* norm_txt_info;
    /* Occlusion texture info (default: null) */
    struct texture_info* occ_txt_info;
    /* Detail diffuse / base texture info (default: null) */
    struct texture_info* kdd_txt_info;
    /* Detail normal texture info (default: null) */
    struct texture_info* normd_txt_info;
};

/* Shape data represented as an indexed array.
 * May contain only one of the points/lines/triangles/quads. */
struct shape {
    /* Name (default: "") */
    const char* name;
    /* Material ref (default: null) */
    struct material* mat;

    /* Points */
    int* points;
    size_t num_points;
    /* Lines */
    vec2i* lines;
    size_t num_lines;
    /* Triangles */
    vec3i* triangles;
    size_t num_triangles;
    /* Quads */
    vec4i* quads;
    size_t num_quads;
    /* Face-varying indices for position. */
    vec4i* quads_pos;
    size_t num_quads_pos;
    /* Face-varying indices for normal. */
    vec4i* quads_norm;
    size_t num_quads_norm;
    /* Face-varying indices for texcoord. */
    vec4i* quads_texcoord;
    size_t num_quads_texcoord;
    /* Bezier. */
    vec4i* beziers;
    size_t num_beziers;

    /* Vertex position */
    vec3f* pos;
    size_t num_pos;
    /* Vertex normals */
    vec3f* norm;
    size_t num_norm;
    /* Vertex texcoord */
    vec2f* texcoord;
    size_t num_texcoord;
    /* Vertex additional texcoord */
    vec2f* texcoord1;
    size_t num_texcoord1;
    /* Vertex color */
    vec4f* color;
    size_t num_color;
    /* Per-vertex radius */
    float* radius;
    size_t num_radius;
    /* Vertex tangent space */
    vec4f* tangsp;
    size_t num_tangsp;
    /* Vertex skinning weights */
    vec4f* skin_weights;
    size_t num_skin_weights;
    /* Vertex skinning joint indices */
    vec4i* skin_joints;
    size_t num_skin_joints;

    /* Number of times to subdivide (default: 0) */
    int subdivision;
    /* Whether to use Catmull-Clark subdivision (default: 0) */
    int catmullclark;
};

/* Group of shapes. */
struct mesh {
    /* Name (default: "") */
    const char* name;
    /* Path used for saving (default: "") */
    const char* path;
    /* Shapes */
    struct shape** shapes;
    size_t num_shapes;
};

/* Mesh instance. */
struct mesh_instance {
    /* Name (default: "") */
    const char* name;
    /* Transform frame (default: identity_frame3f) */
    frame3f frame;
    /* Mesh instance ref (default: null) */
    struct mesh* msh;
};

/* Envinonment map. */
struct environment {
    /* Name (default: "") */
    const char* name;
    /* Transform frame (default: identity_frame3f) */
    frame3f frame;
    /* Emission coefficient (default: {0, 0, 0}) */
    vec3f ke;
    /* Emission texture ref (default: null) */
    struct texture* ke_txt;
    /* Emission texture info (default: null) */
    struct texture_info* ke_txt_info;
};

/* Node in a transform hierarchy */
struct node {
    /* Name (default: "") */
    const char* name;
    /* Parent node ref (default: null) */
    struct node* parent;
    /* Transform frame (default: identity_frame3f) */
    frame3f frame;
    /* Translation (default: {0, 0, 0}) */
    vec3f translation;
    /* Rotation (default: {0, 0, 0, 1}) */
    quat4f rotation;
    /* Scaling (default: {1, 1, 1}) */
    vec3f scaling;
    /* Camera ref the node points to (default: null) */
    struct camera* cam;
    /* Mesh instance ref the node points to (default: null) */
    struct mesh_instance* ist;
    /* Skin ref the node points to (default: null) */
    struct skin* skn;
    /* Environment ref the node points to (default: null) */
    struct environment* env;
    /* Child nodes. This is a computed value only stored for convenience */
    struct node** children;
    size_t num_children;
};

/* Keyframe type */
enum keyframe_interpolation {
    /* Linear interpolation */
    KEYFRAME_INTERPOLATION_LINEAR      = 0,
    /* Step function */
    KEYFRAME_INTERPOLATION_STEP        = 1,
    /* Catmull-Rom spline interpolation */
    KEYFRAME_INTERPOLATION_CATMULL_ROM = 2,
    /* Cubic Bezier spline interpolation */
    KEYFRAME_INTERPOLATION_BEZIER      = 3,
};

/* Keyframe data */
struct animation {
    /* Name (default: "") */
    const char* name;
    /* Interpolation (default: KEYFRAME_INTERPOLATION_LINEAR) */
    enum keyframe_interpolation type;
    /* Times */
    float* times;
    size_t num_times;
    /* Translation */
    vec3f* translations;
    size_t num_translations;
    /* Rotation */
    quat4f* rotations;
    size_t num_rotations;
    /* Scaling */
    vec3f* scalings;
    size_t num_scalings;
};

/* Animation made of multiple keyframed values. */
struct animation_group {
    /* Name (default: "") */
    const char* name;
    /* Path  used when writing files on disk (default: "") */
    const char* path;
    /* Keyframed values */
    struct animation** animations;
    size_t num_animations;
    /* Binds keyframe values to nodes refs */
    struct {struct animation* a; struct node* n;}* targets;
    size_t num_targets;
};

/* Skin */
struct skin {
    /* Name (default: "") */
    const char* name;
    /* Path (only used when writing files on disk) (default: "") */
    const char* path;
    /* Inverse bind matrix */
    mat4f* pose_matrices;
    size_t num_pose_matrices;
    /* Joints */
    struct node** joints;
    size_t num_joints;
    /* Skeleton root node (default: null) */
    struct node* root;
};

/* Scene comprised an array of objects whose memory is owned by the scene.
 * All members are optional, but different algorithm might require different
 * data to be laoded. Scene objects (camera, instances, environments) have
 * transforms defined internally. A scene can optionally contain a
 * node hierarchy where each node might point to a camera, instance or
 * environment. In that case, the element transforms are computed from
 * the hierarchy. Animation is also optional, with keyframe data that
 * updates node transformations only if defined. */
struct scene {
    /* Meshes */
    struct mesh** meshes;
    size_t num_meshes;
    /* Mesh instances */
    struct mesh_instance** instances;
    size_t num_instances;
    /* Materials */
    struct material** materials;
    size_t num_materials;
    /* Textures */
    struct texture** textures;
    size_t num_textures;
    /* Cameras */
    struct camera** cameras;
    size_t num_cameras;
    /* Environments */
    struct environment** environments;
    size_t num_environments;
    /* Node hierarchy */
    struct node** nodes;
    size_t num_nodes;
    /* Node animations */
    struct animation_group** animations;
    size_t num_animations;
    /* Skins */
    struct skin** skins;
    size_t num_skins;
};

/* Add elements options. */
struct add_elements_options {
    /* Add missing normal. */
    int smooth_normals;
    /* Add missing trangent space. */
    int tangent_space;
    /* Add empty texture data. */
    int texture_data;
    /* Add instances. */
    int shape_instances;
    /* Add default names. */
    int default_names;
    /* Add default paths. */
    int default_paths;
};

/* Datatype constructors / destructors */
void camera_init(struct camera* c);
void camera_destroy(struct camera* c);
void texture_init(struct texture* t);
void texture_destroy(struct texture* t);
void texture_info_default(struct texture_info* ti);
void material_init(struct material* m);
void material_destroy(struct material* m);
void shape_init(struct shape* s);
void shape_destroy(struct shape* s);
void _mesh_init(struct mesh* m); /* TODO: Resolve naming conflict */
void _mesh_destroy(struct mesh* m); /* TODO: Resolve naming conflict */
void mesh_instance_init(struct mesh_instance* mi);
void mesh_instance_destroy(struct mesh_instance* mi);
void environment_init(struct environment* e);
void environment_destroy(struct environment* e);
void node_init(struct node* n);
void node_destroy(struct node* n);
void animation_init(struct animation* a);
void animation_destroy(struct animation* a);
void animation_group_init(struct animation_group* ag);
void animation_group_destroy(struct animation_group* ag);
void skin_init(struct skin* s);
void skin_destroy(struct skin* s);
void scene_init(struct scene* s);
void scene_destroy(struct scene* s);

/* Add elements */
void scene_add_elements(struct scene* scn, const struct add_elements_options* opts);
/* Update node transforms. */
void scene_update_transforms(struct scene* scn);
/* Compute animation range. */
vec2f scene_compute_animation_range(const struct scene* scn);
/* Update the normals of a shape. Supports only non-facevarying shapes. */
void shape_compute_normals(struct shape* shp);
/* Compute per-vertex tangent frame for triangle meshes. */
void shape_compute_tangent_frame(struct shape* shp);
/* Computes a shape bounding box using a quick computation that ignores radius. */
bbox3f shape_compute_bounds(const struct shape* shp);
/* Compute a scene bounding box. */
bbox3f scene_compute_bounds(const struct scene* scn);
/* Flatten scene instances into separate shapes. */
void scene_flatten_instances(struct scene* scn);
/* Print scene information. */
void scene_print_info(const struct scene* scn);

#endif /* ! _SCENE_ASSET_H_ */
