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

typedef struct aux_raster_buf {
 void      *buf;          /* points to malloced or mmaped memory            */

 uint16_t   w;            /* width in pixels, excluding pads                */
 uint16_t   h;            /* height in pixels                               */
 uint32_t   stride;       /* bytes per image row                            */
 uint32_t   byte_order;   /* color component byte order                     */
 uint32_t   color_unit;   /* order of color components in color unit(pixel) */
 uint32_t   size;         /* size of image  data                            */
 int        shm_fd;       /* file descriptor of a shared memory             */
 uint8_t    scanline_pad; /* right pad in bits                              */
 uint8_t    bpp;          /* bits per pixel, >= depth                       */
 uint8_t    depth;        /* depth in bits                                  */
 uint8_t    xcb_img_fmt;  /* xcb_image_format_t                             */

 uint8_t    pad[4]; /* */
}aux_raster_buf;

#define AUX_RASTER_INVERT_Y_AXIS 1

/* */
int aux_raster_putpix(uint32_t         x,
                      uint32_t         y,
                      uint32_t         color,
                      aux_raster_buf  *pixels);

/* */
uint32_t aux_raster_getpix(uint32_t         x,
                           uint32_t         y,
                           aux_raster_buf  *pixels);

/* */
int aux_raster_line_h(uint32_t         x,
                      uint32_t         y,
                      uint32_t         w,
                      uint32_t         color,
                      aux_raster_buf  *pixels);

/* */
int aux_raster_line_v(uint32_t         x,
                      uint32_t         y,
                      uint32_t         h,
                      uint32_t         color,
                      aux_raster_buf  *pixels);

/* */
int aux_raster_fill_rc(uint32_t         x0, uint32_t y0,
                       uint32_t         w,  uint32_t h,
                       uint32_t         color,
                       aux_raster_buf  *aux_buf);

/* */
int aux_raster_circle(uint32_t         x,
                      uint32_t         y,
                      uint32_t         r,
                      uint32_t         color,
					  aux_raster_buf  *pixels);

/*
int aux_raster_bezier_q(uint32_t         x0,
                        uint32_t         y0,
                        uint32_t         x1,
                        uint32_t         y1,
                        uint32_t         x2,
                        uint32_t         y2,
                        uint32_t         color,
					    aux_raster_buf  *pixels);
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
					    aux_raster_buf  *pixels);
*/

/* */
int fill_rbuf(uint32_t color, aux_raster_buf  *pixels);

#ifdef __cplusplus
}
#endif

#endif
