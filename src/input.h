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
#ifndef _INPUT_H_
#define _INPUT_H_

/*==============================================================
/ key
/  * The key values that we will get events for
/===============================================================*/
enum key
{
    KEY_UNKNOWN       = -1,
    KEY_SPACE         = 32,
    KEY_APOSTROPHE    = 39,
    KEY_COMMA         = 44,
    KEY_MINUS         = 45,
    KEY_PERIOD        = 46,
    KEY_SLASH         = 47,
    KEY_NUM0          = 48,
    KEY_NUM1          = 49,
    KEY_NUM2          = 50,
    KEY_NUM3          = 51,
    KEY_NUM4          = 52,
    KEY_NUM5          = 53,
    KEY_NUM6          = 54,
    KEY_NUM7          = 55,
    KEY_NUM8          = 56,
    KEY_NUM9          = 57,
    KEY_SEMICOLON     = 59,
    KEY_EQUAL         = 61,
    KEY_A             = 65,
    KEY_B             = 66,
    KEY_C             = 67,
    KEY_D             = 68,
    KEY_E             = 69,
    KEY_F             = 70,
    KEY_G             = 71,
    KEY_H             = 72,
    KEY_I             = 73,
    KEY_J             = 74,
    KEY_K             = 75,
    KEY_L             = 76,
    KEY_M             = 77,
    KEY_N             = 78,
    KEY_O             = 79,
    KEY_P             = 80,
    KEY_Q             = 81,
    KEY_R             = 82,
    KEY_S             = 83,
    KEY_T             = 84,
    KEY_U             = 85,
    KEY_V             = 86,
    KEY_W             = 87,
    KEY_X             = 88,
    KEY_Y             = 89,
    KEY_Z             = 90,
    KEY_LEFT_BRACKET  = 91,
    KEY_BACKSLASH     = 92,
    KEY_RIGHT_BRACKET = 93,
    KEY_GRAVE_ACCENT  = 96,
    KEY_WORLD1        = 161,
    KEY_WORLD2        = 162,
    KEY_ESCAPE        = 256,
    KEY_ENTER         = 257,
    KEY_TAB           = 258,
    KEY_BACKSPACE     = 259,
    KEY_INSERT        = 260,
    KEY_DELETE        = 261,
    KEY_RIGHT         = 262,
    KEY_LEFT          = 263,
    KEY_DOWN          = 264,
    KEY_UP            = 265,
    KEY_PAGE_UP       = 266,
    KEY_PAGE_DOWN     = 267,
    KEY_HOME          = 268,
    KEY_END           = 269,
    KEY_CAPS_LOCK     = 280,
    KEY_SCROLL_LOCK   = 281,
    KEY_NUM_LOCK      = 282,
    KEY_PRINT_SCREEN  = 283,
    KEY_PAUSE         = 284,
    KEY_F1            = 290,
    KEY_F2            = 291,
    KEY_F3            = 292,
    KEY_F4            = 293,
    KEY_F5            = 294,
    KEY_F6            = 295,
    KEY_F7            = 296,
    KEY_F8            = 297,
    KEY_F9            = 298,
    KEY_F10           = 299,
    KEY_F11           = 300,
    KEY_F12           = 301,
    KEY_F13           = 302,
    KEY_F14           = 303,
    KEY_F15           = 304,
    KEY_F16           = 305,
    KEY_F17           = 306,
    KEY_F18           = 307,
    KEY_F19           = 308,
    KEY_F20           = 309,
    KEY_F21           = 310,
    KEY_F22           = 311,
    KEY_F23           = 312,
    KEY_F24           = 313,
    KEY_F25           = 314,
    KEY_KP0           = 320,
    KEY_KP1           = 321,
    KEY_KP2           = 322,
    KEY_KP3           = 323,
    KEY_KP4           = 324,
    KEY_KP5           = 325,
    KEY_KP6           = 326,
    KEY_KP7           = 327,
    KEY_KP8           = 328,
    KEY_KP9           = 329,
    KEY_KP_DECIMAL    = 330,
    KEY_KP_DIVIDE     = 331,
    KEY_KP_MULTIPLY   = 332,
    KEY_KP_SUBTRACT   = 333,
    KEY_KP_ADD        = 334,
    KEY_KP_ENTER      = 335,
    KEY_KP_EQUAL      = 336,
    KEY_LEFT_SHIFT    = 340,
    KEY_LEFT_CONTROL  = 341,
    KEY_LEFT_ALT      = 342,
    KEY_LEFT_SUPER    = 343,
    KEY_RIGHT_SHIFT   = 344,
    KEY_RIGHT_CONTROL = 345,
    KEY_RIGHT_ALT     = 346,
    KEY_RIGHT_SUPER   = 347,
    KEY_MENU          = 348,
    KEY_LAST
};

/*==============================================================
/ mouse_button
/  * The mouse values that we will get events for
/===============================================================*/
enum mouse_button
{
    MOUSE_BTN1   = 0,
    MOUSE_BTN2   = 1,
    MOUSE_BTN3   = 2,
    MOUSE_BTN4   = 3,
    MOUSE_BTN5   = 4,
    MOUSE_BTN6   = 5,
    MOUSE_BTN7   = 6,
    MOUSE_BTN8   = 7,
    MOUSE_LEFT   = MOUSE_BTN1,
    MOUSE_RIGHT  = MOUSE_BTN2,
    MOUSE_MIDDLE = MOUSE_BTN3,
    MOUSE_BTN_LAST
};

/*==============================================================
/ joystick
/  * The joystick values that we will get events for
/===============================================================*/
enum joystick
{
    JOYSTICK_BUTTON1  = 0,
    JOYSTICK_BUTTON2  = 1,
    JOYSTICK_BUTTON3  = 2,
    JOYSTICK_BUTTON4  = 3,
    JOYSTICK_BUTTON5  = 4,
    JOYSTICK_BUTTON6  = 5,
    JOYSTICK_BUTTON7  = 6,
    JOYSTICK_BUTTON8  = 7,
    JOYSTICK_BUTTON9  = 8,
    JOYSTICK_BUTTON10 = 9,
    JOYSTICK_BUTTON11 = 10,
    JOYSTICK_BUTTON12 = 11,
    JOYSTICK_BUTTON13 = 12,
    JOYSTICK_BUTTON14 = 13,
    JOYSTICK_BUTTON15 = 14,
    JOYSTICK_BUTTON16 = 15,
    JOYSTICK_BUTTON_LAST
};

/*==============================================================
/ key_action
/  * The subtype of the keypress event
/===============================================================*/
enum key_action
{
    KEY_ACTION_RELEASE = 0,
    KEY_ACTION_PRESS   = 1,
    KEY_ACTION_REPEAT  = 2
};

#endif /* ! _INPUT_H_ */
