#include "wvfobj.h"
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <scene_asset.h>
#include "hashtable.h"

#define OBJ_FLAG_TRIANGULATE (1 << 0)
#define OBJ_MAX_FACES_PER_F_LINE (64)

/*----------------------------------------------------------------------
 * Parsing helpers
 *----------------------------------------------------------------------*/
#define IS_SPACE(x) (((x) == ' ') || ((x) == '\t'))
#define IS_NEW_LINE(x) (((x) == '\r') || ((x) == '\n') || ((x) == '\0'))

static void skip_space(const char** token)
{
    while ((*token)[0] == ' ' || (*token)[0] == '\t')
        (*token)++;
}

static void skip_space_and_cr(const char** token)
{
    while ((*token)[0] == ' ' || (*token)[0] == '\t' || (*token)[0] == '\r')
        (*token)++;
}

static size_t until_space(const char* token)
{
    const char* p = token;
    while (p[0] != '\0' && p[0] != ' ' && p[0] != '\t' && p[0] != '\r')
        p++;
    return p - token;
}

static size_t length_until_newline(const char* token, size_t n)
{
    size_t len = 0;
    /* Assume token[n-1] = '\0' */
    for (len = 0; len < n - 1; len++) {
        if (token[len] == '\n')
            break;
        if ((token[len] == '\r') && ((len < (n - 2)) && (token[len + 1] != '\n')))
            break;
    }
    return len;
}

/* Make index zero-base, and also support relative index. */
static int fix_index(int idx, size_t n)
{
    if (idx > 0)
        return idx - 1;
    if (idx == 0)
        return 0;
    return (int)n + idx; /* negative value = relative */
}

/* Parse raw triples: i, i/j/k, i//k, i/j */
static obj_vertex_index_t parse_raw_triple(const char** token)
{
    obj_vertex_index_t vi;
    /* 0x80000000 = -2147483648 = invalid */
    vi.v_idx  = 0x80000000;
    vi.vn_idx = 0x80000000;
    vi.vt_idx = 0x80000000;

    /* i */
    vi.v_idx = atoi((*token));
    while ((*token)[0] != '\0'
        && (*token)[0] != '/'
        && (*token)[0] != ' '
        && (*token)[0] != '\t'
        && (*token)[0] != '\r')
        (*token)++;

    if ((*token)[0] != '/')
        return vi;

    (*token)++;

    /* i//k */
    if ((*token)[0] == '/') {
        (*token)++;
        vi.vn_idx = atoi((*token));
        while ((*token)[0] != '\0'
            && (*token)[0] != '/'
            && (*token)[0] != ' '
            && (*token)[0] != '\t'
            && (*token)[0] != '\r')
            (*token)++;
        return vi;
    }

    /* i/j/k or i/j */
    vi.vt_idx = atoi((*token));
    while ((*token)[0] != '\0'
        && (*token)[0] != '/'
        && (*token)[0] != ' '
        && (*token)[0] != '\t'
        && (*token)[0] != '\r')
        (*token)++;

    if ((*token)[0] != '/')
        return vi;

    /* i/j/k */
    (*token)++; /* Skip '/' */
    vi.vn_idx = atoi((*token));
    while ((*token)[0] != '\0'
        && (*token)[0] != '/'
        && (*token)[0] != ' '
        && (*token)[0] != '\t'
        && (*token)[0] != '\r')
        (*token)++;

    return vi;
}

static float parse_float(const char** token)
{
    const char* end;
    skip_space(token);
    end = (*token) + until_space((*token));
    double val = strtod((*token), 0); /* */
    end = (*token) + until_space((*token));
    float f = (float)(val);
    (*token) = end;
    return f;
}

static void parse_float2(float* x, float* y, const char** token)
{
    *x = parse_float(token);
    *y = parse_float(token);
}

static void parse_float3(float* x, float* y, float* z, const char** token)
{
    *x = parse_float(token);
    *y = parse_float(token);
    *z = parse_float(token);
}

static int is_line_ending(const char* p, size_t i, size_t end_i)
{
    if (p[i] == '\0')
        return 1;
    if (p[i] == '\n')
        return 1; /* This includes \r\n */
    if (p[i] == '\r' && ((i + 1) < end_i) && (p[i + 1] != '\n'))
        return 1; /* Detect only \r case */
    return 0;
}

/*----------------------------------------------------------------------
 * Wavefront Obj parsing
 *----------------------------------------------------------------------*/
enum command_type {
    COMMAND_EMPTY,
    COMMAND_V,
    COMMAND_VN,
    COMMAND_VT,
    COMMAND_F,
    COMMAND_G,
    COMMAND_O,
    COMMAND_USEMTL,
    COMMAND_MTLLIB
};

struct command {
    enum command_type type;
    float vx, vy, vz;
    float nx, ny, nz;
    float tx, ty;

    obj_vertex_index_t f[OBJ_MAX_FACES_PER_F_LINE]; /* TODO: Use dynamic array ? */
    size_t num_f;
    int f_num_verts[OBJ_MAX_FACES_PER_F_LINE];
    size_t num_f_num_verts;

    const char*  group_name;
    size_t group_name_len;
    const char*  object_name;
    size_t object_name_len;
    const char*  material_name;
    size_t material_name_len;
    const char*  mtllib_name;
    size_t mtllib_name_len;
};

static int parse_line(struct command* command, const char* p, size_t p_len, int triangulate)
{
    char linebuf[4096];
    const char* token;
    assert(p_len < (sizeof(linebuf) - 1));

    memcpy(linebuf, p, p_len);
    linebuf[p_len] = '\0';
    token = linebuf;
    command->type = COMMAND_EMPTY;

    /* Skip leading space. */
    skip_space(&token);
    assert(token);

    /* Empty line */
    if (token[0] == '\0')
        return 0;

    /* Comment line */
    if (token[0] == '#')
        return 0;

    /* Vertex */
    if (token[0] == 'v' && IS_SPACE((token[1]))) {
        float x, y, z;
        token += 2;
        parse_float3(&x, &y, &z, &token);
        command->vx = x;
        command->vy = y;
        command->vz = z;
        command->type = COMMAND_V;
        return 1;
    }

    /* Normal */
    if (token[0] == 'v' && token[1] == 'n' && IS_SPACE((token[2]))) {
        float x, y, z;
        token += 3;
        parse_float3(&x, &y, &z, &token);
        command->nx = x;
        command->ny = y;
        command->nz = z;
        command->type = COMMAND_VN;
        return 1;
    }

    /* Texcoord */
    if (token[0] == 'v' && token[1] == 't' && IS_SPACE((token[2]))) {
        float x, y;
        token += 3;
        parse_float2(&x, &y, &token);
        command->tx = x;
        command->ty = y;
        command->type = COMMAND_VT;
        return 1;
    }

    /* Face */
    if (token[0] == 'f' && IS_SPACE((token[1]))) {
        size_t num_f = 0;
        obj_vertex_index_t f[OBJ_MAX_FACES_PER_F_LINE];
        token += 2;

        skip_space(&token);
        while (!IS_NEW_LINE(token[0])) {
            obj_vertex_index_t vi = parse_raw_triple(&token);
            skip_space_and_cr(&token);
            f[num_f] = vi;
            num_f++;
        }
        command->type = COMMAND_F;

        if (triangulate) {
            assert(3 * num_f < OBJ_MAX_FACES_PER_F_LINE);
            obj_vertex_index_t i0 = f[0];
            obj_vertex_index_t i1;
            obj_vertex_index_t i2 = f[1];
            size_t n = 0;
            for (size_t k = 2; k < num_f; k++) {
                i1 = i2;
                i2 = f[k];
                command->f[3 * n + 0] = i0;
                command->f[3 * n + 1] = i1;
                command->f[3 * n + 2] = i2;
                command->f_num_verts[n] = 3;
                n++;
            }
            command->num_f = 3 * n;
            command->num_f_num_verts = n;
        } else {
            assert(num_f < OBJ_MAX_FACES_PER_F_LINE);
            for (size_t k = 0; k < num_f; k++)
                command->f[k] = f[k];
            command->num_f = num_f;
            command->f_num_verts[0] = num_f;
            command->num_f_num_verts = 1;
        }

        return 1;
    }

    /* Use mtl */
    if ((0 == strncmp(token, "usemtl", 6)) && IS_SPACE((token[6]))) {
        token += 7;
        skip_space(&token);
        command->material_name = p + (token - linebuf);
        command->material_name_len = length_until_newline(token, (p_len - (size_t)(token - linebuf)) + 1);
        command->type = COMMAND_USEMTL;
        return 1;
    }

    /* Load mtl */
    if ((strncmp(token, "mtllib", 6) == 0) && IS_SPACE((token[6]))) {
        /* By specification, `mtllib` should be appear only once in .obj */
        token += 7;
        skip_space(&token);
        command->mtllib_name = p + (token - linebuf);
        command->mtllib_name_len = length_until_newline(token, p_len - (size_t)(token - linebuf)) + 1;
        command->type = COMMAND_MTLLIB;
        return 1;
    }

    /* Group name */
    if (token[0] == 'g' && IS_SPACE((token[1]))) {
        /* TODO: multiple group name? */
        token += 2;
        command->group_name = p + (token - linebuf);
        command->group_name_len = length_until_newline(token, p_len - (size_t)(token - linebuf)) + 1;
        command->type = COMMAND_G;
        return 1;
    }

    /* Object name */
    if (token[0] == 'o' && IS_SPACE((token[1]))) {
        /* TODO: multiple object name? */
        token += 2;
        command->object_name = p + (token - linebuf);
        command->object_name_len = length_until_newline(token, p_len - (size_t)(token - linebuf)) + 1;
        command->type = COMMAND_O;
        return 1;
    }

    return 0;
}

struct line_info {
    size_t pos;
    size_t len;
};

static int obj_parse_obj(obj_attrib_t* attrib, obj_shape_t** shapes, size_t* num_shapes, const char* buf, size_t len, unsigned int flags)
{
    assert(len >= 1);
    assert(attrib);
    assert(shapes);
    assert(num_shapes);
    assert(buf);

    /* Find '\n' and create line data */
    size_t end_idx = len - 1;
    /* Count # of lines. */
    size_t num_lines = 0;
    for (size_t i = 0; i < end_idx; i++) { /* Assume last char is '\0' */
        if (is_line_ending(buf, i, end_idx) || (i == end_idx - 1))
            num_lines++;
    }
    if (num_lines == 0)
        return -1;

    /* Fill line infos */
    size_t prev_pos = 0, line_no  = 0;
    struct line_info* line_infos = malloc(sizeof(*line_infos) * num_lines);
    for (size_t i = 0; i < end_idx; i++) {
        if (is_line_ending(buf, i, end_idx) || (i == end_idx - 1)) {
            line_infos[line_no].pos = prev_pos;
            line_infos[line_no].len = i - prev_pos;
            prev_pos = i + 1;
            line_no++;
        }
    }

    /* Parse each line */
    struct hash_table* material_table = hash_table_create(hash_table_string_hash, hash_table_string_eql);
    size_t num_v = 0, num_vn = 0, num_vt = 0, num_f = 0, num_faces = 0;
    struct command* commands = malloc(sizeof(*commands) * num_lines);
    for (size_t i = 0; i < num_lines; i++) {
        int ret = parse_line(&commands[i], &buf[line_infos[i].pos], line_infos[i].len, flags & OBJ_FLAG_TRIANGULATE);
        if (ret) {
            if (commands[i].type == COMMAND_V) {
                num_v++;
            } else if (commands[i].type == COMMAND_VN) {
                num_vn++;
            } else if (commands[i].type == COMMAND_VT) {
                num_vt++;
            } else if (commands[i].type == COMMAND_F) {
                num_f += commands[i].num_f;
                num_faces += commands[i].num_f_num_verts;
            }
        }
    }
    free(line_infos);

    /* Construct attributes */
    memset(attrib, 0, sizeof(*attrib));
    attrib->vertices           = malloc(num_v * 3 * sizeof(float));
    attrib->num_vertices       = num_v;
    attrib->normals            = malloc(num_vn * 3 * sizeof(float));
    attrib->num_normals        = num_vn;
    attrib->texcoords          = malloc(num_vt * 2 * sizeof(float));
    attrib->num_texcoords      = num_vt;
    attrib->faces              = malloc(num_f * sizeof(*attrib->faces));
    attrib->num_faces          = num_f;
    attrib->face_num_verts     = malloc(num_faces * sizeof(*attrib->face_num_verts));
    attrib->material_ids       = malloc(num_faces * sizeof(*attrib->material_ids));
    attrib->num_face_num_verts = num_faces;

    int material_id = -1; /* -1 = default unknown material. */
    size_t v_count = 0, n_count = 0, t_count = 0, f_count = 0, face_count = 0;
    for (size_t i = 0; i < num_lines; i++) {
        if (commands[i].type == COMMAND_EMPTY) {
            continue;
        } else if (commands[i].type == COMMAND_USEMTL) {
            if (commands[i].material_name && commands[i].material_name_len > 0) {
                /* Create a null terminated string */
                char* material_name_null_term = malloc(commands[i].material_name_len + 1);
                memcpy(material_name_null_term, commands[i].material_name, commands[i].material_name_len);
                material_name_null_term[commands[i].material_name_len - 1] = 0;
                hash_val_t* v = hash_table_search(material_table, (hash_key_t)material_name_null_term);
                if (v)
                    material_id = *v;
                else
                    material_id = -1;
                free(material_name_null_term);
            }
        } else if (commands[i].type == COMMAND_V) {
            attrib->vertices[3 * v_count + 0] = commands[i].vx;
            attrib->vertices[3 * v_count + 1] = commands[i].vy;
            attrib->vertices[3 * v_count + 2] = commands[i].vz;
            v_count++;
        } else if (commands[i].type == COMMAND_VN) {
            attrib->normals[3 * n_count + 0] = commands[i].nx;
            attrib->normals[3 * n_count + 1] = commands[i].ny;
            attrib->normals[3 * n_count + 2] = commands[i].nz;
            n_count++;
        } else if (commands[i].type == COMMAND_VT) {
            attrib->texcoords[2 * t_count + 0] = commands[i].tx;
            attrib->texcoords[2 * t_count + 1] = commands[i].ty;
            t_count++;
        } else if (commands[i].type == COMMAND_F) {
            for (size_t k = 0; k < commands[i].num_f; k++) {
                obj_vertex_index_t vi = commands[i].f[k];
                int v_idx  = fix_index(vi.v_idx,  v_count);
                int vn_idx = fix_index(vi.vn_idx, n_count);
                int vt_idx = fix_index(vi.vt_idx, t_count);
                attrib->faces[f_count + k].v_idx  = v_idx;
                attrib->faces[f_count + k].vn_idx = vn_idx;
                attrib->faces[f_count + k].vt_idx = vt_idx;
            }
            for (size_t k = 0; k < commands[i].num_f_num_verts; k++) {
                attrib->material_ids[face_count + k] = material_id;
                attrib->face_num_verts[face_count + k] = commands[i].f_num_verts[k];
            }
            f_count    += commands[i].num_f;
            face_count += commands[i].num_f_num_verts;
        }
    }

    /* Find the number of shapes in .obj */
    size_t n = 0;
    for (size_t i = 0; i < num_lines; i++) {
        if (commands[i].type == COMMAND_O || commands[i].type == COMMAND_G)
            n++;
    }

    /* Allocate array of shapes with maximum possible size (+1 for unnamed group/object).
     * Actual # of shapes found in .obj is determined in the later */
    const char* shape_name = 0, *prev_shape_name = 0;
    size_t shape_name_len = 0, prev_shape_name_len = 0;
    size_t prev_shape_face_offset = 0, prev_face_offset = 0;
    obj_shape_t prev_shape = {0, 0, 0};

    /* Construct shape information. */
    size_t shape_idx = 0;
    *shapes = malloc((n + 1) * sizeof(**shapes));
    face_count = 0;
    for (size_t i = 0; i < num_lines; i++) {
        if (commands[i].type == COMMAND_O || commands[i].type == COMMAND_G) {
            if (commands[i].type == COMMAND_O) {
                shape_name = commands[i].object_name;
                shape_name_len = commands[i].object_name_len;
            } else {
                shape_name = commands[i].group_name;
                shape_name_len = commands[i].group_name_len;
            }

            if (face_count == 0) {
                /* 'o' or 'g' appears before any 'f' */
                prev_shape_name = shape_name;
                prev_shape_name_len = shape_name_len;
                prev_shape_face_offset = face_count;
                prev_face_offset = face_count;
            } else {
                if (shape_idx == 0) {
                    /* 'o' or 'g' after some 'v' lines. */
                    (*shapes)[shape_idx].name = strndup(prev_shape_name, prev_shape_name_len); /* May be 0 */
                    (*shapes)[shape_idx].face_offset = prev_shape.face_offset;
                    (*shapes)[shape_idx].length = face_count - prev_face_offset;
                    shape_idx++;
                    prev_face_offset = face_count;
                } else {
                    if ((face_count - prev_face_offset) > 0) {
                        (*shapes)[shape_idx].name = strndup(prev_shape_name, prev_shape_name_len);
                        (*shapes)[shape_idx].face_offset = prev_face_offset;
                        (*shapes)[shape_idx].length = face_count - prev_face_offset;
                        shape_idx++;
                        prev_face_offset = face_count;
                    }
                }
                /* Record shape info for succeeding 'o' or 'g' command. */
                prev_shape_name = shape_name;
                prev_shape_name_len = shape_name_len;
                prev_shape_face_offset = face_count;
            }
        }
        if (commands[i].type == COMMAND_F)
            face_count++;
    }

    if ((face_count - prev_face_offset) > 0) {
        size_t length = face_count - prev_shape_face_offset;
        if (length > 0) {
            (*shapes)[shape_idx].name = strndup(prev_shape_name, prev_shape_name_len);
            (*shapes)[shape_idx].face_offset = prev_face_offset;
            (*shapes)[shape_idx].length = face_count - prev_face_offset;
            shape_idx++;
        }
    } else {
        /* Guess no 'v' line occurrence after 'o' or 'g', so discards current
         * shape information. */
    }
    *num_shapes = shape_idx;

    free(commands);
    hash_table_destroy(material_table);
    return 0;
}

obj_model_t* obj_model_parse(const char* buf, size_t len)
{
    obj_model_t* obj = calloc(1, sizeof(*obj));
    int r = obj_parse_obj(&obj->attrib, &obj->shapes, &obj->num_shapes, buf, len, OBJ_FLAG_TRIANGULATE);
    if (r == -1) {
        obj_model_free(obj);
        return 0;
    }
    /* If default name is missing, set it to "root_group" */
    if (obj->num_shapes == 1 && strcmp(obj->shapes[0].name, "") == 0) {
        free((void*)obj->shapes[0].name);
        obj->shapes[0].name = strdup("root_group");
    }
    return obj;
}

void obj_model_free(obj_model_t* obj)
{
    obj_shape_t* shapes = obj->shapes;
    obj_attrib_t* attrib = &obj->attrib;
    for (size_t i = 0; i < obj->num_shapes; i++)
        free((void*)shapes[i].name);
    free(shapes);
    free(attrib->vertices);
    free(attrib->normals);
    free(attrib->texcoords);
    free(attrib->faces);
    free(attrib->face_num_verts);
    free(attrib->material_ids);
    free(obj);
}

/*----------------------------------------------------------------------
 * Obj model to scene datatype conversion
 *----------------------------------------------------------------------*/
static uint32_t face_hash(hash_key_t k)
{
    obj_vertex_index_t* vi = (obj_vertex_index_t*)k;
    uint32_t h = vi->v_idx;
    return (0x811C9DC5 ^ h) * 0x01000193;
}

static int face_eql(hash_key_t k1, hash_key_t k2)
{
    obj_vertex_index_t* vi1 = (obj_vertex_index_t*)k1;
    obj_vertex_index_t* vi2 = (obj_vertex_index_t*)k2;
    return memcmp(vi1, vi2, sizeof(obj_vertex_index_t)) == 0;
}

static void obj_flatten_shape(obj_model_t* obj,
                              size_t sidx,
                              float** positions,
                              float** normals,
                              float** texcoords,
                              size_t* num_vertices,
                              uint32_t** indices,
                              size_t* num_indices)
{
    size_t off_faces = obj->shapes[sidx].face_offset * 3;
    size_t num_indcs = obj->shapes[sidx].length * 3;
    size_t num_verts = 0;
    uint32_t* ind = calloc(num_indcs, sizeof(*ind));
    float* pos    = calloc(num_indcs, 3 * sizeof(float));
    float* nrm    = calloc(num_indcs, 3 * sizeof(float));
    float* tco    = calloc(num_indcs, 2 * sizeof(float));
    struct hash_table* face_table = hash_table_create(face_hash, face_eql);
    for (size_t i = 0; i < num_indcs; ++i) {
        obj_vertex_index_t* vi = &obj->attrib.faces[off_faces + i];
        hash_val_t* e = hash_table_search(face_table, (hash_key_t)vi);
        uint32_t vidx;
        if (e) {
            vidx = (uint32_t)*e;
        } else {
            memcpy(pos + num_verts * 3, obj->attrib.vertices  + vi->v_idx  * 3, 3 * sizeof(float));
            memcpy(nrm + num_verts * 3, obj->attrib.normals   + vi->vn_idx * 3, 3 * sizeof(float));
            memcpy(tco + num_verts * 2, obj->attrib.texcoords + vi->vt_idx * 2, 2 * sizeof(float));
            vidx = num_verts++;
            hash_table_insert(face_table, (hash_key_t)vi, (hash_val_t)vidx);
        }
        ind[i] = vidx;
    }
    hash_table_destroy(face_table);
    *positions    = pos;
    *normals      = nrm;
    *texcoords    = tco;
    *indices      = ind;
    *num_vertices = num_verts;
    *num_indices  = num_indcs;
}

struct scene* obj_to_scene(obj_model_t* obj)
{
    struct scene* scn = calloc(1, sizeof(*scn));
    scene_init(scn);
    scn->num_meshes = obj->num_shapes;
    scn->meshes = calloc(scn->num_meshes, sizeof(*scn->meshes));
    scn->num_instances = obj->num_shapes;
    scn->instances = calloc(scn->num_instances, sizeof(*scn->instances));
    scn->num_nodes = obj->num_shapes;
    scn->nodes = calloc(scn->num_nodes, sizeof(*scn->nodes));

    for (size_t k = 0; k < obj->num_shapes; ++k) {
        obj_shape_t* obj_shp = &obj->shapes[k];
        struct node* node         = calloc(1, sizeof(*node));
        node_init(node);
        struct mesh* msh          = calloc(1, sizeof(*msh));
        _mesh_init(msh);
        struct mesh_instance* ist = calloc(1, sizeof(*ist));
        mesh_instance_init(ist);
        struct shape* shp         = calloc(1, sizeof(*shp));
        shape_init(shp);

        msh->num_shapes = 1;
        msh->shapes     = calloc(msh->num_shapes, sizeof(*msh->shapes));
        msh->shapes[0]  = shp;
        ist->msh        = msh;
        node->ist       = ist;
        node->name      = strdup(obj_shp->name);

        float *positions, *normals, *texcoords; uint32_t* indices; size_t num_verts, num_indcs;
        obj_flatten_shape(obj, k, &positions, &normals, &texcoords, &num_verts, &indices, &num_indcs);

        shp->num_pos       = shp->num_norm = shp->num_texcoord = num_verts;
        shp->pos           = (vec3f*)positions;
        shp->norm          = (vec3f*)normals;
        shp->texcoord      = (vec2f*)texcoords;
        shp->num_triangles = num_indcs / 3;
        shp->triangles     = (vec3i*)indices;

        scn->nodes[k]     = node;
        scn->instances[k] = ist;
        scn->meshes[k]    = msh;
    }

    return scn;
}
