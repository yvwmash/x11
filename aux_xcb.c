#include <assert.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "aux_xcb.h"
#include "aux_xcb_keymap.h"

/* *********************************************************************************** */

static const char *randr_rotation_strings[] = {
    [1] = "XCB_RANDR_ROTATION_0",
    [2] = "XCB_RANDR_ROTATION_90",
    [4] = "XCB_RANDR_ROTATION_180",
    [8] = "XCB_RANDR_ROTATION_270",
    [16] = "XCB_RANDR_ROTATION_REFLECT_X",
    [32] = "XCB_RANDR_ROTATION_REFLECT_Y",
};

static const char *randr_connection_strings[] = {
    "XCB_RANDR_CONNECTION_CONNECTED",
    "XCB_RANDR_CONNECTION_DISCONNECTED",
    "XCB_RANDR_CONNECTION_UNKOWN",
};

static const char *propstatus_strings[] = {
 "NEW", "DELETE",
};

/* *********************************************************************************** */

/* */
int aux_xcb_destroy_window(aux_xcb_ctx *ctx)
{
 xcb_void_cookie_t    cookie;
 xcb_generic_error_t *error;
 xcb_connection_t    *c = ctx->conn;
 int                  status = 0;

 status  = aux_xcb_free_colormap    (ctx);
 status |= aux_xcb_free_pixmap      (ctx);
 status |= aux_xcb_destroy_front_buf(ctx);

 if(((uint32_t)ctx->window) != (uint32_t)0){
  cookie      = xcb_destroy_window_checked(c, ctx->window);
  ctx->window = (uint32_t)0;
  ctx->win_x         = -1;
  ctx->win_y         = -1;
  ctx->win_w         = -1;
  ctx->win_h         = -1;
  ctx->f_window_should_close = false; /* reset window flags */
  ctx->f_window_expose       = false; /* reset window flags */
  ctx->pixmap_format         = -1;    /* in case XCB pixmap formats start from zero */
  error       = xcb_request_check(ctx->conn, cookie);
  if(error){
   AUX_XCB_PRINT_X11_ERROR(ctx, error)
   free(error);
   return -1;
  }
 }

 return status;
}

/* */
int aux_xcb_creat_window(aux_xcb_ctx *ctx, int w, int h)
{
 xcb_void_cookie_t    cookie;
 xcb_generic_error_t *error;
 xcb_connection_t    *c = ctx->conn;
 xcb_window_t         window;
 int                  status = 0;

 /* free window */
 status = aux_xcb_destroy_window(ctx);

 /* colormap */
 if(aux_xcb_creat_colormap(ctx) < 0){
  fprintf(stderr, " * aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
  return -1;
 }

 /* actual window creation */
 {
  unsigned int         cw_mask     = XCB_CW_BORDER_PIXEL
                              |      XCB_CW_EVENT_MASK
                             |       XCB_CW_COLORMAP
  ;
  /* values must follow in the incresing order of the cw_mask constants */
  unsigned int         cw_values[] = {ctx->screen->white_pixel,
                                      XCB_EVENT_MASK_EXPOSURE         | XCB_EVENT_MASK_BUTTON_PRESS    |
                                      XCB_EVENT_MASK_BUTTON_RELEASE   | XCB_EVENT_MASK_POINTER_MOTION  |
                                      XCB_EVENT_MASK_ENTER_WINDOW     | XCB_EVENT_MASK_LEAVE_WINDOW    |
                                      XCB_EVENT_MASK_KEY_PRESS        | XCB_EVENT_MASK_KEY_RELEASE     |
							          XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_PROPERTY_CHANGE |
							          XCB_EVENT_MASK_FOCUS_CHANGE,
                                      ctx->colormap
  };

  window = xcb_generate_id(c);
  cookie = xcb_create_window_checked(c,
                                     ctx->depth->depth,
                                     window,
									 ctx->screen->root,
                                     0, 0,
                                     w, h,
                                     1,
									 XCB_WINDOW_CLASS_INPUT_OUTPUT,
                                     ctx->visual->visual_id,
                                     cw_mask,
                                     cw_values
								    );

  error = xcb_request_check(c, cookie);
  if (error) {
   AUX_XCB_PRINT_X11_ERROR(ctx, error)
   free(error);
   return -1;
  }
 } /* done create window */

 ctx->window = window;
 ctx->win_w  = w;
 ctx->win_h  = h;

 /* register for XPresent events */
 if(ctx->x11ext_present) {
  xcb_void_cookie_t    cookie;
  xcb_generic_error_t *error;
  uint32_t             mask = XCB_PRESENT_EVENT_MASK_COMPLETE_NOTIFY
                              | XCB_PRESENT_EVENT_MASK_IDLE_NOTIFY;

  ctx->x11ext_present_eid = xcb_generate_id(c);
  cookie = xcb_present_select_input_checked(c, window, ctx->x11ext_present_eid, mask);
  error  = xcb_request_check(c, cookie);
  if (error) {
   fprintf(stderr, " * aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
   AUX_XCB_PRINT_X11_ERROR(ctx, error)
   ctx->x11ext_present      = false;
   ctx->x11ext_present_eid  = (xcb_present_event_t)-1;
   ctx->x11ext_present_ev_base  = 0;
   ctx->x11ext_present_err_base = 0;
  }
  free(error);
 }

 /* register for XRandR events */
 if(ctx->x11ext_randr) {
  xcb_void_cookie_t    cookie;
  xcb_generic_error_t *error;
  uint32_t             mask =   XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE
                              | XCB_RANDR_NOTIFY_MASK_OUTPUT_CHANGE
                              | XCB_RANDR_NOTIFY_MASK_CRTC_CHANGE
                              | XCB_RANDR_NOTIFY_MASK_OUTPUT_PROPERTY
                              | XCB_RANDR_NOTIFY_MASK_PROVIDER_CHANGE
                              | XCB_RANDR_NOTIFY_MASK_PROVIDER_PROPERTY
                              | XCB_RANDR_NOTIFY_MASK_RESOURCE_CHANGE
                              ;
  cookie = xcb_randr_select_input_checked(c, window, mask);
  error  = xcb_request_check(c, cookie);
  if (error) {
   fprintf(stderr, " * aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
   AUX_XCB_PRINT_X11_ERROR(ctx, error)
   ctx->x11ext_randr      = false;
  }
  free(error);
 }

 return status;
}

/* */
int aux_xcb_map_window(aux_xcb_ctx *ctx)
{
 xcb_map_window(ctx->conn, ctx->window);
 return 0;
}

/* */
int aux_xcb_empty_events(aux_xcb_ctx *ctx)
{
 xcb_generic_event_t  *ev;

 do{
  ev = xcb_poll_for_event(ctx->conn);
  if (ev && (ev->response_type == 0)){
   fprintf(stderr, " * aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
   AUX_XCB_PRINT_X11_ERROR(ctx, (xcb_generic_error_t*)ev)
   free(ev);
   return -1;
  }
  if(ev)
   free(ev);
 }while(NULL != ev);

 return 0;
}

static void on_ev_randr_notify(xcb_connection_t *c,xcb_generic_event_t *event) {
 xcb_randr_notify_event_t *ev   = (xcb_randr_notify_event_t *)event;
 uint8_t                   code = ev->subCode;

 (void)c;
 (void)randr_connection_strings;
 (void)propstatus_strings;

 printf(" ! xcb-aux: RandR: XCB_RANDR_NOTIFY\n");

 switch(code) {
 case XCB_RANDR_NOTIFY_CRTC_CHANGE: {
  xcb_randr_crtc_change_t *e = &ev->u.cc;
  (void)e;
  printf(" ! xcb-aux: RandR: \tXCB_RANDR_NOTIFY_CRTC_CHANGE\n");
  printf("\t\t\t\tposition of the CRTC on the screen, rotation, scaling, resolution,\n\t\t\t\trefresh rate, orientation, outputs (monitors) changed, disable/enable\n");
  break;
 }
 case XCB_RANDR_NOTIFY_OUTPUT_CHANGE: {
  xcb_randr_output_change_t *e = &ev->u.oc;
  (void)e;
  printf(" ! xcb-aux: RandR: \tXCB_RANDR_NOTIFY_OUTPUT_CHANGE\n");
  printf("\t\t\t\tconnector state, resolution, refresh rate,\n\t\t\t\torientation, brightness, color settings\n");
  break;
 }
 case XCB_RANDR_NOTIFY_OUTPUT_PROPERTY: {
  xcb_randr_output_property_t   *e = &ev->u.op;
  (void)e;
  printf(" ! xcb-aux: RandR: XCB_RANDR_NOTIFY_OUTPUT_PROPERTY\n");
  break;
 }
 case XCB_RANDR_NOTIFY_PROVIDER_CHANGE: {
  xcb_randr_provider_change_t *e = &ev->u.pc;
  (void)e;
  printf(" ! xcb-aux: RandR: XCB_RANDR_NOTIFY_PROVIDER_CHANGE\n");
  printf("\t\t\t\tGPU state, capabilities, linked CRTCs/outputs\n");
  break;
 }
 case XCB_RANDR_NOTIFY_PROVIDER_PROPERTY: {
  xcb_randr_provider_property_t *e = &ev->u.pp;
  (void)e;
  printf(" ! xcb-aux: RandR: XCB_RANDR_NOTIFY_PROVIDER_PROPERTY\n");
  break;
 }
 case XCB_RANDR_NOTIFY_RESOURCE_CHANGE: {
  xcb_randr_resource_change_t *e = &ev->u.rc;
  (void)e;
  printf(" ! xcb-aux: RandR: XCB_RANDR_NOTIFY_RESOURCE_CHANGE\n");
  printf("\t\t\t\ta new/delete/remove of crtc, output, or mode, something reconfigured\n");
  break;
 }
 }
}

/* similar to X11 screen. represents single FB for compositing WM. */
static void on_ev_randr_screen_change(xcb_generic_event_t *event) {
 xcb_randr_screen_change_notify_event_t *ev = (xcb_randr_screen_change_notify_event_t *)event;

 printf(" ! xcb-aux: RandR: XCB_RANDR_SCREEN_CHANGE_NOTIFY\n");
 printf(" ! xcb-aux: RandR: \troot 0x%x, timestamp %u, config_timestamp %u\n", ev->root, ev->timestamp, ev->config_timestamp);
 printf(" ! xcb-aux: RandR: \trotation %s\n", randr_rotation_strings[ev->rotation]);
 printf(" ! xcb-aux: RandR: \twidth %d, height %d, mwidth %d, mheight %d\n", ev->width, ev->height, ev->mwidth, ev->mheight);
}

/* */
int aux_xcb_ev_func(aux_xcb_ctx *ctx)
{
 xcb_generic_event_t   *event;
 uint32_t               kb_key;
 uint16_t               kb_key_state;

 ctx->has_input = false;

 event = xcb_poll_for_event(ctx->conn);

 if(NULL == event)
  return 0;

 ctx->f_window_expose = false;
 ctx->f_eq_changed    = false;

 while(event){
  unsigned etyp = event->response_type & 0x7f;

  if (event && (etyp == 0)){ /* error recieved */
   AUX_XCB_PRINT_X11_ERROR(ctx, (xcb_generic_error_t*)event)
   free(event);
   return -1;
  }

  /* extension events */
  if(ctx->x11ext_randr) {
   if(etyp == ((unsigned)ctx->x11ext_randr_ev_base + XCB_RANDR_NOTIFY)) { /* XCB_RANDR_NOTIFY */
    on_ev_randr_notify(ctx->conn, event);
    ctx->f_eq_changed = true;
    goto l_next_event;
   } else if(etyp == ((unsigned)ctx->x11ext_randr_ev_base + XCB_RANDR_SCREEN_CHANGE_NOTIFY)) { /* XCB_RANDR_SCREEN_CHANGE_NOTIFY */
    on_ev_randr_screen_change(event);
    ctx->f_eq_changed = true;
    goto l_next_event;
   }
  }

  switch (etyp) {

  /* window graphics content needs update */
  case XCB_EXPOSE: {
   xcb_expose_event_t *expose = (xcb_expose_event_t *)event;
   if(expose->count == 0){
    ctx->f_window_expose = true;
   }
  } /* end case */
  break;

  /* mouse button press */
  case XCB_BUTTON_PRESS: {
   xcb_button_press_event_t *bp = (xcb_button_press_event_t *)event;
   switch (bp->detail) {
   case 4:
	break;
   case 5:
	break;
   default:
   break;
   }
  } /* end case */
  break;

  /* mouse button release */
  case XCB_BUTTON_RELEASE: {
   xcb_button_release_event_t *ev = (xcb_button_release_event_t *)event;
   (void)ev;
  } /* end case */
  break;

  /* mouse moved in window */
  case XCB_MOTION_NOTIFY: {
   xcb_motion_notify_event_t *motion = (xcb_motion_notify_event_t *)event;
   (void)motion;
  } /* end case */
  break;

  /* mouse entered window */
  case XCB_ENTER_NOTIFY: {
   xcb_enter_notify_event_t *enter = (xcb_enter_notify_event_t *)event;
   (void)enter;
  } /* end case */
  break;

  /* mouse left window */
  case XCB_LEAVE_NOTIFY: {
   xcb_leave_notify_event_t *leave = (xcb_leave_notify_event_t *)event;
   (void)leave;
  } /* end case */
  break;

  /* keyboard key press */
  case XCB_KEY_PRESS: {
   xcb_key_press_event_t *ke = (xcb_key_press_event_t *)event;

   ctx->has_input = true;

   aux_xcb_map_keycode(ctx->kb_syms, ke->detail, ke->state, &kb_key, &kb_key_state);
   ctx->kb_keys[kb_key] = true;
  } /* end case */
  break;

  /* keyboard key release */
  case XCB_KEY_RELEASE: {
  } /* end case */
  break;

  /* WM and other messages */
  case XCB_CLIENT_MESSAGE: {
   xcb_client_message_event_t *ce = (xcb_client_message_event_t*)event;
   if(ce->data.data32[0] == ctx->atom_wm_p_dw){ /* window should close */
	printf(" !\tmessage: close window\n");
    ctx->f_window_should_close = true;
   }
  } /* end case */
  break;

  /* window is mapped or window size, position, border, stacking order have changed */
  case XCB_CONFIGURE_NOTIFY: {
   xcb_configure_notify_event_t *ce = (xcb_configure_notify_event_t*)event;
   ctx->win_x = ce->x;
   ctx->win_y = ce->y;
   ctx->win_w = ce->width;
   ctx->win_h = ce->height;
  } /* end case */
  break;

  /* window is being destroyed */
  case XCB_DESTROY_NOTIFY: {
   xcb_destroy_notify_event_t *de = (xcb_destroy_notify_event_t*)event;
   (void)de;
  } /* end case */
  break;

  /* */
  case XCB_PROPERTY_NOTIFY: {
   xcb_property_notify_event_t *pe = (xcb_property_notify_event_t*)event;
   (void)pe;
  } /* end case */
  break;

  /* */
  case XCB_GE_GENERIC:
   break;

  default: {
   /* unknown event type, ignore it */
  } /* end default case */
  break;
  } /* end event switch */

l_next_event:
  free(event);
  event = xcb_poll_for_event(ctx->conn);
 } /* end while loop */

 return 1;
}

/* */
void aux_zero_xcb_ctx(aux_xcb_ctx *ctx)
{
 memset(ctx, 0, sizeof(aux_xcb_ctx));
 ctx->fd            = -1;  /* assign (-1) to X11 connection fd to not confuse it with stdin */
 ctx->screen_n      = -1;  /* X11 screens start from zero, so assing (-1)                   */
 ctx->pixmap_format = -1;  /* in case xcb pixmaps start from zero                           */
 ctx->win_x         = -1;
 ctx->win_y         = -1;
 ctx->win_w         = -1;
 ctx->win_h         = -1;
 ctx->img_raster_buf.shm_fd = -1;

 /* default out XPresent extension */
 ctx->x11ext_present      = false;
 ctx->x11ext_present_eid  = (xcb_present_event_t)-1;

 /* default out XRandR extension */
 ctx->x11ext_randr        = false;
}

/* */
int aux_xcb_connect(aux_xcb_ctx  *ctx,
                    const char   *conn_string,
                    int           screen)
{
 void                 *x11_dpy;
 xcb_connection_t     *c;
 xcb_key_symbols_t    *xcb_keysyms;
 xcb_errors_context_t *xcb_err_ctx;
 xcb_atom_t            a_wm_p;
 xcb_atom_t            a_wm_p_dw;
 xcb_atom_t            a_wm_nh;
 xcb_atom_t            a_wm_nh_sz;
 xcb_atom_t            a_wm_mvsz;
 xcb_atom_t            a_wm_wtyp;
 xcb_atom_t            a_wm_wtyp_desk;
 xcb_atom_t            a_wm_wtyp_norm;
 xcb_atom_t            a_wm_worka_desk;
 xcb_atom_t            a_wm_worka_win;
 xcb_atom_t            a_wm_frame_win;
 xcb_atom_t            a_wm_frame_win_msg;

 aux_xcb_disconnect(ctx);

 /* connect X11 way */
 x11_dpy = XOpenDisplay(conn_string);
 if(NULL == x11_dpy) {
  fprintf(stderr, " * aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
  return -1;
 }

 /* Get the XCB connection from the display */
 c = XGetXCBConnection(x11_dpy);
 if(NULL == c) {
  XCloseDisplay(x11_dpy);
  fprintf(stderr, " * aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
  return -1;
 }

 /* XCB owns the event queue */
 XSetEventQueueOwner(x11_dpy, XCBOwnsEventQueue);

 /* save X11 display */
 ctx->x11_dpy = x11_dpy;

 /* xcb error context. utility library. */
 if(xcb_errors_context_new(c, &xcb_err_ctx) < 0){
  fprintf(stderr, " * aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
  return -1;
 }

 /* keysymbols */
 if(NULL == (xcb_keysyms = xcb_key_symbols_alloc(c))){
  fprintf(stderr, " * aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
  return -1;
 }

 ctx->fd       = xcb_get_file_descriptor(c); /* X11 connection fd */
 ctx->conn     = c;                          /* XCB connection */
 ctx->screen_n = screen;                     /* integral screen number */
 ctx->setup    = xcb_get_setup(c);
 ctx->screen   = xcb_setup_roots_iterator(ctx->setup).data;
 ctx->err_ctx  = xcb_err_ctx; /* utility lib, error print. */
 ctx->kb_syms  = xcb_keysyms; /* alloc'ed keysymbols       */

 /* atom */
 if(aux_xcb_get_atom(ctx, "WM_PROTOCOLS", &a_wm_p) < 0){
  fprintf(stderr, " * aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
  return -1;
 }
 /* atom */
 if(aux_xcb_get_atom(ctx, "WM_DELETE_WINDOW", &a_wm_p_dw) < 0){
  fprintf(stderr, " * aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
  return -1;
 }
 /* atom */
 if(aux_xcb_get_atom(ctx, "WM_NORMAL_HINTS", &a_wm_nh) < 0){
  fprintf(stderr, " * aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
  return -1;
 }
 /* atom */
 if(aux_xcb_get_atom(ctx, "WM_SIZE_HINTS", &a_wm_nh_sz) < 0){
  fprintf(stderr, " * aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
  return -1;
 }
 /* atom */
 if(aux_xcb_get_atom(ctx, "_NET_MOVERESIZE_WINDOW", &a_wm_mvsz) < 0){
  fprintf(stderr, " * aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
  return -1;
 }
 /* atom */
 if(aux_xcb_get_atom(ctx, "_NET_WM_WINDOW_TYPE", &a_wm_wtyp) < 0){
  fprintf(stderr, " * aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
  return -1;
 }
 /* atom */
 if(aux_xcb_get_atom(ctx, "_NET_WM_WINDOW_TYPE_DESKTOP", &a_wm_wtyp_desk) < 0){
  fprintf(stderr, " * aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
  return -1;
 }
 /* atom */
 if(aux_xcb_get_atom(ctx, "_NET_WM_WINDOW_TYPE_NORMAL", &a_wm_wtyp_norm) < 0){
  fprintf(stderr, " * aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
  return -1;
 }
 /* atom */
 if(aux_xcb_get_atom(ctx, "_NET_WORKAREA", &a_wm_worka_desk) < 0){
  fprintf(stderr, " * aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
  return -1;
 }
 /* atom */
 if(aux_xcb_get_atom(ctx, "_WIN_WORKAREA", &a_wm_worka_win) < 0){
  fprintf(stderr, " * aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
  return -1;
 }
 /* atom */
 if(aux_xcb_get_atom(ctx, "_NET_FRAME_EXTENTS", &a_wm_frame_win) < 0){
  fprintf(stderr, " * aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
  return -1;
 }
 /* atom */
 if(aux_xcb_get_atom(ctx, "_NET_REQUEST_FRAME_EXTENTS", &a_wm_frame_win_msg) < 0){
  fprintf(stderr, " * aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
  return -1;
 }

 ctx->atom_wm_p              = a_wm_p;
 ctx->atom_wm_p_dw           = a_wm_p_dw;
 ctx->atom_wm_n_h            = a_wm_nh;
 ctx->atom_wm_n_h_sz         = a_wm_nh_sz;
 ctx->atom_wm_mvsz           = a_wm_mvsz;
 ctx->atom_wm_wtyp           = a_wm_wtyp;
 ctx->atom_wm_wtyp_desk      = a_wm_wtyp_desk;
 ctx->atom_wm_wtyp_norm      = a_wm_wtyp_norm;
 ctx->atom_wm_worka_desk     = a_wm_worka_desk;
 ctx->atom_wm_worka_win      = a_wm_worka_win;
 ctx->atom_wm_frame_win      = a_wm_frame_win;
 ctx->atom_wm_frame_win_msg  = a_wm_frame_win_msg;

 putchar('\n');
 printf(" ! aux-xcb: X11 extensions\n");

 /* query for XPresent extension */
 {
  const xcb_query_extension_reply_t *rep;
  xcb_present_query_version_reply_t *present_version;

  present_version = xcb_present_query_version_reply(c, xcb_present_query_version(c, 1, 2), NULL);
  if (!present_version) {
   fprintf(stderr, "Failed to query Present version\n");
  }else {
   printf(" ! aux-xcb: Present version: %u.%u\n", present_version->major_version, present_version->minor_version);
   free(present_version);
  }

  rep = xcb_get_extension_data(c, &xcb_present_id); /* xcb_present_id is extern variable in lib-xcb */
  if (!rep || !rep->present) {
   fprintf(stderr, " ! aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
   fprintf(stderr, " ! aux-xcb: no X11 XPresent extension.\n");
  } else {
   fprintf(stderr, " ! aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
   fprintf(stderr, " ! aux-xcb: \tX11 XPresent extension AVAILABLE.\n");
   ctx->x11ext_present          = true;
   ctx->x11ext_present_ev_base  = rep->first_event;
   ctx->x11ext_present_err_base = rep->first_error;
   printf(" ! aux-xcb: XPresent: event base: %u, error base: %u\n", rep->first_event, rep->first_error);
  }
 }

 /* query for XRandR extension */
 {
  const xcb_query_extension_reply_t  *rep;
  xcb_randr_query_version_reply_t    *randr_version;

  randr_version = xcb_randr_query_version_reply(c, xcb_randr_query_version(c, 1, 5), NULL);
  if (!randr_version) {
   fprintf(stderr, "Failed to query RandR version\n");
  }else {
   printf(" ! aux-xcb: RandR version: %u.%u\n", randr_version->major_version, randr_version->minor_version);
   free(randr_version);
  }

  rep = xcb_get_extension_data(c, &xcb_randr_id); /* xcb_randr_id is extern variable in lib-xcb */
  if (!rep || !rep->present) {
   fprintf(stderr, " ! aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
   fprintf(stderr, " ! aux-xcb: no X11 XRandR extension.\n");
  } else {
   fprintf(stderr, " ! aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
   fprintf(stderr, " ! aux-xcb: \tX11 XRandR extension AVAILABLE.\n");
   ctx->x11ext_randr = true;
   ctx->x11ext_randr_ev_base  = rep->first_event;
   ctx->x11ext_randr_err_base = rep->first_error;
   printf(" ! aux-xcb: XRandR: event base: %u, error base: %u\n", rep->first_event, rep->first_error);
  }
 }
 putchar('\n');

 return 0;
}

/* */
int aux_xcb_disconnect(aux_xcb_ctx *ctx)
{
 printf(" ! aux-xcb: disconnect\n");

 /* free xcb error context */
 if(ctx->err_ctx){
  xcb_errors_context_free(ctx->err_ctx);
 }

 /* free allocated keysyms */
 if(ctx->kb_syms){
  xcb_key_symbols_free(ctx->kb_syms);
 }

 /* destroy window, if any */
 aux_xcb_destroy_window(ctx);

 /* free X11 connection
  * SO: Do I need to disconnect an xcb_connection_t that I got from XGetXCBConnection?
  * 	https://stackoverflow.com/questions/11059756/do-i-need-to-disconnect-an-xcb-connection-t-that-i-got-from-xgetxcbconnection
  *		XCloseDisplay explicitly calls xcb_disconnect
  *     	https://cgit.freedesktop.org/xorg/lib/libX11/tree/src/ClDisplay.c
 */
 if(ctx->x11_dpy) {
  XCloseDisplay(ctx->x11_dpy);
 }

 /* zero out context */
 aux_zero_xcb_ctx(ctx);

 return 0;
}

/* */
int aux_xcb_flush(aux_xcb_ctx *ctx)
{
 xcb_flush(ctx->conn);
 return 0;
}

/* */
int aux_xcb_select_depth(aux_xcb_ctx *ctx, int depth)
{
 xcb_depth_iterator_t i = xcb_screen_allowed_depths_iterator(ctx->screen);
 ctx->depth = NULL;

 while (i.rem) {
  if(i.data->depth == depth && i.data->visuals_len) {
   ctx->depth = i.data;
   return 0;
  }
  xcb_depth_next(&i);
 }

 return -1;
}

/* */
int aux_xcb_select_visual(aux_xcb_ctx *ctx, int config[])
{
 xcb_visualtype_iterator_t i = xcb_depth_visuals_iterator(ctx->depth);
 bool      has_class_f = false, has_bp_channel_f = false;
 int       class_v     = 0, bp_channel_v     = 0;
 uint32_t  r_mask, g_mask, b_mask;
 uint8_t   depth = ctx->depth->depth;

 for(int *ci = config; *ci; ci += 2){
  switch(*ci){
  case AUX_XCB_CONF_VISUAL_CLASS:
   has_class_f = true;
   class_v     = ci[1];
   break;
  case AUX_XCB_CONF_VISUAL_BITS_PER_CHANNEL:
   bp_channel_v = ci[1];
   if(AUX_XCB_CONF_VISUAL_BITS_PER_CHANNEL_DEFAULT != bp_channel_v)
    has_bp_channel_f = true;
   break;
  default:
   break;
  }
 }

 ctx->visual = NULL;

 if(!has_class_f){
  class_v = XCB_VISUAL_CLASS_TRUE_COLOR;
 }

 while(i.rem){
  if(has_class_f && (i.data->_class != class_v)){
   goto xcb_vis_i_next;
  }
  if(has_bp_channel_f && (i.data->bits_per_rgb_value != bp_channel_v)){
   goto xcb_vis_i_next;
  }
  ctx->visual = i.data;
  r_mask = i.data->red_mask;
  g_mask = i.data->green_mask;
  b_mask = i.data->blue_mask;
  printf(" ! xcb visual:\n"
         " ! \t id                : %d\n"
         " ! \t depth             : %d\n"
         " ! \t class             : %d\n"
         " ! \t bits per channel  : %d\n"
         " ! \t red mask          : %.8x\n"
         " ! \t green mask        : %.8x\n"
         " ! \t blue mask         : %.8x\n",
         i.data->visual_id,
         depth,
         i.data->_class,
         i.data->bits_per_rgb_value,
         r_mask,
         g_mask,
         b_mask);
  {
   const char *s_r = "RR";
   const char *s_g = "GG";
   const char *s_b = "BB";
   char        cbuf[12] = {0};
   int         zero_msb_red_bits = __builtin_clz(r_mask);
   int         zero_msb_gre_bits = __builtin_clz(g_mask);
   int         zero_msb_blu_bits = __builtin_clz(b_mask);
   if(depth == 32){
    strcpy(cbuf, "AAAAAAAA");
   }else if(depth == 24){
    strcpy(cbuf, "XXXXXXXX");
   }
   memcpy(cbuf + (zero_msb_red_bits / 4), s_r, 2);
   memcpy(cbuf + (zero_msb_gre_bits / 4), s_g, 2);
   memcpy(cbuf + (zero_msb_blu_bits / 4), s_b, 2);
   printf(" ! \t \"C\" hex notation  : %s\n", cbuf);
   putchar('\n');
  }
  ctx->color_unit_red_mask = i.data->red_mask;
  ctx->color_unit_gre_mask = i.data->green_mask;
  ctx->color_unit_blu_mask = i.data->blue_mask;

  return 0;
xcb_vis_i_next:
  xcb_visualtype_next(&i);
 }

 return -1;
}

#define WM_EWMH_CLIENT_SOURCE_TYPE_NONE      0  /** No source at all (for clients supporting an older version of
                                                    EWMH specification)
											    */
#define WM_EWMH_CLIENT_SOURCE_TYPE_NORMAL    1  /** Normal application */
#define WM_EWMH_CLIENT_SOURCE_TYPE_OTHER     2  /** Pagers and other clients that represent direct user actions */

#define WM_EWMH_FLAG_MOVERESIZE_HAS_X     (1 << 8)
#define WM_EWMH_FLAG_MOVERESIZE_HAS_Y     (1 << 9)
#define WM_EWMH_FLAG_MOVERESIZE_HAS_W     (1 << 10)
#define WM_EWMH_FLAG_MOVERESIZE_HAS_H     (1 << 11)

static int xcb_send_client_wm_ev(aux_xcb_ctx     *ctx,
                                 xcb_window_t     dst_win,
                                 xcb_atom_t       atom,
                                 uint32_t         data_len,
                                 const uint32_t  *data
						        )
{
 xcb_client_message_event_t  ev;
 xcb_void_cookie_t           cookie;
 xcb_generic_error_t        *error;
 xcb_connection_t           *c      = ctx->conn;
 xcb_window_t                window = ctx->window;
 int                         mask   = XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
                                    | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
                                    | XCB_EVENT_MASK_STRUCTURE_NOTIFY
                                    | XCB_EVENT_MASK_PROPERTY_CHANGE
 ;

 assert(data_len <= (5 * sizeof(uint32_t)));

 memset(&ev, 0, sizeof(xcb_client_message_event_t));

 ev.response_type = XCB_CLIENT_MESSAGE;
 ev.window = window;
 ev.format = 32;
 ev.type = atom;

 memcpy(ev.data.data32, data, data_len);

 cookie = xcb_send_event(c,               /* xcb connection */
                         0,               /* propagate */
                         dst_win,         /* destination */
                         mask,            /* who recieves the event, mask */
                         (const char*)&ev /* event */
						);
 error = xcb_request_check(c, cookie);
 if (error) {
  AUX_XCB_PRINT_X11_ERROR(ctx, error)
  free(error);
  return -1;
 }

 return 0;
}

static int xcb_request_moveresize_window(aux_xcb_ctx   *ctx,
                                         xcb_gravity_t  gravity,
                                         int            source_indication_flag,
                                         int            flags,
										 uint32_t       x,
                                         uint32_t       y,
                                         uint32_t       w,
                                         uint32_t       h
                                        )
{
 const uint32_t data[] = {gravity | flags | source_indication_flag << 12,
                          x, y, w, h
 };
 return xcb_send_client_wm_ev(ctx,                /* context */
                              ctx->screen->root,  /* destination */
                              ctx->atom_wm_mvsz,  /* atom */
                              sizeof(data),       /* data len */
                              data                /* data */
							 );
}

/* */
int aux_xcb_move_window(aux_xcb_ctx *ctx, int x, int y)
{
 return xcb_request_moveresize_window(ctx,                                /* context */
                                      0,                                  /* gravity */
                                      WM_EWMH_CLIENT_SOURCE_TYPE_NORMAL,  /* source_indication */
                                      WM_EWMH_FLAG_MOVERESIZE_HAS_X       /* flags */
                                      | WM_EWMH_FLAG_MOVERESIZE_HAS_Y,    /* flags */
                                      x, y,                               /* x,y position */
                                      -1, -1                              /* w,h */
                                     );
}

/* */
int aux_xcb_get_extents_window(aux_xcb_ctx *ctx, uint32_t extents[4]) /* l,r,t,b */
{
 int                   status = 0;
 uint32_t             *rc     = NULL;
 xcb_generic_event_t  *ev     = NULL;

 status = xcb_send_client_wm_ev(ctx,                         /* context */
                                ctx->screen->root,           /* destination */
                                ctx->atom_wm_frame_win_msg,  /* atom */
                                0,                           /* data len */
                                NULL                         /* data */
	   				           );
 if(status < 0)
  return status;

 while(1){
  ev = xcb_wait_for_event(ctx->conn);
  if(NULL == ev){ /* I/O error */
   fprintf(stderr, " * aux-xcb: I/O error: %s:%s:%d\n", __FILE__, __func__, __LINE__);
   status = -1;
   goto done_get_extents;
  }
  if (ev && (ev->response_type == 0)){ /* error */
   fprintf(stderr, " * aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
   AUX_XCB_PRINT_X11_ERROR(ctx, (xcb_generic_error_t*)ev)
   status = -1;
   goto done_get_extents;
  }
  if((ev->response_type & ~0x80) == XCB_PROPERTY_NOTIFY){ /* property changed */
   xcb_property_notify_event_t *pe = (xcb_property_notify_event_t*)ev;
   if(pe->atom == ctx->atom_wm_frame_win){ /* window frame atom */
	if(aux_xcb_get_prop(ctx, AUX_WM_PROP_WIN_FRAME, &ctx->window, NULL, (void**)&rc) < 0){
     status = -1;
     goto done_get_extents;
    }
    memcpy(extents,rc,sizeof(uint32_t) * 4);
    goto done_get_extents;
   }
  }
  if(ev){ /* event is malloc'ed every loop step */
   free(ev);
  }
 }

done_get_extents:
 if(ev){ /* event is malloc'ed */
  free(ev);
 }
 if(rc){ /* rc may be malloc'ed inside a aux_xcb_get_prop function */
  free(rc);
 }

 return status;
}

#define MAX_PROPERTY_VALUE_LEN 4096

#define WM_ICCCM_SIZE_HINT_US_POSITION     (1 << 0)
#define WM_ICCCM_SIZE_HINT_US_SIZE         (1 << 1)
#define WM_ICCCM_SIZE_HINT_P_POSITION      (1 << 2) /* ignored by X11 */
#define WMICCCM_SIZE_HINT_P_SIZE           (1 << 3)
#define WM_ICCCM_SIZE_HINT_P_MIN_SIZE      (1 << 4)
#define WM_ICCCM_SIZE_HINT_P_MAX_SIZE      (1 << 5)
#define WM_ICCCM_SIZE_HINT_P_RESIZE_INC    (1 << 6)
#define WM_ICCCM_SIZE_HINT_P_ASPECT        (1 << 7)
#define WM_ICCCM_SIZE_HINT_BASE_SIZE       (1 << 8)
#define WM_ICCCM_SIZE_HINT_P_WIN_GRAVITY   (1 << 9)

typedef struct {
uint32_t  flags;                          /** User specified flags */
int32_t   x, y;                           /** User-specified position */
int32_t   width, height;                  /** User-specified size */
int32_t   min_width, min_height;          /** Program-specified minimum size */
int32_t   max_width, max_height;          /** Program-specified maximum size */
int32_t   width_inc, height_inc;          /** Program-specified resize increments */
int32_t   min_aspect_num, min_aspect_den; /** Program-specified minimum aspect ratios */
int32_t   max_aspect_num, max_aspect_den; /** Program-specified maximum aspect ratios */
int32_t   base_width, base_height;        /** Program-specified base size */
uint32_t  win_gravity;                    /** Program-specified window gravity */
} wm_size_hints_t;

typedef struct aux_wm_prop_moveresize_t {
 /* flags:
     bits 0..7   => xcb_gravity_t. byte of zero - use size_hints value.
     bits 8..11  => presence of x,y,w,h.
     bits 12..15 => source of request. 0001 - app, 0010 - pager or taskbar.
     bits 16..31 => zero.
 */
 uint32_t flags;
 uint32_t x;
 uint32_t y;
 uint32_t w;
 uint32_t h;
}aux_wm_prop_moveresize_t;

#define NLIM_BUF 18

/* */
int aux_xcb_change_prop(aux_xcb_ctx *ctx, uint8_t mode, uint32_t flag, const void *data)
{
 xcb_void_cookie_t    cookie;
 xcb_generic_error_t *error;
 xcb_connection_t    *c      = ctx->conn;
 xcb_window_t         window = ctx->window;
 xcb_atom_t           prop;
 xcb_atom_enum_t      type;
 uint8_t              format   = 32;
 uint32_t             data_len = 1;
 uint32_t             ibuf[NLIM_BUF];

 /* */
 memset(ibuf, 0, sizeof(ibuf));

 switch(flag){
 case AUX_WM_PROP_DELETE_WINDOW: { /* in data := NULL, ignored */
   prop     = ctx->atom_wm_p;
   type     = XCB_ATOM_ATOM;
   format   = 32;
   data_len = 1;
   data     = &ctx->atom_wm_p_dw;
   break;
  }
 case AUX_WM_PROP_GRAVITY: { /* in data := xcb_gravity_t enum */
   wm_size_hints_t  *msg  = (wm_size_hints_t*)ibuf;
   xcb_gravity_t     flag = *(xcb_gravity_t*)data;

   prop             = ctx->atom_wm_n_h;
   type             = ctx->atom_wm_n_h_sz;
   format           = 32;
   data_len         = sizeof(wm_size_hints_t) / (32 / CHAR_BIT);
   msg->flags       = WM_ICCCM_SIZE_HINT_P_WIN_GRAVITY;
   msg->win_gravity = flag;
   data             = msg;
   break;
  }
 default: {
   fprintf(stderr, " * aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
   return -1;
   break;
  }
 }

 cookie = xcb_change_property_checked(c,               /* xcb connection                 */
                                      mode,            /* mode                           */
                                      window,          /* window                         */
                                      prop,            /* the property to change         */
                                      type,            /* type of the property           */
                                      format,          /* format(bits)                   */
									  data_len,        /* number of elements(see format) */
                                      data             /* property data                  */
                                     );
 error = xcb_request_check(c, cookie);
 if (error) {
  AUX_XCB_PRINT_X11_ERROR(ctx, error)
  free(error);
  return -1;
 }

 return 0;
}

/* */
int aux_xcb_get_prop(aux_xcb_ctx      *ctx,
                     uint32_t          flag,
                     const void       *arg,
                     unsigned long    *size,
                     void           **data)
{
 xcb_get_property_cookie_t   cookie;
 xcb_get_property_reply_t   *reply;
 xcb_generic_error_t        *error;
 xcb_connection_t           *c = ctx->conn;
 xcb_window_t                window;
 xcb_atom_t                  prop;
 xcb_atom_enum_t             type;
 void                       *prop_val;
 unsigned long               prop_size;
 int                         status = 0;

 if(data)
  *data = NULL;
 if(size)
  *size = 0;

 switch(flag){
 case AUX_WM_PROP_WIN_TYP: { /* ret data := atom */
   window = *(xcb_window_t*)arg;
   prop   = ctx->atom_wm_wtyp;
   type   = XCB_ATOM_ATOM;
   break;
  }
 case AUX_WM_PROP_WORKAREA: { /* ret data := CARDINAL[][4]/32 - x,y,w,h for each desktop */
   window = *((xcb_window_t*)arg);
   prop   = ctx->atom_wm_worka_desk;
   type   = XCB_ATOM_CARDINAL;
   break;
  }
 case AUX_WM_PROP_WIN_FRAME: { /* ret data := CARDINAL[4]/32. l,r,t,b widths of borders */
   window = *((xcb_window_t*)arg);
   prop   = ctx->atom_wm_frame_win;
   type   = XCB_ATOM_CARDINAL;
   break;
  }

 }

 cookie = xcb_get_property(c,                          /* xcb connection */
                           0,                          /* should delete flag */
                           window,                     /* window */
                           prop,                       /* prop(atom) */
                           type,                       /* type(atom) */
                           0,                          /* off(32 units) */
                           MAX_PROPERTY_VALUE_LEN / 4  /* len(32 units) */
 );

 reply  = xcb_get_property_reply(c, cookie, &error);
 if(NULL == reply){
  AUX_XCB_PRINT_X11_ERROR(ctx, error)
  status = -1;
  goto done_prop_func;
 }
 if(reply->type != type) { /* print type of the request and reply. return error. */
  xcb_get_atom_name_reply_t  *nm_reply_req;
  xcb_get_atom_name_reply_t  *nm_reply_ret;
  char                       *nm_req;
  char                       *nm_ret;
  int                         nm_req_len = 0;
  int                         nm_ret_len = 0;

  nm_reply_req = xcb_get_atom_name_reply(c, xcb_get_atom_name(c, type),        NULL);
  nm_reply_ret = xcb_get_atom_name_reply(c, xcb_get_atom_name(c, reply->type), NULL);
  if(nm_reply_req){
   nm_req_len   = xcb_get_atom_name_name_length(nm_reply_req);
   nm_req       = xcb_get_atom_name_name(nm_reply_req);
  }
  if(nm_reply_ret){
   nm_ret_len   = xcb_get_atom_name_name_length(nm_reply_ret);
   nm_ret = xcb_get_atom_name_name(nm_reply_ret);
  }

  fprintf(stderr, " * aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
  if(nm_req_len)
   fprintf(stderr, " * \taux-xcb: requested atom :%.*s\n", nm_req_len, nm_req);
  if(nm_ret_len)
   fprintf(stderr, " * \taux-xcb: returned atom  :%.*s\n", nm_ret_len, nm_ret);
  status = -1;
  if(nm_reply_req)
   free(nm_reply_req);
  if(nm_reply_ret)
   free(nm_reply_ret);
  goto done_prop_func;
 } /* end - print type of the request and reply. */

 prop_size  = xcb_get_property_value_length(reply);
 prop_val   = xcb_get_property_value       (reply);

 if(0 == prop_size){
  fprintf(stderr, " * aux-xcb: %s:%s:%d\nno such prop\n", __FILE__, __func__, __LINE__);
  status = 1;
  goto done_prop_func;
 }

 /* null terminate the result to make string handling easier */
 if(data){
  *data = malloc(prop_size + 1);
  if(NULL == (*data)){
   fprintf(stderr, " * aux-xcb: %s:%s:%d\nmalloc failed.\n", __FILE__, __func__, __LINE__);
   status = 1;
   goto done_prop_func;
  }
  memcpy(*data, prop_val, prop_size);
  ((char*)(*data))[prop_size] = '\0';
 }

 if(size) {
  *size = prop_size;
 }

done_prop_func:
 if(reply)
  free(reply);
 if(error)
  free(error);

 return status;
}

/* */
int aux_xcb_get_desk_worka(aux_xcb_ctx *ctx, uint32_t rc[4])
{
 int            status = 0;
 unsigned long  size = 0;
 uint32_t      *p;

 status = aux_xcb_get_prop(ctx, AUX_WM_PROP_WORKAREA, &ctx->screen->root, &size, (void**)&p);
 fprintf(stderr, " =========================== N = %lu\n", size);
 if(0 == status){
  memcpy(rc, p, sizeof(uint32_t) * 4);
  free(p);
 }

 return status;
}

/* */
int aux_xcb_free_pixmap(aux_xcb_ctx *ctx)
{
 xcb_void_cookie_t    cookie;
 xcb_generic_error_t *error;
 xcb_connection_t    *c = ctx->conn;

 if(((uint32_t)ctx->pixmap) != (uint32_t)0){
  cookie      = xcb_free_pixmap_checked(c, ctx->pixmap);
  ctx->pixmap = (uint32_t)0;
  error       = xcb_request_check(c, cookie);
  if(error) {
   AUX_XCB_PRINT_X11_ERROR(ctx, error)
   free(error);
   return -1;
  }
 }

 return 0;
}

/* */
int aux_xcb_creat_pixmap(aux_xcb_ctx *ctx)
{
 xcb_void_cookie_t    cookie;
 xcb_pixmap_t         pixmap;
 xcb_generic_error_t *error;
 xcb_connection_t    *c = ctx->conn;
 int                  status = 0;

 status = aux_xcb_free_pixmap(ctx);
 pixmap = xcb_generate_id(c);
 cookie = xcb_create_pixmap_checked(c,
                                    ctx->depth->depth,
                                    pixmap,
                                    ctx->window,
                                    ctx->win_w,
                                    ctx->win_h
								   );

 error  = xcb_request_check(c, cookie);
 if(error) {
  AUX_XCB_PRINT_X11_ERROR(ctx, error)
  free(error);
  return -1;
 }

 ctx->pixmap        = pixmap;
 ctx->pixmap_format = XCB_IMAGE_FORMAT_Z_PIXMAP;

 return status;
}

/* returns first key symbol generated from key */
int aux_xcb_map_keycode(void           *xcb_keysyms,
                        xcb_keycode_t   code,
                        uint16_t        state,
						uint32_t       *aux_key,
                        uint16_t       *aux_key_state)
{
 xcb_keysym_t       keysym = XCB_NO_SYMBOL;
 xcb_key_symbols_t *syms   = (xcb_key_symbols_t*)xcb_keysyms;

 *aux_key_state = state;
 keysym = xcb_key_symbols_get_keysym(syms, code, 0);

 if((keysym >= XK_space) && (keysym <= XK_asciitilde)){ /* maps to ascii */
  *aux_key = keysym;
  goto past_ext_keys;
 }

 switch(keysym){
  case XK_BackSpace:   *aux_key = AUX_X11_KEYMAP_BS;    break;
  case XK_Tab:         *aux_key = AUX_X11_KEYMAP_VT;    break;
  case XK_Return:      *aux_key = AUX_X11_KEYMAP_CR;    break;
  case XK_Pause:       *aux_key = AUX_X11_KEYMAP_PAUSE; break;
  case XK_Scroll_Lock: *aux_key = AUX_X11_KEYMAP_SCRLL; break;
  case XK_Sys_Req:     *aux_key = AUX_X11_KEYMAP_SYSRQ; break;
  case XK_Escape:      *aux_key = AUX_X11_KEYMAP_ESC;   break;
  case XK_Delete:      *aux_key = AUX_X11_KEYMAP_DEL;   break;

  case XK_Home:      *aux_key = AUX_X11_KEYMAP_HOME;   break;
  case XK_End:       *aux_key = AUX_X11_KEYMAP_END;    break;
  case XK_Page_Up:   *aux_key = AUX_X11_KEYMAP_PGUP;   break;
  case XK_Page_Down: *aux_key = AUX_X11_KEYMAP_PGDN;   break;

  case XK_Left:      *aux_key = AUX_X11_KEYMAP_LEFT;   break;
  case XK_Right:     *aux_key = AUX_X11_KEYMAP_RIGHT;  break;
  case XK_Up:        *aux_key = AUX_X11_KEYMAP_UP;     break;
  case XK_Down:      *aux_key = AUX_X11_KEYMAP_DOWN;   break;

  case XK_Insert:    *aux_key = AUX_X11_KEYMAP_INS;       break;
  case XK_Menu:      *aux_key = AUX_X11_KEYMAP_MENU;      break;
  case XK_Num_Lock:  *aux_key = AUX_X11_KEYMAP_NUMLOCK;   break;
  case XK_Shift_L:   *aux_key = AUX_X11_KEYMAP_SHIFT_L;   break;
  case XK_Shift_R:   *aux_key = AUX_X11_KEYMAP_SHIFT_R;   break;
  case XK_Control_L: *aux_key = AUX_X11_KEYMAP_CTRL_L;    break;
  case XK_Control_R: *aux_key = AUX_X11_KEYMAP_CTRL_R;    break;
  case XK_Caps_Lock: *aux_key = AUX_X11_KEYMAP_CAPSLOCK;  break;
  case XK_Alt_L:     *aux_key = AUX_X11_KEYMAP_ALT_L;     break;
  case XK_Alt_R:     *aux_key = AUX_X11_KEYMAP_ALT_R;     break;

  case XK_F1:      *aux_key = AUX_X11_KEYMAP_F1;    break;
  case XK_F2:      *aux_key = AUX_X11_KEYMAP_F2;    break;
  case XK_F3:      *aux_key = AUX_X11_KEYMAP_F3;    break;
  case XK_F4:      *aux_key = AUX_X11_KEYMAP_F4;    break;
  case XK_F5:      *aux_key = AUX_X11_KEYMAP_F5;    break;
  case XK_F6:      *aux_key = AUX_X11_KEYMAP_F6;    break;
  case XK_F7:      *aux_key = AUX_X11_KEYMAP_F7;    break;
  case XK_F8:      *aux_key = AUX_X11_KEYMAP_F8;    break;
  case XK_F9:      *aux_key = AUX_X11_KEYMAP_F9;    break;
  case XK_F10:     *aux_key = AUX_X11_KEYMAP_F10;   break;
  case XK_F11:     *aux_key = AUX_X11_KEYMAP_F11;   break;
  case XK_F12:     *aux_key = AUX_X11_KEYMAP_F12;   break;

  case XK_Super_L:  *aux_key = AUX_X11_KEYMAP_SUP_L;   break;
  case XK_Super_R:  *aux_key = AUX_X11_KEYMAP_SUP_R;   break;

  default: break;
 }

past_ext_keys:

 return 0;
}

/* */
int aux_xcb_free_gc(aux_xcb_ctx *ctx)
{
 xcb_void_cookie_t    cookie;
 xcb_generic_error_t *error;
 xcb_connection_t    *c = ctx->conn;

 if(((uint32_t)ctx->gc) != (uint32_t)0){
  cookie   = xcb_free_gc_checked(c, ctx->gc);
  ctx->gc  = (uint32_t)0;
  error    = xcb_request_check(c, cookie);
  if(error) {
   AUX_XCB_PRINT_X11_ERROR(ctx, error)
   free(error);
   return -1;
  }
 }

 return 0;
}

/* */
int aux_xcb_creat_gc(aux_xcb_ctx *ctx)
{
 xcb_void_cookie_t    cookie;
 xcb_generic_error_t *error;
 xcb_gcontext_t       gc;
 xcb_connection_t    *c = ctx->conn;
 int                  status = 0;

 status = aux_xcb_free_gc(ctx);
 gc     = xcb_generate_id(c);

 /* actual create graphics context */
 {
  //~ uint32_t mask     = XCB_GC_GRAPHICS_EXPOSURES;
  //~ uint32_t values[] = {0, 0 /* zero terminate */};

  cookie = xcb_create_gc_checked(c, gc, ctx->window, 0, NULL);
  error  = xcb_request_check(c, cookie);
  if(error) {
   AUX_XCB_PRINT_X11_ERROR(ctx, error)
   free(error);
   return -1;
  }
 }

 ctx->gc = gc;

 return status;
}

/* */
int aux_xcb_free_colormap(aux_xcb_ctx *ctx)
{
 xcb_void_cookie_t    cookie;
 xcb_generic_error_t *error;
 xcb_connection_t    *c = ctx->conn;

 if(((uint32_t)ctx->colormap) != (uint32_t)0){
  cookie        = xcb_free_colormap_checked(c, ctx->colormap);
  ctx->colormap = (uint32_t)0;
  error         = xcb_request_check(c, cookie);
  if(error) {
   AUX_XCB_PRINT_X11_ERROR(ctx, error)
   free(error);
   return -1;
  }
 }

 return 0;
}

/* */
int aux_xcb_creat_colormap(aux_xcb_ctx *ctx)
{
 xcb_void_cookie_t    cookie;
 xcb_generic_error_t *error;
 xcb_colormap_t       colormap;
 xcb_connection_t    *c = ctx->conn;
 int                  status = 0;

 status = aux_xcb_free_colormap(ctx);

 colormap = xcb_generate_id(c);
 cookie   = xcb_create_colormap_checked(c,
                                        XCB_COLORMAP_ALLOC_NONE,
                                        colormap,
                                        ctx->screen->root,
                                        ctx->visual->visual_id
                                       );

 error = xcb_request_check(c, cookie);
 if(error) {
  AUX_XCB_PRINT_X11_ERROR(ctx, error)
  free(error);
  return -1;
 }

 ctx->colormap = colormap;

 return status;
}

int aux_xcb_aux_creat_win(aux_xcb_ctx  *xcb_ctx,
                          int           config[]
                         )
{
 bool  has_w_f      = false, has_h_f      = false;
 bool  has_depth_f  = false;
 int   w_v      = 0, h_v       = 0;
 int   depth_v  = 0;

 for(int *ci = config; *ci; ci += 2){
  switch(*ci){
   case AUX_XCB_CONF_WIN_DEPTH : {
    has_depth_f = true;
    depth_v     = ci[1];
    break;
   }
   case AUX_XCB_CONF_WIN_WIDTH : {
    has_w_f = true;
    w_v     = ci[1];
    break;
   }
   case AUX_XCB_CONF_WIN_HEIGHT : {
    has_h_f = true;
    h_v     = ci[1];
    break;
   }
   default:
    break;
  }
 }

 if(!has_depth_f){
  depth_v = AUX_XCB_CONF_WIN_DEPTH_DEFAULT;
 }
 if(!has_w_f){
  fprintf(stderr, " * aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
  fprintf(stderr, " *        : no window width in config.\n");
  return -1;
 }
 if(!has_h_f){
  fprintf(stderr, " * aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
  fprintf(stderr, " *        : no window height in config.\n");
  return -1;
 }

 /* color depth */
 if(aux_xcb_select_depth(xcb_ctx, depth_v) < 0){
  return -1;
 }

 /* visual */
 if(aux_xcb_select_visual(xcb_ctx, config) < 0){
  return -1;
 }

 /* window */
 if(aux_xcb_creat_window(xcb_ctx, w_v, h_v) < 0){
  return -1;
 }

 /* intercept window close event */
 { /* wm prop: intercept close */
  if(aux_xcb_change_prop(xcb_ctx, AUX_PROP_MODE_REPLACE, AUX_WM_PROP_DELETE_WINDOW, NULL) < 0){
   return -1;
  }
 }

 /* get desktop workarea size, set window gravity, move window center of the workarea
    breaks in the process of togglin switches for xfwm4.
 */
 { /* wm prop workarea */
  uint32_t       *desk; /* desktops X {x,y,w,h} */
  uint32_t       *p;
  unsigned long   len;
  if(aux_xcb_get_prop(xcb_ctx, AUX_WM_PROP_WORKAREA, &xcb_ctx->screen->root, &len, (void**)&desk) < 0){
   return -1;
  }
  len = len / (sizeof(uint32_t) * 4);
  p   = desk;
  printf(" ! aux-xcb: WM_PROP_WORKAREA: # desktops is [%lu]\n", len);
  for(unsigned i = 0; i < len; ++i, p += 4) {
   printf(" !  aux-xcb: WM_PROP_WORKAREA: desktop[%u]: x:%u, y:%u, w:%u, h:%u\n", i,
                                                  p[0],
                                                  p[1],
                                                  p[2],
                                                  p[3]);
  }
  { /* wm prop: set gravity to center of the window */
   int flag = XCB_GRAVITY_CENTER;
   if(aux_xcb_change_prop(xcb_ctx, AUX_PROP_MODE_REPLACE, AUX_WM_PROP_GRAVITY, &flag) < 0){
    return -1;
   }
  }

  aux_xcb_flush(xcb_ctx);

  { /* wm move window to center of the 1st desktop */
   int x = (desk[0] + desk[2]) / 2;
   int y = (desk[1] + desk[3]) / 2;
   if(aux_xcb_move_window(xcb_ctx, x, y) < 0){
    return -1;
   }
  }

  if(desk)
   free(desk);
 }

 { /* create GC for the window */
  if(aux_xcb_creat_gc(xcb_ctx) < 0){
   return -1;
  }
 }

 { /* create pixel buffer for window area update */
  if(aux_xcb_creat_front_buf(xcb_ctx, config) < 0){
   return -1;
  }
 }

 { /* eat up all previous events */
  aux_xcb_flush(xcb_ctx);

  if(aux_xcb_empty_events(xcb_ctx) < 0){
   return -1;
  }
 }

 { /* request window frames(message) */
  uint32_t rc[4];

  if(aux_xcb_get_extents_window(xcb_ctx, rc)){
   return -1;
  }
  printf(" ! xcb: WM_MSG_WIN_FRAME: l:%u, r:%u, t:%u, b:%u\n", rc[0], rc[1], rc[2], rc[3]);
 }

 /* XCB, map, flush events */
 aux_xcb_flush(xcb_ctx);
 aux_xcb_map_window(xcb_ctx);

 printf(" ! aux-xcb: window: %x, root: %x\n", xcb_ctx->window, xcb_ctx->screen->root);

 return 0;
}

/* */
int aux_xcb_get_atom(aux_xcb_ctx  *ctx,
                     const char   *name,
                     xcb_atom_t   *atom)
{
 xcb_intern_atom_cookie_t  cookie;
 xcb_generic_error_t      *error;
 xcb_intern_atom_reply_t  *reply;

 cookie = xcb_intern_atom(ctx->conn, 0, strlen(name), name);
 reply  = xcb_intern_atom_reply(ctx->conn, cookie, &error);
 if(NULL == reply){
  AUX_XCB_PRINT_X11_ERROR(ctx, error)
  free(error);
  return -1;
 }

 *atom = reply->atom;
 free(reply);

 return 0;
}

/* */
static int print_xcb_img_stat(aux_raster_buf *aux_rbuf)
{
 const char *msb = "MSB first";
 const char *lsb = "LSB first";
 const char *vfmt[] = {
 "ARGB", "XRGB", "BGRA", "BGRX",
 };
 const char *ubo = aux_rbuf->byte_order == AUX_RASTER_COLOR_UNIT32_MSB_FIRST? msb:lsb;
 const char *fmt = NULL;

 switch(aux_rbuf->color_unit){
 case AUX_RASTER_COLOR_UNIT32_ARGB : fmt = vfmt[0]; break;
 case AUX_RASTER_COLOR_UNIT32_XRGB : fmt = vfmt[1]; break;
 case AUX_RASTER_COLOR_UNIT32_BGRA : fmt = vfmt[2]; break;
 case AUX_RASTER_COLOR_UNIT32_BGRX : fmt = vfmt[3]; break;
 default:
  fmt = "not assigned.";
 }

 printf(" ! pixel buffer(xcb_image):\n");
 printf(" ! \tw                  : %u\n", aux_rbuf->w);
 printf(" ! \th                  : %u\n", aux_rbuf->h);
 printf(" ! \tscanline_pad       : %u\n", aux_rbuf->scanline_pad);
 printf(" ! \tstride             : %u\n", aux_rbuf->stride);
 printf(" ! \tbpp                : %u\n", aux_rbuf->bpp);
 printf(" ! \tdepth              : %u\n", aux_rbuf->depth);
 printf(" ! \tsize               : %u\n", aux_rbuf->size);
 printf(" ! \tserver byte order  : %s\n", ubo);
 printf(" ! \tcolor unit         : %s\n", fmt);
 printf(" ! \tshared memory      : %s\n", (aux_rbuf->shm_fd < 0)?"no":"yes");
 printf(" ! \tmemory allocated   : %s\n", (aux_rbuf->buf)?"yes":"no");
 printf("\n");

 return 0;
}

/* this intermediate function
   creates an image that is native to XCB.
   flags are:  - "header only" image
               - image with malloc'ed data.
*/
static int creat_xcb_img(aux_xcb_ctx  *ctx,
                         xcb_image_t **img,
                         int           w,
                         int           h,
                         int           flag
                        )
{
 xcb_image_t  *r    = NULL;
 uint32_t      size = 0;

 switch(flag){
 case AUX_XCB_CONF_IMG_CONF_FLAG_MLC: /* malloc */
  break;
 case AUX_XCB_CONF_IMG_CONF_FLAG_HDR: /* return struct, but no allocation */
  size = ~0;
  break;
 default:
  fprintf(stderr, " * aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
  return -1;
 }

 r = xcb_image_create_native(ctx->conn,                 /* connection */
                             w,                         /* w */
                             h,                         /* h */
                             XCB_IMAGE_FORMAT_Z_PIXMAP, /* format */
                             ctx->depth->depth,         /* depth */
                             NULL,                      /* base */
                             size,                      /* size */
                             NULL                       /* data */
 );
 if(NULL == img)
  return -1;

 *img = r;

 return 0;
}

/* create a buffer that is used
   to put a picture on a window.
*/
int aux_xcb_creat_front_buf(aux_xcb_ctx  *ctx,
                            int           config[]
                           )
{
 xcb_image_t  *xcb_img = NULL;
 bool  has_w_f  = false, has_h_f      = false;
 bool  has_fb_f = false, has_mem_fd_f = false;
 int   w_v      = 0, h_v       = 0;
 int   fb_v     = 0, mem_fd_v  = -1;

 /* check for zero config */
 if(NULL == config){
  fprintf(stderr, " * aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
  fprintf(stderr, " *        : zero config.\n");
  return -1;
 }

 /* free front buffer */
 aux_xcb_destroy_front_buf(ctx);

 /* config */
 for(int *ci = config; *ci; ci += 2){
  switch(*ci){
   case AUX_XCB_CONF_FB : {
    has_fb_f = true;
    fb_v     = ci[1];
    break;
   }
   case AUX_XCB_CONF_FB_WIDTH : {
    has_w_f = true;
    w_v     = ci[1];
    break;
   }
   case AUX_XCB_CONF_FB_HEIGHT : {
    has_h_f = true;
    h_v     = ci[1];
    break;
   }
   case AUX_XCB_CONF_FB_MFD_FD : {
    has_mem_fd_f = true;
    mem_fd_v     = ci[1];
    break;
   }
   default:
    break;
  }
 }

 /* check config arguments */
 if(!has_fb_f){ /* default front buffer if none were given in config */
  fb_v = AUX_XCB_CONF_IMG_CONF_FLAG_MLC;
 }
 if(has_mem_fd_f && (fb_v != AUX_XCB_CONF_IMG_CONF_FLAG_MFD)){ /* fd argument passed, but config flag is wrong */
  fprintf(stderr, " * aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
  fprintf(stderr, " *        : memory mmaped fd passed, but the config flag does not support it.\n");
  return -1;
 }
 if((fb_v == AUX_XCB_CONF_IMG_CONF_FLAG_MFD) && !has_mem_fd_f){ /* config requires fd argument with it. */
  fprintf(stderr, " * aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
  fprintf(stderr, " *        : use of existing mmap requested, but no fd supplied.\n");
  return -1;
 }
 if(!has_w_f){ /* default to window width */
  w_v = ctx->win_w;
 }
 if(!has_h_f){ /* default to window height */
  h_v = ctx->win_h;
 }

 /* allocate memory */
 if(AUX_XCB_CONF_IMG_CONF_FLAG_MLC == fb_v){ /* allocate private memory */
  if(creat_xcb_img(ctx, &xcb_img, w_v, h_v, AUX_XCB_CONF_IMG_CONF_FLAG_MLC) < 0)
   return -1;
 }else {
  /* stat image */
  if(creat_xcb_img(ctx, &xcb_img, w_v, h_v, AUX_XCB_CONF_IMG_CONF_FLAG_HDR) < 0)
   return -1;
  if(AUX_XCB_CONF_IMG_CONF_FLAG_HDR == fb_v){ /* stat only */
   goto done_allocate;
  }else if(AUX_XCB_CONF_IMG_CONF_FLAG_SHR == fb_v){ /* create shared memory mapping */
   int    mem_fd = -1;
   void  *p      = NULL;

   if(creat_xcb_img(ctx, &xcb_img, w_v, h_v, AUX_XCB_CONF_IMG_CONF_FLAG_HDR) < 0)
    return -1;
   mem_fd = memfd_create("_aux_xcb_img_fb", 0);
   if(mem_fd < 0){
    fprintf(stderr, " * aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
    return -1;
   }
   if(ftruncate(mem_fd, xcb_img->size) < 0){
    fprintf(stderr, " * aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
    close(mem_fd);
    return -1;
   }
   p = mmap(NULL, xcb_img->size, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, 0);
   if(MAP_FAILED == p){
    fprintf(stderr, " * aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
	close(mem_fd);
    return -1;
   }
   xcb_img->data = p;
   mem_fd_v      = mem_fd;
  }else if(AUX_XCB_CONF_IMG_CONF_FLAG_MFD == fb_v){ /* use fd backed by memory */
   int          mem_fd  = mem_fd_v;
   void        *p       = NULL;
   struct stat  st;

   if(fstat(mem_fd, &st) < 0){
    fprintf(stderr, " * aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
    return -1;
   }
   p = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, 0);
   if(MAP_FAILED == p){
    fprintf(stderr, " * aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
	close(mem_fd);
    return -1;
   }
   xcb_img->data = p;
  }
 }

done_allocate:

 /* save stat */
 ctx->img_raster_buf.shm_fd       = mem_fd_v;              /* fd that is memory backed, if any */
 ctx->img_raster_buf.size         = xcb_img->size;         /* image size, bytes                */
 ctx->img_raster_buf.stride       = xcb_img->stride;       /* bytes per image row              */
 ctx->img_raster_buf.w            = xcb_img->width;        /* image width                      */
 ctx->img_raster_buf.h            = xcb_img->height;       /* image height                     */
 ctx->img_raster_buf.scanline_pad = xcb_img->scanline_pad; /* right pad in bits                */
 ctx->img_raster_buf.bpp          = xcb_img->bpp;          /* bits per pixel, >= depth         */
 ctx->img_raster_buf.depth        = xcb_img->depth;        /* depth, bits                      */
 ctx->img_raster_buf.xcb_img_fmt  = xcb_img->format;       /* xcb_image_format_t               */
 if(xcb_img->byte_order == XCB_IMAGE_ORDER_MSB_FIRST){
  ctx->img_raster_buf.byte_order = AUX_RASTER_COLOR_UNIT32_MSB_FIRST; /* color component byte order     */
  ctx->img_raster_buf.color_unit = AUX_RASTER_COLOR_UNIT32_BGRA;      /* color components in this order */
 }else {
  ctx->img_raster_buf.byte_order = AUX_RASTER_COLOR_UNIT32_LSB_FIRST; /* color component byte order     */
  ctx->img_raster_buf.color_unit = AUX_RASTER_COLOR_UNIT32_ARGB;      /* color components in this order */
 }
 ctx->img_raster_buf.buf          = xcb_img->data; /* actual pointer to memory */

 /* print image stat */
 print_xcb_img_stat(&ctx->img_raster_buf);

 /* free intermediate */
 free(xcb_img);

 return 0;
}

/* send image(front buffer) to X server. */
int aux_xcb_flush_front_buf(aux_xcb_ctx *ctx)
{
 xcb_void_cookie_t     cookie;
 xcb_generic_error_t  *error;
 xcb_connection_t     *c = ctx->conn;
 aux_raster_buf       *i = &ctx->img_raster_buf;

 cookie = xcb_put_image(c,              /* xcb connection   */
                        i->xcb_img_fmt, /* xcb image format */
                        ctx->window,    /* xcb connection   */
                        ctx->gc,        /* xcb connection   */
                        i->w,           /* width            */
                        i->h,           /* height           */
                        0, 0,           /* x,y              */
                        0,              /* left_pad         */
                        i->depth,       /* depth            */
                        i->size,        /* size             */
                        i->buf          /* data             */
 );
 error  = xcb_request_check(c, cookie);
 if(error) {
  AUX_XCB_PRINT_X11_ERROR(ctx, error)
  free(error);
  return -1;
 }

 xcb_flush(ctx->conn);

 return 0;
}

/* */
int aux_xcb_destroy_front_buf(aux_xcb_ctx  *ctx)
{
 aux_raster_buf  *fb =  &ctx->img_raster_buf;

 /* destroy front buffer */
 if(fb->shm_fd != -1){ /* has memory backed file descriptor */
  if(fb->buf)
   munmap(fb->buf, fb->size);
  close(fb->shm_fd);
 }else { /* memory may be malloc'ed */
  if(fb->buf)
   free(fb->buf);
 }
 memset(&ctx->img_raster_buf, 0, sizeof(aux_raster_buf));
 ctx->img_raster_buf.shm_fd = -1;

 return 0;
}
