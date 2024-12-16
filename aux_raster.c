#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "aux_raster.h"

/* */
int aux_raster_putpix(int              x,
                      int              y, 
                      uint32_t         color, 
                      aux_raster_buf  *pixels)
{
 uint32_t *p = (uint32_t*)pixels->buf;
 
#if AUX_RASTER_INVERT_Y_AXIS == 1
 y = pixels->h - y;
#endif

  x = (x < pixels->w)? x : pixels->w - 1;
  y = (y < pixels->h)? y : pixels->h - 1;

  p += y * pixels->w + x;
 *p  = color;

 return 0;
}

/* */
uint32_t aux_raster_getpix(int              x,
                           int              y,
                           aux_raster_buf  *pixels)
{
 uint32_t *p = (uint32_t*)pixels->buf;

#if AUX_RASTER_INVERT_Y_AXIS == 1
 y = pixels->h - y;
#endif

  x = (x < pixels->w)? x : pixels->w - 1;
  y = (y < pixels->h)? y : pixels->h - 1;

  p += y * pixels->w + x;

 return *p;
}


/* */
int aux_raster_line(int              x0,
                    int              y0, 
                    int              x1, 
                    int              y1, 
                    uint32_t         color, 
                    aux_raster_buf  *pixels)
{
    int dx = x1 - x0;
    // if x1 == x2, then it does not matter what we set here
    int ix = ((dx > 0) - (dx < 0));

    dx = abs(dx) << 1;

    int dy = y1 - y0;
    // if y1 == y2, then it does not matter what we set here
    int iy = ((dy > 0) - (dy < 0));
    dy = abs(dy) << 1;

    aux_raster_putpix(x0, y0, color, pixels);

    if (dx >= dy)
    {
        // error may go below zero
        int error = (dy - (dx >> 1));

        while (x0 != x1)
        {
            if ((error >= 0) && (error || (ix > 0)))
            {
                error -= dx;
                y0 += iy;
            }
            // else do nothing

            error += dy;
            x0 += ix;

            aux_raster_putpix(x0, y0, color, pixels);
        }
    }
    else
    {
        // error may go below zero
        int error = (dx - (dy >> 1));

        while (y0 != y1)
        {
            if ((error >= 0) && (error || (iy > 0)))
            {
                error -= dy;
                x0 += ix;
            }
            // else do nothing

            error += dx;
            y0 += iy;

            aux_raster_putpix(x0, y0, color, pixels);
        }
    }

 return 0;
}

/* */
int aux_raster_line_h(int              x, 
                      int              y, 
                      int              w, 
                      uint32_t         color, 
                      aux_raster_buf  *pixels)
{
 int dst_x = x + w + 1;

 for(; x < dst_x; ++x){
  aux_raster_putpix(x, y, color, pixels);
 }

 return 0;
}

/* */
int aux_raster_line_v(int              x, 
                      int              y, 
                      int              h, 
                      uint32_t         color, 
                      aux_raster_buf  *pixels)
{
 int dst_y = y + h + 1;

 for(; y < dst_y; ++y){
  aux_raster_putpix(x, y, color, pixels);
 }

 return 0;
}

/* */
int aux_raster_fill_rc(int              x0, int  y0,
                       int              w,  int  h,
                       uint32_t         color,
                       aux_raster_buf  *pixels)
{
 int dst_x = x0 + w + 1;
 int dst_y = y0 + h + 1;

 for(; x0 < dst_x; ++x0){
  for(int y = y0; y < dst_y; ++y){
   aux_raster_putpix(x0, y, color, pixels);
  }
 }

 return 0;
}

/* plot ellipse inside a specified rectangle 
 *
 */
int aux_raster_ellipse_rc(int              x0, 
                          int              y0, 
                          int              x1, 
                          int              y1,
                          uint32_t         color,
					      aux_raster_buf  *pixels)
{
   int a = abs(x1-x0), b = abs(y1-y0), b1 = b&1; /* values of diameter */
   long dx = 4*(1-a)*b*b, dy = 4*(b1+1)*a*a; /* error increment */
   long err = dx+dy+b1*a*a, e2; /* error of 1.step */

   if (x0 > x1) { x0 = x1; x1 += a; } /* if called with swapped points */
   if (y0 > y1) y0 = y1; /* .. exchange them */
   y0 += (b+1)/2; y1 = y0-b1;   /* starting pixel */
   a *= 8*a; b1 = 8*b*b;

   do {
       aux_raster_putpix(x1, y0, color, pixels); /*   I. Quadrant */
       aux_raster_putpix(x0, y0, color, pixels); /*  II. Quadrant */
       aux_raster_putpix(x0, y1, color, pixels); /* III. Quadrant */
       aux_raster_putpix(x1, y1, color, pixels); /*  IV. Quadrant */
       e2 = 2*err;
       if (e2 <= dy) { y0++; y1--; err += dy += a; }  /* y step */ 
       if (e2 >= dx || 2*err > dy) { x0++; x1--; err += dx += b1; } /* x step */
   } while (x0 <= x1);
   
   while (y0-y1 < b) {  /* too early stop of flat ellipses a=1 */
       aux_raster_putpix(x0-1, y0,   color, pixels); /* -> finish tip of ellipse */
       aux_raster_putpix(x1+1, y0++, color, pixels); 
       aux_raster_putpix(x0-1, y1,   color, pixels);
       aux_raster_putpix(x1+1, y1--, color, pixels); 
   }

 return 0;
}

int aux_raster_circle(int              x, 
                      int              y, 
                      int              r,
                      uint32_t         color,
					  aux_raster_buf  *pixels)
{
 int px = -r, py = 0, err = 2 - 2 * r; /* II. quadrant */

 do{
  aux_raster_putpix(x - px, y + py, color, pixels); /*   I. quadrant */
  aux_raster_putpix(x - py, y - px, color, pixels); /*  II. quadrant */
  aux_raster_putpix(x + px, y - py, color, pixels); /* III. quadrant */
  aux_raster_putpix(x + py, y + px, color, pixels); /*  IV. quadrant */
  r = err;
  if (r <= py) /* e_xy + e_y < 0 */
   err += ++py * 2 + 1;            
  if (r > px || err > py) /* e_xy + e_x > 0 or no 2nd y-step */
   err += ++px * 2 + 1; 
 }while(px < 0);

 return 0; 
}

int aux_raster_bezier_q(int              x0, 
                        int              y0, 
                        int              x1, 
                        int              y1,
                        int              x2,
                        int              y2,
                        uint32_t         color,
					    aux_raster_buf  *pixels)
{
 double a0, b0, a1, b1;
 double e = 0.01;
 double t = e;
 double x, y, px = x0, py = y0;

 for(; t < 1.0 + e; t += e){
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

int aux_raster_bezier_c(int              x0, 
                        int              y0, 
                        int              x1, 
                        int              y1,
                        int              x2,
                        int              y2,
                        int              x3,
                        int              y3,
                        uint32_t         color,
					    aux_raster_buf  *pixels)
{
 double a0, b0, a1, b1, a2, b2, c0, d0, c1, d1;
 double e = 0.01;
 double t = e;
 double x, y, px = x0, py = y0;

 for(; t < 1.0 + e; t += e){
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

/* */
int fill_rbuf(uint32_t color, aux_raster_buf  *pixels)
{
 assert((NULL != pixels) && "raster buffer struct pointer is NULL");
 assert((NULL != pixels->buf) && "raster buffer data pointer is NULL");
 assert((32 == pixels->scanline_pad) && "raster buffer padding is not 32 bit");

 uint32_t const * const e = (uint32_t*)pixels->buf + pixels->size / 4;

 for(uint32_t *p = (uint32_t*)pixels->buf; p < e; ++p){
  *p = color;
 }
 return 0;
}
