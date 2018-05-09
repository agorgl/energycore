#include "resource.h"
#include <string.h>
#include <assert.h>
#include "opengl.h"
#include "asset.h"
#include "vcopt.h"

int rid_null(rid id)
{
    return !slot_map_key_valid(id);
}

void resmgr_init(struct resmgr* rmgr)
{
    slot_map_init(&rmgr->textures, sizeof(struct render_texture));
    slot_map_init(&rmgr->materials, sizeof(struct render_material));
    slot_map_init(&rmgr->meshes, sizeof(struct render_mesh));
}

static void render_texture_destroy(struct render_texture* rt)
{
    glDeleteTextures(1, &rt->id);
}

static void render_mesh_destroy(struct render_mesh* rm)
{
    for (size_t i = 0; i < rm->num_shapes; ++i) {
        struct render_shape* s = &rm->shapes[i];
        glDeleteBuffers(1, &s->ebo);
        glDeleteBuffers(1, &s->vbo);
        glDeleteVertexArrays(1, &s->vao);
    }
}

void resmgr_destroy(struct resmgr* rmgr)
{
    for (size_t i = 0; i < rmgr->textures.size; ++i)
        render_texture_destroy(slot_map_data(&rmgr->textures, i));
    slot_map_destroy(&rmgr->textures);

    slot_map_destroy(&rmgr->materials);

    for (size_t i = 0; i < rmgr->meshes.size; ++i)
        render_mesh_destroy(slot_map_data(&rmgr->meshes, i));
    slot_map_destroy(&rmgr->meshes);
}

static GLint wrap_mode(enum texture_wrap w)
{
    switch (w) {
        case TEXTURE_WRAP_REPEAT:
            return GL_REPEAT;
        case TEXTURE_WRAP_CLAMP:
            return GL_CLAMP_TO_BORDER;
        case TEXTURE_WRAP_MIRROR:
            return GL_MIRRORED_REPEAT;
    }
    return GL_REPEAT;
}

static GLint filter_mode(enum texture_filter f)
{
    switch (f) {
        case TEXTURE_FILTER_LINEAR:
            return GL_LINEAR;
        case TEXTURE_FILTER_NEAREST:
            return GL_NEAREST;
        case TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
            return GL_LINEAR_MIPMAP_LINEAR;
        case TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
            return GL_NEAREST_MIPMAP_NEAREST;
        case TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
            return GL_LINEAR_MIPMAP_NEAREST;
        case TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
            return GL_NEAREST_MIPMAP_LINEAR;
    }
    return GL_LINEAR;
}

static void setup_texture_parameters(struct render_texture_info* rti)
{
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4.0f);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, rti->filter_min);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, rti->filter_mag);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, rti->wrap_s);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, rti->wrap_t);
}

static void setup_default_texture_parameters()
{
    struct render_texture_info rti = {
        .wrap_s = GL_REPEAT,
        .wrap_t = GL_REPEAT,
        .filter_mag = GL_LINEAR,
        .filter_min = GL_LINEAR_MIPMAP_LINEAR,
        .scale = 1.0
    };
    setup_texture_parameters(&rti);
}

static GLint gl_internal_format(image im)
{
#define check_pair(ch, bd) if (im.channels == ch && im.bit_depth == bd)
    check_pair(1, 8)
        return GL_R8;
    check_pair(1, 16)
        return GL_R16F;
    check_pair(2, 8)
        return GL_RG8;
    check_pair(2, 16)
        return GL_RG16F;
    check_pair(3, 8)
        return GL_RGB8;
    check_pair(3, 16)
        return GL_RGB16F;
    check_pair(4, 8)
        return GL_RGBA8;
    check_pair(4, 16)
        return GL_RGBA16F;
#undef check_pair
    assert(0 && "Could not find suitable internal format");
    return GL_RGBA;
}

static GLenum gl_format(image im)
{
    switch(im.channels) {
        case 1:
            return GL_RED;
        case 2:
            return GL_RG;
        case 3:
            return GL_RGB;
        case 4:
            return GL_RGBA;
        default:
            break;
    }
    assert(0 && "Could not find suitable format");
    return GL_RGBA;
}

static GLenum gl_pixel_data_type(image im)
{
    switch (im.bit_depth) {
        case 8:
            return GL_UNSIGNED_BYTE;
        case 16:
            return GL_FLOAT;
        case 32:
            return GL_FLOAT;
        default:
            break;
    }
    assert(0 && "Could not find pixel data type");
    return GL_UNSIGNED_BYTE;
}

static GLuint upload_texture_img(image im)
{
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    if (im.compression_type == 0) {
        glTexImage2D(GL_TEXTURE_2D,
            0, gl_internal_format(im), im.w, im.h,
            0, gl_format(im), gl_pixel_data_type(im),
            im.data);
    } else {
        glCompressedTexImage2D(
            GL_TEXTURE_2D, 0,
            im.compression_type,
            im.w, im.h, 0,
            im.sz, im.data);
    }
    glGenerateMipmap(GL_TEXTURE_2D);
    return id;
}

static int hcross_face_map[6][2] = {
    {2, 1}, /* Pos X */
    {0, 1}, /* Neg X */
    {1, 0}, /* Pos Y */
    {1, 2}, /* Neg Y */
    {1, 1}, /* Pos Z */
    {3, 1}  /* Neg Z */
};

/* In pixels */
static size_t hcross_face_offset(int face, size_t face_size)
{
    size_t stride = 4 * face_size;
    return hcross_face_map[face][1] * face_size * stride + hcross_face_map[face][0] * face_size;
}

static unsigned int tex_env_from_hcross(void* data, unsigned int width, unsigned int height, unsigned int channels)
{
    (void) height;
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, id);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    /* Set row read stride */
    unsigned int stride = width;
    unsigned int face_size = width / 4;
    glPixelStorei(GL_UNPACK_ROW_LENGTH, stride);

    /* Upload image data */
    for (int i = 0; i < 6; ++i) {
        int target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + i;
        glTexImage2D(
            target, 0,
            channels == 4 ? GL_SRGB8_ALPHA8 : GL_SRGB8,
            face_size, face_size, 0,
            channels == 4 ? GL_RGBA : GL_RGB,
            GL_UNSIGNED_BYTE,
            data + hcross_face_offset(i, face_size) * channels);
    }
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    return id;
}

rid resmgr_add_texture(struct resmgr* rmgr, struct texture* tex)
{
    struct render_texture rt = {
        .id = upload_texture_img(tex->img),
    };
    setup_default_texture_parameters();
    return slot_map_insert(&rmgr->textures, &rt);
}

rid resmgr_add_texture_env(struct resmgr* rmgr, struct texture* tex, int hcross)
{
    (void) hcross;
    struct render_texture rt = {
        .id = tex_env_from_hcross(tex->img.data, tex->img.w, tex->img.h, tex->img.channels),
    };
    setup_default_texture_parameters();
    return slot_map_insert(&rmgr->textures, &rt);
}

rid resmgr_add_texture_file(struct resmgr* rmgr, const char* filepath)
{
    const char* ext = strrchr(filepath, '.');
    GLuint id = 0;
    if (strcmp(ext, ".hdr") == 0)
        id = texture_cubemap_from_hdr(filepath);
    else if (strcmp(ext, ".ktx") == 0)
        id = texture_from_ktx(filepath);
    else
        return INVALID_RID;
    struct render_texture rt = {
        .id = id,
    };
    setup_default_texture_parameters();
    return slot_map_insert(&rmgr->textures, &rt);
}

struct render_texture_info resmgr_texture_info_populate(struct texture_info* ti)
{
    return (struct render_texture_info) {
        .wrap_s = wrap_mode(ti->wrap_s),
        .wrap_t = wrap_mode(ti->wrap_t),
        .filter_mag = filter_mode(ti->filter_mag),
        .filter_min = filter_mode(ti->filter_min),
        .scale = ti->scale
    };
}

void resmgr_default_rmat(struct render_material* rmat)
{
    struct material mat;
    material_init(&mat);
    struct texture_info dti;
    texture_info_default(&dti);

    *rmat = (struct render_material) {
        .type = mat.type,
        .ke   = mat.ke,
        .kd   = mat.kd,
        .ks   = mat.ks,
        .kr   = mat.kr,
        .kt   = mat.kt,
        .rs   = mat.rs,
        .op   = mat.op,
        .ke_txt    = INVALID_RID,
        .kd_txt    = INVALID_RID,
        .ks_txt    = INVALID_RID,
        .kr_txt    = INVALID_RID,
        .kt_txt    = INVALID_RID,
        .rs_txt    = INVALID_RID,
        .bump_txt  = INVALID_RID,
        .disp_txt  = INVALID_RID,
        .norm_txt  = INVALID_RID,
        .occ_txt   = INVALID_RID,
        .kdd_txt   = INVALID_RID,
        .normd_txt = INVALID_RID,
        .ke_txt_info    = resmgr_texture_info_populate(&dti),
        .kd_txt_info    = resmgr_texture_info_populate(&dti),
        .ks_txt_info    = resmgr_texture_info_populate(&dti),
        .kr_txt_info    = resmgr_texture_info_populate(&dti),
        .kt_txt_info    = resmgr_texture_info_populate(&dti),
        .rs_txt_info    = resmgr_texture_info_populate(&dti),
        .bump_txt_info  = resmgr_texture_info_populate(&dti),
        .disp_txt_info  = resmgr_texture_info_populate(&dti),
        .norm_txt_info  = resmgr_texture_info_populate(&dti),
        .occ_txt_info   = resmgr_texture_info_populate(&dti),
        .kdd_txt_info   = resmgr_texture_info_populate(&dti),
        .normd_txt_info = resmgr_texture_info_populate(&dti),
    };
}

rid resmgr_add_material(struct resmgr* rmgr, struct render_material* rmat)
{
    return slot_map_insert(&rmgr->materials, rmat);
}

static void mesh_calc_aabb(struct shape* s, float min[3], float max[3])
{
    memset(min, 0, 3 * sizeof(float));
    memset(max, 0, 3 * sizeof(float));
    for (size_t i = 0; i < s->num_pos; ++i) {
        vec3f* p = s->pos + i;
        for (unsigned int j = 0; j < 3; ++j) {
            if (((float*)p)[j] < min[j])
                min[j] = ((float*)p)[j];
            else if (((float*)p)[j] > max[j])
                max[j] = ((float*)p)[j];
        }
    }
}

static void optimize_indices(unsigned int* indices, size_t num_indices, size_t num_verts)
{
    unsigned int* optimized_indices = calloc(num_indices, sizeof(*optimized_indices));
    forsyth_reorder_indices(optimized_indices, indices, num_indices / 3, num_verts);
    memcpy(indices, optimized_indices, num_indices * sizeof(*indices));
    free(optimized_indices);
}

void add_shape(struct render_shape* rsh, struct shape* sh)
{
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    size_t vsz = \
        sizeof(*sh->pos)      /* Position */
      + sizeof(*sh->texcoord) /* UV */
      + sizeof(*sh->norm)     /* Normal */
      + sizeof(*sh->tangsp);  /* Tangent */
    size_t num_verts = sh->num_pos;
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, num_verts * vsz, 0, GL_STATIC_DRAW);

    size_t offset = 0;
    void* vbuf = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    const GLuint pos_attrib = 0;
    glEnableVertexAttribArray(pos_attrib);
    glVertexAttribPointer(pos_attrib, 3, GL_FLOAT, GL_FALSE, sizeof(*sh->pos), (GLvoid*)offset);
    memcpy(vbuf + offset, sh->pos, sizeof(*sh->pos) * num_verts);
    offset += sizeof(*sh->pos) * num_verts;

    const GLuint uv_attrib = 1;
    glEnableVertexAttribArray(uv_attrib);
    glVertexAttribPointer(uv_attrib, 2, GL_FLOAT, GL_FALSE, sizeof(*sh->texcoord), (GLvoid*)offset);
    memcpy(vbuf + offset, sh->texcoord, sizeof(*sh->texcoord) * num_verts);
    offset += sizeof(*sh->texcoord) * num_verts;

    const GLuint nm_attrib = 2;
    glEnableVertexAttribArray(nm_attrib);
    glVertexAttribPointer(nm_attrib, 3, GL_FLOAT, GL_FALSE, sizeof(*sh->norm), (GLvoid*)offset);
    memcpy(vbuf + offset, sh->norm, sizeof(*sh->norm) * num_verts);
    offset += sizeof(*sh->norm) * num_verts;

    const GLuint tn_attrib = 3;
    glEnableVertexAttribArray(tn_attrib);
    glVertexAttribPointer(tn_attrib, 3, GL_FLOAT, GL_FALSE, sizeof(*sh->tangsp), (GLvoid*)offset);
    memcpy(vbuf + offset, sh->tangsp, sizeof(*sh->tangsp) * num_verts);
    offset += sizeof(*sh->tangsp) * num_verts;
    glUnmapBuffer(GL_ARRAY_BUFFER);

    optimize_indices((unsigned int*)sh->triangles, sh->num_triangles * 3, sh->num_pos);
    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sh->num_triangles * sizeof(*sh->triangles), sh->triangles, GL_STATIC_DRAW);
    size_t num_elems = sh->num_triangles * 3;

    *rsh = (struct render_shape) {
        .vao = vao,
        .vbo = vbo,
        .ebo = ebo,
        .num_elems = num_elems
    };
    mesh_calc_aabb(sh, rsh->bb_min, rsh->bb_max);
}

rid resmgr_add_mesh(struct resmgr* rmgr, struct mesh* m)
{
    assert(m->num_shapes < 16 && "Unsupported number of shapes");
    struct render_mesh rm;
    memset(&rm, 0, sizeof(rm));
    rm.num_shapes = m->num_shapes;
    for (size_t i = 0; i < m->num_shapes; ++i) {
        struct shape* shp = m->shapes[i];
        shape_compute_normals(shp);
        shape_compute_tangent_frame(shp);
        add_shape(&rm.shapes[i], shp);
    }
    return slot_map_insert(&rmgr->meshes, &rm);
}

struct render_texture* resmgr_get_texture(struct resmgr* rmgr, rid id)
{
    return slot_map_key_valid(id) ? slot_map_lookup(&rmgr->textures, id) : 0;
}

struct render_material* resmgr_get_material(struct resmgr* rmgr, rid id)
{
    return slot_map_key_valid(id) ? slot_map_lookup(&rmgr->materials, id) : 0;
}

struct render_mesh* resmgr_get_mesh(struct resmgr* rmgr, rid id)
{
    return slot_map_key_valid(id) ? slot_map_lookup(&rmgr->meshes, id) : 0;
}
