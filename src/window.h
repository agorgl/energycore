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
#ifndef _WINDOW_H_
#define _WINDOW_H_

/* Window datatype */
struct window;

/* Event callback function types */
typedef void(*mouse_button_fn)(struct window*, int, int, int);
typedef void(*cursor_pos_fn)(struct window*, double, double);
typedef void(*cursor_enter_fn)(struct window*, int);
typedef void(*scroll_fn)(struct window*, double, double);
typedef void(*key_fn)(struct window*, int, int, int, int);
typedef void(*char_fn)(struct window*, unsigned int);
typedef void(*char_mods_fn)(struct window*, unsigned int, int);

/* Set of event callbacks */
struct window_callbacks
{
    mouse_button_fn mouse_button_cb;
    cursor_pos_fn cursor_pos_cb;
    cursor_enter_fn cursor_enter_cb;
    scroll_fn scroll_cb;
    key_fn key_cb;
    char_fn char_cb;
    char_mods_fn char_mods_cb;
};

/* Creates new window */
struct window* window_create();

/* Closes given window */
void window_destroy(struct window*);

/* Registers given callback functions */
void window_set_callbacks(struct window*, struct window_callbacks*);

/* Polls for stored events and calls the registered callbacks */
void window_poll_events(struct window*);

/* Swaps backbuffer with front buffer */
void window_swap_buffers(struct window* wnd);

/* Sets userdata pointer to be assosiated with given window */
void window_set_userdata(struct window* wnd, void* userdata);

/* Retrieves userdata pointer assisiated with given window */
void* window_get_userdata(struct window* wnd);

#endif /* ! _WINDOW_H_ */
