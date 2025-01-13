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

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation-deprecated-sync"
#pragma clang diagnostic ignored "-Wreserved-macro-identifier"
#pragma clang diagnostic ignored "-Wc++98-compat-pedantic"
#pragma clang diagnostic ignored "-Wold-style-cast"

extern "C" {
#include <json-c/json.h>
}

#pragma clang diagnostic pop


extern "C" {
#include "aux_egl.h"
#include "aux_gl.h"
}

#include "vg_algebra/geometry.h"

/* C++, OpenCV */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#pragma clang diagnostic ignored "-Wreserved-macro-identifier"
#pragma clang diagnostic ignored "-Wreserved-identifier"
#pragma clang diagnostic ignored "-Wcast-align"
#pragma clang diagnostic ignored "-Wc++98-compat-pedantic"
#pragma clang diagnostic ignored "-Wweak-vtables"
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma clang diagnostic ignored "-Wextra-semi-stmt"
#pragma clang diagnostic ignored "-Wdeprecated-copy-with-user-provided-dtor"
#pragma clang diagnostic ignored "-Wdeprecated-dynamic-exception-spec"
#pragma clang diagnostic ignored "-Wsuggest-destructor-override"
#pragma clang diagnostic ignored "-Wdouble-promotion"
#pragma clang diagnostic ignored "-Wswitch-default"
#pragma clang diagnostic ignored "-Wswitch-enum"
#pragma clang diagnostic ignored "-Wextra-semi"
#pragma clang diagnostic ignored "-Wdeprecated-anon-enum-enum-conversion"
#pragma clang diagnostic ignored "-Wnewline-eof"
#pragma clang diagnostic ignored "-Wdocumentation-unknown-command" /*  */
#pragma clang diagnostic ignored "-Wfloat-conversion" /* C style casts, all legal */
#pragma clang diagnostic ignored "-Wimplicit-float-conversion"
#pragma clang diagnostic ignored "-Wimplicit-int-conversion"
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#pragma clang diagnostic ignored "-Wexit-time-destructors" /* warning: declaration requires an exit-time destructor => static cv::Mutex mutex; */
#pragma clang diagnostic ignored "-Wundefined-reinterpret-cast" /* yes they do know their trade */
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion" /* hope they know their trade */
#pragma clang diagnostic ignored "-Winconsistent-missing-destructor-override"
#pragma clang diagnostic ignored "-Wshadow-field" /* thats unfortunate :p */
#pragma clang diagnostic ignored "-Wc11-extensions" /* warning: '_Atomic' is a C11 extension [-Wc11-extensions] */
#pragma clang diagnostic ignored "-Wcast-qual" /* drop constness, perfectly OK for C style */
#pragma clang diagnostic ignored "-Wpadded" /* struct padding, OK */
#pragma clang diagnostic ignored "-Wfloat-equal" /* all case are safe to a degree. still better use epsilons */
#pragma clang diagnostic ignored "-Wsign-conversion" /* no explicit conversion */
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage" /* hundredth of it. isn't the "p + off" is a normal way to access memory in C/C++? */

#include <opencv2/opencv.hpp>

#pragma clang diagnostic pop

extern "C" {
#include "aux_xcb.h"
#include "aux_drm.h"
}

/* C++ */
#include <vector>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"

/* ============================================================================================== */

typedef pt2<double>  pt2d;
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

/* ************************************************************************* */

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

static int read_contours(const char *fn, std::vector<std::vector<pt2d>> &out) {
 int                 status = 0;
 int                 fd = open(fn, O_CLOEXEC|O_RDONLY);
 struct json_object *parsed_json = NULL;
 size_t              np;

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
 np = json_object_array_length(parsed_json);
 out.reserve(np); /* reserve # polygons */

 /* iterate through polygons */
 for (unsigned pi = 0; pi < np; pi += 1) {
  struct json_object *a_poly = json_object_array_get_idx(parsed_json, pi);
  size_t              n;
  std::vector<pt2d>   vp;

  if (!a_poly || (json_object_get_type(a_poly) != json_type_array)) {
   fprintf(stderr, " * invalid polygon format at index %u\n", pi);
   continue;
  }

  /* vertices in a polygon */
  n = json_object_array_length(a_poly);
  vp.reserve(n); /* reserve for # vertices */

  /* polygon contour */
  for (unsigned ci = 0; ci < n; ci += 1) {
   struct json_object *pt = json_object_array_get_idx(a_poly, ci);
   struct json_object *x, *y;
   pt2d                point;

   /* add a point to polygon */
   if (json_object_object_get_ex(pt, "x", &x) && json_object_object_get_ex(pt, "y", &y)) {
    vp.emplace_back((double)json_object_get_double(x), (double)json_object_get_double(y));
   } else {
    fprintf(stderr, " * invalid point format at polygon %u, vertex %u\n", pi, ci);
   }
  }

  out.emplace_back(vp); /* vp will be created anew next iteration */
 }

l_end_read_contours:
 if(NULL != parsed_json) {
  json_object_put(parsed_json);
 }

 return status;
}

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

 /* polygons             */
 std::vector<std::vector<pt2d>> polygons;

 /* zero xcb context */
 aux_zero_xcb_ctx(&xcb_ctx);

 /* zero drm context */
 aux_drm_zero_ctx(&drm_ctx);

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

 /* display polygon vertices */
 {
  unsigned  bf_w = xcb_ctx.img_raster_buf.w;
  unsigned  bf_h = xcb_ctx.img_raster_buf.h;
  auto     *pbf  = &xcb_ctx.img_raster_buf;
  double    U    = 2.0 / std::min(bf_w, bf_h);
  double    R    = 7.0 * U;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-but-set-variable"
#pragma clang diagnostic ignored "-Wunused-variable"

  /* return minimum distance between line segment vw and point p */
  auto dist_segment = [](pt2d p, pt2d v, pt2d w) -> double {
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
  };

  /* return minimum distance between line segment vw and point p */
  auto sq_dist_segment = [](pt2d p, pt2d v, pt2d w) -> double {
   vec2d  r  = w - v;
   double l2 = r.x * r.x + r.y * r.y;  /* segment length squared */

   if (l2 == 0.0) return distance(p, v);   /* v == w case */
   /* consider the line extending the segment, parameterized as v + t (w - v). */
   /* find projection of point p onto the line.                                */
   /* it falls where t = [(p-v) . (w-v)] / |w-v|^2                             */
   /* clamp t from [0,1] to handle points outside the segment vw.              */
   double t = std::clamp(dot(p - v, r) / l2, 0.0, 1.0);

   v = v + t * r;  /* projection falls on the segment */
   r = v - p;
   return r.x*r.x + r.y*r.y; /* squared distance */
  };

  auto distance_sq = [](auto &l, auto &r) -> double {
    return pow(r.x - l.x, 2.0) + pow(r.y - l.y, 2.0);
  };

  /* return minimum distance from a point to closest polygons line segment */
  auto sq_dist_all_polygons = [&](auto& P, pt2d& p) -> double {
   double t = 1e18;
   pt2d   v0, v1;

   for(std::vector<pt2d> &vertices : P) {
   	for(size_t vi = 0; vi < (vertices.size() - 1); vi += 1) {
   	 v0 = vertices[vi];
   	 v1 = vertices[vi + 1];

   	 /* minimum squared distance to *all* polygon segments */
   	 t = std::min(t, sq_dist_segment(p, v0, v1));
   	}
   }

   return t;
  };

#pragma clang diagnostic pop

  assert(xcb_ctx.img_raster_buf.bpp == 32);
  assert(xcb_ctx.img_raster_buf.bpp % CHAR_BIT == 0);
  assert(xcb_ctx.img_raster_buf.stride % (xcb_ctx.img_raster_buf.bpp / CHAR_BIT) == 0);
  assert((xcb_ctx.img_raster_buf.color_unit == AUX_RASTER_COLOR_UNIT32_ARGB) || (xcb_ctx.img_raster_buf.color_unit == AUX_RASTER_COLOR_UNIT32_XRGB));
  assert(xcb_ctx.img_raster_buf.byte_order == AUX_RASTER_COLOR_UNIT32_LSB_FIRST);

  double  th     = 3.0 * U;   /* thickness */
  double  th_2   = th * th;   /* thinckness squared */
  double  bl     = 1.0 * U;   /* blend distance */
  double  lim    = th + bl;   /* limit to test distance function for the line */
  double  lim_2  = lim * lim; /* limit squared */
  double  r_2    = R * R;     /* vertex circle radius squared */
  /* colors */
  vec3d   c_v    = vec3d(0.0, 0.0, 1.0);  /* RGB, BLUE  */
  vec3d   c_c    = vec3d(1.0, 1.0, 1.0);  /* RGB, WHITE */
  vec3d   c_b    = vec3d(0.0, 0.0, 0.0);  /* RGB, BLACK */
  vec3d   c_r    = vec3d(1.0, 0.0, 0.0);  /* RGB, RED   */
  vec3d   dst_c;
  vec3d   src_c;
  vec4d   fout_c;

  /* read polygon contours */
  if(0 != read_contours("./out/out0.json", polygons)) {
   status = 2;
   goto main_terminate;
  }

  printf(" ! U   : %f\n", U);
  printf(" ! U2  : %f\n", U * U);
  printf(" ! R   : %f\n", R);
  printf(" ! th_2: %f\n", th_2);
  printf(" ! L2  : %f\n", lim_2);
  printf(" ! lim : %f\n", lim);

  printf(" LERP: %f\n", std::lerp(1.0, 0.0, 1.0));
  vec3d vec = mix( vec3d{1.0, 1.0, 1.0}, vec3d{0.0, 0.0, 0.0}, 1.0);
  printf(" MIX : %f\n", vec.x);
  vec4d out_c = vec4d(mix(vec3d{1.0, 1.0, 1.0}, {0,0,0}, 1.0), 1.0);
  printf(" MIX : %f %f %f %f\n", out_c.x, out_c.y, out_c.z, out_c.w);
  out_c.x = 2.0; out_c.y = 0.0; out_c.z = 0.0; out_c.w = 0.0;
  printf(" UI  : %08x\n", ui_argb(out_c));

  for(unsigned pix_x = 0; pix_x < bf_w; pix_x += 1) { /* pixels */
   for(unsigned pix_y = 0; pix_y < bf_h; pix_y += 1) {
    double    x   = 2.0 * pix_x / (double)bf_w - 1.0;
    double    y   = 2.0 * pix_y / (double)bf_h - 1.0;
    pt2d      p   = {x,y};
    pt2d      v0, v1;
    double    d2;
    double    t = 0.0; /* pure dst color */
    std::vector<std::reference_wrapper<std::vector<pt2d>>> ref_polygons;

    /* get dst color */
    rgb_ui(aux_raster_getpix(pix_x, pix_y, pbf), dst_c);

    /* vertex */
	for(std::vector<pt2d> &vertices : polygons) { /* all polygons */
     for(auto &v : vertices) { /* vertices */
	  d2 = distance_sq(p, v);
      if(d2 < r_2) { /* its a vertex pixel */
       src_c = c_v;
       t     = 1.0; /* pure src color */
       goto l_mix_colour;
      }
     }
    }

    /* is pixel inside a polygon? */
    for(auto &polygon :  polygons) {
     if( pnpoly(polygon, p) ) { /* https://wrfranklin.org/Research/Short_Notes/pnpoly.html */
      fout_c = vec4d(1.0, c_b);
      goto l_end_pixel;
     }
    }

    /* check every line segment, of every polygon, for the distance */
	t      = sq_dist_all_polygons(polygons, p);
	t      = sqrt(t) / 2.0; /* to normal t, will be within {0, 1} */
	t     *= 20.0; /* intensify RED, because it is not seen on my screen */
    if(t > 0.5)
     t = 0.5;
	src_c  = c_r;
	goto l_mix_colour;

l_mix_colour:
    fout_c = vec4d(1.0, mix(dst_c, src_c, t));
l_end_pixel: ;
    aux_raster_putpix(pix_x, pix_y, ui_argb(fout_c), pbf);
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
