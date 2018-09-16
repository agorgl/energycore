#include "ddsfile.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

/* The magic of a DDS file */
#define DDS_MAGIC "DDS "

/* Texture contains alpha data; rgbalpha_bit_mask contains valid data. */
#define DDPF_ALPHAPIXELS    0x00000001
/* Used in some older DDS files for alpha channel only uncompressed data
 * (rgb_bit_count contains the alpha channel bitcount, abit_mask contains valid data) */
#define DDPF_ALPHA          0x00000002
/* Texture contains compressed RGB data, four_cc contains valid data */
#define DDPF_FOURCC	        0x00000004
/* Texture contains uncompressed RGB data, rgb_bit_count
 * and the RGB masks (rbit_mask, gbit_mask, bbit_mask) contain valid data */
#define DDPF_RGB            0x00000040
/* Used in some older DDS files for YUV uncompressed data
 * (rgb_bit_count contains the YUV bit count, rbit_mask contains the Y mask, gbit_mask contains the U mask, bbit_mask contains the V mask) */
#define DDPF_YUV            0x00000200
/* Used in some older DDS files for single channel color uncompressed data
 * (rgb_bit_count contains the luminance channel bit count; rbit_mask contains the channel mask).
 * Can be combined with DDPF_ALPHAPIXELS for a two channel DDS file */
#define DDPF_LUMINANCE      0x00020000

/* Indicates, member 'caps' contains valid data.
 * Required in every .dds file. */
#define DDSD_CAPS           0x00000001
/* Indicates, member 'height' contains valid data.
 * Required in every .dds file. */
#define DDSD_HEIGHT	        0x00000002
/* Indicates, member 'width' contains valid data.
 * Required in every .dds file. */
#define DDSD_WIDTH          0x00000004
/* Indicates, member pitch_or_linear_size contains valid data.
 * Required when pitch is provided for an uncompressed texture. */
#define DDSD_PITCH          0x00000008
/* Indicates, member ddspf contains valid data.
 * Required in every .dds file. */
#define DDSD_PIXELFORMAT    0x00001000
/* Indicates, member mip_map_count contains valid data.
 * Required in a mipmapped texture. */
#define DDSD_MIPMAPCOUNT    0x00020000
/* Indicates, member pitch_or_linear_size contains valid data.
 * Required when pitch is provided for a compressed texture. */
#define DDSD_LINEARSIZE     0x00080000
/* Indicates, member depth contains valid data.
 * Required in a depth texture. */
#define DDSD_DEPTH          0x00800000

/* Optional, must be used on any file that contains more than
 * one surface (a mipmap, a cubic environment map, or mipmapped volume texture). */
#define DDSCAPS_COMPLEX     0x00000008
/* Optional, should be used for a mipmap. */
#define DDSCAPS_MIPMAP      0x00400000
/* Required */
#define DDSCAPS_TEXTURE     0x00001000

/* Required for a cube map. */
#define DDSCAPS2_CUBEMAP            0x00000200
/* Required when these surfaces are stored in a cube map. */
#define DDSCAPS2_CUBEMAP_POSITIVEX	0x00000400
/* Required when these surfaces are stored in a cube map. */
#define DDSCAPS2_CUBEMAP_NEGATIVEX	0x00000800
/* Required when these surfaces are stored in a cube map. */
#define DDSCAPS2_CUBEMAP_POSITIVEY	0x00001000
/* Required when these surfaces are stored in a cube map. */
#define DDSCAPS2_CUBEMAP_NEGATIVEY	0x00002000
/* Required when these surfaces are stored in a cube map. */
#define DDSCAPS2_CUBEMAP_POSITIVEZ	0x00004000
/* Required when these surfaces are stored in a cube map. */
#define DDSCAPS2_CUBEMAP_NEGATIVEZ	0x00008000
/* Required for a volume texture. */
#define DDSCAPS2_VOLUME             0x00200000

#define MAKEFOURCC(ch0, ch1, ch2, ch3)  \
    ((uint32_t)(uint8_t)(ch0) | ((uint32_t)(uint8_t)(ch1) << 8) |  \
    ((uint32_t)(uint8_t)(ch2) << 16) | ((uint32_t)(uint8_t)(ch3) << 24 ));
#define FOURCC_DXT1 0x31545844 /* MAKEFOURCC('D','X','T','1') */
#define FOURCC_DXT3 0x33545844 /* MAKEFOURCC('D','X','T','3') */
#define FOURCC_DXT5 0x35545844 /* MAKEFOURCC('D','X','T','5') */

/* Surface pixel format. */
struct dds_pixelformat {
    /* Structure size, set to 32 (bytes) */
    uint32_t size;
    /* Values which indicate what type of data is in the surface */
    uint32_t flags;
    /* Four-character codes for specifying compressed or custom formats.
     * Possible values include: DXT1, DXT2, DXT3, DXT4, or DXT5.
     * A FourCC of DX10 indicates the prescense of the DDS_HEADER_DXT10 extended header,
     * and the dxgiFormat member of that structure indicates the true format.
     * When using a four-character code, flags must include DDPF_FOURCC. */
    uint32_t four_cc;
    /* Number of bits in an RGB (possibly including alpha) format.
     * Valid when flags includes DDPF_RGB, DDPF_LUMINANCE, or DDPF_YUV. */
    uint32_t rgb_bit_count;
    /* Red (or lumiannce or Y) mask for reading color data.
     * For instance, given the A8R8G8B8 format, the red mask would be 0x00ff0000. */
    uint32_t rbit_mask;
    /* Green (or U) mask for reading color data.
     * For instance, given the A8R8G8B8 format, the green mask would be 0x0000ff00. */
    uint32_t gbit_mask;
    /* Blue (or V) mask for reading color data.
     * For instance, given the A8R8G8B8 format, the blue mask would be 0x000000ff. */
    uint32_t bbit_mask;
    /* Alpha mask for reading alpha data.
     * Field flags must include DDPF_ALPHAPIXELS or DDPF_ALPHA.
     * For instance, given the A8R8G8B8 format, the alpha mask would be 0xff000000. */
    uint32_t abit_mask;
};

/* Describes a DDS file header */
struct dds_header {
    /* Size of structure, set to 124 (bytes) */
    uint32_t size;
    /* Flags to indicate which members contain valid data. */
    uint32_t flags;
    /* Surface height (in pixels). */
    uint32_t height;
    /* Surface width (in pixels). */
    uint32_t width;
    /* The pitch or number of bytes per scan line in an uncompressed texture;
     * the total number of bytes in the top level texture for a compressed texture. */
    uint32_t pitch_or_linear_size;
    /* Depth of a volume texture (in pixels), otherwise unused. */
    uint32_t depth;
    /* Number of mipmap levels, otherwise unused. */
    uint32_t mip_map_count;
    /* Unused */
    uint32_t reserved1[11];
    /* The pixel format */
    struct dds_pixelformat ddspf;
    /* Specifies the complexity of the surfaces stored. */
    uint32_t caps;
    /* Additional detail about the surfaces stored. */
    uint32_t caps2;
    /* Unused */
    uint32_t caps3;
    /* Unused */
    uint32_t caps4;
    /* Unused */
    uint32_t reserved2;
};

static int is_compressed(enum dds_texture_format format)
{
    return (format == DDS_TEXTURE_FORMAT_COMPRESSED_RGBA_S3TC_DXT1)
        || (format == DDS_TEXTURE_FORMAT_COMPRESSED_RGBA_S3TC_DXT3)
        || (format == DDS_TEXTURE_FORMAT_COMPRESSED_RGBA_S3TC_DXT5);
}

/* Calculates size of DXTC texture in bytes */
static unsigned int size_dxtc(unsigned int width, unsigned int height, int format)
{
    return ((width + 3) / 4) * ((height + 3) / 4) * (format == DDS_TEXTURE_FORMAT_COMPRESSED_RGBA_S3TC_DXT1 ? 8 : 16);
}

/* Calculates size of uncompressed RGB texture in bytes */
static unsigned int size_rgb(unsigned int width, unsigned int height, int components)
{
    return width * height * components;
}

struct iterator {
    const void* data;
    size_t rem;
};

static void it_read(void* dst, struct iterator* it, size_t bytes)
{
    assert(bytes <= it->rem);
    memcpy(dst, it->data, bytes);
    it->data += bytes;
    it->rem  -= bytes;
}

#define clamp_size(s) (s <= 0 ? 1 : s)

struct dds_image* dds_image_read(const void* fdata, size_t fsz)
{
    /* File data iterator */
    struct iterator it = { fdata, fsz };

    /* Read file marker */
    char filecode[4];
    it_read(&filecode, &it, 4);
    if (strncmp(filecode, DDS_MAGIC, 4) != 0) {
        /* Not a DDS file */
        return 0;
    }

    /* Read header */
    struct dds_header ddsh;
    it_read(&ddsh, &it, sizeof(ddsh));

    /* Default to 2D texture type */
    enum dds_texture_type type = DDS_TEXTURE_TYPE_2D;

    /* Check if image is cubemap */
    if (ddsh.caps2 & DDSCAPS2_CUBEMAP)
        type = DDS_TEXTURE_TYPE_CUBEMAP;

    /* Check if image is a volume texture */
    if ((ddsh.caps2 & DDSCAPS2_VOLUME) && (ddsh.depth > 0))
        type = DDS_TEXTURE_TYPE_3D;

    /* Figure out what the image format is */
    enum dds_texture_format format; int components;
    if (ddsh.ddspf.flags & DDPF_FOURCC) {
        switch (ddsh.ddspf.four_cc) {
            case FOURCC_DXT1:
                format = DDS_TEXTURE_FORMAT_COMPRESSED_RGBA_S3TC_DXT1;
                components = 3;
                break;
            case FOURCC_DXT3:
                format = DDS_TEXTURE_FORMAT_COMPRESSED_RGBA_S3TC_DXT3;
                components = 4;
                break;
            case FOURCC_DXT5:
                format = DDS_TEXTURE_FORMAT_COMPRESSED_RGBA_S3TC_DXT5;
                components = 4;
                break;
            default:
                /* Unknown error compression */
                return 0;
        }
    } else {
        if (ddsh.ddspf.rgb_bit_count == 32 &&
                   ddsh.ddspf.rbit_mask == 0x000000FF &&
                   ddsh.ddspf.gbit_mask == 0x0000FF00 &&
                   ddsh.ddspf.bbit_mask == 0x00FF0000 &&
                   ddsh.ddspf.abit_mask == 0xFF000000) {
            format = DDS_TEXTURE_FORMAT_RGBA;
            components = 4;
        } else if (ddsh.ddspf.rgb_bit_count == 32 &&
            ddsh.ddspf.rbit_mask == 0x00FF0000 &&
            ddsh.ddspf.gbit_mask == 0x0000FF00 &&
            ddsh.ddspf.bbit_mask == 0x000000FF &&
            ddsh.ddspf.abit_mask == 0xFF000000) {
            format = DDS_TEXTURE_FORMAT_BGRA;
            components = 4;
        } else if (ddsh.ddspf.rgb_bit_count == 24 &&
                   ddsh.ddspf.rbit_mask == 0x000000FF &&
                   ddsh.ddspf.gbit_mask == 0x0000FF00 &&
                   ddsh.ddspf.bbit_mask == 0x00FF0000) {
            format = DDS_TEXTURE_FORMAT_RGB;
            components = 3;
        } else if (ddsh.ddspf.rgb_bit_count == 24 &&
                   ddsh.ddspf.rbit_mask == 0x00FF0000 &&
                   ddsh.ddspf.gbit_mask == 0x0000FF00 &&
                   ddsh.ddspf.bbit_mask == 0x000000FF) {
            format = DDS_TEXTURE_FORMAT_BGR;
            components = 3;
        } else if (ddsh.ddspf.rgb_bit_count == 8) {
            format = DDS_TEXTURE_FORMAT_R;
            components = 1;
        } else {
            /* Unknown texture format */
            return 0;
        }
    }

    /* DDS object */
    struct dds_image* dds = calloc(1, sizeof(*dds));
    dds->type = type;

    /* Store primary surface width height and depth */
    unsigned int width, height, depth;
    width  = ddsh.width;
    height = ddsh.height;
    depth  = clamp_size(ddsh.depth);

    /* Compressed flag */
    int compressed = is_compressed(format);
    dds->format = format;

    /* Allocate surfaces in dds object */
    dds->num_surfaces = (type == DDS_TEXTURE_TYPE_CUBEMAP ? 6 : 1);
    dds->surfaces = calloc(dds->num_surfaces, sizeof(*dds->surfaces));

    /* Load all surfaces for the image */
    for (unsigned int n = 0; n < dds->num_surfaces; ++n) {
        /* Allocate mipmaps in dds surface */
        struct dds_surface* surface = &dds->surfaces[n];
        surface->num_mipmaps = clamp_size(ddsh.mip_map_count);
        surface->mipmaps = calloc(surface->num_mipmaps, sizeof(*surface->mipmaps));
        /* Load all mipmaps for current surface */
        unsigned int w = width, h = height, d = depth;
        for (unsigned int i = 0; i < surface->num_mipmaps; ++i) {
            /* Calculate size */
            unsigned int size = (compressed ? size_dxtc(width, height, format) : size_rgb(width, height, components)) * depth;
            /* Read pixel data */
            uint8_t* pixel_data = malloc(size);
            it_read(pixel_data, &it, size);
            /* Store info in object */
            struct dds_mipmap* m = &surface->mipmaps[i];
            m->width  = width;
            m->height = height;
            m->depth  = depth;
            m->size   = size;
            m->pixels = pixel_data;
            /* Shrink to next power of 2 */
            w = clamp_size(w >> 1);
            h = clamp_size(h >> 1);
            d = clamp_size(d >> 1);
        }
    }

    return dds;
}

void dds_image_free(struct dds_image* img)
{
    for (unsigned int i = 0; i < img->num_surfaces; ++i) {
        struct dds_surface* s = &img->surfaces[i];
        for (unsigned int j = 0; j < s->num_mipmaps; ++j)
            free(s->mipmaps[j].pixels);
        free(s->mipmaps);
    }
    free(img->surfaces);
    free(img);
}
