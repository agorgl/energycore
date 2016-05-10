#include "game.h"

void init(struct game_context* ctx)
{
    (void) ctx;
}

void update(void* userdata, float dt)
{
    (void) dt;

    struct game_context* ctx = userdata;
    *ctx->should_terminate = 1;
}

void render(void* userdata, float interpolation)
{
    (void) interpolation;

    struct game_context* ctx = userdata;
    (void) ctx;
}

void shutdown(struct game_context* ctx)
{
    (void) ctx;
}
