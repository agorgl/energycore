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
#ifndef _ASSET_DATA_H_
#define _ASSET_DATA_H_

#include <stdlib.h>

struct image_data {
    void* data;
    size_t data_sz;
    unsigned int width;
    unsigned int height;
    unsigned short channels;
    unsigned short bit_depth;
    unsigned short compression_type;
    struct {
        int compressed : 1;
        int hdr : 1;
    } flags;
};

struct model_data {
    struct mesh_data {
        size_t num_verts;
        float (*positions)[3];
        float (*normals)[3];
        float (*texcoords)[2];
        float (*tangents)[3];
        size_t num_triangles;
        unsigned int (*triangles)[3];
        size_t mat_idx;
        size_t group_idx;
    }* meshes;
    size_t num_meshes;
    const char** group_names;
    size_t num_group_names;
};

void model_data_from_file(struct model_data* mdl, const char* filepath);
void model_data_free(struct model_data* mdl);
void image_data_from_file(struct image_data* img, const char* filepath);
void image_data_free(struct image_data* img);

#endif /* ! _ASSET_DATA_H_ */
