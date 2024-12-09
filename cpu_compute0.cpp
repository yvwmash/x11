extern "C" {
#include <climits>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
}

extern "C" {
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/signalfd.h>
#include <sys/epoll.h>
}

extern "C" {
#include <json-c/json.h>

#include "aux_egl.h"
#include "aux_gl.h"
}

#include "vg_algebra/geometry.h"

extern "C" {
#include "aux_xcb.h"
#include "aux_drm.h"
}

/* ============================================================================================== */

typedef pt2<double> pt2d;

/* ============================================================================================== */

/* defines for epoll */
#define MAXEVENTS 64
#define SET_EV(_ev,_fd,_events) {\
                                  memset(&_ev, 0, sizeof(struct epoll_event));\
                                  _ev.data.fd = _fd;                          \
                                  _ev.events = _events;                       \
        }

/* ============================================================================================== */

/* global exit flags */
static volatile bool f_win_close        = false; /* exit process => window closed    */
static volatile bool f_exit_sig         = false; /* exit process => signal           */
// static bool f_exit_user  = false; /* exit process => by user request  */
static volatile bool f_win_expose       = false; /* what if i want to assign it in alarm, timer, or other thread? */
static          bool f_display_change   = false; /* */

/* ============================================================================================== */

/*  */
static int read_contours(const char *fn, pt2d **va, unsigned *n_p, unsigned **n_v) {
 int                 status = 0;
 int                 fd = open(fn, O_CLOEXEC|O_RDONLY);
 unsigned            tv = 0;
 struct json_object *parsed_json;

 *va  = NULL;
 *n_p = 0;
 *n_v = NULL;

 if (fd < 0) {
  fprintf(stderr, " * open contours file %s\n\t%s\n", fn, strerror(errno));
  status = 1;
  goto l_end_read_contours;
 }

 parsed_json = json_object_from_fd(fd);
 close(fd);

 if ((NULL == parsed_json) || (json_object_get_type(parsed_json) != json_type_array)) {
  fprintf(stderr, " * parsing JSON or invalid format\n");
  status = 2;
  goto l_end_read_contours;
 }

 /* # of polygons */
 *n_p = json_object_array_length(parsed_json);

 /* total # of vertices */
 for (unsigned pi = 0; pi < *n_p; ++pi) {
  struct json_object *a_poly = json_object_array_get_idx(parsed_json, pi);

  if (!a_poly || (json_object_get_type(a_poly) != json_type_array)) {
   fprintf(stderr, " * invalid polygon format at index %u\n", pi);
   continue;
  }
  tv += json_object_array_length(a_poly);
 }


 /* allocate */
 *n_v = (unsigned*)malloc(*n_p * sizeof(unsigned));
 if(NULL == *n_v) {
  status = 3;
  goto l_end_read_contours;
 }
 *va = (pt2d*)malloc(tv * sizeof(pt2d));
 if(NULL == *va) {
  status = 3;
  goto l_end_read_contours;
 }

 /* iterate through polygons */
 tv = 0;
 for (unsigned pi = 0; pi < *n_p; ++pi) {
  struct json_object *a_poly = json_object_array_get_idx(parsed_json, pi);
  unsigned            n;
 
  if (!a_poly || (json_object_get_type(a_poly) != json_type_array)) {
   fprintf(stderr, " * invalid polygon format at index %u\n", pi);
   continue;
  }
 
  n          = json_object_array_length(a_poly);
  (*n_v)[pi] = n;
 
  /* polygon contour */
  for (unsigned ci = 0; ci < n; ++ci, ++tv) {
   struct json_object *pt = json_object_array_get_idx(a_poly, ci);
   struct json_object *x, *y;

   if (json_object_object_get_ex(pt, "x", &x) && json_object_object_get_ex(pt, "y", &y)) {
    *(*va + tv) = {(double)json_object_get_double(x), (double)json_object_get_double(y)};
   } else {
    fprintf(stderr, " * invalid point format at polygon %u, vertex %u\n", pi, ci);
   }
  }
 }

l_end_read_contours:
 if(0 != status) {
  if(NULL != *va) {
   free(*va);
   *va = NULL;
  }
  *n_p = 0;
  if(NULL != *n_v) {
   free(*n_v);
   *n_v = NULL;
  }
 }
 if(NULL != parsed_json) {
  json_object_put(parsed_json);
 }
 
 return status;
}

/* ============================================================================================== */

/* */
int main(int argc, char *argv[])
{
 (void)argc;
 (void)argv;

 int                      status = 0;
 int                      on = 1;
 int                      sig_fd = -1, ep_fd = -1, x11_fd = -1, dri_fd = -1;
 sigset_t                 mask_sigs, mask_osigs;
 struct epoll_event       ep_ev, *ep_evs = NULL;
 struct signalfd_siginfo  siginf;
 aux_xcb_ctx              xcb_ctx;
 aux_drm_ctx              drm_ctx;

 /* polygons */
 pt2d                    *v_poly_vert;
 unsigned                *n_poly_vert;
 unsigned                 n_poly;

 /* set up signals */
 sigemptyset(&mask_sigs);
 sigaddset(&mask_sigs, SIGINT);  /* these signals will be blocked. the signals will arrive */
 sigaddset(&mask_sigs, SIGQUIT); /*  to epoll and not to a default signal handler.         */

 /* save old sigmask, replace it with new sigmask */
 if(sigprocmask(SIG_BLOCK, &mask_sigs, &mask_osigs) < 0){
  perror(" * sigprocmask(SIG_BLOCK, &mask_sigs, &mask_osigs)");
  status = 1;
  goto main_terminate;
 }

 /* get signal file descriptor */
 if((sig_fd = signalfd(-1, &mask_sigs, 0)) < 0){
  perror(" * signalfd(-1, &mask_sigs, 0)");
  status = 1;
  goto main_terminate;
 }

 /* set signal fd as non-blocking */
 if(ioctl(sig_fd, FIONBIO, (char *)&on) < 0){
  perror(" * ioctl(sig_fd, FIONBIO)");
  status = 1;
  goto main_terminate;
 }

 /* zero drm context */
 aux_drm_zero_ctx(&drm_ctx);

 /* open DRM fd */
 dri_fd = aux_drm_open_fd("card0", &drm_ctx);
 if(dri_fd < 0) {
  status = 1;
  goto main_terminate;
 }

 /* set non-blocking mode */
 if(ioctl(dri_fd, FIONBIO, (char *)&on) < 0){
  perror(" * ioctl(dri_fd, FIONBIO)");
  status = 1;
  goto main_terminate;
 }

 /* zero xcb context */
 aux_zero_xcb_ctx(&xcb_ctx);

 /* connect XCB, create window */
 if(aux_xcb_connect(&xcb_ctx, ":0", 0) < 0){
  status = 1;
  goto main_terminate;
 }

 /* init aux-DRM context */
 if(aux_drm_init_ctx(&drm_ctx) == false) {
  status = 1;
  goto main_terminate;
 }

 /* print some DRM statistics */
 aux_drm_print_ctx(&drm_ctx);

 /* create shared memory that will be used later 
    as a raster buffer of the window. 
 */
 {
  int mem_fd = -1;
   mem_fd = memfd_create("_aux_xcb_img_fb", 0);
   if(mem_fd < 0){
    fprintf(stderr, " * aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
    return -1;
   }
   if(ftruncate(mem_fd, 311 * 673 * 4) < 0){
    fprintf(stderr, " * aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
    close(mem_fd);
    return -1;
   }

  int config[] = {AUX_XCB_CONF_WIN_WIDTH,  311,
                  AUX_XCB_CONF_WIN_HEIGHT, 673,
                  AUX_XCB_CONF_FB,         AUX_XCB_CONF_IMG_CONF_FLAG_MFD,
                  AUX_XCB_CONF_FB_MFD_FD,  mem_fd,
                  0,0,
  };

  /* auxiliary function for window creation. */
  if(aux_xcb_aux_creat_win(&xcb_ctx, config) < 0) {
   status = 1;
   goto main_terminate;
  }
 }

 { /* paint green rect on a window */
   aux_raster_fill_rc(0, 0,
                      xcb_ctx.win_w, xcb_ctx.win_h,
					  0xff00ff00, /* so this is green */
                      &xcb_ctx.img_raster_buf);
   aux_xcb_flush_front_buf(&xcb_ctx);
 }

 /* get x11 connection file descriptor */
 x11_fd = xcb_ctx.fd;

 /* set X11 connection fd as non-blocking */
 if(ioctl(x11_fd, FIONBIO, (char *)&on) < 0){
  perror(" * ioctl(x11_fd, FIONBIO)");
  status = 1;
  goto main_terminate;
 }

 /* create epoll main file descriptor */
 if((ep_fd = epoll_create(1)) < 0){
  perror(" * epoll_create");
  status = 1;
  goto main_terminate;
 }

 /* add signal event */
 SET_EV(ep_ev,sig_fd,EPOLLIN);
 if(epoll_ctl (ep_fd, EPOLL_CTL_ADD, sig_fd, &ep_ev) < 0){
  perror(" * epoll_ctl sig_fd");
  status = 1;
  goto main_terminate;
 }

 /* add DRM event */
 SET_EV(ep_ev, dri_fd, EPOLLIN);
 if(epoll_ctl (ep_fd, EPOLL_CTL_ADD, dri_fd, &ep_ev) < 0){
  perror(" * epoll_ctl dri_fd");
  status = 1;
  goto main_terminate;
 }

 /* add X11 event */
 SET_EV(ep_ev,x11_fd,EPOLLIN);
 if(epoll_ctl (ep_fd, EPOLL_CTL_ADD, x11_fd, &ep_ev) < 0){
  perror(" * epoll_ctl x11_fd");
  status = 1;
  goto main_terminate;
 }

 /* allocate events for epoll queue */
 if(NULL == (ep_evs = (struct epoll_event*)calloc(MAXEVENTS,sizeof(ep_ev)))){
  perror(" * calloc(MAXEVENTS)");
  status = 1;
  goto main_terminate;
 }

 /* read polygon contours */
 if(0 != read_contours("./out/out0.json", &v_poly_vert, &n_poly, &n_poly_vert)) {
  status = 2;
  goto main_terminate;
 }
 {
  unsigned i = 0;
  unsigned vi;
  for(unsigned pi = 0; pi < n_poly; ++pi) {
   unsigned n = n_poly_vert[pi];
 
   printf("poly: %u\n", pi);
   for(vi = 0; vi < n; ++vi) {
    printf("\tv[%u]: %f, %f\n", vi, v_poly_vert[i + vi].x, v_poly_vert[i + vi].y);
   }
   i += vi;
  }
 }

 /* display polygon vertices */
 {
  #define COLOR 0xFF0000FF

  unsigned  bf_w = xcb_ctx.img_raster_buf.w;
  unsigned  bf_h = xcb_ctx.img_raster_buf.h;
  auto     *pbf  = &xcb_ctx.img_raster_buf;
  double    U    = 1.0 / std::max(bf_w, bf_h);
  double    R    = 30.0 * U;

  printf(" ! U: %f\n", U);
  printf(" ! R: %f\n", R);

  assert(xcb_ctx.img_raster_buf.bpp == 32);
  assert(xcb_ctx.img_raster_buf.bpp % CHAR_BIT == 0);
  assert(xcb_ctx.img_raster_buf.stride % (xcb_ctx.img_raster_buf.bpp / CHAR_BIT) == 0);
  assert((xcb_ctx.img_raster_buf.color_unit == AUX_RASTER_COLOR_UNIT32_ARGB) || (xcb_ctx.img_raster_buf.color_unit == AUX_RASTER_COLOR_UNIT32_XRGB));

  for(unsigned pix_x = 0; pix_x < bf_w; pix_x += 1) {
   for(unsigned pix_y = 0; pix_y < bf_h; pix_y += 1) {
    double    x   = 2.0 * pix_x / (double)bf_w - 1.0;
    double    y   = 2.0 * pix_y / (double)bf_h - 1.0;
    unsigned  tvi = 0;
    unsigned  vi;

	for(unsigned pi = 0; pi < n_poly; pi += 1) {
	 unsigned  nv  = n_poly_vert[pi];

     for(vi = 0;  vi < nv; vi += 1) {
      pt2d   pt = v_poly_vert[tvi + vi];
      double d	= distance({x,y}, pt);

	  if(d < 0.001)
      printf(" ! P = {%u, %u}, PX = {%f, %f}, V = {%f, %f}, d = %f\n", pix_x, pix_y, x, y, pt.x, pt.y, d);

      if(d < R) {
       aux_raster_putpix(pix_x, pix_y, COLOR, pbf);
      }
     }
     tvi += vi;
    }
   }
  }
  
 }

 /* loop untill exit signal arrives, or until window is closed */
 status = 0;
 while(1){
  int  n, i, fd;

  n = epoll_pwait (ep_fd, ep_evs, MAXEVENTS, -1, &mask_sigs); /* wait, signal safe */

  for(i = 0; i < n; ++i) { // service epoll events
   fd = ep_evs[i].data.fd;
   if(fd == sig_fd){ /* signal */
    puts("signal arrived");
    status = read(fd, &siginf, sizeof(siginf));
   if(status != sizeof(siginf)) {
    fprintf(stderr,"read != sizeof(siginf)");
    goto main_terminate;
   }
   if(siginf.ssi_signo == SIGINT) {
     printf("got SIGINT\n");
     f_exit_sig = true;
    }else if(siginf.ssi_signo == SIGQUIT) {
     printf("got SIGQUIT\n");
     f_exit_sig = true;
     goto main_terminate;
    }else {
     printf("got unregistered signal\n");
    }
   }else if(fd == x11_fd) { /* x11 event */
    aux_xcb_ev_func(&xcb_ctx);
    f_win_close      = xcb_ctx.f_window_should_close;
    f_win_expose     = xcb_ctx.f_window_expose;
    f_display_change = xcb_ctx.f_eq_changed;
   }else if(fd == dri_fd) { /* DRM event */
	struct  aux_drm_event_crtc_sq vev[4];

    /* drm events are written in full */
    do {
     status = read(fd, &vev, 4 * sizeof(struct aux_drm_event_crtc_sq));
     if(status < 0) {
      if((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
       break;
      }
      perror(" * read DRM fd: ");
      goto main_terminate;
     }
     if(0 == status) {
      break;
     }
    }while(status == (4 * sizeof(struct aux_drm_event_crtc_sq)));
   }
  } /* finish service epoll events */
  
  if(f_win_close){ /* window closed by input */
   goto main_terminate;
  }
  if(f_exit_sig){ /* process needs to stop */
   goto main_terminate;
  }
  if(f_win_expose){ /* window needs update its graphics */
   aux_xcb_flush_front_buf(&xcb_ctx);
  }
  if(f_display_change) {  /* display status changed */
   aux_drm_init_ctx(&drm_ctx);
   aux_drm_print_ctx(&drm_ctx);
  }

 } /* end while(1) */

main_terminate:

 if(aux_xcb_disconnect(&xcb_ctx) < 0){
  status = 1;
 }
 if (sigprocmask(SIG_SETMASK, &mask_osigs, NULL) < 0){
  perror(" * sigprocmask(SIG_SETMASK, &mask_osigs, NULL)");
  status = 1;
 }
 if(dri_fd > 0) {
  close(dri_fd);
 }
 if(sig_fd != -1){
  close(sig_fd);
 }
 if(ep_fd != -1){
  close(ep_fd);
 }
 if(ep_evs){
  free(ep_evs);
 }
 if(NULL != v_poly_vert) {
  free(v_poly_vert);
 }
 if(NULL != n_poly_vert) {
  free(n_poly_vert);
 }

 printf("normal process termination\n");

 return status;
}