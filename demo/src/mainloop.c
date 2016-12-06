#include "mainloop.h"
#include <prof.h>

typedef unsigned long timepoint_type;

timepoint_type clock_msec()
{
    return millisecs();
}

void mainloop(struct mainloop_data* loop_data)
{
    timepoint_type next_update = clock_msec();
    const float skip_ticks = 1000 / loop_data->updates_per_second;

    int loops;
    float interpolation;

    while (!loop_data->should_terminate) {
        loops = 0;
        while (clock_msec() > next_update && loops < loop_data->max_frameskip) {
            loop_data->update_callback(loop_data->userdata, skip_ticks);
            next_update += skip_ticks;
            ++loops;
        }

        timepoint_type rt = clock_msec();
        interpolation = (rt + skip_ticks - next_update) / skip_ticks;
        loop_data->render_callback(loop_data->userdata, interpolation);

        if (loop_data->perf_callback) {
            loop_data->perf_samples[loop_data->perf_cur_sample_cnt++] = clock_msec() - rt;
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
