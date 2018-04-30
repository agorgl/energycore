#include "tripack.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

#define TP_SWAP(type, a, b) { type tmp = (a); (a) = (b); (b) = tmp; }

#if defined(_MSC_VER) && !defined(__cplusplus) /* TODO: specific versions only? */
#define inline __inline
#endif

static inline int      tp_mini      (int     a, int     b) { return a < b ? a : b; }
static inline int      tp_maxi      (int     a, int     b) { return a > b ? a : b; }
static inline int      tp_absi      (int     a           ) { return a < 0 ? -a : a; }

typedef struct tp_vec2 { float x, y; } tp_vec2;
static inline tp_vec2  tp_v2i       (int     x, int     y) { tp_vec2 v = { (float)x, (float)y }; return v; }
static inline tp_vec2  tp_v2        (float   x, float   y) { tp_vec2 v = { x, y }; return v; }
static inline tp_vec2  tp_mul2      (tp_vec2 a, tp_vec2 b) { return tp_v2(a.x * b.x, a.y * b.y); }

typedef struct tp_vec3 { float x, y, z; } tp_vec3;
static inline tp_vec3  tp_v3        (float   x, float   y, float   z) { tp_vec3 v = { x, y, z }; return v; }
static inline tp_vec3  tp_add3      (tp_vec3 a, tp_vec3 b) { return tp_v3(a.x + b.x, a.y + b.y, a.z + b.z); }
static inline tp_vec3  tp_sub3      (tp_vec3 a, tp_vec3 b) { return tp_v3(a.x - b.x, a.y - b.y, a.z - b.z); }
static inline tp_vec3  tp_scale3    (tp_vec3 a, float   b) { return tp_v3(a.x * b, a.y * b, a.z * b); }
static inline tp_vec3  tp_div3      (tp_vec3 a, float   b) { return tp_scale3(a, 1.0f / b); }
static inline float    tp_dot3      (tp_vec3 a, tp_vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static inline float    tp_length3sq (tp_vec3 a           ) { return a.x * a.x + a.y * a.y + a.z * a.z; }
static inline float    tp_length3   (tp_vec3 a           ) { return sqrtf(tp_length3sq(a)); }
static inline tp_vec3  tp_normalize3(tp_vec3 a           ) { return tp_div3(a, tp_length3(a)); }

typedef struct
{
    int a_index;
    short w, x, h, hflip;
    /*
     *       C           -
     *     * |  *        | h
     *   *   |     *     |
     * B-----+--------A  -
     * '--x--'        |
     * '-------w------'
     */
} tp_triangle;

static int tp_triangle_cmp(const void* a, const void* b)
{
    tp_triangle* ea = (tp_triangle*)a;
    tp_triangle* eb = (tp_triangle*)b;
    int dh = eb->h - ea->h;
    return dh != 0 ? dh : (eb->w - ea->w);
}

#ifdef TP_DEBUG_OUTPUT
static void tp_line(unsigned char* data, int w, int h,
                    int x0, int y0, int x1, int y1,
                    unsigned char r, unsigned char g, unsigned char b)
{
    int dx = tp_absi(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = tp_absi(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2, e2;
    for(;;) {
        unsigned char* p = data + (y0 * w + x0) * 3;
        p[0] = r; p[1] = g; p[2] = b;

        if (x0 == x1 && y0 == y1) break;
        e2 = err;
        if (e2 > -dx) { err -= dy; x0 += sx; }
        if (e2 <  dy) { err += dx; y0 += sy; }
    }
}

static tp_bool tp_save_bgr_tga(const char* filename, const unsigned char* image, int w, int h)
{
    unsigned char header[18] = { 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, w & 0xff, (w >> 8) & 0xff, h & 0xff, (h >> 8) & 0xff, 24, 0 };
#if defined(_MSC_VER) && _MSC_VER >= 1400
    FILE* file;
    if (fopen_s(&file, filename, "wb") != 0) return TP_FALSE;
#else
    FILE* file = fopen(filename, "wb");
    if (!file) return TP_FALSE;
#endif
    fwrite(header, 1, sizeof(header), file);
    fwrite(image, 1, w * h * 3 , file);
    fclose(file);
    return TP_TRUE;
}
#endif

static void tp_wave_surge(int* wave, int right, int x0, int y0, int x1, int y1)
{
    int dx = tp_absi(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = tp_absi(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2, e2;
    for(;;) {
        if (right)
            wave[y0] = x0 > wave[y0] ? x0 : wave[y0];
        else
            wave[y0] = x0 < wave[y0] ? x0 : wave[y0];
        if (x0 == x1 && y0 == y1) break;
        e2 = err;
        if (e2 > -dx) { err -= dy; x0 += sx; }
        if (e2 <  dy) { err += dx; y0 += sy; }
    }
}

static int tp_wave_wash_up(int* wave, int right, int height, int y0, int x1, int y1, int spacing)
{
    int x0 = 0;
    int dx = tp_absi(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = tp_absi(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2, e2;
    int x = wave[y0];
    for(;;) {
        int x_dist = wave[y0] - x0 - x;
        for (int y = tp_maxi(y0 - spacing, 0); y <= tp_mini(y0 + spacing, height - 1); y++)
            x_dist = right ? tp_maxi(wave[y] - x0 - x, x_dist) : tp_mini(wave[y] - x0 - x, x_dist);
        if ((right && x_dist > 0) || (!right && x_dist < 0))
            x += x_dist;
        if (x0 == x1 && y0 == y1)
            break;
        e2 = err;
        if (e2 > -dx) { err -= dy; x0 += sx; }
        if (e2 <  dy) { err += dx; y0 += sy; }
    }
    return x;
}

int tp_pack_with_fixed_scale_into_rect(const float* positions, int vertex_count, float scale3Dto2D, int width, int height, int border, int spacing, float* out_uvs)
{
    tp_triangle* tris = (tp_triangle*)TP_CALLOC(vertex_count / 3, sizeof(tp_triangle));
    tp_vec3* p = (tp_vec3*)positions;
    tp_vec2* uv = (tp_vec2*)out_uvs;

    for (int i = 0; i < vertex_count / 3; i++) {
        tp_vec3 tp[3], tv[3];
        tp[0] = tp_scale3(p[i * 3 + 0], scale3Dto2D);
        tp[1] = tp_scale3(p[i * 3 + 1], scale3Dto2D);
        tp[2] = tp_scale3(p[i * 3 + 2], scale3Dto2D);
        tv[0] = tp_sub3(tp[1], tp[0]);
        tv[1] = tp_sub3(tp[2], tp[1]);
        tv[2] = tp_sub3(tp[0], tp[2]);
        float tvlsq[3] = { tp_length3sq(tv[0]), tp_length3sq(tv[1]), tp_length3sq(tv[2]) };

        /* Find long edge */
        int maxi;        float maxl = tvlsq[0]; maxi = 0;
        if (tvlsq[1] > maxl) { maxl = tvlsq[1]; maxi = 1; }
        if (tvlsq[2] > maxl) { maxl = tvlsq[2]; maxi = 2; }
        int nexti = (maxi + 1) % 3;

        /* Measure triangle */
        float w = sqrtf(maxl);
        float x = -tp_dot3(tv[maxi], tv[nexti]) / w;
        float h = tp_length3(tp_sub3(tp_add3(tv[maxi], tv[nexti]), tp_scale3(tp_normalize3(tv[maxi]), w - x)));

        /* Store entry */
        tp_triangle* e = tris + i;
        e->a_index = i * 3 + maxi;
        e->w = (int)ceilf(w);
        e->x = (int)ceilf(x);
        e->h = (int)ceilf(h);
        e->hflip = 0;
    }
    qsort(tris, vertex_count / 3, sizeof(tp_triangle), tp_triangle_cmp);

    tp_vec2 uv_scale = tp_v2(1.0f / width, 1.0f / height);

#ifdef TP_DEBUG_OUTPUT
    unsigned char* data;
    if (uv)
        data = (unsigned char*)TP_CALLOC(width * height, 3);
#endif

    int processed;
    int* waves[2];
    waves[0] = (int*)TP_CALLOC(2 * height, sizeof(int));
    waves[1] = waves[0] + height;
    for (int i = 0; i < height; i++) {
        waves[0][i] = width - 1;// - border;
        waves[1][i] = border;
    }
    int pass = 0;

    int row_y = border;
    int row_h = tris[0].h;
    int vflip = 0;
    for (processed = 0; processed < vertex_count / 3; processed++) {
        tp_triangle* e = tris + processed;
        int ymin, ystart, yend, xmin[2], x, hflip;
retry:
        ymin = vflip ? row_y + row_h - e->h : row_y;
        ystart = vflip ? ymin + e->h : ymin;
        yend = vflip ? ymin : ymin + e->h;

        /* Calculate possible x values for the triangle in the current row */
        hflip = processed & 1; /* seems to work better than the heuristics below!? */
        if (pass < 3) { /* left to right (first three passes) */
            xmin[0] = tp_wave_wash_up(waves[1], 1, height, ystart,        e->x, yend, spacing);
            xmin[1] = tp_wave_wash_up(waves[1], 1, height, ystart, e->w - e->x, yend, spacing); /* flipped horizontally */
            //hflip = (xmin[1] < xmin[0] || (xmin[1] == xmin[0] && e->x > e->w / 2)) ? 1 : 0;
        } else if (pass < 5) { /* right to left (last two passes) */
            // TODO: fix spacing!
            xmin[0] = tp_wave_wash_up(waves[0], 0, height, ystart, -       e->x, yend, spacing) - e->w - 1;
            xmin[1] = tp_wave_wash_up(waves[0], 0, height, ystart, -e->w + e->x, yend, spacing) - e->w - 1; /* flipped horizontally */
            //hflip = (xmin[1] > xmin[0] || (xmin[1] == xmin[0] && e->x < e->w / 2)) ? 1 : 0;
        } else
            goto finish;

        /* Execute hflip (and choose best x) */
        e->x = hflip ? e->w - e->x : e->x;
        e->hflip ^= hflip;
        x = xmin[hflip];

        /* Check if it fits into the specified rect
         * (else advance to next row or do another pass over the rect) */
        if (x + e->w + border >= width || x < border) {
            row_y += row_h + spacing + 1; // next row
            row_h = e->h;
            if (row_y + row_h + border >= height) {
                ++pass; // next pass
                row_y = border;
            }
            goto retry;
        }

        /* Found a space for the triangle. Update waves */
        tp_wave_surge(waves[0], 0, x        - spacing - 1, ystart, x + e->x - spacing - 1, yend); /* left side  */
        tp_wave_surge(waves[1], 1, x + e->w + spacing + 1, ystart, x + e->x + spacing + 1, yend); /* right side */

        /* Calc & store UVs */
        if (uv) {
#ifdef TP_DEBUG_OUTPUT
            tp_line(data, width, height, x       , ystart, x + e->w, ystart, 255, 255, 255);
            tp_line(data, width, height, x       , ystart, x + e->x, yend  , 255, 255, 255);
            tp_line(data, width, height, x + e->w, ystart, x + e->x, yend  , 255, 255, 255);
#endif
            int tri = e->a_index - (e->a_index % 3);
            int Ai = e->a_index;
            int Bi = tri + ((e->a_index + 1) % 3);
            int Ci = tri + ((e->a_index + 2) % 3);
            if (e->hflip) TP_SWAP(int, Ai, Bi);
            uv[Ai] = tp_mul2(tp_v2i(x + e->w, ystart), uv_scale);
            uv[Bi] = tp_mul2(tp_v2i(x       , ystart), uv_scale);
            uv[Ci] = tp_mul2(tp_v2i(x + e->x, yend  ), uv_scale);
        }
        vflip = !vflip;
    }

finish:
#ifdef TP_DEBUG_OUTPUT
    if (uv) {
        /*
        for (int i = 0; i < height; i++) {
            // left
            int x = waves[0][i];// +spacing;
            while (x >= 0)
                data[(i * width + x--) * 3 + 2] = 255;
            // right
            x = waves[1][i] - spacing;
            while (x < width)
                data[(i * width + x++) * 3] = 255;
        }
        */
        if (tp_save_bgr_tga("debug_triangle_packing.tga", data, width, height))
            printf("Saved debug_triangle_packing.tga\n");
        TP_FREE(data);
    }
#endif

    TP_FREE(waves[0]);
    TP_FREE(tris);

    return processed * 3;
}

tp_bool tp_pack_into_rect(const float* positions, int vertex_count,
                          int width, int height, int border, int spacing,
                          float* out_uvs, float* out_scale3Dto2D)
{
    float test_scale = 1.0f;
    int processed = tp_pack_with_fixed_scale_into_rect(positions, vertex_count, test_scale, width, height, border, spacing, 0);
    int increase = processed < vertex_count ? 0 : 1;
    float last_fit_scale = 0.0f;
    float multiplicator = 0.5f;

    if (increase) {
        while (!(processed < vertex_count)) {
            test_scale *= 2.0f;
            //printf("inc testing scale %f\n", testScale);
            processed = tp_pack_with_fixed_scale_into_rect(positions, vertex_count, test_scale, width, height, border, spacing, 0);
        }
        last_fit_scale = test_scale / 2.0f;
        //printf("inc scale %f fits\n", lastFitScale);
        multiplicator = 0.75f;
    }

    for (int j = 0; j < 16; j++) {
        //printf("dec multiplicator %f\n", multiplicator);
        for (int i = 0; processed < vertex_count && i < 2; i++) {
            test_scale *= multiplicator;
            //printf("dec testing scale %f\n", test_scale);
            processed = tp_pack_with_fixed_scale_into_rect(positions, vertex_count, test_scale, width, height, border, spacing, 0);
        }
        if (!(processed < vertex_count)) {
            processed = 0;
            //printf("scale %f fits\n", test_scale);
            last_fit_scale = test_scale;
            test_scale /= multiplicator;
            multiplicator = (multiplicator + 1.0f) * 0.5f;
        }
    }

    if (last_fit_scale > 0.0f) {
        *out_scale3Dto2D = last_fit_scale;
        processed = tp_pack_with_fixed_scale_into_rect(positions, vertex_count, last_fit_scale, width, height, border, spacing, out_uvs);
        assert(processed == vertex_count);
        return TP_TRUE;
    }
    return TP_FALSE;
}
