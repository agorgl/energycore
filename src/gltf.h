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
#ifndef _GLTF_H_
#define _GLTF_H_

#include <stdlib.h>

/* Enum values */
#define GLTF_UNSIGNED_BYTE 5121
#define GLTF_UNSIGNED_SHORT 5123
#define GLTF_UNSIGNED_INT 5125
#define GLTF_POINTS 0
#define GLTF_LINES 1
#define GLTF_LINE_LOOP 2
#define GLTF_LINE_STRIP 3
#define GLTF_TRIANGLES 4
#define GLTF_TRIANGLE_STRIP 5
#define GLTF_TRIANGLE_FAN 6
#define GLTF_NEAREST 9728
#define GLTF_LINEAR 9729
#define GLTF_NEAREST_MIPMAP_NEAREST 9984
#define GLTF_LINEAR_MIPMAP_NEAREST 9985
#define GLTF_NEAREST_MIPMAP_LINEAR 9986
#define GLTF_LINEAR_MIPMAP_LINEAR 9987
#define GLTF_CLAMP_TO_EDGE 33071
#define GLTF_MIRRORED_REPEAT 33648
#define GLTF_REPEAT 10497
#define GLTF_BYTE 5120
#define GLTF_SHORT 5122
#define GLTF_FLOAT 5126
#define GLTF_ARRAY_BUFFER 34962
#define GLTF_ELEMENT_ARRAY_BUFFER 34963

/* Primitive types */
typedef int gltf_id;

/* A perspective camera containing properties to create a perspective projection matrix */
struct gltf_camera_perspective {
    /* The floating-point aspect ratio of the field of view */
    float aspect_ratio;
    /* The floating-point vertical field of view in radians */
    float yfov;
    /* The floating-point distance to the far clipping plane */
    float zfar;
    /* The floating-point distance to the near clipping plane */
    float znear;
};

/* An orthographic camera containing properties to create an orthographic projection matrix */
struct gltf_camera_orthographic {
    /* The floating-point horizontal magnification of the view. Must not be zero */
    float xmag;
    /* The floating-point vertical magnification of the view. Must not be zero */
    float ymag;
    /* The floating-point distance to the far clipping plane. `zfar` must be greater than `znear` */
    float zfar;
    /* The floating-point distance to the near clipping plane */
    float znear;
};

/* A camera's projection.
 * A node can reference a camera to apply a transform to place the camera in the scene */
struct gltf_camera {
    /* The user-defined name of this object */
    const char* name;
    /* Specifies if the camera uses a perspective or orthographic projection */
    const char* type;
    /* An orthographic camera containing properties to create an orthographic projection matrix */
    struct gltf_camera_orthographic* orthographic;
    /* A perspective camera containing properties to create a perspective projection matrix */
    struct gltf_camera_perspective* perspective;
};

/* Image data used to create a texture. Image can be referenced by URI
 * or `bufferView` index. The `mimeType` is required in the latter case */
struct gltf_image {
    /* The user-defined name of this object */
    const char* name;
    /* The uri of the image */
    const char* uri;
    /* The image's MIME type */
    const char* mime_type;
    /* The index of the bufferView that contains the image.
     * Use this instead of the image's uri property */
    gltf_id buffer_view;
};

/* Texture sampler properties for filtering and wrapping modes */
struct gltf_sampler {
    /* The user-defined name of this object */
    const char* name;
    /* Magnification filter */
    int mag_filter;
    /* Minification filter */
    int min_filter;
    /* s wrapping mode */
    int wrap_s;
    /* t wrapping mode */
    int wrap_t;
};

/* A texture and its sampler */
struct gltf_texture {
    /* The user-defined name of this object */
    const char* name;
    /* The index of the sampler used by this texture.
     * When undefined, a sampler with repeat wrapping and auto filtering should be used */
    gltf_id sampler;
    /* The index of the image used by this texture */
    gltf_id source;
};

/* Reference to a texture */
struct gltf_texture_info {
    /* The index of the texture */
    gltf_id index;
    /* The set index of texture's TEXCOORD attribute used for texture coordinate mapping */
    int tex_coord;
};

/* Reference to a texture */
struct gltf_material_normal_texture_info {
    /* The index of the texture */
    gltf_id index;
    /* The set index of texture's TEXCOORD attribute used for texture coordinate mapping */
    int tex_coord;
    /* The scalar multiplier applied to each normal vector of the normal texture */
    float scale;
};

/* Reference to a texture */
struct gltf_material_occlusion_texture_info {
    /* The index of the texture */
    gltf_id index;
    /* The set index of texture's TEXCOORD attribute used for texture coordinate mapping */
    int tex_coord;
    /* A scalar multiplier controlling the amount of occlusion applied */
    float strength;
};

/* A set of parameter values that are used to define the metallic-roughness
 * material model from Physically-Based Rendering (PBR) methodology */
struct gltf_material_pbr_metallic_roughness {
    /* The material's base color factor */
    float base_color_factor[4];
    /* The base color texture */
    struct gltf_texture_info* base_color_texture;
    /* The metalness of the material */
    float metallic_factor;
    /* The roughness of the material */
    float roughness_factor;
    /* The metallic-roughness texture */
    struct gltf_texture_info* metallic_roughness_texture;
};

/* The material appearance of a primitive */
struct gltf_material {
    /* The user-defined name of this object */
    const char* name;
    /* A set of parameter values that are used to define the metallic-roughness
     * material model from Physically-Based Rendering (PBR) methodology.
     * When not specified, all the default values of `pbrMetallicRoughness` apply */
    struct gltf_material_pbr_metallic_roughness* pbr_metallic_roughness;
    /* The normal map texture */
    struct gltf_material_normal_texture_info* normal_texture;
    /* The occlusion map texture */
    struct gltf_material_occlusion_texture_info* occlusion_texture;
    /* The emissive map texture */
    struct gltf_texture_info* emissive_texture;
    /* The emissive color of the material */
    float emissive_factor[3];
    /* The alpha rendering mode of the material */
    const char* alpha_mode;
    /* The alpha cutoff value of the material */
    float alpha_cutoff;
    /* Specifies whether the material is double sided */
    int double_sided;
};

/* A key-value pair */
struct gltf_mesh_primitive_attribute {
    /* Mesh attribute semantic */
    const char* semantic;
    /* The index of the accessor containing attribute's data */
    int accessor_index;
};

/* Geometry to be rendered with the given material */
struct gltf_mesh_primitive {
    /* A dictionary object, where each key corresponds to mesh attribute semantic
     * and each value is the index of the accessor containing attribute's data */
    struct gltf_mesh_primitive_attribute* attributes;
    size_t num_attributes;
    /* The index of the accessor that contains the indices */
    gltf_id indices;
    /* The index of the material to apply to this primitive when rendering */
    gltf_id material;
    /* The type of primitives to render */
    int mode;
    /* An array of Morph Targets, each  Morph Target is a dictionary mapping
     * attributes (only `POSITION`, `NORMAL`, and `TANGENT` supported)
     * to their deviations in the Morph Target */
    struct gltf_mesh_primitive_attribute (*targets)[3];
    size_t num_targets;
};

/* A set of primitives to be rendered.
 * A node can contain one mesh.
 * A node's transform places the mesh in the scene */
struct gltf_mesh {
    /* The user-defined name of this object */
    const char* name;
    /* An array of primitives, each defining geometry to be rendered with a material */
    struct gltf_mesh_primitive* primitives;
    size_t num_primitives;
    /* Array of weights to be applied to the Morph Targets */
    float* weights;
    size_t num_weights;
};

/* Joints and matrices defining a skin */
struct gltf_skin {
    /* The user-defined name of this object */
    const char* name;
    /* The index of the accessor containing the floating-point 4x4 inverse-bind matrices.
     * The default is that each matrix is a 4x4 identity matrix,
     * which implies that inverse-bind matrices were pre-applied */
    gltf_id inverse_bind_matrices;
    /* The index of the node used as a skeleton root.
     * When undefined, joints transforms resolve to scene root */
    gltf_id skeleton;
    /* Indices of skeleton nodes, used as joints in this skin */
    gltf_id* joints;
    size_t num_joints;
};

/* The index of the node and TRS property that an animation channel targets */
struct gltf_animation_channel_target {
    /* The index of the node to target */
    gltf_id node;
    /* The name of the node's TRS property to modify, or
     * the "weights" of the Morph Targets it instantiates.
     * For the "translation" property, the values that are
     * provided by the sampler are the translation along
     * the x, y, and z axes. For the "rotation" property,
     * the values are a quaternion in the order (x, y, z, w),
     * where w is the scalar. For the "scale" property,
     * the values are the scaling factors along the x, y, and z axes */
    const char* path;
};

/* Targets an animation's sampler at a node's property */
struct gltf_animation_channel {
    /* The index of a sampler in this animation used to compute the value for the target */
    gltf_id sampler;
    /* The index of the node and TRS property to target */
    struct gltf_animation_channel_target* target;
};

/* Combines input and output accessors with an interpolation algorithm to define a keyframe graph (but not its target) */
struct gltf_animation_sampler {
    /* The index of an accessor containing keyframe input values, e.g., time */
    gltf_id input;
    /* The index of an accessor, containing keyframe output values */
    gltf_id output;
    /* Interpolation algorithm */
    const char* interpolation;
};

/* A keyframe animation */
struct gltf_animation {
    /* The user-defined name of this object */
    const char* name;
    /* An array of channels, each of which targets an animation's sampler at a node's property.
     * Different channels of the same animation can't have equal targets */
    struct gltf_animation_channel* channels;
    size_t num_channels;
    /* An array of samplers that combines input and output accessors with
     * an interpolation algorithm to define a keyframe graph (but not its target) */
    struct gltf_animation_sampler* samplers;
    size_t num_samplers;
};

/* A node in the node hierarchy.
 * When the node contains `skin`, all `mesh.primitives` must contain `JOINTS_0` and `WEIGHTS_0` attributes.
 * A node can have either a `matrix` or any combination of `translation`/`rotation`/`scale` (TRS) properties.
 * TRS properties are converted to matrices and postmultiplied in the `T * R * S` order to compose
 * the transformation matrix; first the scale is applied to the vertices, then the rotation, and then
 * the translation. If none are provided, the transform is the identity. When a node is targeted for
 * animation (referenced by an animation.channel.target), only TRS properties may be present; `matrix`
 * will not be present */
struct gltf_node {
    /* The user-defined name of this object */
    const char* name;
    /* The index of the camera referenced by this node */
    gltf_id camera;
    /* The indices of this node's children */
    gltf_id* children;
    size_t num_children;
    /* The index of the skin referenced by this node */
    gltf_id skin;
    /* A floating-point 4x4 transformation matrix stored in column-major order */
    float matrix[16];
    /* The index of the mesh in this node */
    gltf_id mesh;
    /* The node's unit quaternion rotation in the order (x, y, z, w), where w is the scalar */
    float rotation[4];
    /* The node's non-uniform scale, given as the scaling factors along the x, y, and z axes */
    float scale[3];
    /* The node's translation along the x, y, and z axes */
    float translation[3];
    /* The weights of the instantiated Morph Target.
     * Number of elements must match number of Morph Targets of used mesh */
    float* weights;
    size_t num_weights;
};

/* The root nodes of a scene */
struct gltf_scene {
    /* The user-defined name of this object */
    const char* name;
    /* The indices of each root node */
    gltf_id* nodes;
    size_t num_nodes;
};

/* A buffer points to binary geometry, animation, or skins */
struct gltf_buffer {
    /* The user-defined name of this object */
    const char* name;
    /* The uri of the buffer */
    const char* uri;
    /* The length of the buffer in bytes */
    int byte_length;
    /* Loaded data */
    unsigned char* data;
};

/* A view into a buffer generally representing a subset of the buffer */
struct gltf_buffer_view {
    /* The user-defined name of this object */
    const char* name;
    /* The index of the buffer */
    gltf_id buffer;
    /* The offset into the buffer in bytes */
    int byte_offset;
    /* The length of the bufferView in bytes */
    int byte_length;
    /* The stride, in bytes */
    int byte_stride;
    /* The target that the GPU buffer should be bound to */
    int target;
};

/* Indices of those attributes that deviate from their initialization value */
struct gltf_accessor_sparse_indices {
    /* The index of the bufferView with sparse indices.
     * Referenced bufferView can't have ARRAY_BUFFER or ELEMENT_ARRAY_BUFFER target */
    gltf_id buffer_view;
    /* The offset relative to the start of the bufferView in bytes. Must be aligned */
    int byte_offset;
    /* The indices data type */
    int component_type;
};

/* Array of size `accessor.sparse.count` times number of components storing
 * the displaced accessor attributes pointed by `accessor.sparse.indices` */
struct gltf_accessor_sparse_values {
    /* The index of the bufferView with sparse values.
     * Referenced bufferView can't have ARRAY_BUFFER or ELEMENT_ARRAY_BUFFER target */
    gltf_id buffer_view;
    /* The offset relative to the start of the bufferView in bytes. Must be aligned */
    int byte_offset;
};

/* Sparse storage of attributes that deviate from their initialization value */
struct gltf_accessor_sparse {
    /* Number of entries stored in the sparse array */
    int count;
    /* Index array of size `count` that points to those accessor attributes
     * that deviate from their initialization value. Indices must strictly increase */
    struct gltf_accessor_sparse_indices* indices;
    /* Array of size `count` times number of components, storing the displaced accessor
     * attributes pointed by `indices`. Substituted values must have the same
     * `componentType` and number of components as the base accessor */
    struct gltf_accessor_sparse_values* values;
};

/* A typed view into a bufferView.
 * A bufferView contains raw binary data.
 * An accessor provides a typed view into a bufferView or a subset of a bufferView
 * similar to how WebGL's `vertexAttribPointer()` defines an attribute in a buffer */
struct gltf_accessor {
    /* The user-defined name of this object */
    const char* name;
    /* The index of the bufferView */
    gltf_id buffer_view;
    /* The offset relative to the start of the bufferView in bytes */
    int byte_offset;
    /* The datatype of components in the attribute */
    int component_type;
    /* Specifies whether integer data values should be normalized */
    int normalized;
    /* The number of attributes referenced by this accessor */
    int count;
    /* Specifies if the attribute is a scalar, vector, or matrix */
    const char* type;
    /* Maximum value of each component in this attribute */
    float* max;
    size_t num_max;
    /* Minimum value of each component in this attribute */
    float* min;
    size_t num_min;
    /* Sparse storage of attributes that deviate from their initialization value */
    struct gltf_accessor_sparse* sparse;
};

/* Metadata about the glTF asset */
struct gltf_asset {
    /* A copyright message suitable for display to credit the content creator */
    const char* copyright;
    /* Tool that generated this glTF model. Useful for debugging */
    const char* generator;
    /* The glTF version that this asset targets */
    const char* version;
    /* The minimum glTF version that this asset targets */
    const char* min_version;
};

/* The root object for a glTF asset */
struct gltf {
    /* Metadata about the glTF asset */
    struct gltf_asset* asset;
    /* The index of the default scene */
    gltf_id scene;
    /* An array of accessors */
    struct gltf_accessor* accessors;
    size_t num_accessors;
    /* An array of keyframe animations */
    struct gltf_animation* animations;
    size_t num_animations;
    /* An array of buffers */
    struct gltf_buffer* buffers;
    size_t num_buffers;
    /* An array of bufferViews */
    struct gltf_buffer_view* buffer_views;
    size_t num_buffer_views;
    /* An array of cameras */
    struct gltf_camera* cameras;
    size_t num_cameras;
    /* An array of images */
    struct gltf_image* images;
    size_t num_images;
    /* An array of materials */
    struct gltf_material* materials;
    size_t num_materials;
    /* An array of meshes */
    struct gltf_mesh* meshes;
    size_t num_meshes;
    /* An array of nodes */
    struct gltf_node* nodes;
    size_t num_nodes;
    /* An array of samplers */
    struct gltf_sampler* samplers;
    size_t num_samplers;
    /* An array of scenes */
    struct gltf_scene* scenes;
    size_t num_scenes;
    /* An array of skins */
    struct gltf_skin* skins;
    size_t num_skins;
    /* An array of textures */
    struct gltf_texture* textures;
    size_t num_textures;
    /* Names of glTF extensions used somewhere in this asset */
    const char** extensions_used;
    size_t num_extensions_used;
    /* Names of glTF extensions required to properly load this asset */
    const char** extensions_required;
    size_t num_extensions_required;
};

struct gltf* gltf_file_load(const char* filepath);
void gltf_destroy(struct gltf* gltf);
struct scene* gltf_to_scene(const struct gltf* gltf);

#endif /* ! _GLTF_H_ */
