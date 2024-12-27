#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/event.h>

#include "aux_xcb.h"
#include "aux_drm.h"

/* global exit flags */
static volatile bool f_win_close        = false; /* exit process => window closed    */
static volatile bool f_exit_sig         = false; /* exit process => signal           */
// static bool f_exit_user  = false; /* exit process => by user request  */
static volatile bool f_win_expose       = false; /* what if i want to assign it in alarm, timer, or other thread? */
static          bool f_display_change   = false; /* */

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
  if(kevent(kq_fd, evs, n_signals, NULL, 0,	NULL) < 0) {
   perror(" * kevent(n_signals)");
   status = 4;
   goto l_end_flt_signals;
  }
 }

l_end_flt_signals:
 return status;
}

/* ************************************************************************* */

/* https://gitlab.freedesktop.org/xorg/xserver/-/blame/master/hw/xfree86/drivers/modesetting/vblank.c#L309
   xserver/hw/xfree86/drivers/modesetting/vblank.c:ms_queue_vblank()
   uses drmCrtcGetSequence and drmCrtcQueueSequence if possible.
   fallbacks to drmWaitVBlank. i wont use drmWaitVBlank at all because of _SECONDARY flag is too hassle to guess. and my setup simply fails w/o the flag.

   WM_PROP_WORKAREA breaks eventually, unreliable with my setup.
   DRM resource report breaks eventually. unreliable with my setup. still, what works is iterating through planes to get FBs.
   goal is to know window-on-composite FB position. from that, i can determine a CRTC the window is positioned on.
*/

struct frame {
 uint64_t  rp_sq;
 uint64_t  delivery_sq;
 int64_t   delivery_tm;
};

#define TOP_HZ (100) /* highest monitor refresh rate */

static struct frame vframe[4][TOP_HZ] = {0};
static int          nframe[4]         = {0};
static uint64_t     tmframes[4]       = {0};
static uint64_t     sqframes[4]       = {0};

static void FPS(aux_drm_ctx *ctx) {
 bool             f          = false;
 static bool      once       = true;
 static int       vfreq[4]   = {0};
 int64_t          delta      = 0;
 uint64_t         save_tm    = 0;
 uint64_t         save_sq    = 0;
 uint64_t         total      = 0;
 int64_t          lo_dev, hi_dev, avg, lo_late, hi_late;
 unsigned         nlate, nless;
 struct frame    *p;

 if(once) {
  aux_drm_crtcs_freq(ctx, 4, vfreq);
  once = false;
 }

 for(unsigned i = 0; i < 4; ++i) {
  if(nframe[i] == vfreq[i]) {
   f = true;
  }
 }

 if( false == f ) {
  return;
 }

 for(unsigned i = 0; i < 4; ++i) {
  if(!aux_drm_is_crtc_active_idx(ctx, i)) {
   continue;
  }
  if(nframe[i] != vfreq[i]) {
   continue;
  }

 avg     = 0;
 lo_dev  = 0;
 hi_dev  = 0;
 nlate   = 0;
 nless   = 0;
 lo_late = 0;
 hi_late = 0;

  save_tm = tmframes[i];
  for(int fi = 0; fi < vfreq[i]; ++fi) {
   p        = &vframe[i][fi];
   delta    = p->delivery_tm - save_tm;
   save_tm  = p->delivery_tm;
   total   += delta;
  }

  avg = total / nframe[i];
  save_tm = tmframes[i];
  save_sq = sqframes[i];
  for(int fi = 0; fi < vfreq[i]; ++fi) {
   int64_t t, sq;

   p        = &vframe[i][fi];
   delta    = p->delivery_tm - save_tm;
   save_tm  = p->delivery_tm;
   t        = avg - delta;
   sq       = p->delivery_sq - save_sq;
   save_sq  = p->delivery_sq;
   if(delta < 0) { /* time frame N > time frame N + 1 */
    nless += 1;
   }
   if( (t < 0) && (lo_dev > t) ) {
    lo_dev = t;
   }
   if( (t > 0) && (hi_dev < t) ) {
    hi_dev = t;
   }
   if(sq > 1) { /* late frame */
    nlate += 1;
    if(lo_late > sq) {
     lo_late = sq;
    }
    if(hi_late < sq) {
     hi_late = sq;
    }
   }
  }
  printf(" ! aux-drm: CRTC {%u}: freq: %3d, # frames: %3d, time: %lu\n", i, nframe[i], vfreq[i], total);
  printf(" ! aux-drm: \tavg: %ld, lo_dev: %ld, hi_dev: %ld\n", avg, lo_dev, hi_dev);
  printf(" ! aux-drm: \t# less: %u, # late: %u, lo_late: %u, hi_late: %u\n", nless, nless, lo_late, hi_late);

  nframe[i]   = 0;
  tmframes[i] = save_tm;
  sqframes[i] = save_sq;
 }

}

/* ************************************************************************* */

int main(int argc, char *argv[])
{
 (void)argc;
 (void)argv;

 int                      status = 0;
 int                      on = 1;
 int                      kq_fd = -1, x11_fd = -1, dri_fd = -1;
 sigset_t                 mask_osigs = {0};
 struct kevent            kq_evs[5];
 aux_xcb_ctx              xcb_ctx;
 aux_drm_ctx              drm_ctx;

 /* zero DRM context */
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

 /* zero XCB context */
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

 /* kqueue: add X11 event, add DRM event*/
 {
  struct kevent evs[2];

  EV_SET(&evs[0], x11_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
  EV_SET(&evs[1], dri_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
  if(kevent(kq_fd, evs, 2, NULL, 0,	NULL) < 0) {
   perror(" * kqueue()");
   status = 1;
   goto main_terminate;
  }
 }

 /* enqueue all CRTCs active */
 static uint32_t vcrtc_idx  [4];
 static uint64_t vrep_sq    [4];
 {
  for(unsigned i = 0; i < 4; ++i) {
   if(aux_drm_is_crtc_active_idx(&drm_ctx, i)) {
    vcrtc_idx[i] = i;
    if(0 == aux_drm_queue_sq_idx(&drm_ctx, i, AUX_DRM_CRTC_SEQUENCE_RELATIVE, 1, &vrep_sq[i], (uint64_t)(&vcrtc_idx[i]))) {
     struct frame *p = &vframe[i][nframe[i]];
     p->rp_sq   = vrep_sq[i];
    }
   }
  }
 }

 /* loop untill exit signal arrives, or until window is closed */
 status = 0;
 while(1){
  int        n, i;
  u_short    flags;
  u_int      fflags;
  short      filter;
  uintptr_t  id;

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

   if(flags == EVFILT_SIGNAL) { /* signal */
    fprintf(stderr, " ! got %s\n", strsignal((int)id));
    f_exit_sig = true;
   }else if(id == x11_fd) { /* x11 event */
    aux_xcb_ev_func(&xcb_ctx);
    f_win_close      = xcb_ctx.f_window_should_close;
    f_win_expose     = xcb_ctx.f_window_expose;
    f_display_change = xcb_ctx.f_eq_changed;
   }else if(id == dri_fd) { /* DRM event */
	struct   aux_drm_event_crtc_sq  vev[4];
    ssize_t                         nread;

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
     for(unsigned i = 0; i < (nread / sizeof(struct aux_drm_event_crtc_sq)); ++i) {
      if(vev[i].base.typ == AUX_DRM_EVENT_CRTC_SEQUENCE) {
       uint32_t      crtc_idx = *(uint32_t*)(vev[i].user_data);
       uint32_t      n        = nframe[crtc_idx];
       struct frame *p        = &vframe[crtc_idx][n];

	   /* save starting point */
       if(tmframes[crtc_idx] == 0) {
        tmframes[crtc_idx] = vev[i].time_ns;
        sqframes[crtc_idx] = vev[i].sequence;
        goto l_enqueue_new_sq;
       }

       /* save frame */
       p->delivery_sq    = vev[i].sequence;
       p->delivery_tm    = vev[i].time_ns;
	   nframe[crtc_idx] += 1;

       /* FPS */
       FPS(&drm_ctx);

l_enqueue_new_sq:
       /* enqueue new sequence */
       if(aux_drm_is_crtc_active_idx(&drm_ctx, crtc_idx)) {
        if(0 == aux_drm_queue_sq_idx(&drm_ctx, crtc_idx, AUX_DRM_CRTC_SEQUENCE_RELATIVE, 1, &vrep_sq[crtc_idx], (uint64_t)&vcrtc_idx[crtc_idx])) {
         p->rp_sq = vrep_sq[crtc_idx];
        }
       }

      }else { /* skip */
       puts("DRM got unknown event type");
      }

     }
    }while(nread == (4 * sizeof(struct aux_drm_event_crtc_sq)));
   }
  } /* finish service kqueue events */

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
 if(kq_fd != -1){
  close(kq_fd);
 }

 printf("normal process termination\n");

 return status;
}
