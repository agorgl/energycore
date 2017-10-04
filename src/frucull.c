#include "frucull.h"

/* False if fully outside, True if inside or intersects */
/* http://www.iquilezles.org/www/articles/frustumcorrect/frustumcorrect.htm */
int box_in_frustum(vec3 fru_points[8], vec4 fru_planes[6], vec3 box_mm[2])
{
    /* Check box outside/inside of frustum */
    for (int i = 0; i < 6; ++i) {
        int out = 0;
        out += ((vec4_dot(fru_planes[i], vec4_new(box_mm[0].x, box_mm[0].y, box_mm[0].z, 1.0f)) < 0.0 ) ? 1 : 0);
        out += ((vec4_dot(fru_planes[i], vec4_new(box_mm[1].x, box_mm[0].y, box_mm[0].z, 1.0f)) < 0.0 ) ? 1 : 0);
        out += ((vec4_dot(fru_planes[i], vec4_new(box_mm[0].x, box_mm[1].y, box_mm[0].z, 1.0f)) < 0.0 ) ? 1 : 0);
        out += ((vec4_dot(fru_planes[i], vec4_new(box_mm[1].x, box_mm[1].y, box_mm[0].z, 1.0f)) < 0.0 ) ? 1 : 0);
        out += ((vec4_dot(fru_planes[i], vec4_new(box_mm[0].x, box_mm[0].y, box_mm[1].z, 1.0f)) < 0.0 ) ? 1 : 0);
        out += ((vec4_dot(fru_planes[i], vec4_new(box_mm[1].x, box_mm[0].y, box_mm[1].z, 1.0f)) < 0.0 ) ? 1 : 0);
        out += ((vec4_dot(fru_planes[i], vec4_new(box_mm[0].x, box_mm[1].y, box_mm[1].z, 1.0f)) < 0.0 ) ? 1 : 0);
        out += ((vec4_dot(fru_planes[i], vec4_new(box_mm[1].x, box_mm[1].y, box_mm[1].z, 1.0f)) < 0.0 ) ? 1 : 0);
        if (out == 8)
            return 0;
    }

    /* Check frustum outside/inside box */
    int out;
    out = 0; for (int i = 0; i < 8; ++i) out += ((fru_points[i].x > box_mm[1].x) ? 1 : 0); if (out == 8) return 0;
    out = 0; for (int i = 0; i < 8; ++i) out += ((fru_points[i].x < box_mm[0].x) ? 1 : 0); if (out == 8) return 0;
    out = 0; for (int i = 0; i < 8; ++i) out += ((fru_points[i].y > box_mm[1].y) ? 1 : 0); if (out == 8) return 0;
    out = 0; for (int i = 0; i < 8; ++i) out += ((fru_points[i].y < box_mm[0].y) ? 1 : 0); if (out == 8) return 0;
    out = 0; for (int i = 0; i < 8; ++i) out += ((fru_points[i].z > box_mm[1].z) ? 1 : 0); if (out == 8) return 0;
    out = 0; for (int i = 0; i < 8; ++i) out += ((fru_points[i].z < box_mm[0].z) ? 1 : 0); if (out == 8) return 0;

    return 1;
}
