#include "panicscr.h"
#include <string.h>
#include <stdio.h>
#include "opengl.h"
#include <GL/glu.h>
#include "dbgtxt.h"
#include "prof.h"

/* Character spawn interval in msecs */
#define INTERVAL_MSEC 1

#ifdef __GLIBC__
#include <execinfo.h>
#define NUM_BT_ITEMS 16

static const char* bt_str()
{
    void* bt_items[NUM_BT_ITEMS];
    size_t sz = backtrace(bt_items, NUM_BT_ITEMS);
    char** syms = backtrace_symbols(bt_items, sz);

    char* bt_str = 0;
    size_t bt_str_len = 0;
    const char* title = "Backtrace:\n";
    bt_str_len += strlen(title);
    bt_str = realloc(bt_str, bt_str_len + 1);
    strcpy(bt_str, title);
    /* Skip first 3 entries corresponding to:
     * #0: bt_str
     * #1: gl_debug_proc
     * #2: gl_debuf_proc_caller (inside driver) */
    for (unsigned int i = 3; i < sz; ++i) {
        const char* sym = syms[i];
        const char* fmt = "    #%u: %s\n";
        unsigned int idx = i - 3;
        size_t sz = snprintf(0, 0, fmt, idx, sym);
        bt_str = realloc(bt_str, bt_str_len + sz + 1);
        snprintf(bt_str + bt_str_len, sz + 1, fmt, idx, sym);
        bt_str_len += sz;
    }
    free(syms);
    return bt_str;
}
#endif

static void APIENTRY gl_debug_proc(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* user_param)
{
    (void) length;

    struct panicscr_rndr* ps_rndr = (struct panicscr_rndr*) user_param;
    const char* source_str = (const char*) gluErrorString(source);
    source_str = source_str ? source_str : "???";
    const char* severity_str = (const char*) gluErrorString(severity);
    severity_str = severity_str ? : "???";

    if (type == GL_DEBUG_TYPE_ERROR) {
        /* OpenGL error */
        const char* msg_fmt =
            "PANIC!\n"
            "OpenGL error (%u) occurred!\n"
            "Source: (%d) %s\n"
            "Severity: (%d) %s\n"
            "Message:\n    %s.\n";
        size_t msg_len = snprintf(0, 0, msg_fmt, id, source, source_str, severity, severity_str, message) + 1;
        char* msg = malloc(msg_len);
        snprintf(msg, msg_len, msg_fmt, id, source, source_str, severity, severity_str);
        panicscr_settxt(ps_rndr, msg);
        free(msg);

#ifdef __GLIBC__
        /* Backtrace */
        const char* bt = bt_str();
        panicscr_addtxt(ps_rndr, bt);
        free((void*)bt);
#endif
    }
}

void panicscr_init(struct panicscr_rndr* ps_rndr)
{
    ps_rndr->should_show = 0;
    ps_rndr->txt_buf = 0;
    ps_rndr->txt_len = 0;
    ps_rndr->cur_pos = 0;
    ps_rndr->next_timepoint = 0;
}

void panicscr_settxt(struct panicscr_rndr* ps_rndr, const char* txt)
{
    if (ps_rndr->txt_buf)
        free((void*)ps_rndr->txt_buf);
    ps_rndr->txt_buf = strdup(txt);
    ps_rndr->txt_len = strlen(txt);
    ps_rndr->cur_pos = 0;
    ps_rndr->next_timepoint = millisecs();
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
    if (ps_rndr->cur_pos < ps_rndr->txt_len) {
        timepoint_t cur_ms = millisecs();
        if (cur_ms > ps_rndr->next_timepoint) {
            ps_rndr->cur_pos++;
            ps_rndr->next_timepoint = cur_ms + INTERVAL_MSEC;
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    dbgtxt_prntnc(ps_rndr->txt_buf, ps_rndr->cur_pos,
                  10, 20,
                  1.0f, 1.0f, 1.0f, 1.0f);
}

void panicscr_register_gldbgcb(struct panicscr_rndr* ps_rndr)
{
    glDebugMessageCallback(gl_debug_proc, ps_rndr);
}

void panicscr_destroy(struct panicscr_rndr* ps_rndr)
{
    if (ps_rndr->txt_buf)
        free((void*)ps_rndr->txt_buf);
}
