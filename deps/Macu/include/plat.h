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
#ifndef _PLAT_H_
#define _PLAT_H_

#ifdef __cplusplus
extern "C" {
#endif


/* Platform detection */
#ifdef _WIN32
    /* Windows (32-bit and 64-bit, common define) */
    #define OS_WINDOWS
    #if defined(_WIN32) && !defined(_WIN64)
        /* Windows (32-bit only) */
        #define OS_WIN32
    #endif
    #ifdef _WIN64
        /* Windows (64-bit only) */
        #define OS_WIN64
    #endif
#elif __APPLE__
    #include "TargetConditionals.h"
    #if defined(TARGET_IPHONE_SIMULATOR) || defined(TARGET_OS_IPHONE)
        #define OS_IOS
        #if TARGET_IPHONE_SIMULATOR
            /* iOS Simulator */
            #define OS_IOS_SIMUL
        #elif TARGET_OS_IPHONE
            /* iOS Device */
            #define OS_IOS_PHONE
        #endif
    #elif TARGET_OS_MAC
        /* Mac OSX */
        #define OS_OSX
    #endif
#elif __ANDROID__
    /* Android */
    #define OS_ANDROID
#elif __linux__
    /* Linux */
    #define OS_LINUX
#elif __unix__ /* All unixes not caught above */
    /* Unix */
    #define OS_UNIX
#elif defined(_POSIX_VERSION)
    /* POSIX */
#else
    /* Unknown OS */
    #define OS_UNKNOWN
#endif


#ifdef __cplusplus
}
#endif

#endif /* ! _PLAT_H_ */
