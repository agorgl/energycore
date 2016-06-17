#include "dbc.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#ifdef _MSC_VER
#include <windows.h>
#include <crtdbg.h>
#endif

/*----------------------------------------------------------------------
 * Defaults
 *----------------------------------------------------------------------*/
#define DEFAULT_ASSERT_EXITCODE -2
#ifdef _MSC_VER
#define DEFAULT_ASSERT_HANDLER dbc_gui_assert_handler
#else
#define DEFAULT_ASSERT_HANDLER dbc_shell_assert_handler
#endif

/*----------------------------------------------------------------------
 * Handler
 *----------------------------------------------------------------------*/
/* Function pointer that is used to hold the assertion handler */
static dbc_assert_handler_type assert_handler = DEFAULT_ASSERT_HANDLER;

void dbc_set_assert_handler(dbc_assert_handler_type handler)
{
    /* Use previous handler to test the one being set */
    require(handler != 0);
    assert_handler = handler;
}

dbc_assert_handler_type dbc_get_assert_handler()
{
    return assert_handler;
}

/*----------------------------------------------------------------------
 * Exit code
 *----------------------------------------------------------------------*/
/* The exitcode used by predifiened assert handlers */
static int dbc_assert_exit_code = DEFAULT_ASSERT_EXITCODE;

void dbc_set_assert_exit_code(int ec)
{
    dbc_assert_exit_code = ec;
}

int dbc_get_assert_exit_code()
{
    return dbc_assert_exit_code;
}

/*----------------------------------------------------------------------
 * Handler impls
 *----------------------------------------------------------------------*/
/* Gui assert handler that displays a MessageBox */
void dbc_gui_assert_handler(const char* message)
{
#ifdef _MSC_VER
    int rVal = MessageBox(0, message, "Assertion!", MB_ICONERROR | MB_ABORTRETRYIGNORE);
    switch (rVal) {
        case IDABORT:
            break;
        case IDRETRY:
            /* Call debugger */
            if (IsDebuggerPresent())
                __debugbreak();
            return;
        case IDIGNORE:
            return;
    }
#else
    (void) message;
#endif
    exit(dbc_assert_exit_code);
}

/* Shell assert handler that outputs to stderr */
void dbc_shell_assert_handler(const char* message)
{
    fprintf(stderr, "%s", message);
    exit(dbc_assert_exit_code);
}
