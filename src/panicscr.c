#include "panicscr.h"
#include <string.h>
#include <stdio.h>
#include "opengl.h"
#include "dbgtxt.h"
#include "prof.h"

/* Character spawn interval in msecs */
#define INTERVAL_MSEC 2.0

void panicscr_init(struct panicscr_rndr* ps_rndr)
{
    ps_rndr->should_show = 0;
    ps_rndr->txt_buf = 0;
    ps_rndr->txt_len = 0;
    ps_rndr->cur_pos = 0;
    ps_rndr->prev_timepoint = 0;
}

void panicscr_settxt(struct panicscr_rndr* ps_rndr, const char* txt)
{
    if (ps_rndr->txt_buf)
        free((void*)ps_rndr->txt_buf);
    ps_rndr->txt_buf = strdup(txt);
    ps_rndr->txt_len = strlen(txt);
    ps_rndr->cur_pos = 0;
    ps_rndr->prev_timepoint = millisecs();
    ps_rndr->should_show = 1;
}

void panicscr_addtxt(struct panicscr_rndr* ps_rndr, const char* txt)
{
    /* If text buf is empty behave like panicscr_settxt */
    if (!ps_rndr->txt_buf) {
        panicscr_settxt(ps_rndr, txt);
        return;
    }
    ps_rndr->txt_len += strlen(txt);
    ps_rndr->txt_buf = realloc((void*)ps_rndr->txt_buf, ps_rndr->txt_len + 1);
    strcat((char*)ps_rndr->txt_buf, txt);
    ps_rndr->should_show = 1;
}

void panicscr_show(struct panicscr_rndr* ps_rndr)
{
    size_t rem_len = ps_rndr->txt_len - ps_rndr->cur_pos;
    if (rem_len > 0) {
        timepoint_t cur_ms = millisecs();
        timepoint_t elapsd = cur_ms - ps_rndr->prev_timepoint;
        if (elapsd > INTERVAL_MSEC) {
            size_t nch = (float) elapsd / INTERVAL_MSEC;
            ps_rndr->cur_pos += (nch < rem_len ? nch : rem_len);
            ps_rndr->prev_timepoint = cur_ms;
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    dbgtxt_setfnt(FNT_GOHU);
    dbgtxt_prntnc(ps_rndr->txt_buf, ps_rndr->cur_pos,
                  10, 20,
                  1.0f, 1.0f, 1.0f, 1.0f);
}

void panicscr_destroy(struct panicscr_rndr* ps_rndr)
{
    if (ps_rndr->txt_buf)
        free((void*)ps_rndr->txt_buf);
}
