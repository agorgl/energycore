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
#ifndef _PARSHP_H_
#define _PARSHP_H_

#include <stdint.h>

/* Indice type */
typedef int32_t parshp_index_type_t;

/* Mesh type */
struct parshp_mesh {
    float* points;                  /* Flat list of 3-tuples (x y z x y z...) */
    float* normals;                 /* Optional list of 3-tuples (x y z x y z...) */
    float* tcoords;                 /* Optional list of 2-tuples (u v u v u v...) */
    int npoints;                    /* Number of points */
    parshp_index_type_t* triangles; /* Flat list of 3-tuples (i j k i j k...) */
    int ntriangles;                 /* Number of triangles */
};
void parshp_free_mesh(struct parshp_mesh*);

/*-----------------------------------------------------------------
 * Generators
 *-----------------------------------------------------------------*/
/* Instance a cylinder that sits on the z=0 plane using the given tessellation
 * levels across the UV domain. Think of "slices" like a number of pizza
 * slices, and "stacks" like a number of stacked rings. Height and radius are
 * both 1.0, but they can easily be changed with parshp_scale. */
struct parshp_mesh* parshp_create_cylinder(int slices, int stacks);

/* Create a donut that sits on the z=0 plane with the specified inner radius.
 * The outer radius can be controlled with parshp_scale. */
struct parshp_mesh* parshp_create_torus(int slices, int stacks, float radius);

/* Create a sphere with texture coordinates and small triangles near the poles. */
struct parshp_mesh* parshp_create_parametric_sphere(int slices, int stacks);

/* Approximate a sphere with a subdivided icosahedron, which produces a nice
 * distribution of triangles, but no texture coordinates. Each subdivision
 * level scales the number of triangles by four, so use a very low number. */
struct parshp_mesh* parshp_create_subdivided_sphere(int nsubdivisions);

/* More parametric surfaces. */
struct parshp_mesh* parshp_create_klein_bottle(int slices, int stacks);
struct parshp_mesh* parshp_create_trefoil_knot(int slices, int stacks, float radius);
struct parshp_mesh* parshp_create_hemisphere(int slices, int stacks);
struct parshp_mesh* parshp_create_plane(int slices, int stacks);

/* Create a parametric surface from a callback function that consumes a 2D
 * point in [0,1] and produces a 3D point. */
typedef void (*parshp_fn)(float const*, float*, void*);
struct parshp_mesh* parshp_create_parametric(parshp_fn, int slices, int stacks, void* userdata);

/* Generate points for a 20-sided polyhedron that fits in the unit sphere.
 * Texture coordinates and normals are not generated. */
struct parshp_mesh* parshp_create_icosahedron();

/* Generate points for a 12-sided polyhedron that fits in the unit sphere.
 * Again, texture coordinates and normals are not generated. */
struct parshp_mesh* parshp_create_dodecahedron();

/* More platonic solids. */
struct parshp_mesh* parshp_create_octahedron();
struct parshp_mesh* parshp_create_tetrahedron();
struct parshp_mesh* parshp_create_cube();

/* Generate an orientable disk shape in 3-space.
 * Does not include normals or texture coordinates. */
struct parshp_mesh* parshp_create_disk(float radius, int slices, float const* center, float const* normal);

/* Create an empty shape.
 * Useful for building scenes with merge_and_free. */
struct parshp_mesh* parshp_create_empty();

/* Generate a rock shape that sits on the y=0 plane, and sinks into it a bit.
 * This includes smooth normals but no texture coordinates. Each subdivision
 * level scales the number of triangles by four, so use a very low number. */
struct parshp_mesh* parshp_create_rock(int seed, int nsubdivisions);

/* Create trees or vegetation by executing a recursive turtle graphics program.
 * The program is a list of command-argument pairs.
 * Texture coordinates and normals are not generated. */
struct parshp_mesh* parshp_create_lsystem(char const* program, int slices, int maxdepth);

/*-----------------------------------------------------------------
 * Queries
 *-----------------------------------------------------------------*/
/* Dump out a text file conforming to the venerable OBJ format. */
void parshp_export(struct parshp_mesh const*, char const* objfile);

/* Take a pointer to 6 floats and set them to min xyz, max xyz. */
void parshp_compute_aabb(struct parshp_mesh const* mesh, float* aabb);

/* Make a deep copy of a mesh. To make a brand new copy, pass null to "target".
 * To avoid memory churn, pass an existing mesh to "target". */
struct parshp_mesh* parshp_clone(struct parshp_mesh const* mesh, struct parshp_mesh* target);

/*-----------------------------------------------------------------
 * Transformations
 *-----------------------------------------------------------------*/
/* Common transformations. */
void parshp_merge(struct parshp_mesh* dst, struct parshp_mesh const* src);
void parshp_translate(struct parshp_mesh*, float x, float y, float z);
void parshp_rotate(struct parshp_mesh*, float radians, float const* axis);
void parshp_scale(struct parshp_mesh*, float x, float y, float z);
void parshp_merge_and_free(struct parshp_mesh* dst, struct parshp_mesh* src);

/* Reverse the winding of a run of faces. Useful when drawing the inside of
 * a Cornell Box. Pass 0 for nfaces to reverse every face in the mesh. */
void parshp_invert(struct parshp_mesh*, int startface, int nfaces);

/* Remove all triangles whose area is less than minarea. */
void parshp_remove_degenerate(struct parshp_mesh*, float minarea);

/* Dereference the entire index buffer and replace the point list.
 * This creates an inefficient structure, but is useful for drawing facets.
 * If create_indices is true, a trivial "0 1 2 3..." index buffer is generated. */
void parshp_unweld(struct parshp_mesh* mesh, int create_indices);

/* Merge colocated verts, build a new index buffer, and return the
 * optimized mesh. Epsilon is the maximum distance to consider when
 * welding vertices. The mapping argument can be null, or a pointer to
 * npoints integers, which gets filled with the mapping from old vertex
 * indices to new indices. */
struct parshp_mesh* parshp_weld(struct parshp_mesh const*, float epsilon, parshp_index_type_t* mapping);

/* Compute smooth normals by averaging adjacent facet normals. */
void parshp_compute_normals(struct parshp_mesh* m);

#endif /* ! _PARSHP_H_ */
