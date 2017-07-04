#include "mainloop.h"
#include <prof.h>

typedef unsigned long timepoint_type;
typedef timepoint_type timepoint_diff;

static inline timepoint_type clock_msec()
{
    return millisecs();
}

void mainloop(struct mainloop_data* loop_data)
{
    const float ms_per_update = 1000 / loop_data->updates_per_second;

    float interpolation;
    timepoint_type previous, current, lag;
    timepoint_diff elapsed;

    lag = 0;
    previous = clock_msec();
    while (!loop_data->should_terminate) {
        current = clock_msec();
        elapsed = current - previous;
        previous = current;
        lag += elapsed;

        while (lag > ms_per_update) {
            loop_data->update_callback(loop_data->userdata, ms_per_update);
            lag -= ms_per_update;
        }

        interpolation = lag / ms_per_update;
        loop_data->render_callback(loop_data->userdata, interpolation);

        if (loop_data->perf_callback) {
            loop_data->perf_samples[loop_data->perf_cur_sample_cnt++] = elapsed;
            if (loop_data->perf_cur_sample_cnt >= ML_PERF_SAMPLES) {
                float avgms = 0.0f;
                for (unsigned int i = 0; i < ML_PERF_SAMPLES; ++i)
                    avgms += loop_data->perf_samples[i];
                avgms /= ML_PERF_SAMPLES;
                loop_data->perf_callback(loop_data->userdata, avgms, 1000.0f / avgms);
                loop_data->perf_cur_sample_cnt = 0;
            }
        }
    }
}
