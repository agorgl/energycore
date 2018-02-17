#include <energycore/asset.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "opengl.h"
#include "tar.h"

/*-----------------------------------------------------------------
 * Helpers
 *-----------------------------------------------------------------*/
/* Reads file from disk to memory allocating needed space */
static void* read_file_to_mem_buf(const char* fpath)
{
    /* Check file for existence */
    FILE* f = 0;
    f = fopen(fpath, "rb");
    if (!f)
        return 0;

    /* Gather size */
    fseek(f, 0, SEEK_END);
    long file_sz = ftell(f);
    if (file_sz == -1) {
        fclose(f);
        return 0;
    }

    /* Read contents into memory buffer */
    rewind(f);
    void* data_buf = malloc(file_sz);
    fread(data_buf, 1, file_sz, f);

    return data_buf;
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
    void* data_buf = read_file_to_mem_buf(fpath);

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

/*-----------------------------------------------------------------
 * Hdr Loading
 *-----------------------------------------------------------------*/
/* Standard conversion from RGBE to float pixels */
/* note: Ward uses ldexp(col+0.5,exp-(128+8)).  However we wanted pixels */
/*       in the range [0,1] to map back into the range [0,1].            */
static inline void rgbe2float(float* red, float* green, float* blue, unsigned char rgbe[4])
{
    float f;
    if (rgbe[3]) { /* nonzero pixel */
        f = ldexp(1.0, rgbe[3] - (int)(128 + 8));
        *red   = rgbe[0] * f;
        *green = rgbe[1] * f;
        *blue  = rgbe[2] * f;
    } else
        *red = *green = *blue = 0.0;
}

static void hdr_read_uncompressed(float* out, unsigned char* in, unsigned int width, unsigned int height)
{
    unsigned int num_pixels = width * height;
    while(num_pixels-- > 0) {
        rgbe2float(&out[0],&out[1], &out[2], in);
        in += 4; out += 3;
    }
}

static void hdr_read_rle(float* out, unsigned char* in, unsigned int width, unsigned int height)
{
    /* Read in each successive scanline */
    unsigned char* scanline_buffer = 0;
    while (height > 0) {
        /* Read next 4 bytes into memory buffer */
        unsigned char rgbe[4];
        memcpy(rgbe, in, sizeof(rgbe)); in += sizeof(rgbe);
        if ((rgbe[0] != 2) || (rgbe[1] != 2) || (rgbe[2] & 0x80)) {
            /* This file is not run length encoded */
            rgbe2float(&out[0], &out[1], &out[2], rgbe);
            out += 3;
            free(scanline_buffer);
            hdr_read_uncompressed(out, in, width, height - 1);
            return;
        }

        if ((((int)rgbe[2]) << 8 | rgbe[3]) != (int)width) {
            free(scanline_buffer);
            assert(0 && "wrong scanline width");
        }

        if (scanline_buffer == 0)
            scanline_buffer = malloc(width * 4);

        /* Read each of the four channels for the scanline into the buffer */
        unsigned char* ptr = &scanline_buffer[0];
        for (unsigned int i = 0; i < 4; ++i) {
            unsigned char* ptr_end = &scanline_buffer[(i + 1) * width];
            while (ptr < ptr_end) {
                unsigned char buf[2];
                memcpy(buf, in, sizeof(buf)); in += sizeof(buf);
                if (buf[0] > 128) {
                    /* A run of the same value */
                    int count = buf[0] - 128;
                    if ((count == 0) || (count > ptr_end - ptr)) {
                        free(scanline_buffer);
                        assert(0 && "bad scanline data");
                    }
                    while (count-- > 0)
                        *ptr++ = buf[1];
                } else {
                    /* A non-run */
                    int count = buf[0];
                    if ((count == 0) || (count > ptr_end - ptr)) {
                        free(scanline_buffer);
                        assert(0 && "bad scanline data");
                    }
                    *ptr++ = buf[1];
                    if (--count > 0) {
                        memcpy(ptr, in, count); in += count;
                        ptr += count;
                    }
                }
            }
        }

        /* Now convert data from buffer into floats */
        for (unsigned int i = 0; i < width; ++i) {
            rgbe[0] = scanline_buffer[i];
            rgbe[1] = scanline_buffer[i + width];
            rgbe[2] = scanline_buffer[i + 2 * width];
            rgbe[3] = scanline_buffer[i + 3 * width];
            rgbe2float(&out[0], &out[1], &out[2], rgbe);
            out += 3;
        }
        height--;
    }
    free(scanline_buffer);
}

int texture_data_from_hdr(float** data, unsigned int* w, unsigned int* h, const char* fpath)
{
    /* Read file */
    void* data_buf = read_file_to_mem_buf(fpath);
    const char* p = data_buf;
    unsigned int tex = 0;

    /* Check magic */
    const char* magics[] = {"#?RADIANCE\n", "#?RGBE\n"};
    int is_hdr = 0;
    for (unsigned int i = 0; i < 2; ++i) {
        size_t mlen = strlen(magics[i]);
        if (memcmp(p, magics[i], mlen) == 0) {
            p += mlen;
            is_hdr = 1;
            break;
        }
    }
    if (!is_hdr)
        goto cleanup;

    /* Parse headers */
    int is_rle_rgbe = 0;
    for (;;) {
        if (p[0] == '#') {
            /* Comment, skip */
            p = strchr(p, '\n') + 1;
        } else {
            const char* eol = strchr(p, '\n');
            const char* c = p;
            while (*c == ' ' && c < eol) ++c;
            if (c == eol) {
                /* End of header */
                ++p;
                break;
            } else {
                if (strncmp(p, "FORMAT=32-bit_rle_rgbe", eol - p) == 0)
                    is_rle_rgbe = 1;
            }
            p = eol + 1;
        }
    }
    if (!is_rle_rgbe)
        goto cleanup;

    /* Parse dimensions */
    int width = 0, height = 0;
    sscanf(p, "-Y %d +X %d", &height, &width);
    p = strchr(p, '\n') + 1;

    /* Gather data */
    float* img_data = calloc(1, width * height * 3 * sizeof(float));
    if ((width < 8) || (width > 0x7fff)) {
        /* Run length encoding is not allowed so read flat */
        hdr_read_uncompressed(img_data, (unsigned char*)p, width, height);
    } else {
        hdr_read_rle(img_data, (unsigned char*)p, width, height);
    }

    /* Return data */
    *data = img_data;
    *w = width;
    *h = height;

cleanup:
    free(data_buf);
    return tex;
}

unsigned int texture_from_hdr(const char* fpath)
{
    /* Gather texture data */
    float* data; unsigned int width, height;
    texture_data_from_hdr(&data, &width, &height, fpath);

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
    float (*data)[3]; unsigned int width, height;
    texture_data_from_hdr((float**)&data, &width, &height, fpath);

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
