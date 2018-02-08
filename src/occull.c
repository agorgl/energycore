#include "occull.h"
#include <stdlib.h>
#include <hashmap.h>
#include "opengl.h"
#include "bbrndr.h"

struct occlusion_info {
    GLuint query;
    GLuint last_result;
};

void occull_init(struct occull_state* st)
{
    st->occlusion_db = calloc(1, sizeof(struct hashmap));
    hashmap_init(st->occlusion_db, hm_u64_hash, hm_u64_eql);
    st->bbox_rndr = calloc(1, sizeof(struct bbox_rndr));
    bbox_rndr_init(st->bbox_rndr);
}

void occull_object_begin(struct occull_state* st, unsigned int handle)
{
    hm_ptr* p = hashmap_get(st->occlusion_db, hm_cast(handle));
    struct occlusion_info* oi = 0;
    int begin_new_query = 0;
    if (!p) {
        /* Allocate and store occlusion info object */
        oi = calloc(1, sizeof(struct occlusion_info));
        oi->last_result = 1;
        hashmap_put(st->occlusion_db, hm_cast(handle), hm_cast(oi));
        /* Generate occlusion query if current object is not assosiated with one */
        glGenQueries(1, &oi->query);
        begin_new_query = 1;
    } else {
        oi = (struct occlusion_info*)hm_pcast(*p);
        /* Get query result performed in previous frame */
        GLint result_available = 0;
        glGetQueryObjectiv(oi->query, GL_QUERY_RESULT_AVAILABLE, &result_available);
        if (result_available) {
            glGetQueryObjectuiv(oi->query, GL_QUERY_RESULT, &oi->last_result);
            begin_new_query = 1;
        }
    }
    st->cur_obj_visible = oi->last_result;
    /* Begin new occlusion query for current object */
    if (begin_new_query)
        glBeginQuery(GL_ANY_SAMPLES_PASSED, oi->query);
    st->query_is_active = begin_new_query;
}

int occull_should_render(struct occull_state* st, float bbmin[3], float bbmax[3])
{
    /* If object was not previously visible */
    if (!st->cur_obj_visible) {
        /* Disable writing to any buffer and render the object's bounding box.
         * Also temporarily disable face culling to render objects that
         * we are -inside- their bounding box */
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);
        bbox_rndr_render(st->bbox_rndr, bbmin, bbmax);
        glEnable(GL_CULL_FACE);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthMask(GL_TRUE);
    }
    return st->cur_obj_visible;
}

void occull_object_end(struct occull_state* st)
{
    if (st->query_is_active)
        glEndQuery(GL_ANY_SAMPLES_PASSED);
}

void occull_destroy(struct occull_state* st)
{
    bbox_rndr_destroy(st->bbox_rndr);
    free(st->bbox_rndr);
    struct hashmap_iter it;
    hashmap_for(*st->occlusion_db, it) {
        struct occlusion_info* oi = hm_pcast(it.p->value);
        glDeleteQueries(1, &oi->query);
        free(oi);
    }
    hashmap_destroy(st->occlusion_db);
    free(st->occlusion_db);
}
