#include "mainloop.h"
#include <prof.h>

static inline float clock_msec()
{
    return nanosecs() / 1.0e6;
}

void mainloop(struct mainloop_data* loop_data)
{
    const float ms_per_update = 1000.0f / loop_data->updates_per_second;

    float interpolation;
    float previous, current, lag;
    float elapsed;

    lag = 0.0f;
    previous = clock_msec();
    while (!loop_data->should_terminate) {
        current = clock_msec();
        elapsed = current - previous;
        previous = current;
        lag += elapsed;

        while (lag > ms_per_update) {
            loop_data->update_callback(loop_data->userdata, ms_per_update / 1000.0f);
            lag -= ms_per_update;
        }

        interpolation = lag / ms_per_update;
        loop_data->render_callback(loop_data->userdata, interpolation);

        if (loop_data->perf_callback) {
            loop_data->perf_samples_sum += elapsed;
            ++loop_data->perf_samples_cnt;
            if (loop_data->perf_samples_sum / 1000.0f >= loop_data->perf_refr_rate) {
                float avgms = loop_data->perf_samples_sum / loop_data->perf_samples_cnt;
                loop_data->perf_callback(loop_data->userdata, avgms, 1000.0f / avgms);
                loop_data->perf_samples_sum = loop_data->perf_samples_cnt = 0;
            }
        }
    }
}
