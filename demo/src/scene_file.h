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
#ifndef _SCENE_FILE_H_
#define _SCENE_FILE_H_

#include <stdlib.h>

struct scene_file {
    /* Models */
    struct scene_model {
        const char* ref;
        const char* path;
    }* models;
    size_t num_models;
    /* Textures */
    struct scene_texture {
        const char* ref;
        const char* path;
    }* textures;
    size_t num_textures;
    /* Materials */
    struct scene_material {
        const char* ref;
        struct {
            const char* tex_ref;
            enum {
                STT_ALBEDO = 0,
                STT_NORMAL,
                STT_ROUGHNESS,
                STT_METALLIC,
                STT_SPECULAR,
                STT_GLOSSINESS,
                STT_EMISSION,
                STT_OCCLUSION,
                STT_DETAIL_ALBEDO,
                STT_DETAIL_NORMAL,
                STT_PARALLAX,
                STT_MAX
            } type;
            float scale[2];
        } textures[11];
    }* materials;
    size_t num_materials;
    /* Objects */
    struct scene_object {
        const char* ref;
        const char* name;
        struct {
            float translation[3];
            float rotation[4];
            float scaling[3];
        } transform;
        const char* mdl_ref;
        const char* mgroup_name;
        const char** mat_refs;
        size_t num_mat_refs;
        const char* parent_ref;
    }* objects;
    size_t num_objects;
    /* Lights */
    struct scene_light {
        const char* ref;
        enum {
            SLT_DIRECTIONAL,
            SLT_POINT,
            SLT_SPOT
        } type;
        float color[3];
        float intensity;
        float position[3];
        float falloff;
        float direction[3];
        float inner_cone;
        float outer_cone;
    }* lights;
    size_t num_lights;
    /* Cameras */
    struct scene_camera {
        const char* ref;
        float position[3];
        float target[3];
    }* cameras;
    size_t num_cameras;
};

struct scene_file* scene_file_load(const char* filepath);
void scene_file_destroy(struct scene_file* sc);

#endif /* ! _SCENE_FILE_H_ */
