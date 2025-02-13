#ifndef AUX_SVG_H
#define AUX_SVG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* */
extern int aux_svg_parse_fn(const char *path);

typedef enum {
    ST_ERROR             = 1,
    ST_MOVETO_START_PATH = 2,
    ST_MOVETO_SUBPATH    = 3,
    ST_LINETO            = 4,
    ST_LINETO_V          = 5,
    ST_LINETO_H          = 6
} svg_st_t;

typedef enum {
    ST_PEN_REL     = 1,
    ST_PEN_ABS     = 2
} svg_st_pen_t;

typedef struct {
 double x, y;
}svg_coordinate;

typedef struct aux_svg_ctx_s aux_svg_ctx_t;

typedef  unsigned      (*pfn_moveto)(aux_svg_ctx_t *ctx, svg_coordinate to_point);
typedef  unsigned      (*pfn_lineto)(aux_svg_ctx_t *ctx, svg_coordinate to_point);
typedef  unsigned      (*pfn_closepath)(aux_svg_ctx_t *ctx, svg_coordinate initial_point);
typedef  unsigned      (*pfn_push_coordinate)(double x, double y);
typedef  svg_coordinate(*pfn_get_coordinate)(unsigned index);
typedef  unsigned      (*pfn_end_path)(void);
typedef  unsigned      (*pfn_clear_stack)(void);

typedef struct aux_svg_ctx_s{
 pfn_moveto          fn_moveto;
 pfn_lineto          fn_lineto;
 pfn_push_coordinate fn_push_coordinate;
 pfn_get_coordinate  fn_get_coordinate;
 pfn_closepath       fn_closepath;
 pfn_end_path        fn_end_path;
 pfn_clear_stack     fn_clear_stack;

 svg_coordinate  initial_point;
 svg_coordinate  current_point;
 unsigned        coordinates_stack_e;
 unsigned        coordinates_stack_b;
}aux_svg_ctx_t;

void             aux_svg_zero_ctx();
void             aux_svg_init_ctx();
unsigned         aux_svg_push_coordinate(double  x, double  y);
void             aux_svg_set_moveto_cb(pfn_moveto p);
void             aux_svg_set_lineto_cb(pfn_lineto p);
void             aux_svg_set_push_coordinate_cb(pfn_push_coordinate p);
void             aux_svg_set_get_coordinate_cb(pfn_get_coordinate p);
void             aux_svg_set_closepath_cb(pfn_closepath p);
void             aux_svg_set_end_path_cb(pfn_end_path p);
void             aux_svg_set_clear_stack_cb(pfn_clear_stack p);

void             aux_svg_moveto(void);
void             aux_svg_lineto(void);
void             aux_svg_closepath(void);
void             aux_svg_end_path();

svg_coordinate   aux_svg_get_current_point();
svg_coordinate   aux_svg_get_initial_point();
void             aux_svg_set_op_state(svg_st_t     st_op);
void             aux_svg_set_pe_state(svg_st_pen_t st_pen);
svg_st_t         aux_svg_get_op_state(void);
svg_st_pen_t     aux_svg_get_pe_state(void);

#ifdef __cplusplus
}
#endif

#endif
