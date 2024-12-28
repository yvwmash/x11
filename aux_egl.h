#ifndef AUX_EGL_H
#define AUX_EGL_H

#include <stdbool.h>

#define AUX_EGL_MAX_DEVICES  4
#define AUX_EGL_SZ_DEV_PATH  15

#define AUX_EGL_PRINT_ERROR   fprintf(stderr, " * aux-egl: %s:%s:%d\n", __FILE__, __func__, __LINE__);\
                              aux_egl_print_error();

#define AUX_EGL_CTX_CONFIG_X11_RED_MASK (1 << 1)
#define AUX_EGL_CTX_CONFIG_X11_GRE_MASK (1 << 2)
#define AUX_EGL_CTX_CONFIG_X11_BLU_MASK (1 << 3)
#define AUX_EGL_CTX_CONFIG_FB_W         (1 << 4)
#define AUX_EGL_CTX_CONFIG_FB_H         (1 << 5)
#define AUX_EGL_CTX_CONFIG_MEMORY_FB    (1 << 6)
#define AUX_EGL_CTX_CONFIG_COMPAT_FLG   (1 << 7)
#define AUX_EGL_CTX_CONFIG_MAJ_VER      (1 << 8)
#define AUX_EGL_CTX_CONFIG_MIN_VER      (1 << 9)
#define AUX_EGL_CTX_CONFIG_DBG          (1 << 10)

#define AUX_EGL_HOST_PIXEL_FORMAT_ARGB (1 << 1)
#define AUX_EGL_HOST_PIXEL_FORMAT_BGRA (1 << 2)

#define AUX_EGL_COMPUTE_UNI2UI (1UL << 1UL)

extern const int AUX_EGL_SSBO_MAP_R;
extern const int AUX_EGL_SSBO_MAP_W;
extern const int AUX_EGL_SSBO_MAP_P;
extern const int AUX_EGL_SSBO_MAP_C;

/* */
typedef struct aux_glsl_uniform {
 const char *p_nm;
 const void *p_v;
}aux_glsl_uniform;

typedef unsigned int(*aux_egl_uint_pf)();
typedef const char *(*aux_egl_cstr_pf)();
typedef void       *(*aux_egl_vptr_pf)();

typedef struct aux_egl_dev {
 void  *ph;
 char   path[AUX_EGL_SZ_DEV_PATH + 1];
}aux_egl_dev;

/* */
typedef struct aux_egl_ctx {
 aux_egl_dev  devices[AUX_EGL_MAX_DEVICES]; /* EGLDeviceEXT */
 void        *dpy;                          /* EGLDisplay                               */
 void        *rctx;                         /* EGLContext(OpenGL, OpenGL ES contexts)   */
 unsigned     n_devices;
 int          host_pixel_format;
 int          gl_pixel_format;
 int          gl_pixel_type;
 int          gl_compute_work_grp_cnt[3];
 int          gl_compute_work_grp_siz[3];
 int          gl_compute_work_grp_inv;
 uint8_t      pad[4];
}aux_egl_ctx;

/* ***************************************************************8 */

/* */
void  aux_egl_print_error(void);

/* */
int aux_egl_disconnect(aux_egl_ctx *ctx);

/* */
int aux_egl_connect(aux_egl_ctx  *ctx);

/* */
int aux_egl_creat_rctx(aux_egl_ctx *ctx, const char *drm_fn_path, int config[]);

/* EGL client extensions */
bool aux_egl_has_c_ext(const char *nm);

/* EGL device extensions */
bool aux_egl_has_de_ext(aux_egl_ctx *ctx, void *dev, const char *nm);

/* EGL display extensions */
bool aux_egl_has_di_ext(void *dpy, const char *nm);

/* EGL display extensions */
bool aux_egl_has_dext(aux_egl_ctx *ctx, const char *nm);

/* */
void  aux_zero_egl_ctx(aux_egl_ctx *ctx);

#endif
