# x11
x11 protocol probes

The repository aim is to creat a template for a code. By the code I mean some, possibly graphical, application that uses `epoll` syscall to wait for events.

## ** STRUCTURE **

`aux_*` files represent auxiliary modules.

`prob_*` files are templates.

`aux_xcb`    - module that does a communication with X11 by means of XCB. all functionality are done as much **synchronous** as possible. that is done deliberately, to ease debugging.

`aux_raster` - module that carry almost no use, except to creat image buffer that will be sent to X11 for display.

`aux_drm`    - module that wraps libDRM functionality.

`aux_egl`    - module that wraps libEGL.

`aux_gl`     - module that wraps libGL.

===

`prob_xcb`     - simplest application that creates XCB window and listens for events.

`prob_xcb_dri` - same as prob_xcb. new thing is that application enqueues messages for screen refresh updates and gets update events.

`prob_egl`     - enumerate GPUs via EGL. no display server(X11) required. map EGL "devices" to DRM ones. 
                 mapping done through comparison of EGL EGL_DRM_DEVICE_FILE_EXT device string <=> with DRM /dev/dri/card{0} filesystem path.

## ** PREREQUISITES **

tested on Debian Linux Mint.

  > `libx11-dev`, `libx11-xcb-dev`, `libxcb1-dev`, `libxcb-keysyms1-dev`, `libxcb-util-dev`, `libxcb-image0-dev`, `libxcb-present-dev`, `libxcb-randr0-dev`
  
  > `prob_xcb_dri` needs: `libdrm-dev`

## ** COMPILE && INSTALL **

  * `make`
   

## ** RUN **

  * `./bin/<binary name>`
   

## ** LYRICS **

### /* possible source of latency */

 * user => compositing WM => X server => DRM => GPU

### /* possible source of latency */

 * user => compositing WM => X server => compositing WM => X server => DRM => GPU

### /* X11 and GPU driver */

* X11, or GPU driver should distribute and sync a FB to monitors. 

### /* do compositing WM always have one big FB? */

* many FBs with a different scaling and whatelse is left behind. for now.

### /* VBLANK event for a window? */

* it is really **NOT a question** of getting VBLANK for a CRTC. it is **a question**, which VBLANK events will window receive?

### /* window and CRTC interop */

* window <=> CRTC

  - if it moves.
    
  - if it power-offs and switches to another CRTC.
  
  - if the window is visible on many physical monitors.
