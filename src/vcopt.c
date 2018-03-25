#include "vcopt.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Set these to adjust the performance and result quality */
#define FORSYTH_VERTEX_CACHE_SIZE 24
#define FORSYTH_CACHE_FUNCTION_LENGTH 32

/* The size of these data types affect the memory usage */
typedef uint16_t forsyth_score_type;
#define FORSYTH_SCORE_SCALING 7281

typedef uint8_t forsyth_adjacency_type;
#define FORSYTH_MAX_ADJACENCY UINT8_MAX

typedef int8_t forsyth_cache_pos_type;
typedef int32_t forsyth_triangle_index_type;
typedef int32_t forsyth_array_index_type;

/* The size of the precalculated tables */
#define FORSYTH_CACHE_SCORE_TABLE_SIZE 32
#define FORSYTH_VALENCE_SCORE_TABLE_SIZE 32
#if FORSYTH_CACHE_SCORE_TABLE_SIZE < FORSYTH_VERTEX_CACHE_SIZE
#error Vertex score table too small
#endif

/* Precalculated tables */
static forsyth_score_type forsyth_cache_position_score[FORSYTH_CACHE_SCORE_TABLE_SIZE];
static forsyth_score_type forsyth_valence_score[FORSYTH_VALENCE_SCORE_TABLE_SIZE];

#define FORSYTH_ISADDED(x) (triangle_added[(x) >> 3] & (1 << (x & 7)))
#define FORSYTH_SETADDED(x) (triangle_added[(x) >> 3] |= (1 << (x & 7)))

/* Score function constants */
#define FORSYTH_CACHE_DECAY_POWER 1.5
#define FORSYTH_LAST_TRI_SCORE 0.75
#define FORSYTH_VALENCE_BOOST_SCALE 2.0
#define FORSYTH_VALENCE_BOOST_POWER 0.5

/* Precalculate the tables */
static void forsyth_init() {
    for (int i = 0; i < FORSYTH_CACHE_SCORE_TABLE_SIZE; i++) {
        float score = 0;
        if (i < 3) {
            /* This vertex was used in the last triangle,
             * so it has a fixed score, which ever of the three
             * it's in. Otherwise, you can get very different
             * answers depending on whether you add
             * the triangle 1,2,3 or 3,1,2 - which is silly */
            score = FORSYTH_LAST_TRI_SCORE;
        } else {
            /* Points for being high in the cache. */
            const float scaler = 1.0f / (FORSYTH_CACHE_FUNCTION_LENGTH - 3);
            score = 1.0f - (i - 3) * scaler;
            score = powf(score, FORSYTH_CACHE_DECAY_POWER);
        }
        forsyth_cache_position_score[i] = (forsyth_score_type)(FORSYTH_SCORE_SCALING * score);
    }

    for (int i = 1; i < FORSYTH_VALENCE_SCORE_TABLE_SIZE; i++) {
        /* Bonus points for having a low number of tris still to
         * use the vert, so we get rid of lone verts quickly */
        float valence_boost = powf(i, -FORSYTH_VALENCE_BOOST_POWER);
        float score = FORSYTH_VALENCE_BOOST_SCALE * valence_boost;
        forsyth_valence_score[i] = (forsyth_score_type)(FORSYTH_SCORE_SCALING * score);
    }
}

/* Calculate the score for a vertex */
static forsyth_score_type forsyth_find_vertex_score(int num_active_tris, int cache_position) {
    if (num_active_tris == 0) {
        /* No triangles need this vertex! */
        return 0;
    }

    forsyth_score_type score = 0;
    if (cache_position < 0) {
        /* Vertex is not in LRU cache - no score */
    } else {
        score = forsyth_cache_position_score[cache_position];
    }

    if (num_active_tris < FORSYTH_VALENCE_SCORE_TABLE_SIZE)
        score += forsyth_valence_score[num_active_tris];
    return score;
}

/* The main reordering function */
forsyth_vertex_index_type* forsyth_reorder_indices(forsyth_vertex_index_type* out_indices,
                                                   const forsyth_vertex_index_type* indices, int n_triangles,
                                                   int n_vertices)
{
    static int init = 1;
    if (init) {
        forsyth_init();
        init = 0;
    }

    forsyth_adjacency_type* num_active_tris = (forsyth_adjacency_type*)malloc(sizeof(forsyth_adjacency_type) * n_vertices);
    memset(num_active_tris, 0, sizeof(forsyth_adjacency_type) * n_vertices);

    /* First scan over the vertex data, count the total number of occurrances of each vertex */
    for (int i = 0; i < 3 * n_triangles; i++) {
        if (num_active_tris[indices[i]] == FORSYTH_MAX_ADJACENCY) {
            /* Unsupported mesh, vertex shared by too many triangles */
            free(num_active_tris);
            return NULL;
        }
        num_active_tris[indices[i]]++;
    }

    /* Allocate the rest of the arrays */
    forsyth_array_index_type* offsets = (forsyth_array_index_type*)malloc(sizeof(forsyth_array_index_type) * n_vertices);
    forsyth_score_type* last_score = (forsyth_score_type*)malloc(sizeof(forsyth_score_type) * n_vertices);
    forsyth_cache_pos_type* cache_tag = (forsyth_cache_pos_type*)malloc(sizeof(forsyth_cache_pos_type) * n_vertices);

    uint8_t* triangle_added = (uint8_t*)malloc((n_triangles + 7) / 8);
    forsyth_score_type* triangle_score = (forsyth_score_type*)malloc(sizeof(forsyth_score_type) * n_triangles);
    forsyth_triangle_index_type* triangle_indices = (forsyth_triangle_index_type*)malloc(sizeof(forsyth_triangle_index_type) * 3 * n_triangles);
    memset(triangle_added, 0, sizeof(uint8_t) * ((n_triangles + 7) / 8));
    memset(triangle_score, 0, sizeof(forsyth_score_type) * n_triangles);
    memset(triangle_indices, 0, sizeof(forsyth_triangle_index_type) * 3 * n_triangles);

    /* Count the triangle array offset for each vertex, initialize the rest of the data. */
    int sum = 0;
    for (int i = 0; i < n_vertices; i++) {
        offsets[i] = sum;
        sum += num_active_tris[i];
        num_active_tris[i] = 0;
        cache_tag[i] = -1;
    }

    /* Fill the vertex data structures with indices to the triangles using each vertex */
    for (int i = 0; i < n_triangles; i++) {
        for (int j = 0; j < 3; j++) {
            int v = indices[3 * i + j];
            triangle_indices[offsets[v] + num_active_tris[v]] = i;
            num_active_tris[v]++;
        }
    }

    /* Initialize the score for all vertices */
    for (int i = 0; i < n_vertices; i++) {
        last_score[i] = forsyth_find_vertex_score(num_active_tris[i], cache_tag[i]);
        for (int j = 0; j < num_active_tris[i]; j++)
            triangle_score[triangle_indices[offsets[i] + j]] += last_score[i];
    }

    /* Find the best triangle */
    int best_triangle = -1;
    int best_score = -1;

    for (int i = 0; i < n_triangles; i++) {
        if (triangle_score[i] > best_score) {
            best_score = triangle_score[i];
            best_triangle = i;
        }
    }

    /* Allocate the output array */
    forsyth_triangle_index_type* out_triangles = (forsyth_triangle_index_type*)malloc(sizeof(forsyth_triangle_index_type) * n_triangles);
    int out_pos = 0;

    /* Initialize the cache */
    int cache[FORSYTH_VERTEX_CACHE_SIZE + 3];
    for (int i = 0; i < FORSYTH_VERTEX_CACHE_SIZE + 3; i++)
        cache[i] = -1;

    int scan_pos = 0;

    /* Output the currently best triangle, as long as there are triangles left to output */
    while (best_triangle >= 0) {
        /* Mark the triangle as added */
        FORSYTH_SETADDED(best_triangle);
        /* Output this triangle */
        out_triangles[out_pos++] = best_triangle;
        for (int i = 0; i < 3; i++) {
            /* Update this vertex */
            int v = indices[3 * best_triangle + i];

            /* Check the current cache position, if it is in the cache */
            int endpos = cache_tag[v];
            if (endpos < 0)
                endpos = FORSYTH_VERTEX_CACHE_SIZE + i;
            if (endpos > i) {
                /* Move all cache entries from the previous position
                 * in the cache to the new target position (i) one
                 * step backwards */
                for (int j = endpos; j > i; j--) {
                    cache[j] = cache[j - 1];
                    /* If this cache slot contains a real
                     * vertex, update its cache tag */
                    if (cache[j] >= 0)
                        cache_tag[cache[j]]++;
                }
                /* Insert the current vertex into its new target slot */
                cache[i] = v;
                cache_tag[v] = i;
            }

            /* Find the current triangle in the list of active
             * triangles and remove it (moving the last
             * triangle in the list to the slot of this triangle). */
            for (int j = 0; j < num_active_tris[v]; j++) {
                if (triangle_indices[offsets[v] + j] == best_triangle) {
                    triangle_indices[offsets[v] + j] = triangle_indices[offsets[v] + num_active_tris[v] - 1];
                    break;
                }
            }
            /* Shorten the list */
            num_active_tris[v]--;
        }
        /* Update the scores of all triangles in the cache */
        for (int i = 0; i < FORSYTH_VERTEX_CACHE_SIZE + 3; i++) {
            int v = cache[i];
            if (v < 0)
                break;
            /* This vertex has been pushed outside of the actual cache */
            if (i >= FORSYTH_VERTEX_CACHE_SIZE) {
                cache_tag[v] = -1;
                cache[i] = -1;
            }
            forsyth_score_type new_score = forsyth_find_vertex_score(num_active_tris[v], cache_tag[v]);
            forsyth_score_type diff = new_score - last_score[v];
            for (int j = 0; j < num_active_tris[v]; j++)
                triangle_score[triangle_indices[offsets[v] + j]] += diff;
            last_score[v] = new_score;
        }
        /* Find the best triangle referenced by vertices in the cache */
        best_triangle = -1;
        best_score = -1;
        for (int i = 0; i < FORSYTH_VERTEX_CACHE_SIZE; i++) {
            if (cache[i] < 0)
                break;
            int v = cache[i];
            for (int j = 0; j < num_active_tris[v]; j++) {
                int t = triangle_indices[offsets[v] + j];
                if (triangle_score[t] > best_score) {
                    best_triangle = t;
                    best_score = triangle_score[t];
                }
            }
        }
        /* If no active triangle was found at all, continue
         * scanning the whole list of triangles */
        if (best_triangle < 0) {
            for (; scan_pos < n_triangles; scan_pos++) {
                if (!FORSYTH_ISADDED(scan_pos)) {
                    best_triangle = scan_pos;
                    break;
                }
            }
        }
    }

    /* Convert the triangle index array into a full triangle list */
    out_pos = 0;
    for (int i = 0; i < n_triangles; i++) {
        int t = out_triangles[i];
        for (int j = 0; j < 3; j++) {
            int v = indices[3 * t + j];
            out_indices[out_pos++] = v;
        }
    }

    /* Clean up */
    free(triangle_indices);
    free(offsets);
    free(last_score);
    free(num_active_tris);
    free(cache_tag);
    free(triangle_added);
    free(triangle_score);
    free(out_triangles);

    return out_indices;
}
