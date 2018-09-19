#version 430 core
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
#include "eyeadapt_buf.glsl"

// Time since last exposure update
uniform float delta_time;

// Filters the dark/bright part of the histogram when computing the average luminance
// to avoid very dark/bright pixels from contributing to the auto exposure.
const vec2 active_range = vec2(0.45, 0.95); // [0,1]

// Minimum/Maximum average luminance to consider for auto exposure (in EV).
const vec2 avglum_range = vec2(-5, 1);

// Exposure bias. Use this to control the global exposure of the scene.
// Used when dynamic_key_value is false
const float fixed_key_value = 0.25;

// Set this to true to handle the key value automatically based on average luminance.
const bool dynamic_key_value = true;

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
float luminance_from_histogram_position(float position)
{
    float multiply = 1.0 / (lum_range.y - lum_range.x);
    float add = -lum_range.x * multiply;
    return exp2((position - add) / multiply);
}

float interpolate_exposure(float new_exposure, float old_exposure, float delta_time, float speed_up, float speed_down)
{
    float delta = new_exposure - old_exposure;
    float speed = delta > 0.0 ? speed_up : speed_down;
    float exposure = old_exposure + delta * (1.0 - exp2(-delta_time * speed));
    //float exposure = old_exposure + delta * (delta_time * speed);
    return exposure;
}

float exposure_value()
{
    // Find maximum bin value
    float max_value = 0.0;
    for (uint i = 0; i < NUM_HISTOGRAM_BINS; ++i)
        max_value = max(max_value, histogram[i]);
    max_value = 1.0 / max_value;

    // Sum all bins
    float sum = 0;
    for (uint i = 0; i < NUM_HISTOGRAM_BINS; ++i)
        sum += histogram[i] * max_value;

    // Skip darker and lighter parts of the histogram to stabilize the auto exposure
    float filtered_sum = 0.0;
    float accumulator = 0.0;
    vec2 fraction_sums = sum * active_range;
    for (uint i = 0; i < NUM_HISTOGRAM_BINS; ++i) {
        float bin_value = histogram[i] * max_value;
        // Filter dark areas
        float offs = min(fraction_sums.x, bin_value);
        bin_value -= offs;
        fraction_sums -= vec2(offs);
        // Filter highlights
        bin_value = min(fraction_sums.y, bin_value);
        fraction_sums.y -= bin_value;
        // Luminance at bin
        float luminance = luminance_from_histogram_position(float(i) / NUM_HISTOGRAM_BINS);
        filtered_sum += luminance * bin_value;
        accumulator  += bin_value;
    }
    // Average luminance
    float avg_lum = clamp(filtered_sum / max(accumulator, 1.0e-4), avglum_range.x, avglum_range.y);

    // Exposure value
    avg_lum = max(1.0e-4, avg_lum);
    float key_value = dynamic_key_value ? (1.03 - (2.0 / (2.0 + log2(avg_lum + 1.0)))) : fixed_key_value;
    float exposure = key_value / avg_lum;
    return exposure;
}

void main()
{
    float exposure_prev = exposure_val;
    float exposure = exposure_value();
    exposure_val = progressive_adaptation
        ? interpolate_exposure(exposure, exposure_prev, delta_time, speed_up, speed_down)
        : exposure;
}
