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

/* global exit flags */
static volatile bool f_win_close  = false; /* exit process => window closed    */
static volatile bool f_exit_sig   = false; /* exit process => signal           */
// static bool f_exit_user  = false; /* exit process => by user request  */
static volatile bool f_win_expose = false; /* exit process => by user request  */

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

int main(int argc, char *argv[])
{
 int                      status = 0, on = 1;
 int                      kq_fd = -1, x11_fd = -1, mem_fd_imgbuf = -1;
 sigset_t                 mask_osigs = {0};
 struct kevent            kq_evs[5];
 aux_xcb_ctx              xcb_ctx;

 (void)argc;
 (void)argv;

 /* zero XCB context */
 aux_zero_xcb_ctx(&xcb_ctx);

 /* connect XCB, create window */
 if(aux_xcb_connect(&xcb_ctx, ":0", 0) < 0) {
  status = 1;
  goto main_terminate;
 }

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
 if(ioctl(x11_fd, FIONBIO, (char *)&on) < 0) {
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

 /* loop untill exit signal arrives, or until window is closed */
 status = 0;
 while(1) {
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

   (void)flags;
   (void)fflags;

   if(filter == EVFILT_SIGNAL) { /* signal */
    fprintf(stderr, " ! got %s\n", strsignal((int)id));
    f_exit_sig = true;
   }else if((int)id == x11_fd) { /* x11 event */
    aux_xcb_ev_func(&xcb_ctx);
    f_win_close  = xcb_ctx.f_window_should_close;
    f_win_expose = xcb_ctx.f_window_expose;
   }
  } /* finish service kqueue events */

  if(f_win_close) { /* window closed by input */
   goto main_terminate;
  }
  if(f_exit_sig) { /* process needs to stop */
   goto main_terminate;
  }
  if(f_win_expose) { /* window needs update its graphics */
   aux_xcb_flush_front_buf(&xcb_ctx);
  }

 } /* end while(1) */

main_terminate:

 if(aux_xcb_disconnect(&xcb_ctx) < 0) {
  status = 1;
 }
 if (sigprocmask(SIG_SETMASK, &mask_osigs, NULL) < 0) {
  perror(" * sigprocmask(SIG_SETMASK, &mask_osigs, NULL)");
  status = 1;
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
