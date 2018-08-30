#include <energycore/scene_asset.h>
#include <string.h>
#include <math.h>

const quat4f identity_quat4f = {0, 0, 0, 1};
const frame3f identity_frame3f = {{1,0,0}, {0,1,0}, {0,0,1}, {0,0,0}};
const mat4f identity_mat4f = {{1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0,0,1}};

void camera_init(struct camera* c)
{
    c->name     = 0;
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
    t->name = 0;
    t->path = 0;
    t->img  = (image){
        .w                = 0,
        .h                = 0,
        .channels         = 0,
        .bit_depth        = 0,
        .data             = 0,
        .sz               = 0,
        .compression_type = 0
    };
}

void texture_destroy(struct texture* t)
{
    free((void*)t->name);
    free((void*)t->path);
    free(t->img.data);
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
    m->name = 0;
    m->type = MATERIAL_TYPE_METALLIC_ROUGHNESS;
    m->ke = (vec3f){0, 0, 0};
    m->kd = (vec3f){0, 0, 0};
    m->ks = (vec3f){0, 0, 0};
    m->kr = (vec3f){0, 0, 0};
    m->kt = (vec3f){0, 0, 0};
    m->rs = 0.0001;
    m->op = 1.0;
    m->double_sided = 0;

    m->ke_txt    = 0;
    m->kd_txt    = 0;
    m->ks_txt    = 0;
    m->kr_txt    = 0;
    m->kt_txt    = 0;
    m->rs_txt    = 0;
    m->bump_txt  = 0;
    m->disp_txt  = 0;
    m->norm_txt  = 0;
    m->occ_txt   = 0;
    m->kdd_txt   = 0;
    m->normd_txt = 0;

    m->ke_txt_info    = 0;
    m->kd_txt_info    = 0;
    m->ks_txt_info    = 0;
    m->kr_txt_info    = 0;
    m->kt_txt_info    = 0;
    m->rs_txt_info    = 0;
    m->bump_txt_info  = 0;
    m->disp_txt_info  = 0;
    m->norm_txt_info  = 0;
    m->occ_txt_info   = 0;
    m->kdd_txt_info   = 0;
    m->normd_txt_info = 0;
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
    free(m->kdd_txt_info);
    free(m->normd_txt_info);
    memset(m, 0, sizeof(*m));
}

void shape_init(struct shape* s)
{
    s->name = 0;
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
    m->name       = 0;
    m->path       = 0;
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
    mi->name  = 0;
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
    e->name        = 0;
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
    n->name         = 0;
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
    a->name             = 0;
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
    ag->name           = 0;
    ag->path           = 0;
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
    s->name              = 0;
    s->path              = 0;
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

void scene_destroy(struct scene* s)
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
    free(s);
}

static vec3f vec3f_add(vec3f v1, vec3f v2){ return (vec3f){v1.x + v2.x, v1.y + v2.y, v1.z + v2.z}; }
static vec3f vec3f_sub(vec3f v1, vec3f v2){ return (vec3f){v1.x - v2.x, v1.y - v2.y, v1.z - v2.z}; }
static float vec3f_dot(vec3f a, vec3f b){ return a.x * b.x + a.y * b.y + a.z * b.z; }

static vec3f vec3f_cross(vec3f v1, vec3f v2)
{
    return (vec3f){
        (v1.y * v2.z) - (v1.z * v2.y),
        (v1.z * v2.x) - (v1.x * v2.z),
        (v1.x * v2.y) - (v1.y * v2.x)
    };
}

static vec3f vec3f_normalize(vec3f v)
{
    float len = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    return len == 0.f
        ? (vec3f){0.f, 0.f, 0.f}
        : (vec3f){v.x/len, v.y/len, v.z/len};
}

static vec3f vec3f_orthonormalize(vec3f a, vec3f b)
{
    float dot = vec3f_dot(a, b);
    return vec3f_normalize((vec3f){
        a.x - b.x * dot,
        a.y - b.y * dot,
        a.z - b.z * dot
    });
}

void shape_compute_normals(struct shape* shp)
{
    if (shp->norm)
        return;

    shp->num_norm = shp->num_pos;
    shp->norm = calloc(shp->num_norm, sizeof(*shp->norm));
    for (size_t i = 0; i < shp->num_triangles; ++i) {
        vec3i tr = shp->triangles[i];
        vec3f e1 = vec3f_sub(shp->pos[tr.y], shp->pos[tr.x]);
        vec3f e2 = vec3f_sub(shp->pos[tr.z], shp->pos[tr.x]);
        vec3f nm = vec3f_cross(e1, e2);
        nm = vec3f_normalize(nm);
        shp->norm[tr.x] = vec3f_add(shp->norm[tr.x], nm);
        shp->norm[tr.y] = vec3f_add(shp->norm[tr.y], nm);
        shp->norm[tr.z] = vec3f_add(shp->norm[tr.z], nm);
    }

    for (size_t i = 0; i < shp->num_norm; ++i)
        shp->norm[i] = vec3f_normalize(shp->norm[i]);
}

static void triangle_tangents_from_uv(vec3f* tu, vec3f* tv, vec3f v[3], vec2f uv[3])
{
    vec3f p = vec3f_sub(v[1], v[0]);
    vec3f q = vec3f_sub(v[2], v[0]);
    vec2f s = {uv[1].x - uv[0].x, uv[2].x - uv[0].x};
    vec2f t = {uv[1].y - uv[0].y, uv[2].y - uv[0].y};
    float d = s.x * t.y - s.y * t.x;
    if (d != 0.0) {
        *tu = (vec3f){(t.y * p.x - t.x * q.x) / d,
                      (t.y * p.y - t.x * q.y) / d,
                      (t.y * p.z - t.x * q.z) / d};
        *tv = (vec3f){(s.x * q.x - s.y * p.x) / d,
                      (s.x * q.y - s.y * p.y) / d,
                      (s.x * q.z - s.y * p.z) / d};
    } else {
        *tu = (vec3f){1, 0, 0};
        *tv = (vec3f){0, 1, 0};
    }
}

/* Compute per-vertex tangent frame for triangle meshes.
 * Tangent space is defined by a four component vector.
 * The first three components are the tangent with respect to the U texcoord.
 * The fourth component is the sign of the tangent wrt the V texcoord.
 * Tangent frame is useful in normal mapping. */
void shape_compute_tangent_frame(struct shape* shp)
{
    if (shp->tangsp)
        return;

    vec3f* tngu = calloc(shp->num_pos, sizeof(*tngu));
    vec3f* tngv = calloc(shp->num_pos, sizeof(*tngv));
    for (size_t i = 0; i < shp->num_triangles; ++i) {
        vec3i tr = shp->triangles[i];
        vec3f tu, tv;
        triangle_tangents_from_uv(&tu, &tv,
            (vec3f[]){shp->pos[tr.x],
                      shp->pos[tr.y],
                      shp->pos[tr.z]},
            (vec2f[]){shp->texcoord[tr.x],
                      shp->texcoord[tr.y],
                      shp->texcoord[tr.z]});
        tu = vec3f_normalize(tu);
        tv = vec3f_normalize(tv);

        tngu[tr.x] = vec3f_add(tngu[tr.x], tu);
        tngu[tr.y] = vec3f_add(tngu[tr.y], tu);
        tngu[tr.z] = vec3f_add(tngu[tr.z], tu);

        tngv[tr.x] = vec3f_add(tngv[tr.x], tv);
        tngv[tr.y] = vec3f_add(tngv[tr.y], tv);
        tngv[tr.z] = vec3f_add(tngv[tr.z], tv);
    }
    for (size_t i = 0; i < shp->num_pos; ++i) {
        tngu[i] = vec3f_normalize(tngu[i]);
        tngv[i] = vec3f_normalize(tngv[i]);
    }

    shp->num_tangsp = shp->num_pos;
    shp->tangsp = calloc(shp->num_tangsp, sizeof(*shp->tangsp));
    for (size_t i = 0; i < shp->num_norm; ++i) {
        vec3f nm = shp->norm[i], tu = tngu[i], tv = tngv[i];
        tu = vec3f_orthonormalize(tu, nm);
        float s = (vec3f_dot(vec3f_cross(nm, tu), tv) < 0) ? -1.0f : 1.0f;
        shp->tangsp[i] = (vec4f){tu.x, tu.y, tu.z, s};
    }

    free(tngu);
    free(tngv);
}
