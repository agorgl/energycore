/*********************************************************************************************************************/
/*                                                  /===-_---~~~~~~~~~------____                                     */
/*                                                 |===-~___                _,-'                                     */
/*                  -==\\                         `//~\\   ~~~~`---.___.-~~                                          */
/*              ______-==|                         | |  \\           _-~`                                            */
/*        __--~~~  ,-/-==\\                        | |   `\        ,'                                                */
/*     _-~       /'    |  \\                      / /      \      /                                                  */
/*   .'        /       |   \\                   /' /        \   /'                                                   */
/*  /  ____  /         |    \`\.__/-~~ ~ \ _ _/'  /          \/'                                                     */
/* /-'~    ~~~~~---__  |     ~-/~         ( )   /'        _--~`                                                      */
/*                   \_|      /        _)   ;  ),   __--~~                                                           */
/*                     '~~--_/      _-~/-  / \   '-~ \                                                               */
/*                    {\__--_/}    / \\_>- )<__\      \                                                              */
/*                    /'   (_/  _-~  | |__>--<__|      |                                                             */
/*                   |0  0 _/) )-~     | |__>--<__|     |                                                            */
/*                   / /~ ,_/       / /__>---<__/      |                                                             */
/*                  o o _//        /-~_>---<__-~      /                                                              */
/*                  (^(~          /~_>---<__-      _-~                                                               */
/*                 ,/|           /__>--<__/     _-~                                                                  */
/*              ,//('(          |__>--<__|     /                  .----_                                             */
/*             ( ( '))          |__>--<__|    |                 /' _---_~\                                           */
/*          `-)) )) (           |__>--<__|    |               /'  /     ~\`\                                         */
/*         ,/,'//( (             \__>--<__\    \            /'  //        ||                                         */
/*       ,( ( ((, ))              ~-__>--<_~-_  ~--____---~' _/'/        /'                                          */
/*     `~/  )` ) ,/|                 ~-_~>--<_/-__       __-~ _/                                                     */
/*   ._-~//( )/ )) `                    ~~-'_/_/ /~~~~~~~__--~                                                       */
/*    ;'( ')/ ,)(                              ~~~~~~~~~~                                                            */
/*   ' ') '( (/                                                                                                      */
/*     '   '  `                                                                                                      */
/*********************************************************************************************************************/
#ifndef _DBC_H_
#define _DBC_H_

/* Design by Contract helper */

#ifdef __cplusplus
extern "C" {
#endif

/* Utility macro that stringifies given parameter */
#define TO_STRING(x) #x

/* Utility macro that is used to stringify compiler macros like __LINE__ */
#define COMPILER_MACRO_TO_STRING(x) TO_STRING(x)

/* Type of the assert handler functions */
typedef void(*dbc_assert_handler_type)(const char*);

/* Sets the assertion handler to the given function */
void dbc_set_assert_handler(dbc_assert_handler_type);

/* Retrieves the current assertion handler */
dbc_assert_handler_type dbc_get_assert_handler();

/* Sets the default exit code used by predifined assertions */
void dbc_set_assert_exit_code(int ec);

/* Gets the default exit code used by assertions */
int dbc_get_assert_exit_code();

/* Predifined assertion handlers */
void dbc_gui_assert_handler(const char* message);
void dbc_shell_assert_handler(const char* message);

/* The library assert macro */
#define DBC_ASSERT(condition, error_message)        \
{                                                   \
    if ((condition) == 0) {                         \
        dbc_get_assert_handler()(error_message);    \
    }                                               \
}

#if defined(DEBUG_ONLY_ASSERTIONS) && !_DEBUG
#define require(condition) (void)0
#define require_msg(condition, error_message) (void)0
#define ensure(condition) (void)0
#define ensure_msg(condition, error_message) (void)0
#else
/* Requires the given expression to be true. Commonly used in preconditions */
#define require(condition) \
    DBC_ASSERT(condition,   "Error: expression \"" #condition "\" is required to be true\n" \
                            "File: " __FILE__ "\n"                                          \
                            "Line: " COMPILER_MACRO_TO_STRING(__LINE__))

/* Require that uses a custom user message to be displayed on failure */
#define require_msg(condition, error_message) DBC_ASSERT(condition, error_message)

/* Ensures that the given expression is true. Commonly used in postconditions*/
#define ensure(condition) \
    DBC_ASSERT(condition,   "Error: expression \"" #condition "\" should have evaluated to true\n"  \
                            "File: " __FILE__ "\n"                                                  \
                            "Line: " COMPILER_MACRO_TO_STRING(__LINE__))

/* Ensure version that uses custom user message to be displayed on failure */
#define ensure_msg(condition, error_message) DBC_ASSERT(condition, error_message)
#endif

#ifdef __cplusplus
}
#endif

#endif /* ! _DBC_H_ */
