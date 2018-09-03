#include "shcomp.h"
#include <string.h>
#include <stdint.h>
#include <math.h>

#define PI      3.1415926535897932384626433832795028841971693993751058
#define PI4     12.566370614359172953850573533118011536788677597500423
#define PI16    50.265482457436691815402294132472046147154710390001693
#define PI64    201.06192982974676726160917652988818458861884156000677
#define SQRT_PI 1.7724538509055160272981674833411451827975494561223871

/* 1.0 / (2.0 * SQRT_PI) */
#define K0      0.28209479177
/* sqrt(3.0 / PI4) */
#define K1      0.4886025119
/* sqrt(15.0 / PI4) */
#define K2      1.09254843059
/* -sqrt(15.0 / PI4) */
#define K3     -1.09254843059
/* sqrt(5.0 / PI16) */
#define K4      0.31539156525
/* sqrt(15.0 / PI16) */
#define K5      0.54627421529
/* -sqrt(70.0 / PI64) */
#define K6     -0.59004358992
/* sqrt(105.0 / PI4) */
#define K7      2.89061144264
/* -sqrt(21.0 / PI16) */
#define K8     -0.64636036822
/* sqrt(7.0 / PI16) */
#define K9      0.37317633259
/* -sqrt(42.0 / PI64) */
#define K10    -0.45704579946
/* sqrt(105.0 / PI16) */
#define K11     1.44530572132
/* -sqrt(70.0 / PI64) */
#define K12    -0.59004358992
/* 3.0 * sqrt(35.0 / PI16) */
#define K13     2.5033429418
/* -3.0 * sqrt(70.0 / PI64) */
#define K14    -1.77013076978
/* 3.0 * sqrt(5.0 / PI16) */
#define K15     0.94617469575
/* -3.0 * sqrt(10.0 / PI64) */
#define K16    -1.33809308711
/* 3.0 * sqrt(5.0 / PI64) */
#define K17     0.47308734787
/* 3.0 * sqrt(35.0 / (4.0 * PI64)) */
#define K18     0.62583573544

void sh_eval_basis5(double* sh_basis, const float* dir)
{
    const double x = (double)dir[0];
    const double y = (double)dir[1];
    const double z = (double)dir[2];

    const double x2 = x*x;
    const double y2 = y*y;
    const double z2 = z*z;

    const double z3 = z*z*z;

    const double x4 = x*x*x*x;
    const double y4 = y*y*y*y;
    const double z4 = z*z*z*z;

    /* Equations based on data from: http://ppsloan.org/publications/stupid_sH36.pdf */
    sh_basis[0]  = K0;

    sh_basis[1]  = -K1 * y;
    sh_basis[2]  = K1 * z;
    sh_basis[3]  = -K1 * x;

    sh_basis[4]  = K2 * y * x;
    sh_basis[5]  = K3 * y * z;
    sh_basis[6]  = K4 * (3.0 * z2 - 1.0);
    sh_basis[7]  = K3 * x * z;
    sh_basis[8]  = K5 * (x2 - y2);

    sh_basis[9]  = K6 * y * (3 * x2 - y2);
    sh_basis[10] = K7 * y * x * z;
    sh_basis[11] = K8 * y * (-1.0 + 5.0 * z2);
    sh_basis[12] = K9 * (5.0 * z3 - 3.0 * z);
    sh_basis[13] = K10 * x * (-1.0 + 5.0 * z2);
    sh_basis[14] = K11 * (x2 - y2) * z;
    sh_basis[15] = K12 * x * (x2 - 3.0 * y2);

    sh_basis[16] = K13 * x * y * (x2 - y2);
    sh_basis[17] = K14 * y * z * (3.0 * x2 - y2);
    sh_basis[18] = K15 * y * x * (-1.0 + 7.0 * z2);
    sh_basis[19] = K16 * y * z * (-3.0 + 7.0 * z2);
    sh_basis[20] = (105.0 * z4 -90.0 * z2 + 9.0) / (16.0 * SQRT_PI);
    sh_basis[21] = K16 * x * z * (-3.0 + 7.0 * z2);
    sh_basis[22] = K17 * (x2 - y2) * (-1.0 + 7.0 * z2);
    sh_basis[23] = K14 * x * z * (x2 - 3.0 * y2);
    sh_basis[24] = K18 * (x4 - 6.0 * y2 * x2 + y4);
}

/* Notice: faceSize should not be equal to one! */
static float warp_fixup_factor(float face_size)
{
    /* Edge fixup. */
    /* Based on Nvtt : http://code.google.com/p/nvidia-texture-tools/source/browse/trunk/src/nvtt/cube_surface.cpp */
    if (face_size == 1.0f)
        return 1.0f;
    const float fs = face_size;
    const float fsmo = fs - 1.0f;
    return (fs * fs) / (fsmo * fsmo * fsmo);
}

/* http://www.mpia-hd.mpg.de/~mathar/public/mathar20051002.pdf */
/* http://www.rorydriscoll.com/2012/01/15/cubemap-texel-solid-angle/ */
static float area_element(float x, float y) { return atan2f(x * y, sqrtf(x * x + y * y + 1.0f)); }

/* U and V should be center adressing and in [-1.0+invSize..1.0-invSize] range. */
static float texel_solid_angle(float u, float v, float inv_face_size)
{
    /* Specify texel area. */
    const float x0 = u - inv_face_size;
    const float x1 = u + inv_face_size;
    const float y0 = v - inv_face_size;
    const float y1 = v + inv_face_size;

    /* Compute solid angle of texel area. */
    const float solid_angle = area_element(x1, y1)
                            - area_element(x0, y1)
                            - area_element(x1, y0)
                            + area_element(x0, y0);
    return solid_angle;
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
static const float cm_face_uv_vectors[6][3][3] =
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

/* U and V should be center adressing and in [-1.0+invSize..1.0-invSize] range */
static void cm_texel_coord_to_vec(float* out3f, float u, float v, uint8_t face_id)
{
    /* out = u * face_uv[0] + v * face_uv[1] + face_uv[2] */
    out3f[0] = cm_face_uv_vectors[face_id][0][0] * u + cm_face_uv_vectors[face_id][1][0] * v + cm_face_uv_vectors[face_id][2][0];
    out3f[1] = cm_face_uv_vectors[face_id][0][1] * u + cm_face_uv_vectors[face_id][1][1] * v + cm_face_uv_vectors[face_id][2][1];
    out3f[2] = cm_face_uv_vectors[face_id][0][2] * u + cm_face_uv_vectors[face_id][1][2] * v + cm_face_uv_vectors[face_id][2][2];

    /* Normalize. */
    const float inv_len = 1.0f / sqrtf(out3f[0] * out3f[0] + out3f[1] * out3f[1] + out3f[2] * out3f[2]);
    out3f[0] *= inv_len;
    out3f[1] *= inv_len;
    out3f[2] *= inv_len;
}

/* U and V should be center adressing and in [-1.0+invSize..1.0-invSize] range. */
static void texel_coord_to_vec_warp(float* out3f, float u, float v, uint8_t face_id, float warp_fixup)
{
    u = (warp_fixup * u*u*u) + u;
    v = (warp_fixup * v*v*v) + v;
    cm_texel_coord_to_vec(out3f, u, v, face_id);
}

void sh_coeffs(double sh_coeffs[SH_COEFF_NUM][3], void* data, size_t face_sz)
{
    const float texel_size = 1.0f / (float)face_sz;
    const float warp = warp_fixup_factor(face_sz);

    memset(sh_coeffs, 0, SH_COEFF_NUM * 3 * sizeof(double));
    double weight_accum = 0.0;
    for (int face = 0; face < 6; ++face) {
        for (size_t ydst = 0; ydst < face_sz; ++ydst) {
            for (size_t xdst = 0; xdst < face_sz; ++xdst) {
                /* Current destination pixel location */
                /* Map value to [-1, 1], offset by 0.5 to point to texel center */
                const float v = 2.0f * ((ydst + 0.5f) * texel_size) - 1.0f;
                const float u = 2.0f * ((xdst + 0.5f) * texel_size) - 1.0f;
                /* Get sampling vector for the above u, v set */
                float dir[3];
                texel_coord_to_vec_warp(dir, u, v, face, warp);
                /* Get solid angle for u, v set */
                float solid_angle = texel_solid_angle(u, v, texel_size);
                /* Current pixel values */
                uint8_t* src_ptr = data + (face * face_sz * face_sz + ydst * face_sz + xdst) * 3;
                const double rr = (double)src_ptr[0] / 255.0;
                const double gg = (double)src_ptr[1] / 255.0;
                const double bb = (double)src_ptr[2] / 255.0;
                /* Calculate SH Basis */
                double sh_basis[SH_COEFF_NUM];
                sh_eval_basis5(sh_basis, dir);
                const double weight = (double)solid_angle;
                for (uint8_t ii = 0; ii < SH_COEFF_NUM; ++ii) {
                    sh_coeffs[ii][0] += rr * sh_basis[ii] * weight;
                    sh_coeffs[ii][1] += gg * sh_basis[ii] * weight;
                    sh_coeffs[ii][2] += bb * sh_basis[ii] * weight;
                }
                weight_accum += weight;
            }
        }
    }

    /*
     * Normalization.
     * This is not really necesarry because usually PI*4 - weightAccum ~= 0.000003
     * so it doesn't change almost anything, but it doesn't cost much be more correct.
     */
    const double norm = PI4 / weight_accum;
    for (uint8_t ii = 0; ii < SH_COEFF_NUM; ++ii) {
        sh_coeffs[ii][0] *= norm;
        sh_coeffs[ii][1] *= norm;
        sh_coeffs[ii][2] *= norm;
    }
}
