#include <energycore/scene_asset.h>
#include <string.h>
#include <math.h>

void camera_init(struct camera* c)
{
    c->name     = calloc(1, sizeof(const char));
    c->frame    = identity_frame3f;
    c->ortho    = 0;
    c->yfov     = 2.0 * atan(0.5);
    c->aspect   = 16.0 / 9.0;
    c->focus    = 1;
    c->aperture = 0;
    c->near     = 0.01;
    c->far      = 10000;
}

void camera_destroy(struct camera* c)
{
    free((void*)c->name);
    memset(c, 0, sizeof(*c));
}

void texture_init(struct texture* t)
{
    t->name = calloc(1, sizeof(const char));
    t->path = calloc(1, sizeof(const char));
    t->ldr  = (image4b){.w = 0, .h = 0, .pixels = 0};
    t->hdr  = (image4f){.w = 0, .h = 0, .pixels = 0};
}

void texture_destroy(struct texture* t)
{
    free((void*)t->name);
    free((void*)t->path);
    free(t->ldr.pixels);
    free(t->hdr.pixels);
    memset(t, 0, sizeof(*t));
}

void texture_info_default(struct texture_info* ti)
{
    ti->wrap_s     = TEXTURE_WRAP_REPEAT;
    ti->wrap_t     = TEXTURE_WRAP_REPEAT;
    ti->filter_mag = TEXTURE_FILTER_LINEAR;
    ti->filter_min = TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR;
    ti->scale      = 1.0;
}

void material_init(struct material* m)
{
    m->name = calloc(1, sizeof(const char));
    m->type = MATERIAL_TYPE_METALLIC_ROUGHNESS;
    m->ke = (vec3f){0, 0, 0};
    m->kd = (vec3f){0, 0, 0};
    m->ks = (vec3f){0, 0, 0};
    m->kr = (vec3f){0, 0, 0};
    m->kt = (vec3f){0, 0, 0};
    m->rs = 0.0001;
    m->op = 1.0;
    m->double_sided = 0;

    m->ke_txt   = 0;
    m->kd_txt   = 0;
    m->ks_txt   = 0;
    m->kr_txt   = 0;
    m->kt_txt   = 0;
    m->rs_txt   = 0;
    m->bump_txt = 0;
    m->disp_txt = 0;
    m->norm_txt = 0;
    m->occ_txt  = 0;

    m->ke_txt_info   = 0;
    m->kd_txt_info   = 0;
    m->ks_txt_info   = 0;
    m->kr_txt_info   = 0;
    m->kt_txt_info   = 0;
    m->rs_txt_info   = 0;
    m->bump_txt_info = 0;
    m->disp_txt_info = 0;
    m->norm_txt_info = 0;
    m->occ_txt_info  = 0;
}

void material_destroy(struct material* m)
{
    free((void*)m->name);
    free(m->ke_txt_info);
    free(m->kd_txt_info);
    free(m->ks_txt_info);
    free(m->kr_txt_info);
    free(m->kt_txt_info);
    free(m->rs_txt_info);
    free(m->bump_txt_info);
    free(m->disp_txt_info);
    free(m->norm_txt_info);
    free(m->occ_txt_info);
    memset(m, 0, sizeof(*m));
}

void shape_init(struct shape* s)
{
    s->name = calloc(1, sizeof(const char));
    s->mat  = 0;
    s->points             = 0;
    s->num_points         = 0;
    s->lines              = 0;
    s->num_lines          = 0;
    s->triangles          = 0;
    s->num_triangles      = 0;
    s->quads              = 0;
    s->num_quads          = 0;
    s->quads_pos          = 0;
    s->num_quads_pos      = 0;
    s->quads_norm         = 0;
    s->num_quads_norm     = 0;
    s->quads_texcoord     = 0;
    s->num_quads_texcoord = 0;
    s->beziers            = 0;
    s->num_beziers        = 0;

    s->pos              = 0;
    s->num_pos          = 0;
    s->norm             = 0;
    s->num_norm         = 0;
    s->texcoord         = 0;
    s->num_texcoord     = 0;
    s->texcoord1        = 0;
    s->num_texcoord1    = 0;
    s->color            = 0;
    s->num_color        = 0;
    s->radius           = 0;
    s->num_radius       = 0;
    s->tangsp           = 0;
    s->num_tangsp       = 0;
    s->skin_weights     = 0;
    s->num_skin_weights = 0;
    s->skin_joints      = 0;
    s->num_skin_joints  = 0;

    s->subdivision  = 0;
    s->catmullclark = 0;
}

void shape_destroy(struct shape* s)
{
    free((void*)s->name);
    free(s->points);
    free(s->lines);
    free(s->triangles);
    free(s->quads);
    free(s->quads_pos);
    free(s->quads_norm);
    free(s->quads_texcoord);
    free(s->beziers);
    free(s->pos);
    free(s->norm);
    free(s->texcoord);
    free(s->texcoord1);
    free(s->color);
    free(s->radius);
    free(s->tangsp);
    free(s->skin_weights);
    free(s->skin_joints);
    memset(s, 0, sizeof(*s));
}

/* TODO: Resolve naming conflict */
void _mesh_init(struct mesh* m)
{
    m->name       = calloc(1, sizeof(const char));
    m->path       = calloc(1, sizeof(const char));
    m->shapes     = 0;
    m->num_shapes = 0;
}

/* TODO: Resolve naming conflict */
void _mesh_destroy(struct mesh* m)
{
    free((void*)m->name);
    free((void*)m->path);
    for (size_t i = 0; i < m->num_shapes; ++i) {
        shape_destroy(m->shapes[i]);
        free(m->shapes[i]);
    }
    free(m->shapes);
    memset(m, 0, sizeof(*m));
}

void mesh_instance_init(struct mesh_instance* mi)
{
    mi->name  = calloc(1, sizeof(const char));
    mi->frame = identity_frame3f;
    mi->msh   = 0;
}

void mesh_instance_destroy(struct mesh_instance* mi)
{
    free((void*)mi->name);
    memset(mi, 0, sizeof(*mi));
}

void environment_init(struct environment* e)
{
    e->name        = calloc(1, sizeof(const char));
    e->frame       = identity_frame3f;
    e->ke          = (vec3f){0, 0, 0};
    e->ke_txt      = 0;
    e->ke_txt_info = 0;
}

void environment_destroy(struct environment* e)
{
    /* Suck it Greenpeace */
    free((void*)e->name);
    free(e->ke_txt_info);
    memset(e, 0, sizeof(*e));
}

void node_init(struct node* n)
{
    n->name         = calloc(1, sizeof(const char));
    n->parent       = 0;
    n->frame        = identity_frame3f;
    n->translation  = (vec3f){0, 0, 0};
    n->rotation     = (quat4f){0, 0, 0, 1};
    n->scaling      = (vec3f){1, 1, 1};
    n->cam          = 0;
    n->ist          = 0;
    n->skn          = 0;
    n->env          = 0;
    n->children     = 0;
    n->num_children = 0;
}

void node_destroy(struct node* n)
{
    free((void*)n->name);
    free(n->children);
    memset(n, 0, sizeof(*n));
}

void animation_init(struct animation* a)
{
    a->name             = calloc(1, sizeof(const char));
    a->type             = KEYFRAME_INTERPOLATION_LINEAR;
    a->times            = 0;
    a->num_times        = 0;
    a->translations     = 0;
    a->num_translations = 0;
    a->rotations        = 0;
    a->num_rotations    = 0;
    a->scalings         = 0;
    a->num_scalings     = 0;
}

void animation_destroy(struct animation* a)
{
    free((void*)a->name);
    free(a->times);
    free(a->translations);
    free(a->rotations);
    free(a->scalings);
    memset(a, 0, sizeof(*a));
}

void animation_group_init(struct animation_group* ag)
{
    ag->name           = calloc(1, sizeof(const char));
    ag->path           = calloc(1, sizeof(const char));
    ag->animations     = 0;
    ag->num_animations = 0;
    ag->targets        = 0;
    ag->num_targets    = 0;
}

void animation_group_destroy(struct animation_group* ag)
{
    free((void*)ag->name);
    free((void*)ag->path);
    for (size_t i = 0; i < ag->num_animations; ++i) {
        animation_destroy(ag->animations[i]);
        free(ag->animations[i]);
    }
    free(ag->animations);
    free(ag->targets);
    memset(ag, 0, sizeof(*ag));
}

void skin_init(struct skin* s)
{
    s->name              = calloc(1, sizeof(const char));
    s->path              = calloc(1, sizeof(const char));
    s->pose_matrices     = 0;
    s->num_pose_matrices = 0;
    s->joints            = 0;
    s->num_joints        = 0;
    s->root              = 0;
}

void skin_destroy(struct skin* s)
{
    free((void*)s->name);
    free((void*)s->path);
    free(s->pose_matrices);
    free(s->joints);
    memset(s, 0, sizeof(*s));
}

void scene_init(struct scene* s)
{
    s->meshes           = 0;
    s->num_meshes       = 0;
    s->instances        = 0;
    s->num_instances    = 0;
    s->materials        = 0;
    s->num_materials    = 0;
    s->textures         = 0;
    s->num_textures     = 0;
    s->cameras          = 0;
    s->num_cameras      = 0;
    s->environments     = 0;
    s->num_environments = 0;
    s->nodes            = 0;
    s->num_nodes        = 0;
    s->animations       = 0;
    s->num_animations   = 0;
    s->skins            = 0;
    s->num_skins        = 0;
}

/* TODO: Resolve naming conflict */
void _scene_destroy(struct scene* s)
{
    for (size_t i = 0; i < s->num_meshes; ++i) {
        _mesh_destroy(s->meshes[i]);
        free(s->meshes[i]);
    }
    free(s->meshes);

    for (size_t i = 0; i < s->num_instances; ++i) {
        mesh_instance_destroy(s->instances[i]);
        free(s->instances[i]);
    }
    free(s->instances);

    for (size_t i = 0; i < s->num_materials; ++i) {
        material_destroy(s->materials[i]);
        free(s->materials[i]);
    }
    free(s->materials);

    for (size_t i = 0; i < s->num_textures; ++i) {
        texture_destroy(s->textures[i]);
        free(s->textures[i]);
    }
    free(s->textures);

    for (size_t i = 0; i < s->num_cameras; ++i) {
        camera_destroy(s->cameras[i]);
        free(s->cameras[i]);
    }
    free(s->cameras);

    for (size_t i = 0; i < s->num_environments; ++i) {
        environment_destroy(s->environments[i]);
        free(s->environments[i]);
    }
    free(s->environments);

    for (size_t i = 0; i < s->num_nodes; ++i) {
        node_destroy(s->nodes[i]);
        free(s->nodes[i]);
    }
    free(s->nodes);

    for (size_t i = 0; i < s->num_animations; ++i) {
        animation_group_destroy(s->animations[i]);
        free(s->animations[i]);
    }
    free(s->animations);

    for (size_t i = 0; i < s->num_skins; ++i) {
        skin_destroy(s->skins[i]);
        free(s->skins[i]);
    }
    free(s->skins);
}
