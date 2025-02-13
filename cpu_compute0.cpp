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
#include <sys/event.h>
}

extern "C" {
#include "aux_egl.h"
#include "aux_gl.h"
}

#include "vg_algebra/geometry.h"

extern "C" {
#include "aux_xcb.h"
#include "aux_drm.h"
#include "aux_svg.h"
}

/* C++ */
#include <vector>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"

/* ============================================================================================== */

typedef pt2<double>  pt2d;
typedef pt2<unsigned> pt2u;
typedef pt2<int>      pt2i;
typedef vec2<double> vec2d;
typedef vec3<double> vec3d;
typedef vec4<double> vec4d;

/* ============================================================================================== */

/* global exit flags */
static volatile bool f_win_close        = false; /* exit process => window closed    */
static volatile bool f_exit_sig         = false; /* exit process => signal           */
// static bool f_exit_user  = false; /* exit process => by user request  */
static volatile bool f_win_expose       = false; /* what if i want to assign it in alarm, timer, or other thread? */
static          bool f_display_change   = false; /* */

/* ============================================================================================== */

/* ============================================================================================== */

/* **********************************************************libgdal*************** */

static int filter_signals(int kq_fd, sigset_t *mask_save) {
 int               status = 0;
 sigset_t          mask   = {0};
 int               vmask[] = { /* these signals will be blocked. the signals will arrive */
  SIGINT,                      /*  to kqueue and not to a default signal handler.        */
  SIGQUIT,
 };
 constexpr size_t  n_signals = sizeof(vmask) / sizeof(vmask[0]);
 struct    kevent  evs[n_signals];

 /* set up sigmask */
 if(sigemptyset(&mask) < 0) {
  perror(" * sigemptyset(...)");
  status = 1;
  goto l_end_flt_signals;
 }
 /* add signals to the mask */
 for(unsigned i = 0; i < n_signals; i += 1) {
  if(sigaddset(&mask, vmask[i]) < 0) {
   perror(" * sigaddset(...)");
   status = 2;
   goto l_end_flt_signals;
  }
 }

 /* save old sigmask, replace it with new sigmask */
 if(sigprocmask(SIG_BLOCK, &mask, mask_save) < 0) {
  perror(" * sigprocmask(SIG_BLOCK, &mask, mask_save)");
  status = 3;
  goto l_end_flt_signals;
 }

 /* add signal events */
 for(unsigned i = 0; i < n_signals; i += 1) {
  EV_SET(&evs[i], vmask[i], EVFILT_SIGNAL, EV_ADD | EV_ENABLE, 0, 0, NULL);
 }
 if(kevent(kq_fd, evs, n_signals, NULL, 0,	NULL) < 0) {
  perror(" * kevent(n_signals)");
  status = 4;
  goto l_end_flt_signals;
 }

l_end_flt_signals:
 return status;
}

/* ************************************************************************* */

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"

/* return minimum distance between line segment vw and point p */
static auto dist_segment(pt2d p, pt2d v, pt2d w) -> double {
 vec2d  r  = w - v;
 double l2 = r.x * r.x + r.y * r.y;  /* segment length squared */

 if (l2 == 0.0) return distance(p, v);   /* v == w case */
 /* consider the line extending the segment, parameterized as v + t (w - v). */
 /* find projection of point p onto the line.                                */
 /* it falls where t = [(p-v) . (w-v)] / |w-v|^2                             */
 /* clamp t from [0,1] to handle points outside the segment vw.              */
 /* pp = v + t * r; pp is a point where projection falls on the segment      */
 double t = std::clamp(dot(p - v, r) / l2, 0.0, 1.0);

 return distance(p, v + t * r);
}

/* ************************************************************************* */

static auto distance_sq(auto &l, auto &r) -> double {
 return pow(r.x - l.x, 2.0) + pow(r.y - l.y, 2.0);
}

/* ************************************************************************* */

/* return minimum distance between line segment vw and point p */
static auto sq_dist_segment(pt2d p, pt2d v, pt2d w) -> double {
 vec2d  r  = w - v;
 double l2 = r.x * r.x + r.y * r.y;  /* segment length squared */

 if (l2 == 0.0) return distance_sq(p, v);   /* v == w case */
 /* consider the line extending the segment, parameterized as v + t (w - v). */
 /* find projection of point p onto the line.                                */
 /* it falls where t = [(p-v) . (w-v)] / |w-v|^2                             */
 /* clamp t from [0,1] to handle points outside the segment vw.              */
 double t = std::clamp(dot(p - v, r) / l2, 0.0, 1.0);

 v = v + t * r;  /* projection falls on the segment */
 r = v - p;
 return r.x*r.x + r.y*r.y; /* squared distance */
}

/* ************************************************************************* */

/* return minimum distance from a point to closest polygons line segment */
static auto dist_all_polygons(auto& P, pt2d& p, size_t &pidx) -> double {
 double min = DBL_MAX;
 double ply = DBL_MAX;
 double d;

 for(size_t pi = 0; pi < P.size(); pi += 1) {
  auto &V = P[pi];

  for(size_t vi = 0; vi < (V.size() - 1); vi += 1) {
   /* squared distance to polygon segment */
   d = sq_dist_segment(p, V[vi], V[vi + 1]);
   if(d < ply) {
    ply = d;
   }
  }
  if(ply < min) {
   min  = ply;
   pidx = pi;
  }
 }

 /* map d^2 => d */
 min = sqrt(min);

 /* is point inside a polygon?  */
 if( pnpoly(P[pidx], p) ) {
  min = -min;
 }

 return min;
}

#pragma clang diagnostic pop

/* ************************************************************************* */

/* point in polygon test */
static unsigned pnpoly(const std::vector<pt2d> &polygon, const pt2d &pt) {
 size_t    i, j;
 size_t    nvert = polygon.size();
 unsigned  c = 0;

 for (i = 0, j = nvert - 1; i < nvert; j = i++) {
  if ( ((polygon[i].y > pt.y) != (polygon[j].y > pt.y))
       && (pt.x < (polygon[j].x - polygon[i].x) * (pt.y - polygon[i].y) / (polygon[j].y - polygon[i].y) + polygon[i].x)
     )
  {
   c = !c;
  }
 }
 return (unsigned)c;
}

/* ************************************************************************* */

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wglobal-constructors"
#pragma clang diagnostic ignored "-Wexit-time-destructors"

/* polygons             */
static std::vector<std::vector<pt2d>>  polygons;
static std::vector<svg_coordinate>     svg_stack_coordinates;
static std::vector<pt2d>               svg_polygon;
static std::vector<pt2u>               anchor_points;

#pragma clang diagnostic pop

extern "C" {

static unsigned svg_moveto(aux_svg_ctx_t *ctx, svg_coordinate to_point)
{
 (void)ctx;

 svg_polygon.clear();
 svg_polygon.push_back({ to_point.x, to_point.y });

 return 0;
}

static unsigned svg_lineto(aux_svg_ctx_t *ctx, svg_coordinate to_point)
{
 (void)ctx;

 svg_polygon.push_back({ to_point.x, to_point.y });
 return 0;
}

static unsigned svg_closepath(aux_svg_ctx_t *ctx, svg_coordinate initial_point)
{
 (void)ctx;
 (void)initial_point;
 return 0;
}

static unsigned svg_push_coordinate(double x, double y)
{
 svg_stack_coordinates.push_back({x, y});
 return 0;
}

static svg_coordinate svg_get_coordinate(unsigned index)
{
 return svg_stack_coordinates[index];
}

static unsigned svg_end_path(void)
{
 polygons.push_back(svg_polygon);

 return 0;
}

static unsigned svg_clear_stack(void)
{
 svg_stack_coordinates.clear();
 return 0;
}

} /* extern C */

/* ************************************************************************* */

/* ============================================================================================== */

/* */
int main(int argc, char *argv[])
{
 (void)argc;
 (void)argv;

 int                      status = 0;
 int                      on = 1;
 int                      kq_fd = -1, x11_fd = -1, drm_fd = -1, mem_fd_imgbuf = -1;
 sigset_t                 mask_osigs = {0};
 struct kevent            kq_evs[5];
 aux_xcb_ctx              xcb_ctx;
 aux_drm_ctx              drm_ctx;

 /* zero xcb context */
 aux_zero_xcb_ctx(&xcb_ctx);

 /* zero drm context */
 aux_drm_zero_ctx(&drm_ctx);

 {
  aux_svg_zero_ctx();
  aux_svg_init_ctx();

  aux_svg_set_moveto_cb(svg_moveto);
  aux_svg_set_lineto_cb(svg_lineto);
  aux_svg_set_push_coordinate_cb(svg_push_coordinate);
  aux_svg_set_get_coordinate_cb(svg_get_coordinate);
  aux_svg_set_closepath_cb(svg_closepath);
  aux_svg_set_end_path_cb(svg_end_path);
  aux_svg_set_clear_stack_cb(svg_clear_stack);

  if(0 != aux_svg_parse_fn("bitmap.svg")) {
   status = 1;
   goto main_terminate;
  }
 }

 /* connect XCB, create window
    various extensions of the X11 server will be checked here.
    if DRI3 extension is present, an attempt will be made to aquire DRM file descriptor.
 */
 if(aux_xcb_connect(&xcb_ctx, ":0", 0) < 0){
  status = 1;
  goto main_terminate;
 }

 /* assign DRM fd */
 if(0 != aux_drm_take_xcb_dri_fd(&drm_ctx, &xcb_ctx)) {
  status = 1;
  goto main_terminate;
 }
 drm_fd = drm_ctx.fd;

 /* set DRM fd to non-blocking mode */
 if(ioctl(drm_fd, FIONBIO, (char *)&on) < 0){
  perror(" * ioctl(dri_fd, FIONBIO)");
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

#define WIN_W 1024
#define WIN_H 1024

/* sdf: -1 := inside of a polygon to the edge. +1 := outside of a polygon to the edge. */
/* idx: index of a polygon => pixel {x,y}                                              */
static double sdf_polygon[WIN_W][WIN_H];
static size_t idx_polygon[WIN_W][WIN_H];

 /* create actual X11 window.
    create shared memory that will be used later
    as a raster buffer of the window.
 */
 {
  int config[] = {AUX_XCB_CONF_FB,         AUX_XCB_CONF_IMG_CONF_FLAG_MFD,
                  AUX_XCB_CONF_FB_MFD_FD,  -1,
                  AUX_XCB_CONF_WIN_WIDTH,  WIN_W,
                  AUX_XCB_CONF_WIN_HEIGHT, WIN_H,
                  0,0,
  };
   mem_fd_imgbuf = memfd_create("_aux_xcb_img_fb", 0);
   if(mem_fd_imgbuf < 0) {
    fprintf(stderr, " * aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
    status = 1;
    goto main_terminate;
   }
   if(ftruncate(mem_fd_imgbuf, WIN_W * WIN_H * 4) < 0) {
    fprintf(stderr, " * aux-xcb: %s:%s:%d\n", __FILE__, __func__, __LINE__);
    status = 1;
    goto main_terminate;
   }

   /* mem_fd is initialized, add it to config */
   config[2] = AUX_XCB_CONF_FB_MFD_FD;
   config[3] = mem_fd_imgbuf;

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

 /* create kqueue file descriptor */
 if((kq_fd = kqueuex(KQUEUE_CLOEXEC)) < 0) {
  perror(" * kqueue()");
  status = 1;
  goto main_terminate;
 }

 /* add blocked signals to be reported by kqueue */
 if(0 != filter_signals(kq_fd, &mask_osigs)) {
  status = 1;
  goto main_terminate;
 }

 /* add DRM event */
 {
  struct kevent ev;

  EV_SET(&ev, drm_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
  if(kevent(kq_fd, &ev, 1, NULL, 0,	NULL) < 0) {
   perror(" * kqueue()");
   status = 1;
   goto main_terminate;
  }
 }

 /* add X11 event */
 {
  struct kevent ev;

  EV_SET(&ev, x11_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
  if(kevent(kq_fd, &ev, 1, NULL, 0,	NULL) < 0) {
   perror(" * kqueue()");
   status = 1;
   goto main_terminate;
  }
 }

 /* assert that image buffer is of supported format */
 assert(xcb_ctx.img_raster_buf.bpp == 32);
 assert(xcb_ctx.img_raster_buf.bpp % CHAR_BIT == 0);
 assert(xcb_ctx.img_raster_buf.stride % (xcb_ctx.img_raster_buf.bpp / CHAR_BIT) == 0);
 assert((xcb_ctx.img_raster_buf.color_unit == AUX_RASTER_COLOR_UNIT32_ARGB) || (xcb_ctx.img_raster_buf.color_unit == AUX_RASTER_COLOR_UNIT32_XRGB));
 assert(xcb_ctx.img_raster_buf.byte_order == AUX_RASTER_COLOR_UNIT32_LSB_FIRST);

 /* map polygon coordinates to {-1, +1} range
  * mirror each y coordinate
 */
 {
  unsigned  bf_w = xcb_ctx.img_raster_buf.w;
  unsigned  bf_h = xcb_ctx.img_raster_buf.h;

  for( auto &P : polygons ) {
   for ( auto &v : P ) {
    double    x   = 2.0 * v.x / (double)bf_w - 1.0;
    double    y   = 2.0 * v.y / (double)bf_h - 1.0;
    v.x =  x;
    v.y = -y;
   }
   printf("\n");
  }
 }
 /* display polygon distance fields */
 {
  unsigned  bf_w = xcb_ctx.img_raster_buf.w;
  unsigned  bf_h = xcb_ctx.img_raster_buf.h;
  auto     *pbf  = &xcb_ctx.img_raster_buf;
  double    U    = 2.0 / std::min(bf_w, bf_h);
  double    R    = 7.0 * U;

  /* colors */
  vec3d   c_v    = vec3d(0.0, 0.0, 1.0);  /* RGB, BLUE  */
  vec3d   c_w    = vec3d(1.0, 1.0, 1.0);  /* RGB, WHITE */
  vec3d   c_b    = vec3d(0.0, 0.0, 0.0);  /* RGB, BLACK */
  vec3d   c_r    = vec3d(1.0, 0.0, 0.0);  /* RGB, RED   */
  const vec3d c_pdf[7]  = { /* RGB, polygon closest points */
   {78.0 / 255.0, 6.0 / 255.0, 122.0 / 255.0},
   {161.0 / 255.0, 95.0 / 255.0, 8.0 / 255.0},
   {168.0 / 255.0, 196.0 / 255.0, 8.0 / 255.0},
   {2.0 / 255.0, 191.0 / 255.0, 100.0 / 255.0},
   {139.0 / 255.0, 41.0 / 255.0, 163.0 / 255.0},
   {168.0 / 255.0, 35.0 / 255.0, 108.0 / 255.0},
   {148.0 / 255.0, 179.0 / 255.0, 71.0 / 255.0},
  };
  vec3d   dst_c;
  vec3d   src_c;
  vec4d   fout_c;

  for(unsigned pix_x = 0; pix_x < bf_w; pix_x += 1) { /* pixels */
   for(unsigned pix_y = 0; pix_y < bf_h; pix_y += 1) {
    double    x   = 2.0 * pix_x / (double)bf_w - 1.0;
    double    y   = 2.0 * pix_y / (double)bf_h - 1.0;
    pt2d      p   = {x,y};
    double    d;
    size_t    idx;

    /* get dst color */
    rgb_ui(aux_raster_getpix(pix_x, pix_y, pbf), dst_c);

    /* check every line segment, of every polygon, for the distance  */
	d = dist_all_polygons(polygons, p, idx) / 2.0; /* map from {0, 2} to {0, 1} */
    sdf_polygon[pix_x][pix_y] = d;
    idx_polygon[pix_x][pix_y] = idx;
    if(d < 0.0) { /* is point inside a polygon? */
     fout_c = vec4d(1.0, c_b);
	 goto l_end_pixel0;
    } else if( d < R ) { /* can it be a vertex pixels? */
     /* vertex */
	 for(std::vector<pt2d> &V : polygons) { /* all polygons */
      for(auto &v : V) { /* vertices */
       if(distance_sq(p, v) < (R * R)) { /* its a vertex pixel */
        fout_c = vec4d(1.0, c_v);
        goto l_end_pixel0;
       }
      }
     }
    }

    if(d < (10.0 * U)) { /* limit distance field */
     fout_c = vec4d(1.0, c_r);
    } else {
     fout_c = vec4d(1.0, c_pdf[idx]);
    }

l_end_pixel0: ;
    aux_raster_putpix(pix_x, pix_y, ui_argb(fout_c), pbf);
   }
  }

 }

 static const auto &fn_check_block = [](unsigned pix_x, unsigned pix_y, int clamp_x, int clamp_y) -> unsigned {
  int x1 = (int)pix_x, x0 = (int)pix_x - 1, x2 = (int)pix_x + 1;
  int y1 = (int)pix_y, y0 = (int)pix_y - 1, y2 = (int)pix_y + 1;
  pt2i     b[9] = {
   {x0, y0}, {x1, y0}, {x2, y0},
   {x0, y1}, {x1, y1}, {x2, y1},
   {x0, y2}, {x1, y2}, {x2, y2},
  };
  size_t   idx[9];
  unsigned i = 0, nunique = 1;

  /* assign block indices */
  for(; i < 9; ++i) {
   if( (b[i].x < 0) || (b[i].y < 0) || (b[i].x > clamp_x) || (b[i].y > clamp_y) ) {
    idx[i] = UINT_MAX;
   } else {
    idx[i] = idx_polygon[b[i].x][b[i].y];
   }
  }

  /* count unique indices */
  std::sort(idx, idx + 9);
  for(i = 0; i < 8; ++i) {
   if(idx[i] != idx[i + 1]) {
    nunique += 1;
   }
  }

  return nunique;
 };

 /* scan 3x3 pixel area. display polygon distance border anchors. */
 {
  unsigned  bf_w    = xcb_ctx.img_raster_buf.w;
  unsigned  bf_h    = xcb_ctx.img_raster_buf.h;
  int       clamp_x = (int)bf_w - 1;
  int       clamp_y = (int)bf_h - 1;
  auto     *pbf     = &xcb_ctx.img_raster_buf;

  /* colors */
  vec3d   c_white = vec3d(1.0, 1.0, 1.0);  /* RGB, WHITE */
  vec3d   c_red   = vec3d(1.0, 0.0, 0.0);  /* RGB, RED   */
  vec4d   fout_c;

  for(unsigned pix_x = 0; pix_x < bf_w; pix_x += 1) { /* pixels */
   for(unsigned pix_y = 0; pix_y < bf_h; pix_y += 1) {
    unsigned  nunique = fn_check_block(pix_x, pix_y, clamp_x, clamp_y);

	if(nunique > 2) {
	 printf("NU > 2, {%u, %u}\n", pix_x, pix_y);

	 fout_c = vec4d(1.0, c_red);
	 aux_raster_putpix(pix_x,  pix_y,  ui_argb(fout_c), pbf);
	}
   }
  }
 }

 /* loop untill exit signal arrives, or until window is closed */
 status = 0;
 while(1) {
  int        n, i;
  u_short    flags;
  u_int      fflags;
  short      filter;
  uintptr_t  id;

  (void)flags;
  (void)fflags;

  n = kevent(kq_fd, NULL, 0, kq_evs, 5, NULL); /* wait, signal aware */
  if(n < 0) { /* error, or cancellation point */
   if(EINTR == errno) { /* EINTR */
    f_exit_sig = true;
   }else {
    perror(" * kevent(...) loop interrupted by ");
    status = 1;
    f_exit_sig = true;
   }
  }

  for(i = 0; i < n; ++i) { // service epoll events
   flags  = kq_evs[i].flags;
   id     = kq_evs[i].ident;
   filter = kq_evs[i].filter;
   fflags = kq_evs[i].fflags;

   (void)flags;
   (void)fflags;

   if(filter == EVFILT_SIGNAL) { /* signal */
    fprintf(stderr, " ! got %s\n", strsignal((int)id));
    f_exit_sig = true;
   }else if((int)id == x11_fd) { /* x11 event */
    aux_xcb_ev_func(&xcb_ctx);
    f_win_close  = xcb_ctx.f_window_should_close;
    f_win_expose = xcb_ctx.f_window_expose;
   }else if((int)id == drm_fd) { /* DRM event */
	struct  aux_drm_event_crtc_sq  vev[4];
    ssize_t                        nread;

    /* drm events are written in full */
    do {
     nread = read((int)id, &vev, 4 * sizeof(struct aux_drm_event_crtc_sq));
     if(nread < 0) {
      if((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
       break;
      }
      perror(" * read DRM fd: ");
      goto main_terminate;
     }
     if(0 == nread) {
      break;
     }
    }while(nread == (4 * sizeof(struct aux_drm_event_crtc_sq)));
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
 if(drm_fd > 0) {
  close(drm_fd);
  aux_drm_destroy_ctx(&drm_ctx);
 }
 if(kq_fd != -1) {
  close(kq_fd);
 }
 if(mem_fd_imgbuf != -1) {
 /* ... Instead, the shared memory object will be garbage collected when the last reference to
    the shared memory object is removed. ... */
  close(mem_fd_imgbuf);
 }

 printf("normal process termination\n");

 return status;
}

#pragma clang diagnostic pop
