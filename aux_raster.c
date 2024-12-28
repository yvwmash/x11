#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "aux_raster.h"

/* */
int aux_raster_putpix(uint32_t         x,
                      uint32_t         y,
                      uint32_t         color,
                      aux_raster_buf  *pixels)
{
#if AUX_RASTER_INVERT_Y_AXIS == 1
 y = pixels->h - y;
#endif

 if(!(x < pixels->w)) {
  return 0;
 }
 if(!(y < pixels->h)) {
  return 0;
 }

 ((uint32_t*)(pixels->buf))[y * pixels->w + x] = color;

 return 0;
}

/* */
uint32_t aux_raster_getpix(uint32_t         x,
                           uint32_t         y,
                           aux_raster_buf  *pixels)
{
#if AUX_RASTER_INVERT_Y_AXIS == 1
 y = pixels->h - y;
#endif

 if(!(x < pixels->w)) {
  return 0;
 }
 if(!(y < pixels->h)) {
  return 0;
 }

 return ((uint32_t*)(pixels->buf))[y * pixels->w + x];
}

/* */
int aux_raster_line_h(uint32_t         x,
                      uint32_t         y,
                      uint32_t         w,
                      uint32_t         color,
                      aux_raster_buf  *pixels)
{
 uint32_t dst_x = (x + w + 1) < pixels->w ? (x + w + 1) : pixels->w;

 for(; x < dst_x; ++x) {
  aux_raster_putpix(x, y, color, pixels);
 }

 return 0;
}

/* */
int aux_raster_line_v(uint32_t         x,
                      uint32_t         y,
                      uint32_t         h,
                      uint32_t         color,
                      aux_raster_buf  *pixels)
{
 uint32_t dst_y = (y + h + 1) < pixels->h ? (y + h + 1) : pixels->h;

 for(; y < dst_y; ++y){
  aux_raster_putpix(x, y, color, pixels);
 }

 return 0;
}

/* */
int aux_raster_fill_rc(uint32_t         x0, uint32_t y0,
                       uint32_t         w,  uint32_t h,
                       uint32_t         color,
                       aux_raster_buf  *aux_buf)
{
 uint32_t dst_x = (x0 + w + 1) < aux_buf->w ? (x0 + w + 1) : aux_buf->w;
 uint32_t dst_y = (y0 + h + 1) < aux_buf->h ? (y0 + h + 1) : aux_buf->h;

 for(; x0 < dst_x; ++x0) {
  for(; y0 < dst_y; ++y0) {
   aux_raster_putpix(x0, y0, color, aux_buf);
  }
 }

 return 0;
}

/* */
int aux_raster_circle(uint32_t         x,
                      uint32_t         y,
                      uint32_t         r,
                      uint32_t         color,
					  aux_raster_buf  *pixels)
{
 uint32_t sx = (x < r)             ? 0         : x - r;
 uint32_t sy = (y < r)             ? 0         : y - r;
 uint32_t ex = (x + r) > pixels->w ? pixels->w : x + r;
 uint32_t ey = (y + r) > pixels->h ? pixels->h : y + r;
 uint32_t r2 = r * r;
 uint32_t y2;

 for(uint32_t py = sy; py < ey; py += 1) {
  y2 = py * py;

  for(uint32_t px = sx; px < ex; px += 1) {
   if((px * px + y2) <= r2) {
    aux_raster_putpix(x + px, y + py, color, pixels);
   }
  }
 }

 return 0;
}

/*
int aux_raster_bezier_q(uint32_t         x0,
                        uint32_t         y0,
                        uint32_t         x1,
                        uint32_t         y1,
                        uint32_t         x2,
                        uint32_t         y2,
                        uint32_t         color,
					    aux_raster_buf  *pixels)
{
 double a0, b0, a1, b1;
 double e = 0.01;
 double t = e;
 double x, y, px = x0, py = y0;

 for(; t < 1.0 + e; t += e) {
  t = (t < 1.0)? t : 1.0;
  a0 = (double)x0 - (double)(x0 - x1) * t;
  b0 = (double)y0 - (double)(y0 - y1) * t;
  a1 = (double)x1 - (double)(x1 - x2) * t;
  b1 = (double)y1 - (double)(y1 - y2) * t;
  x = a0 - (a0 - a1) * t;
  y = b0 - (b0 - b1) * t;
  aux_raster_line(px, py, x, y, color, pixels);
  px = x;
  py = y;
 }

 return 0;
}
*/

/*
int aux_raster_bezier_c(uint32_t         x0,
                        uint32_t         y0,
                        uint32_t         x1,
                        uint32_t         y1,
                        uint32_t         x2,
                        uint32_t         y2,
                        uint32_t         x3,
                        uint32_t         y3,
                        uint32_t         color,
					    aux_raster_buf  *pixels)
{
 double a0, b0, a1, b1, a2, b2, c0, d0, c1, d1;
 double e = 0.01;
 double t = e;
 double x, y, px = x0, py = y0;

 for(; t < 1.0 + e; t += e) {
  a0 = (double)x0 - (double)(x0 - x1) * t;
  b0 = (double)y0 - (double)(y0 - y1) * t;
  a1 = (double)x1 - (double)(x1 - x2) * t;
  b1 = (double)y1 - (double)(y1 - y2) * t;
  a2 = (double)x2 - (double)(x2 - x3) * t;
  b2 = (double)y2 - (double)(y2 - y3) * t;
  c0 = a0 - (a0 - a1) * t;
  d0 = b0 - (b0 - b1) * t;
  c1 = a1 - (a1 - a2) * t;
  d1 = b1 - (b1 - b2) * t;
  x = c0 - (c0 - c1) * t;
  y = d0 - (d0 - d1) * t;
  aux_raster_line(px,py,x,y,color,pixels);
  px = x;
  py = y;
 }

 return 0;
}
*/

/* */
int fill_rbuf(uint32_t color, aux_raster_buf  *pixels)
{
 uint32_t const * e;

 assert((NULL != pixels) && "raster buffer struct pointer is NULL");
 assert((NULL != pixels->buf) && "raster buffer data pointer is NULL");
 assert((32 == pixels->scanline_pad) && "raster buffer padding is not 32 bit");

 e = (uint32_t*)pixels->buf + pixels->size / 4;

 for(uint32_t *p = (uint32_t*)pixels->buf; p < e; ++p){
  *p = color;
 }
 return 0;
}
