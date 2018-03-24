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
#ifndef _RESOURCE_H_
#define _RESOURCE_H_

#include "slot_map.h"
#include "scene_asset.h"

/* Abstract resource handle */
#define INVALID_RID SM_INVALID_KEY
typedef sm_key rid;

/* Checks if given resource id reference is null */
int rid_null(rid);

/* Texture resource */
struct render_texture {
    unsigned int id;
};

struct render_texture_info {
    unsigned int wrap_s;
    unsigned int wrap_t;
    unsigned int filter_mag;
    unsigned int filter_min;
    float scale;
};

/* Material resource */
struct render_material {
    enum material_type type;
    vec3f ke;
    vec3f kd;
    vec3f ks;
    vec3f kr;
    vec3f kt;
    float rs;
    float op;
    rid ke_txt;
    rid kd_txt;
    rid ks_txt;
    rid kr_txt;
    rid kt_txt;
    rid rs_txt;
    rid bump_txt;
    rid disp_txt;
    rid norm_txt;
    rid occ_txt;
    rid kdd_txt;
    rid normd_txt;
    struct render_texture_info ke_txt_info;
    struct render_texture_info kd_txt_info;
    struct render_texture_info ks_txt_info;
    struct render_texture_info kr_txt_info;
    struct render_texture_info kt_txt_info;
    struct render_texture_info rs_txt_info;
    struct render_texture_info bump_txt_info;
    struct render_texture_info disp_txt_info;
    struct render_texture_info norm_txt_info;
    struct render_texture_info occ_txt_info;
    struct render_texture_info kdd_txt_info;
    struct render_texture_info normd_txt_info;
};

/* Mesh resource */
struct render_mesh {
    struct render_shape {
        unsigned int vao;
        unsigned int vbo;
        unsigned int ebo;
        unsigned int num_elems;
        float bb_min[3], bb_max[3];
        unsigned int mat_idx;
    } shapes[16];
    size_t num_shapes;
};

/* Resource manager state */
struct resmgr {
    struct slot_map textures;
    struct slot_map materials;
    struct slot_map meshes;
};

/* Resource manager constructor / destructor */
void resmgr_init(struct resmgr* rmgr);
void resmgr_destroy(struct resmgr* rmgr);
void resmgr_default_rmat(struct render_material* rmat);

/* Data to resource */
rid resmgr_add_texture(struct resmgr* rmgr, struct texture* tex);
rid resmgr_add_texture_env(struct resmgr* rmgr, struct texture* tex, int hcross);
rid resmgr_add_texture_file(struct resmgr* rmgr, const char* filepath);
rid resmgr_add_material(struct resmgr* rmgr, struct render_material* rmat);
rid resmgr_add_mesh(struct resmgr* rmgr, struct mesh* sh);

/* Reference to resource */
struct render_texture* resmgr_get_texture(struct resmgr* rmgr, rid id);
struct render_material* resmgr_get_material(struct resmgr* rmgr, rid id);
struct render_mesh* resmgr_get_mesh(struct resmgr* rmgr, rid id);

#endif /* ! _RESOURCE_H_ */
