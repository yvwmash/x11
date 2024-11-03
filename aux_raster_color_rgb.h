#ifndef AUX_RASTER_COLOR_RGB_H
#define AUX_RASTER_COLOR_RGB_H

#ifdef __cplusplus
extern "C" {
#endif

#define RGB_UI_3F(r,g,b) ((uint8_t)0 << 24)             |\
                         (((uint8_t)(r * 255.0)) << 16) |\
                         (((uint8_t)(g * 255.0)) << 8)  |\
                         (((uint8_t)(b * 255.0)) << 0)

#define ARGB_UI_4F(a,r,g,b) (((uint8_t)(r * 255.0)) << 24) |\
                            (((uint8_t)(r * 255.0)) << 16) |\
                            (((uint8_t)(g * 255.0)) << 8)  |\
                            (((uint8_t)(b * 255.0)) << 0)


#ifdef __cplusplus
}
#endif

#endif