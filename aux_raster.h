#ifndef AUX_RASTER_H
#define AUX_RASTER_H

#include <stdint.h>

#include "aux_raster_color_rgb.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AUX_RASTER_COLOR_UNIT32_MSB_FIRST  (1 << 1)
#define AUX_RASTER_COLOR_UNIT32_LSB_FIRST  (1 << 2)

#define AUX_RASTER_COLOR_UNIT32_ARGB (1 << 1)
#define AUX_RASTER_COLOR_UNIT32_XRGB (1 << 2)
#define AUX_RASTER_COLOR_UNIT32_BGRA (1 << 3)
#define AUX_RASTER_COLOR_UNIT32_BGRX (1 << 4)

typedef struct aux_raster_buf{
 uint16_t   w;            /* width in pixels, excluding pads                */
 uint16_t   h;            /* height in pixels                               */
 uint8_t    scanline_pad; /* right pad in bits                              */
 uint32_t   stride;       /* bytes per image row                            */
 uint8_t    bpp;          /* bits per pixel, >= depth                       */
 uint8_t    depth;        /* depth in bits                                  */
 int        xcb_img_fmt;  /* xcb_image_format_t                             */
 uint32_t   byte_order;   /* color component byte order                     */
 uint32_t   color_unit;   /* order of color components in color unit(pixel) */
 uint32_t   size;         /* size of image  data                            */
 int        shm_fd;       /* file descriptor of a shared memory             */
 void      *buf;          /* points to malloced or mmaped memory            */
}aux_raster_buf;

#define AUX__RASTER_INVERT_Y_AXIS 1

/* */
int aux_raster_putpix(int              x,
                      int              y, 
                      uint32_t         color, 
                      aux_raster_buf  *pixels);

/* */
int aux_raster_line(int              x0, 
                    int              y0, 
                    int              x1, 
                    int              y1, 
                    uint32_t         color, 
                    aux_raster_buf  *pixels);

/* */
int aux_raster_line_h(int              x,
                      int              y,
                      int              w,
                      uint32_t         color,
                      aux_raster_buf  *pixels);

/* */
int aux_raster_line_v(int              x,
                      int              y,
                      int              h,
                      uint32_t         color,
                      aux_raster_buf  *pixels);

/* */
int aux_raster_fill_rc(int              x0, int y0,
                       int              w,  int h,
                       uint32_t         color,
                       aux_raster_buf  *aux_buf);

/* */
int aux_raster_circle(int              x, 
                      int              y, 
                      int              r,
                      uint32_t         color,
					  aux_raster_buf  *pixels);

/* */
int aux_raster_ellipse_rc(int              x0, 
                          int              y0, 
                          int              x1, 
                          int              y1,
                          uint32_t         color,
					      aux_raster_buf  *pixels);

/* */
int aux_raster_bezier_q(int              x0, 
                        int              y0, 
                        int              x1, 
                        int              y1,
                        int              x2,
                        int              y2,
                        uint32_t         color,
					    aux_raster_buf  *pixels);

/* */
int aux_raster_bezier_c(int              x0, 
                        int              y0, 
                        int              x1, 
                        int              y1,
                        int              x2,
                        int              y2,
                        int              x3,
                        int              y3,
                        uint32_t         color,
					    aux_raster_buf  *pixels);

/* */
int fill_rbuf(uint32_t color, aux_raster_buf  *pixels);

#ifdef __cplusplus
}
#endif

#endif