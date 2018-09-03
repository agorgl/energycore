#include "ktxfile.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

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

int ktx_image_read(struct ktx_image* ktx, const void* fdata)
{
    /* Header read and check */
    const unsigned char magic[12] = KTX_MAGIC;
    struct ktx_header* h = (struct ktx_header*)fdata;
    if (memcmp(h->identifier, magic, sizeof(magic)) != 0)
        return 0;

    /* Check if endianness matches */
    if (h->endianness != 0x04030201)
        return 0;
    /* Check if array texture */
    if (h->number_of_array_elements > 0)
        return 0;
    /* Check number of faces */
    if (!(h->number_of_faces == 1 || h->number_of_faces == 6))
        return 0;

    /* Texture data iterator */
    const void* ptr = fdata + sizeof(struct ktx_header) + h->bytes_of_key_value_data;

    /* Image byte size and data */
    uint32_t image_size = *((uint32_t*)ptr);
    ptr += sizeof(uint32_t);
    const void* data = ptr;
    ptr += image_size;

    ktx->data             = calloc(1, image_size);
    ktx->data_sz          = image_size;
    memcpy(ktx->data, data, image_size);
    ktx->width            = h->pixel_width;
    ktx->height           = h->pixel_height;
    ktx->channels         = h->gl_format == 0x1907 ? 3 : 4; /* GL_RGB == 0x1907 */
    ktx->bit_depth        = h->gl_type == 0 ? 0 : 8;
    ktx->flags.compressed = h->gl_type == 0;
    ktx->compression_type = h->gl_internal_format;

    int skip = (3 - ((image_size + 3) % 4));
    if (skip > 0)
        ptr += skip;

    return 1;
}

void ktx_image_free(struct ktx_image* ktx)
{
    free(ktx->data);
}
