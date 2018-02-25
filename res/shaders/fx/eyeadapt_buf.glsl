const uint NUM_HISTOGRAM_BINS = 64;
layout(std430, binding = 1) buffer hist_buf {
    vec2 lum_range;
    uint histogram[NUM_HISTOGRAM_BINS];
    float exposure_cur, exposure_prev;
};
