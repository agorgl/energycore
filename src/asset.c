#include <energycore/asset.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "opengl.h"
#include "gltf.h"
#include "wvfobj.h"
#include "hdrfile.h"
#include "ktxfile.h"
#include "ddsfile.h"
#include "tar.h"

/*-----------------------------------------------------------------
 * Helpers
 *-----------------------------------------------------------------*/
/* Reads file from disk to memory allocating needed space */
void read_file_to_mem_buf(void** buf, size_t* buf_sz, const char* fpath)
{
    /* Zero out input */
    *buf = 0;
    *buf_sz = 0;

    /* Check file for existence */
    FILE* f = 0;
    f = fopen(fpath, "rb");
    if (!f)
        return;

    /* Gather size */
    fseek(f, 0, SEEK_END);
    long file_sz = ftell(f);
    if (file_sz == -1) {
        fclose(f);
        return;
    }

    /* Read contents into memory buffer */
    rewind(f);
    void* data_buf = calloc(1, file_sz + 1);
    fread(data_buf, 1, file_sz, f);
    fclose(f);

    /* Store buffer and size */
    *buf = data_buf;
    *buf_sz = file_sz;
}

/*-----------------------------------------------------------------
 * Image Loading
 *-----------------------------------------------------------------*/
static int image_type_supported(const char* ext)
{
    if (strcmp(ext, ".ktx") == 0)
        return 1;
    if (strcmp(ext, ".dds") == 0)
        return 1;
    return 0;
}

image image_from_buffer(const void* buffer, size_t sz, const char* fhint)
{
    image im = {};
    if (!image_type_supported(fhint))
        return im;

    if (strcmp(fhint, ".ktx") == 0) {
        struct ktx_image ktx;
        ktx_image_read(&ktx, buffer);
        im = (image) {
            .w                = ktx.width,
            .h                = ktx.height,
            .channels         = ktx.channels,
            .bit_depth        = ktx.bit_depth,
            .data             = ktx.data,
            .sz               = ktx.data_sz,
            .compression_type = ktx.compression_type
        };
        ktx.data = 0; /* Move ownership to returned struct */
        ktx_image_free(&ktx);
    } else if (strcmp(fhint, ".dds") == 0) {
        struct dds_image* dds = dds_image_read(buffer, sz);
        if (dds->type == DDS_TEXTURE_TYPE_2D) {
            int channels = 0, compression_type = 0;
            switch (dds->format) {
                case DDS_TEXTURE_FORMAT_RGBA:
                case DDS_TEXTURE_FORMAT_BGRA:
                    channels = 4;
                    break;
                case DDS_TEXTURE_FORMAT_RGB:
                case DDS_TEXTURE_FORMAT_BGR:
                    channels = 3;
                    break;
                case DDS_TEXTURE_FORMAT_R:
                    channels = 1;
                    break;
                case DDS_TEXTURE_FORMAT_COMPRESSED_RGBA_S3TC_DXT1:
                    compression_type = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
                    break;
                case DDS_TEXTURE_FORMAT_COMPRESSED_RGBA_S3TC_DXT3:
                    compression_type = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
                    break;
                case DDS_TEXTURE_FORMAT_COMPRESSED_RGBA_S3TC_DXT5:
                    compression_type = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
                    break;
                default:
                    compression_type = 0;
                    break;
            }
            struct dds_mipmap* m = &(dds->surfaces[0].mipmaps[0]);
            im = (image) {
                .w                = m->width,
                .h                = m->height,
                .channels         = channels,
                .bit_depth        = compression_type != 0 ? 0 : 8,
                .data             = m->pixels,
                .sz               = m->size,
                .compression_type = compression_type
            };
            m->pixels = 0; /* Move ownership to returned struct */
        }
        dds_image_free(dds);
    }
    return im;
}

image image_from_file(const char* fpath)
{
    const char* ext = strrchr(fpath, '.');
    image im = {};
    if (!image_type_supported(ext))
        return im;

    void* fdata; size_t fsize;
    read_file_to_mem_buf(&fdata, &fsize, fpath);
    im = image_from_buffer(fdata, fsize, ext);
    free(fdata);

    return im;
}

/*-----------------------------------------------------------------
 * Scene Loading
 *-----------------------------------------------------------------*/
struct scene* scene_from_file(const char* fpath)
{
    const char* ext = strrchr(fpath, '.');
    if (strcmp(ext, ".gltf") == 0) {
        struct gltf* gltf = gltf_file_load(fpath);
        struct scene* scene = gltf_to_scene(gltf);
        gltf_destroy(gltf);
        return scene;
    } else if (strcmp(ext, ".obj") == 0) {
        void* fdata; size_t fsize;
        read_file_to_mem_buf(&fdata, &fsize, fpath);
        obj_model_t* obj = obj_model_parse(fdata, fsize);
        struct scene* scene = obj_to_scene(obj);
        obj_model_free(obj);
        free(fdata);
        return scene;
    }
    return 0;
}

/*-----------------------------------------------------------------
 * Ktx Loading
 *-----------------------------------------------------------------*/
struct ktx_header {
    uint8_t  identifier[12];
    uint32_t endianness;
    uint32_t gl_type;
    uint32_t gl_type_size;
    uint32_t gl_format;
    uint32_t gl_internal_format;
    uint32_t gl_base_internal_format;
    uint32_t pixel_width;
    uint32_t pixel_height;
    uint32_t pixel_depth;
    uint32_t number_of_array_elements;
    uint32_t number_of_faces;
    uint32_t number_of_mipmap_levels;
    uint32_t bytes_of_key_value_data;
};

#define KTX_MAGIC { 0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A }

unsigned int texture_from_ktx(const char* fpath)
{
    /* Read file */
    void* data_buf; size_t data_buf_sz;
    read_file_to_mem_buf(&data_buf, &data_buf_sz, fpath);

    /* Header read and check */
    const unsigned char magic[12] = KTX_MAGIC;
    struct ktx_header* h = (struct ktx_header*)data_buf;
    if (memcmp(h->identifier, magic, sizeof(magic)) != 0) {
        free(data_buf);
        return 0;
    }

    /* Texture data iterator */
    const void* ptr = data_buf + sizeof(struct ktx_header) + h->bytes_of_key_value_data;

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    for (unsigned int level = 0; level < ((h->number_of_mipmap_levels == 0) ? 1 : h->number_of_mipmap_levels); level++) {
        uint32_t image_size = *((uint32_t*)ptr);
        ptr += sizeof(uint32_t);

        const void* data = ptr;
        ptr += image_size;

        if (h->gl_type != 0) {
            glTexImage2D(
                GL_TEXTURE_2D,
                level, h->gl_internal_format,
                (h->pixel_width  >> level),
                (h->pixel_height >> level),
                0, h->gl_format, h->gl_type, data);
        } else {
            glCompressedTexImage2D(
                GL_TEXTURE_2D, level,
                h->gl_internal_format,
                (h->pixel_width  >> level),
                (h->pixel_height >> level),
                0, image_size, data);
        }

        int skip = (3 - ((image_size + 3) % 4));
        if (skip > 0)
            ptr += skip;
    }

    if (h->number_of_mipmap_levels == 0) {
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    } else if (h->number_of_mipmap_levels == 1) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, h->number_of_mipmap_levels);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    free(data_buf);
    return texture;
}

unsigned int texture_from_hdr(const char* fpath)
{
    /* Gather texture data */
    void* fdata; size_t fsize;
    read_file_to_mem_buf(&fdata, &fsize, fpath);
    float* data; unsigned int width, height;
    hdr_image_read(&data, &width, &height, fdata);
    free(fdata);

    /* Upload to GPU */
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    free(data);
    return tex;
}

/*
 *              +----------+
 *              | +---->+x |
 *              | |        |
 *              | |  +y    |
 *              |+z      2 |
 *   +----------+----------+----------+----------+
 *   | +---->+z | +---->+x | +---->-z | +---->-x |
 *   | |        | |        | |        | |        |
 *   | |  -x    | |  +z    | |  +x    | |  -z    |
 *   |-y      1 |-y      4 |-y      0 |-y      5 |
 *   +----------+----------+----------+----------+
 *              | +---->+x |
 *              | |        |
 *              | |  -y    |
 *              |-z      3 |
 *              +----------+
 */
static float cm_face_uv_vectors[6][3][3] =
{
    { /* +x face */
        {  0.0f,  0.0f, -1.0f }, /* u -> -z */
        {  0.0f, -1.0f,  0.0f }, /* v -> -y */
        {  1.0f,  0.0f,  0.0f }, /* +x face */
    },
    { /* -x face */
        {  0.0f,  0.0f,  1.0f }, /* u -> +z */
        {  0.0f, -1.0f,  0.0f }, /* v -> -y */
        { -1.0f,  0.0f,  0.0f }, /* -x face */
    },
    { /* +y face */
        {  1.0f,  0.0f,  0.0f }, /* u -> +x */
        {  0.0f,  0.0f,  1.0f }, /* v -> +z */
        {  0.0f,  1.0f,  0.0f }, /* +y face */
    },
    { /* -y face */
        {  1.0f,  0.0f,  0.0f }, /* u -> +x */
        {  0.0f,  0.0f, -1.0f }, /* v -> -z */
        {  0.0f, -1.0f,  0.0f }, /* -y face */
    },
    { /* +z face */
        {  1.0f,  0.0f,  0.0f }, /* u -> +x */
        {  0.0f, -1.0f,  0.0f }, /* v -> -y */
        {  0.0f,  0.0f,  1.0f }, /* +z face */
    },
    { /* -z face */
        { -1.0f,  0.0f,  0.0f }, /* u -> -x */
        {  0.0f, -1.0f,  0.0f }, /* v -> -y */
        {  0.0f,  0.0f, -1.0f }, /* -z face */
    }
};

/* u and v should be center adressing and in [-1.0+inv_size..1.0-inv_size] range */
static void cm_texel_coord_to_vec(float* out3f, float u, float v, uint8_t face_id)
{
    /* out = u * face_uv[0] + v * face_uv[1] + face_uv[2]. */
    out3f[0] = cm_face_uv_vectors[face_id][0][0] * u + cm_face_uv_vectors[face_id][1][0] * v + cm_face_uv_vectors[face_id][2][0];
    out3f[1] = cm_face_uv_vectors[face_id][0][1] * u + cm_face_uv_vectors[face_id][1][1] * v + cm_face_uv_vectors[face_id][2][1];
    out3f[2] = cm_face_uv_vectors[face_id][0][2] * u + cm_face_uv_vectors[face_id][1][2] * v + cm_face_uv_vectors[face_id][2][2];

    /* Normalize. */
    const float inv_len = 1.0f / sqrtf(out3f[0] * out3f[0] + out3f[1] * out3f[1] + out3f[2] * out3f[2]);
    out3f[0] *= inv_len;
    out3f[1] *= inv_len;
    out3f[2] *= inv_len;
}

static void dir_to_uv_equi(float uv[2], float v[3])
{
    uv[0] = atan2(v[2], v[0]) / (2 * M_PI) + 0.5;
    uv[1] = asin(v[1]) / M_PI + 0.5;
}

unsigned int texture_cubemap_from_hdr(const char* fpath)
{
    /* Gather texture data */
    void* fdata; size_t fsize;
    read_file_to_mem_buf(&fdata, &fsize, fpath);
    float (*data)[3]; unsigned int width, height;
    hdr_image_read((float**)&data, &width, &height, fdata);
    free(fdata);

    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, id);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    unsigned int face_size = width / 4;
    for (int i = 0; i < 6; ++i) {
        /* Populate face data */
        float (*face_data)[3] = calloc(1, face_size * face_size * sizeof(*face_data));
        for (unsigned int k = 0; k < face_size; ++k) {
            for (unsigned int l = 0; l < face_size; ++l) {
                float texel_size = 1.0 / face_size;
                /* Face, U, V -> Direction */
                float u = ((k + 0.5) * texel_size) * 2.0 - 1.0;
                float v = ((l + 0.5) * texel_size) * 2.0 - 1.0;
                float vec[3];
                cm_texel_coord_to_vec(vec, u, v, i);
                /* Direction -> Equirect U, V */
                float uv_equi[2];
                dir_to_uv_equi(uv_equi, vec);
                /* Sample */
                unsigned int x = (unsigned int)(uv_equi[0] * (width  - 1));
                unsigned int y = (unsigned int)(uv_equi[1] * (height - 1));
                float* p = (float*)(data + y * width + x);
                face_data[l * face_size + k][0] = p[0];
                face_data[l * face_size + k][1] = p[1];
                face_data[l * face_size + k][2] = p[2];
            }
        }
        /* Upload image data */
        int target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + i;
        glTexImage2D(target, 0, GL_RGB16F, face_size, face_size, 0, GL_RGB, GL_FLOAT, face_data);
        free(face_data);
    }
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    free(data);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    return id;
}

/*-----------------------------------------------------------------
 * Shader Loading
 *-----------------------------------------------------------------*/
unsigned int shader_from_srcs(const char* vs_src, const char* gs_src, const char* fs_src)
{
    /* Vertex */
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vs_src, 0);
    glCompileShader(vs);
    /* Geometry */
    GLuint gs = 0;
    if (gs_src) {
        gs = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(gs, 1, &gs_src, 0);
        glCompileShader(gs);
    }
    /* Fragment */
    GLuint fs = 0;
    if (fs_src) {
        fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &fs_src, 0);
        glCompileShader(fs);
    }
    /* Create program */
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    if (gs_src)
        glAttachShader(prog, gs);
    if (fs_src)
        glAttachShader(prog, fs);
    glLinkProgram(prog);
    /* Free unnecessary resources */
    glDeleteShader(vs);
    if (gs_src)
        glDeleteShader(gs);
    glDeleteShader(fs);
    return prog;
}

/*-----------------------------------------------------------------
 * Embedded data
 *-----------------------------------------------------------------*/
extern char _binary_res_dat_start[];
extern char _binary_res_dat_end[];

int embedded_file(void** data, size_t* sz, const char* fpath)
{
    void* tar_data = _binary_res_dat_start;
    size_t tar_sz  = _binary_res_dat_end - _binary_res_dat_start;
    return tar_read_file(tar_data, tar_sz, data, sz, fpath);
}
