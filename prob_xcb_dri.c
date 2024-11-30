/* drm_info */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/signalfd.h>
#include <sys/epoll.h>

#include "aux_xcb.h"
#include "aux_drm.h"

/* defines for epoll */
#define MAXEVENTS 64
#define SET_EV(_ev,_fd,_events) {\
                                  memset(&_ev, 0, sizeof(struct epoll_event));\
                                  _ev.data.fd = _fd;                          \
                                  _ev.events = _events;                       \
        }

/* global exit flags */
static volatile bool f_win_close        = false; /* exit process => window closed    */
static volatile bool f_exit_sig         = false; /* exit process => signal           */
// static bool f_exit_user  = false; /* exit process => by user request  */
static volatile bool f_win_expose       = false; /* what if i want to assign it in alarm, timer, or other thread? */
static          bool f_display_change   = false; /* */

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
 int64_t          lo_dev, hi_dev, avg;
 unsigned         nlate, nless, lo_late, hi_late;
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
     for(unsigned i = 0; i < (status / sizeof(struct aux_drm_event_crtc_sq)); ++i) {
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
 //~ if(dri_fd > 0) {
  //~ close(dri_fd);
 //~ }
 if(sig_fd != -1){
  close(sig_fd);
 }
 if(ep_fd != -1){
  close(ep_fd);
 }
 if(ep_evs){
  free(ep_evs);
 }

 printf("normal process termination\n");

 return status;
}