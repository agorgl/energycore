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
#ifndef _DDSFILE_H_
#define _DDSFILE_H_

#include <stdlib.h>

enum dds_texture_type {
    DDS_TEXTURE_TYPE_2D,
    DDS_TEXTURE_TYPE_CUBEMAP,
    DDS_TEXTURE_TYPE_3D
};

enum dds_texture_format {
    DDS_TEXTURE_FORMAT_RGBA,
    DDS_TEXTURE_FORMAT_RGB,
    DDS_TEXTURE_FORMAT_BGRA,
    DDS_TEXTURE_FORMAT_BGR,
    DDS_TEXTURE_FORMAT_R,
    DDS_TEXTURE_FORMAT_COMPRESSED_RGBA_S3TC_DXT1,
    DDS_TEXTURE_FORMAT_COMPRESSED_RGBA_S3TC_DXT3,
    DDS_TEXTURE_FORMAT_COMPRESSED_RGBA_S3TC_DXT5,
};

struct dds_image {
    enum dds_texture_type type;
    enum dds_texture_format format;
    struct dds_surface {
        struct dds_mipmap {
            unsigned int width, height, depth;
            unsigned int size;
            unsigned char* pixels;
        }* mipmaps;
        unsigned int num_mipmaps;
    }* surfaces;
    unsigned int num_surfaces;
};

struct dds_image* dds_image_read(const void* fdata, size_t fsz);
void dds_image_free(struct dds_image* img);

#endif /* ! _DDSFILE_H_ */
