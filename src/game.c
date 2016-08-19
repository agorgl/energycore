#include "game.h"
#include <string.h>
#include "window.h"

static void on_key(struct window* wnd, int key, int scancode, int action, int mods)
{
    (void)key; (void)scancode; (void)mods;
    struct game_context* ctx = get_userdata(wnd);
    if (action == 0)
        *(ctx->should_terminate) = 1;
}

void init(struct game_context* ctx)
{
    /* Create window */
    const char* title = "EnergyCore";
    int width = 800, height = 600, mode = 0;
    ctx->wnd = create_window(title, width, height, mode);

    /* Assosiate context to be accessed from callback functions */
    set_userdata(ctx->wnd, ctx);

    /* Set event callbacks */
    struct window_callbacks wnd_callbacks;
    memset(&wnd_callbacks, 0, sizeof(struct window_callbacks));
    wnd_callbacks.key_cb = on_key;
    set_callbacks(ctx->wnd, &wnd_callbacks);
}

void update(void* userdata, float dt)
{
    (void) dt;
    struct game_context* ctx = userdata;
    /* Process input events */
    poll_events(ctx->wnd);
}

void render(void* userdata, float interpolation)
{
    (void) interpolation;
    struct game_context* ctx = userdata;
    /* Show rendered contents from the backbuffer */
    swap_buffers(ctx->wnd);
}

void shutdown(struct game_context* ctx)
{
    /* Close window */
    destroy_window(ctx->wnd);
}
