#include "hdrfile.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

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

int hdr_image_read(float** data, unsigned int* w, unsigned int* h, void* fdata)
{
    /* Cursor */
    const char* p = fdata;

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
        return 0;

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
        return 0;

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
    return 1;
}
