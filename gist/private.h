/*
 * gist/private.h -
 *
 * Definition for private data and routines needed to communicate between
 * different parts of the code.
 */

#ifndef _GIST_PRIVATE_H
#define _GIST_PRIVATE_H 1

#include <gist/draw.h>

/* Scratch spaces defined in gist.c */
extern int _ga_get_scratch_p(long n);
extern int _ga_get_scratch_s(long n);
extern gp_real_t* _ga_scratch_x;
extern gp_real_t* _ga_scratch_y;
extern short*     _ga_scratch_s;

extern void _gd_kill_systems(void); /* defined in draw.c */
extern void _gd_kill_ring(void *elv); /* defined in draw0.c */
extern int  _gd_draw_ring(void *elv, int xIsLog, int yIsLog,
                          ge_system_t *sys, int t); /* defined in draw.c */

extern void _gd_lines_subset(void *el); /* defined in draw0.c */

extern int  _gd_make_contours(ge_contours_t* con); /* defined in draw.c */

/* Copy mesh properties to gd_properties (defined in draw0.c) */
extern void _gd_get_mesh_xy(void *vMeshEl);

extern void _gd_next_mesh_block(long *ii, long *jj, long len, long iMax,
                                int *reg, int region);

/* Free members of a mesh element (defined in draw0.c) */
extern void _gd_kill_mesh_xy(void *vMeshEl);

/* Get drawing virtual function tables (defined in draw0.c) */
extern gd_operations_t *_gd_get_drawing_operations(void);

extern void _gd_scan_z(long n, const gp_real_t *z,
                       gp_real_t *zmin, gp_real_t *zmax);

/* The nice unit routine defined here is also used in draw.c */
/* In tick.c */
extern gp_real_t _gp_nice_unit(gp_real_t finest, int *base, int *power);

/* Hook for hlevel.c error handling.  */
extern void (*_gh_hook)(gp_engine_t* engine);

#define PL_NEW(number, type)  ((type*)pl_malloc(sizeof(type)*(number)))

#endif /* _GIST_PRIVATE_H */
