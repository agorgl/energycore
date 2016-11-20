#include "mainloop.h"
#include <time.h>

long long clock_msec()
{
    return 1000 * clock() / CLOCKS_PER_SEC;
}

void mainloop(struct mainloop_data* loop_data)
{
    long long next_update = clock_msec();
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

        interpolation = (clock_msec() + skip_ticks - next_update) / skip_ticks;
        loop_data->render_callback(loop_data->userdata, interpolation);
    }
}
