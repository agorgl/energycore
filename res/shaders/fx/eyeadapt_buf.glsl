const uint NUM_HISTOGRAM_BINS = 64;
layout(std430, binding = 1) buffer hist_buf {
    vec2 scale_offst;
    uint histogram[NUM_HISTOGRAM_BINS];
    float exposure_val;
};
