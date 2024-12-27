#ifndef AUX_XCB_H
#define AUX_XCB_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreserved-identifier"
#pragma clang diagnostic ignored "-Wdocumentation"

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xcb_errors.h>
#include <xcb/xcb_image.h>
#include <xcb/present.h>
#include <xcb/randr.h>

#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <X11/Xutil.h>

#pragma clang diagnostic pop

#ifdef __cplusplus
extern "C" {
#endif

#include "aux_xcb_keymap.h"
#include "aux_raster.h"

#define AUX_XCB_MAX_KEY_EVENTS 100

/* *********************************************************************************** */

typedef struct aux_xcb_ctx {
 void                    *x11_dpy;                 /* */
 xcb_connection_t        *conn;                    /* */
 const xcb_setup_t       *setup;                   /* */
 xcb_screen_t            *screen;                  /* */
 xcb_depth_t             *depth;                   /* */
 xcb_visualtype_t        *visual;                  /* */
 xcb_errors_context_t    *err_ctx;                 /* */
 xcb_key_symbols_t       *kb_syms;                 /* */

 xcb_colormap_t           colormap;                /* */
 xcb_window_t             window;                  /* */
 xcb_gcontext_t           gc;                      /* */
 xcb_pixmap_t             pixmap;                  /* */

 xcb_atom_t               atom_wm_p;               /* WM_PROTOCOLS */
 xcb_atom_t               atom_wm_p_dw;            /* WM_PROTOCOLS => WM_DELETE_WINDOW */
 xcb_atom_t               atom_wm_n_h;             /* XCB_ATOM_WM_NORMAL_HINTS */
 xcb_atom_t               atom_wm_n_h_sz;          /* XCB_ATOM_WM_NORMAL_HINTS => XCB_ATOM_WM_SIZE_HINTS */
 xcb_atom_t               atom_wm_mvsz;            /* ROOT WINDOW EVENT => _NET_MOVERESIZE_WINDOW */
 xcb_atom_t               atom_wm_wtyp;            /* WINDOW PROPERTY => _NET_WM_WINDOW_TYPE */
 xcb_atom_t               atom_wm_wtyp_desk;       /* WINDOW PROPERTY => _NET_WM_WINDOW_TYPE_DESKTOP */
 xcb_atom_t               atom_wm_wtyp_norm;       /* WINDOW PROPERTY => _NET_WM_WINDOW_TYPE_NORMAL */
 xcb_atom_t               atom_wm_worka_desk;      /* WINDOW PROPERTY => _NET_WORKAREA */
 xcb_atom_t               atom_wm_worka_win;       /* WINDOW PROPERTY => _WIN_WORKAREA */
 xcb_atom_t               atom_wm_frame_win;       /* WINDOW PROPERTY => _NET_FRAME_EXTENTS */
 xcb_atom_t               atom_wm_frame_win_msg;   /* WINDOW MESSAGE  => _NET_REQUEST_FRAME_EXTENTS */

 int                      screen_n;                /* */
 int                      fd;                      /* */

 uint32_t                 color_unit_red_mask; /* */
 uint32_t                 color_unit_gre_mask; /* */
 uint32_t                 color_unit_blu_mask; /* */

 int16_t                  win_x;  /* */
 int16_t                  win_y;  /* */
 uint16_t                 win_w;  /* */
 uint16_t                 win_h;  /* */
 int                      pixmap_format; /* */

 aux_raster_buf           img_raster_buf;          /* */

 xcb_present_event_t      x11ext_present_eid;

 bool                     x11ext_present;          /* XPresent */
 uint8_t                  x11ext_present_ev_base, x11ext_present_err_base;
 bool                     x11ext_randr;            /* XRandR   */
 uint8_t                  x11ext_randr_ev_base, x11ext_randr_err_base;
 bool                     f_eq_changed;            /* */

 bool                     has_input;     /* */

 volatile bool            f_window_should_close;   /* */
 volatile bool            f_window_expose;         /* */

 bool                     kb_keys[256];  /* */

 uint8_t                  pad[2]; /* */
}aux_xcb_ctx;

/* *********************************************************************************** */

/* aux_xcb_visual.c */
#define AUX_XCB_CONF_VISUAL_CLASS                       (1)
#define AUX_XCB_CONF_VISUAL_CLASS_DEFAULT               XCB_VISUAL_CLASS_TRUE_COLOR
#define AUX_XCB_VISUAL_CLASS_TRUE_COLOR	                XCB_VISUAL_CLASS_TRUE_COLOR

#define AUX_XCB_CONF_VISUAL_BITS_PER_CHANNEL            (2)
#define AUX_XCB_CONF_VISUAL_BITS_PER_CHANNEL_DEFAULT   -1

/* aux_img_buf.c */
#define AUX_XCB_CONF_FB                                 (3)
#define AUX_XCB_CONF_IMG_CONF_FLAG_MLC                  1 /* allocate buffer for pixel data                         */
#define AUX_XCB_CONF_IMG_CONF_FLAG_HDR                  2 /* stat image only, do not allocate buffer for pixel data */
#define AUX_XCB_CONF_IMG_CONF_FLAG_SHR                  3 /* allocate image buffer with a shared memory mmap        */
#define AUX_XCB_CONF_IMG_CONF_FLAG_MFD                  4 /* use a file descriptor backed by memory                 */

#define AUX_XCB_CONF_FB_WIDTH                           (4)

#define AUX_XCB_CONF_FB_HEIGHT                          (5)

#define AUX_XCB_CONF_FB_MFD_FD                          (6) /* fd to mmap */

#define AUX_XCB_CONF_WIN_DEPTH                          (7)
#define AUX_XCB_CONF_WIN_DEPTH_DEFAULT                  24 /* default visual depth */

#define AUX_XCB_CONF_WIN_WIDTH                          (8)

#define AUX_XCB_CONF_WIN_HEIGHT                         (9)


/* *********************************************************************************** */

#define AUX_XCB_PRINT_X11_ERROR(ctx, error) \
    do{\
       xcb_errors_context_t *e_ctx;     \
       const char           *extension; \
       const char           *e_nm;      \
       const char           *e_nm_maj;  \
       const char           *e_nm_min;  \
       uint8_t               e_code;    \
       uint8_t               e_maj;     \
       uint16_t              e_min;     \
                                        \
       e_ctx  = (ctx)->err_ctx;                       \
       e_code = (error)->error_code;                  \
       e_maj  = (error)->major_code;                  \
       e_min  = (error)->minor_code;                  \
                                                      \
       e_nm     = xcb_errors_get_name_for_error(e_ctx, e_code, &extension); \
       e_nm_maj = xcb_errors_get_name_for_major_code(e_ctx, e_maj);         \
       e_nm_min = xcb_errors_get_name_for_minor_code(e_ctx, e_maj, e_min);  \
                                                                        \
       fprintf(stderr, " * aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);       \
	   fprintf(stderr, " * \tname      : %s\n", e_nm?e_nm:"unknown");                 \
	   fprintf(stderr, " * \textension : %s\n", extension?extension:"no extension");  \
	   fprintf(stderr, " * \tmaj       : %s\n", e_nm_maj?e_nm_maj:"no major code");   \
	   fprintf(stderr, " * \tmin       : %s\n", e_nm_min?e_nm_min:"no minor code");   \
    }while(0);

/* *********************************************************************************** */

#define AUX_PROP_MODE_REPLACE  XCB_PROP_MODE_REPLACE
#define AUX_PROP_MODE_PREPEND  XCB_PROP_MODE_PREPEND
#define AUX_PROP_MODE_APPEND   XCB_PROP_MODE_APPEND

#define AUX_WM_PROP_DELETE_WINDOW  (1 << 1)
#define AUX_WM_PROP_GRAVITY        (1 << 2)
#define AUX_WM_PROP_WIN_TYP        (1 << 3)
#define AUX_WM_PROP_WORKAREA       (1 << 4)
#define AUX_WM_PROP_WIN_FRAME      (1 << 5)

/* *********************************************************************************** */

/* */
void  aux_zero_xcb_ctx(aux_xcb_ctx *ctx);

/* */
int   aux_xcb_connect(aux_xcb_ctx  *ctx,
                      const char   *conn_string,
                      int           screen);

/* */
int   aux_xcb_disconnect(aux_xcb_ctx *ctx);

/* */
int   aux_xcb_select_depth(aux_xcb_ctx  *ctx,
                           int           depth);

/* */
int   aux_xcb_select_visual(aux_xcb_ctx  *ctx,
                            int           config[]);

/* */
int   aux_xcb_free_colormap(aux_xcb_ctx *ctx);

/* */
int   aux_xcb_creat_colormap(aux_xcb_ctx *ctx);

/* */
int aux_xcb_free_pixmap(aux_xcb_ctx *ctx);

/* */
int aux_xcb_creat_pixmap(aux_xcb_ctx *ctx);

/* */
int aux_xcb_free_gc(aux_xcb_ctx *ctx);

/* */
int aux_xcb_creat_gc(aux_xcb_ctx *ctx);


/* */
int   aux_xcb_get_atom(aux_xcb_ctx  *ctx,
                       const char   *name,
                       xcb_atom_t   *atom);

/* */
int   aux_xcb_destroy_window(aux_xcb_ctx *ctx);

/* */
int   aux_xcb_creat_window(aux_xcb_ctx *ctx, uint16_t w, uint16_t h);

/* */
int   aux_xcb_map_window(aux_xcb_ctx *ctx);

/* */
int   aux_xcb_flush(aux_xcb_ctx *ctx);

/* */
int   aux_xcb_ev_func(aux_xcb_ctx *ctx);

/* */
int   aux_xcb_empty_events(aux_xcb_ctx *ctx);

/* */
int aux_xcb_change_prop(aux_xcb_ctx  *ctx,
                        uint8_t       mode,
                        uint32_t      flag,
                        const void   *data);

/* */
int aux_xcb_move_window(aux_xcb_ctx *ctx, int x, int y);

/* */
int aux_xcb_get_desk_worka(aux_xcb_ctx *ctx, uint32_t rc[4]);

/* */
int aux_xcb_get_prop(aux_xcb_ctx      *ctx,
                     uint32_t          flag,
                     const void       *arg,
                     unsigned long    *size,
                     void           **data);

/* */
int aux_xcb_creat_front_buf(aux_xcb_ctx  *ctx,
                            int           config[]);

/* */
int aux_xcb_destroy_front_buf(aux_xcb_ctx  *ctx);

/* */
int aux_xcb_flush_front_buf(aux_xcb_ctx *ctx);

/* */
int aux_xcb_get_extents_window(aux_xcb_ctx *ctx, uint32_t extents[4]); /* l,r,t,b */

/* */
int aux_xcb_aux_creat_win(aux_xcb_ctx  *xcb_ctx,
                          int           config[]
                         );

#ifdef __cplusplus
}
#endif

#endif
