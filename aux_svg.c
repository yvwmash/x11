#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "aux_svg.h"

/* ============================================ */

static svg_st_t       st_op = ST_ERROR;
static svg_st_pen_t   st_pe = ST_PEN_ABS;
static aux_svg_ctx_t  svg_ctx;

/* ============================================ */

/* */
void aux_svg_zero_ctx()
{
 memset(&svg_ctx, 0, sizeof(struct aux_svg_ctx_s));
}

/* */
void aux_svg_init_ctx()
{
 svg_ctx.fn_moveto = NULL;

 svg_ctx.initial_point.x = 0;
 svg_ctx.initial_point.y = 0;
 svg_ctx.current_point.x = 0;
 svg_ctx.current_point.y = 0;
 svg_ctx.coordinates_stack_e = 0;
 svg_ctx.coordinates_stack_b = 0;
}

/* */
void  aux_svg_set_op_state(svg_st_t st)
{
 st_op = st;
}

/* */
void  aux_svg_set_pe_state(svg_st_pen_t st)
{
 st_pe = st;
}

/* */
void aux_svg_set_moveto_cb(pfn_moveto p)
{
 svg_ctx.fn_moveto = p;
}

/* */
void aux_svg_set_lineto_cb(pfn_lineto p)
{
 svg_ctx.fn_lineto = p;
}

/* */
void aux_svg_set_push_coordinate_cb(pfn_push_coordinate p)
{
 svg_ctx.fn_push_coordinate = p;
}

/* */
void aux_svg_set_get_coordinate_cb(pfn_get_coordinate p)
{
 svg_ctx.fn_get_coordinate = p;
}

/* */
void aux_svg_set_closepath_cb(pfn_closepath p)
{
 svg_ctx.fn_closepath = p;
}

/* */
void aux_svg_set_end_path_cb(pfn_end_path p)
{
 svg_ctx.fn_end_path = p;
}

/* */
void aux_svg_set_clear_stack_cb(pfn_clear_stack p)
{
 svg_ctx.fn_clear_stack = p;
}

/* */
void aux_svg_moveto(void)
{
 svg_coordinate tp = {0.0, 0.0};

 assert(svg_ctx.coordinates_stack_e > 0);
 assert(st_op != ST_ERROR);
 assert((st_op == ST_MOVETO_START_PATH) || (st_op == ST_MOVETO_SUBPATH));

 if(ST_MOVETO_START_PATH == st_op) { /* absolute coordinates */
  svg_ctx.initial_point = svg_ctx.fn_get_coordinate(0);
  svg_ctx.current_point = svg_ctx.fn_get_coordinate(0);
 } else { /* start sub-path */
  if(ST_PEN_ABS == st_pe) { /* absolute coordinates */
   svg_ctx.initial_point = svg_ctx.fn_get_coordinate(0);
   svg_ctx.current_point = svg_ctx.fn_get_coordinate(0);
  } else { /* relative coordinates */
   svg_ctx.initial_point.x = svg_ctx.fn_get_coordinate(0).x + svg_ctx.current_point.x;
   svg_ctx.initial_point.y = svg_ctx.fn_get_coordinate(0).y + svg_ctx.current_point.y;
   svg_ctx.current_point   = svg_ctx.initial_point;
  }
 }

 /* following moveto's will respect PEN state for initial and current points */
 st_op = ST_MOVETO_SUBPATH;

 /* callback */
 svg_ctx.fn_moveto(&svg_ctx, svg_ctx.initial_point);

 /* produce segments */
 for(svg_ctx.coordinates_stack_b = 1; svg_ctx.coordinates_stack_b < svg_ctx.coordinates_stack_e; svg_ctx.coordinates_stack_b += 1) {
  tp = svg_ctx.fn_get_coordinate(svg_ctx.coordinates_stack_b);
  if(ST_PEN_REL == st_pe) {
   tp.x += svg_ctx.current_point.x;
   tp.y += svg_ctx.current_point.y;
  }
  svg_ctx.fn_lineto(&svg_ctx, tp);
  svg_ctx.current_point = tp; /* set current point */
 }

 assert(svg_ctx.coordinates_stack_b == svg_ctx.coordinates_stack_e);

 /* clear stack */
 svg_ctx.coordinates_stack_b = 0;
 svg_ctx.coordinates_stack_e = 0;
 svg_ctx.fn_clear_stack();
}

/* */
void aux_svg_end_path(void)
{
 svg_ctx.fn_end_path();
}

/* */
void aux_svg_closepath(void)
{
 svg_ctx.fn_lineto(&svg_ctx, svg_ctx.initial_point);

 svg_ctx.current_point = svg_ctx.initial_point;

 svg_ctx.fn_closepath(&svg_ctx, svg_ctx.initial_point);
}

/* */
unsigned aux_svg_push_coordinate(double x, double y)
{
 unsigned save_index;

 save_index                   = svg_ctx.fn_push_coordinate(x, y);
 svg_ctx.coordinates_stack_e += 1;

 return save_index;
}

/* */
svg_coordinate   aux_svg_get_current_point()
{
 return svg_ctx.current_point;
}

/* */
svg_coordinate   aux_svg_get_initial_point()
{
 return svg_ctx.initial_point;
}
