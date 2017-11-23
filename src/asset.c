#include <energycore/asset.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <glad/glad.h>

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

/* Checks and shows last shader compile error */
void gl_check_last_compile_error(GLuint id)
{
    /* Check if last compile was successful */
    GLint compileStatus;
    glGetShaderiv(id, GL_COMPILE_STATUS, &compileStatus);
    if (compileStatus == GL_FALSE) {
        /* Gather the compile log size */
        GLint logLength;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength != 0) {
            /* Fetch and print log */
            GLchar* buf = malloc(logLength * sizeof(GLchar));
            glGetShaderInfoLog(id, logLength, 0, buf);
            fprintf(stderr, "Shader error:\n%s\n", buf);
            free(buf);
            exit(EXIT_FAILURE);
        }
    }
}

/* Checks and shows last shader link error */
void gl_check_last_link_error(GLuint id)
{
    /* Check if last link was successful */
    GLint status;
    glGetProgramiv(id, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        /* Gather the link log size */
        GLint logLength;
        glGetProgramiv(id, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength != 0) {
            /* Fetch and print log */
            GLchar* buf = malloc(logLength * sizeof(GLchar));
            glGetProgramInfoLog(id, logLength, 0, buf);
            fprintf(stderr, "Shader program error:\n%s\n", buf);
            free(buf);
            exit(EXIT_FAILURE);
        }
    }
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
 * Shader Loading
 *-----------------------------------------------------------------*/
unsigned int shader_from_srcs(const char* vs_src, const char* gs_src, const char* fs_src)
{
    /* Vertex */
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vs_src, 0);
    glCompileShader(vs);
    gl_check_last_compile_error(vs);
    /* Geometry */
    GLuint gs = 0;
    if (gs_src) {
        gs = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(gs, 1, &gs_src, 0);
        glCompileShader(gs);
        gl_check_last_compile_error(gs);
    }
    /* Fragment */
    GLuint fs = 0;
    if (fs_src) {
        fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &fs_src, 0);
        glCompileShader(fs);
        gl_check_last_compile_error(fs);
    }
    /* Create program */
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    if (gs_src)
        glAttachShader(prog, gs);
    if (fs_src)
        glAttachShader(prog, fs);
    glLinkProgram(prog);
    gl_check_last_link_error(prog);
    /* Free unnecessary resources */
    glDeleteShader(vs);
    if (gs_src)
        glDeleteShader(gs);
    glDeleteShader(fs);
    return prog;
}
