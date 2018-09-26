#version 430 core
layout(local_size_x = 16, local_size_y = 4, local_size_z = 1) in;
#include "eyeadapt_buf.glsl"

shared uint group_pyramid[NUM_HISTOGRAM_BINS];

// Time since last exposure update
uniform float delta_time;

// Filters the dark/bright part of the histogram when computing the average luminance
// to avoid very dark/bright pixels from contributing to the auto exposure.
const vec2 active_range = vec2(0.45, 0.95); // [0,1]

// Minimum/Maximum average luminance to consider for auto exposure (in EV).
const vec2 avglum_range = vec2(-5, 2);

// Set this to true if you want the auto exposure to be animated (on = progressive, off = fixed)
const bool progressive_adaptation = true;

// Adaptation speed from a dark to a light environment.
const float speed_up = 2.0;

// Adaptation speed from a light to a dark environment.
const float speed_down = 1.0;

// Lower/Upper bounds for the brightness range of the generated histogram (in EV).
// The bigger the spread between min & max, the lower the precision will be.
//const vec2 lum_range = vec2(-8, 4); // [-16,1], [1,16]

// Position is in [0,1] range
float luminance_from_histogram_position(float position, float scale, float offs)
{
    return exp2((position - offs) / scale);
}

float interpolate_exposure(float new_exposure, float old_exposure, float speed_down, float speed_up, float delta_time)
{
    float delta = new_exposure - old_exposure;
    float speed = delta > 0.0 ? speed_down : speed_up;
    float exposure = old_exposure + delta * (1.0 - exp2(-delta_time * speed));
    return exposure;
}

float average_luminance(float low_percent, float high_percent, float max_histogram_value, float scale, float offs)
{
    // Sum all bins
    float sum = 0;
    for (uint i = 0; i < NUM_HISTOGRAM_BINS; ++i)
        sum += histogram[i] * max_histogram_value;

    // Skip darker and lighter parts of the histogram to stabilize the auto exposure
    float filtered_sum = 0.0;
    float accumulator  = 0.0;
    vec2 fraction_sums = sum * vec2(low_percent, high_percent);
    for (uint i = 0; i < NUM_HISTOGRAM_BINS; ++i) {
        float bin_value = histogram[i] * max_histogram_value;
        // Filter dark areas
        float offs = min(fraction_sums.x, bin_value);
        bin_value -= offs;
        fraction_sums -= vec2(offs);
        // Filter highlights
        bin_value = min(fraction_sums.y, bin_value);
        fraction_sums.y -= bin_value;
        // Luminance at bin
        float luminance = luminance_from_histogram_position(float(i) / NUM_HISTOGRAM_BINS, scale, offs);
        filtered_sum += luminance * bin_value;
        accumulator  += bin_value;
    }

    float avg_lum = filtered_sum / max(accumulator, 1.0e-4);
    return avg_lum;
}

float log10(float n)
{
    const float kLogBase10 = 1.0 / log2(10.0);
    return log2(n) * kLogBase10;
}

float exposure_multiplier(float avg_lum)
{
    float alum = max(1.0e-4, avg_lum);
    //float key_value = 1.03 - (2.0 / (2.0 + log10(alum + 1.0)));
    float key_value = 1.0;
    float exposure = key_value / alum;
    return exposure;
}

void main()
{
    const uint thread_id = gl_LocalInvocationIndex;
    group_pyramid[thread_id] = histogram[thread_id];

    memoryBarrierShared();
    barrier();

    // Parallel reduction to find the max value
    for (uint i = NUM_HISTOGRAM_BINS >> 1; i > 0; i >>= 1) {
        if (thread_id < i)
            group_pyramid[thread_id] = max(group_pyramid[thread_id], group_pyramid[thread_id + i]);
        memoryBarrierShared();
        barrier();
    }

    memoryBarrierShared();
    barrier();

    if (thread_id == 0) {
        float exposure_prev = exposure_val;
        float max_value = 1.0 / float(group_pyramid[0]);
        float scale     = 1.0 / (lum_range.y - lum_range.x);
        float offs      = -lum_range.x * scale;
        // Average luminance without outliers from histogram
        float avg_lum   = average_luminance(active_range.x, active_range.y, max_value, scale, offs);
        // Clamp to user brightness range
        float cavg_lum = clamp(avg_lum, exp2(avglum_range.x), exp2(avglum_range.y));
        // Exposure multiplier from clamped average luminance
        float exposure = exposure_multiplier(cavg_lum);
        exposure_val = progressive_adaptation
            ? interpolate_exposure(exposure, exposure_prev, speed_down, speed_up, delta_time)
            : exposure;
    }
}
