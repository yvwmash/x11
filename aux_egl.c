#include <assert.h>
#include <string.h>
#include <stdio.h>

#define EGL_EGLEXT_PROTOTYPES

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglplatform.h>
#include <GL/gl.h>

#include "aux_raster.h"
#include "aux_egl.h"

static PFNEGLQUERYDEVICESEXTPROC        fn_q_devices            = NULL;
static PFNEGLGETPLATFORMDISPLAYEXTPROC  fn_get_platform_display = NULL;
static PFNEGLQUERYDEVICESTRINGEXTPROC   fn_q_device_string      = NULL;

/* */
void aux_zero_egl_ctx(aux_egl_ctx *ctx)
{
 memset(ctx, 0, sizeof(aux_egl_ctx));
}

/* */
static bool search_elist(const char *es, const char *nm) {
 const char *p;
 long        len;

 while((p = strstr(es, nm))) {
  len = strlen(nm);
  if( (p[len] == '\0') || (p[len] == ' ') ) {
   return true;
  }
  es = p + len;
 }

 return false;
}

/* EGL client extensions */
bool aux_egl_has_c_ext(const char *nm)
{
 const char *egl_ext_lst = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
 if(NULL == egl_ext_lst){
  return false;
 }

 return search_elist(egl_ext_lst, nm);
}

/* EGL device extensions */
bool aux_egl_has_de_ext(aux_egl_ctx *ctx, void *dev, const char *nm)
{
 const char *egl_ext_lst = fn_q_device_string(dev, EGL_EXTENSIONS);
 if(NULL == egl_ext_lst){
  return false;
 }

 return search_elist(egl_ext_lst, nm);
}

/* EGL display extensions */
bool aux_egl_has_di_ext(void *dpy, const char *nm)
{
 const char *egl_ext_lst = eglQueryString(dpy, EGL_EXTENSIONS);
 if(NULL == egl_ext_lst){
  return false;
 }

 return search_elist(egl_ext_lst, nm);
}

/* */
int aux_egl_disconnect(aux_egl_ctx *ctx)
{
 printf(" ! aux-egl: disconnect.\n");

 if(NULL == ctx->dpy)
  return 0;

 if(NULL != ctx->rctx) {
  if(0 == eglMakeCurrent(ctx->dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT)) {
   AUX_EGL_PRINT_ERROR
  }
  if(0 == eglDestroyContext(ctx->dpy, ctx->rctx)) {
   AUX_EGL_PRINT_ERROR
  }
 }

 if(0 == eglTerminate(ctx->dpy)) {
  AUX_EGL_PRINT_ERROR
 }

 aux_zero_egl_ctx(ctx);

 return 0;
}

/* */
int   aux_egl_connect(aux_egl_ctx  *ctx)
{
 int         status          = 0;
 int         n_devices_total = 0;
 void       *devices[AUX_EGL_MAX_DEVICES]; /* EGLDeviceEXT */

 static const char *rq_c_ext[] = {
 "EGL_EXT_client_extensions",
 "EGL_EXT_device_query",
 "EGL_EXT_device_enumeration",
 "EGL_EXT_device_base",
 "EGL_EXT_platform_base",
 "EGL_EXT_platform_device",
 NULL,
 };

 static const char *rq_de_ext[] = {
 "EGL_EXT_device_drm",
 NULL,
 };

 static const char *rq_di_ext[] = {
 "EGL_KHR_create_context",
 NULL,
 };

 /* EGL client extensions */
 for(const char **p = rq_c_ext; *p != NULL; ++p) {
  if(false == aux_egl_has_c_ext(*p)) {
   fprintf(stderr, " * aux-egl: one of required CLIENT extensions is missing: %s\n", *p);
   status = 1;
   goto l_end_egl_connect;
  }
 }

 /* at this point client supports below functions */
 fn_q_devices            = eglGetProcAddress("eglQueryDevicesEXT");
 fn_get_platform_display = eglGetProcAddress("eglGetPlatformDisplayEXT");
 fn_q_device_string      = eglGetProcAddress("eglQueryDeviceStringEXT");

 if(NULL == fn_q_devices){
  AUX_EGL_PRINT_ERROR
  status = 2;
  goto l_end_egl_connect;
 }
 if(NULL == fn_get_platform_display){
  AUX_EGL_PRINT_ERROR
  status = 2;
  goto l_end_egl_connect;
 }
 if(NULL == fn_q_device_string){
  AUX_EGL_PRINT_ERROR
  status = 2;
  goto l_end_egl_connect;
 }

 /* GPUs */
 if(0 == fn_q_devices(AUX_EGL_MAX_DEVICES, devices, &n_devices_total)){
  AUX_EGL_PRINT_ERROR
  status = 2;
 }

 /* loop GPUs */
 /* based on https://gitlab.freedesktop.org/mesa/demos/-/blob/master/src/egl/opengl/eglinfo.c */
 {
  void        *dpy;
  const char  *es;
  int          maj, min;

#define SKIP_EGL_DEVICE(i, s) { AUX_EGL_PRINT_ERROR; status = 3; fprintf(stderr, " \t! aux-egl: GPU {%u} skip: %s\n", i, s); goto l_end_loop_gpus; }

  for(int i = 0; i < n_devices_total; ++i) {
   dpy = fn_get_platform_display(EGL_PLATFORM_DEVICE_EXT, devices[i], 0);
   if(EGL_NO_DISPLAY == dpy) {
    SKIP_EGL_DEVICE(i, "can't get platform display")
   }

   printf(" i aux-egl: GPU {%u}:\n", i);
   if (!eglInitialize(dpy, &maj, &min)) {
    SKIP_EGL_DEVICE(i, "eglInitialize()")
   }

   printf("\t i aux-egl: EGL API version:    %d.%d\n", maj, min);
   printf("\t i aux-egl: EGL vendor string:  %s\n", eglQueryString(dpy, EGL_VENDOR));
   printf("\t i aux-egl: EGL version string: %s\n", eglQueryString(dpy, EGL_VERSION));
   printf("\t i aux-egl: EGL client APIs:    %s\n", eglQueryString(dpy, EGL_CLIENT_APIS));

   /* EGL device extensions */
   es = fn_q_device_string(devices[i], EGL_EXTENSIONS);
   if(NULL == es) {
    SKIP_EGL_DEVICE(i, "no device extensions")
   }
   if(true == aux_egl_has_di_ext(dpy, "EGL_MESA_query_driver")) {
    PFNEGLGETDISPLAYDRIVERNAMEPROC get_disp_driver_nm = (PFNEGLGETDISPLAYDRIVERNAMEPROC)eglGetProcAddress("eglGetDisplayDriverName");
    if(NULL != get_disp_driver_nm) {
     printf("\t i aux-drm: EGL driver name:    %s\n", get_disp_driver_nm(dpy));
    }
   }
   for(const char **p = rq_de_ext; *p != NULL; ++p) {
    if(false == aux_egl_has_de_ext(ctx, devices[i], *p)) {
     SKIP_EGL_DEVICE(i, "one of required DEVICE extensions is missing")
    }
   }

   /* device file */
   es = fn_q_device_string(devices[i], EGL_DRM_DEVICE_FILE_EXT);
   if(NULL == es) {
    SKIP_EGL_DEVICE(i, "no device file mapped")
   }
   printf("\t i aux-egl: EGL device file:    %s\n", es);

   /* display extensions */
   for(const char **p = rq_di_ext; *p != NULL; ++p) {
    if(false == aux_egl_has_di_ext(dpy, *p)) {
     SKIP_EGL_DEVICE(i, "one of required DISPLAY extensions is missing")
    }
   }

   /* save */
   ctx->devices[ctx->n_devices].ph = devices[i];
   snprintf(ctx->devices[ctx->n_devices].path, AUX_EGL_SZ_DEV_PATH, "%s", es);
   ctx->n_devices += 1;

l_end_loop_gpus:
   if(EGL_NO_DISPLAY != dpy) {
    eglTerminate(dpy);
   }
  }
 }

l_end_egl_connect:
 return status;
}

/* */
int aux_egl_creat_rctx(aux_egl_ctx  *ctx, const char *drm_fn_path, int config[])
{
 int         status  = 0;
 void       *rctx;
 EGLConfig   egl_cfg = NULL;
 void       *dev     = NULL;
 void       *dpy     = NULL;

 /* find DRM device && intialize EGL */
 {
  for(unsigned i = 0; i < ctx->n_devices; ++i) {
   aux_egl_dev *device = &ctx->devices[i];

   if(strcmp(device->path, drm_fn_path) == 0) {
    dev = device->ph;
    break;
   }
  }

  if(NULL == dev) {
   status = 1;
   fprintf(stderr, " * aux-egl: %s:%s:%d\n", __FILE__, __func__, __LINE__);
   fprintf(stderr, " * aux-egl: no compatible device found with a path of: %s\n", drm_fn_path);
   goto l_end_creat_rctx;
  }

  /* init EGL */
  {
   int                              maj, min;
   PFNEGLGETPLATFORMDISPLAYEXTPROC  fn_get_platform_display = eglGetProcAddress("eglGetPlatformDisplayEXT");

   dpy = fn_get_platform_display(EGL_PLATFORM_DEVICE_EXT, dev, 0);
   if(EGL_NO_DISPLAY == dpy) {
	status = 2;
    AUX_EGL_PRINT_ERROR
    goto l_end_creat_rctx;
   }

   if (!eglInitialize(dpy, &maj, &min)) {
	status = 2;
    AUX_EGL_PRINT_ERROR
    goto l_end_creat_rctx;
   }
  }
 }

 { /* choose egl config */
  int        egl_config[] = { EGL_SURFACE_TYPE,     0,
                              EGL_CONFORMANT,       EGL_OPENGL_BIT,
                              EGL_RENDERABLE_TYPE,  EGL_OPENGL_BIT,
                              EGL_BLUE_SIZE,        0,
                              EGL_GREEN_SIZE,       0,
                              EGL_RED_SIZE,         0,
                              EGL_DEPTH_SIZE,       0,
                              EGL_NONE, EGL_NONE,
  };
  int        n_configs;
  EGLConfig  egl_cfgs[1];

  /* get avaible configs that match first criteria */
  if((0 == eglChooseConfig(dpy, egl_config, egl_cfgs, 1, &n_configs))){
   AUX_EGL_PRINT_ERROR
   status = 2;
   goto l_end_creat_rctx;
  }
  if(0 == n_configs){
   fprintf(stderr, " * yma-egl: %s:%s:%d\n", __FILE__, __func__, __LINE__);
   fprintf(stderr, " * yma-egl: zero configs matched.\n");
   status = 2;
   goto l_end_creat_rctx;
  }

  printf(" ! \t# of context configs       : %d\n", n_configs);
  printf("\n");

  /* printf configs chosen */
  {
   int v = 0;
   printf(" ! - EGL configs chosen: \n");
   for(int i = 0; i < n_configs; ++i){
    eglGetConfigAttrib(dpy, egl_cfgs[i], EGL_CONFIG_ID, &v);
    printf(" ! EGL_CONFIG_ID : %d\n", v);
    eglGetConfigAttrib(dpy, egl_cfgs[i], EGL_SAMPLES, &v);
    printf(" ! EGL_SAMPLES      : %d\n", v);
    eglGetConfigAttrib(dpy, egl_cfgs[i], EGL_DEPTH_SIZE, &v);
    printf(" ! EGL_DEPTH_SIZE   : %d\n", v);
    eglGetConfigAttrib(dpy, egl_cfgs[i], EGL_STENCIL_SIZE, &v);
    printf(" ! EGL_STENCIL_SIZE : %d\n", v);
    printf("\n");
   }

   egl_cfg = egl_cfgs[0];
  }
 }

  /* create OpenGL context */
  {
   int  flags = 0;
   int  mask  = 0;
   int  need_core = 0;
   int  need_dbg  = 0;
   int  vmaj      = 1;
   int  vmin      = 0;
   int  attribs[40];
   int  attrib_indx = 0;

   /* parse config array */
   if(config) {
    for(int *cfg = config; *cfg; cfg += 2){
     switch(*cfg){
      case AUX_EGL_CTX_CONFIG_COMPAT_FLG:
       need_core = !cfg[1];
       break;
      case AUX_EGL_CTX_CONFIG_DBG:
       need_dbg = cfg[1];
       break;
      case AUX_EGL_CTX_CONFIG_MAJ_VER:
       vmaj = cfg[1];
       break;
      case AUX_EGL_CTX_CONFIG_MIN_VER:
       vmin = cfg[1];
       break;
      default:
       break;
     }
    }
   } else {
    need_core = true;
    need_dbg  = true;
    vmaj      = 4;
    vmin      = 6;
   }

   /* assign values */
   if(need_core) {
    mask |= EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR;
   }else {
    mask |= EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT;
   }
   if(need_dbg) {
    flags |= EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR;
   }

   /* assign API to the draw context */
   if(0 == eglBindAPI(EGL_OPENGL_API)) {
    AUX_EGL_PRINT_ERROR
    status = 3;
    goto l_end_creat_rctx;
   }

   #define EGL_CTX_ADD_ATTRIBUTE(a, v) {attribs[attrib_indx] = a; \
                                        attrib_indx += 1;         \
                                        attribs[attrib_indx] = v; \
                                        attrib_indx += 1; }

   /* add attributes to an array */
   if(mask)
    EGL_CTX_ADD_ATTRIBUTE(EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR, mask)
   if(flags)
    EGL_CTX_ADD_ATTRIBUTE(EGL_CONTEXT_FLAGS_KHR,               flags)
   if(vmaj)
    EGL_CTX_ADD_ATTRIBUTE(EGL_CONTEXT_MAJOR_VERSION_KHR,       vmaj)
   if(vmin)
    EGL_CTX_ADD_ATTRIBUTE(EGL_CONTEXT_MINOR_VERSION_KHR,       vmin)

   EGL_CTX_ADD_ATTRIBUTE(EGL_NONE, EGL_NONE)

   /* actual context creation */
   /*                           display   config   share           attributes */
   rctx = eglCreateContext(dpy, egl_cfg, EGL_NO_CONTEXT, attribs);
   if(EGL_NO_CONTEXT == rctx){
    AUX_EGL_PRINT_ERROR
    status = 4;
    goto l_end_creat_rctx;
   }
  } /* end create context */

 /* make current */
 if(0 == eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, rctx)) {
  AUX_EGL_PRINT_ERROR
  status = 3;
  goto l_end_creat_rctx;
 }

 /* endianess and byte order of a color unit(pixel) */
 #define IS_BIG_ENDIAN (!*(unsigned char *)&(uint16_t){1})
 if(config) { /* parse only for color component masks */
  int      *p;
  uint32_t  v;
  uint32_t  red_mask = 0, gre_mask = 0, blu_mask = 0;

  for(p = config; *p; p += 2) {
   v = (uint32_t)p[1];

   switch(*p) {
   case AUX_EGL_CTX_CONFIG_X11_RED_MASK : {
    red_mask = v;
   }
    break;
   case AUX_EGL_CTX_CONFIG_X11_GRE_MASK : {
    gre_mask = v;
   }
    break;
   case AUX_EGL_CTX_CONFIG_X11_BLU_MASK : {
    blu_mask = v;
   }
    break;
   default:
    break;
   }
  }

  if(red_mask && gre_mask && blu_mask) {
   if((red_mask & 0x00ff0000) && (gre_mask & 0x0000ff00) && (blu_mask & 0x000000ff)) { /* X11: ARGB */
    if(IS_BIG_ENDIAN) {
     /* just test it */
    }else { /* "little endian machine" */
	 /* X11 color unit     : ARGB            */
     /* machine byte order : "little endian" */
     /* OpenGL internal    : RGBA            */
     /* To X11(host) color unit: RGBA -> BGRA -> REV = ARGB */
     ctx->host_pixel_format = AUX_EGL_HOST_PIXEL_FORMAT_ARGB;
     ctx->gl_pixel_format   = GL_RGBA;
     ctx->gl_pixel_type     = GL_UNSIGNED_INT_8_8_8_8;
    }
   }
  }else { /* default to X11_ARGB, machine "little endian", OpenGL RGBA */
   /* same, just test it */
  }
 }

 { /* GL context information */
  int            f0   = 0,  f1   = 0, v = 0;
  int            vmaj = -1, vmin = -1;
  unsigned char  b = 0;

  printf(" ! aux-egl: GL context created.\n");
  glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &f0);
  glGetIntegerv(GL_CONTEXT_FLAGS, &f1);
  glGetIntegerv(GL_MAJOR_VERSION, &vmaj);
  glGetIntegerv(GL_MINOR_VERSION, &vmin);
  glGetBooleanv(GL_DOUBLEBUFFER, &b);
  printf(" ! \tGL_VERSION_STRING : %s\n", glGetString(GL_VERSION));
  printf(" ! \tGL_VERSION_NUMBERS: %d.%d\n", vmaj, vmin);
  printf(" ! \tGL_RENDERER       : %s\n", glGetString(GL_RENDERER));
  printf(" ! \tGL_VENDOR         : %s\n", glGetString(GL_VENDOR));
  if(f0 & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT){
   printf(" ! \tGL_PROFILE        : %s\n", "GL_CONTEXT_COMPATIBILITY_PROFILE_BIT");
  }else {
   printf(" ! \tGL_PROFILE        : %s\n", "GL_CONTEXT_CORE_PROFILE_BIT");
  }
  if(f1 & GL_CONTEXT_FLAG_DEBUG_BIT){
   printf(" ! \tGL_CONTEXT_FLAGS  : %s\n", "GL_CONTEXT_FLAG_DEBUG_BIT");
  }else {
   printf(" ! \tGL_CONTEXT_FLAGS  : %s\n", "not a debug context.");
  }
  if(f1 & GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT) {
   printf(" ! \tGL_CONTEXT_FLAGS  : %s\n", "GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT(out of bounds shader memory access)");
  } else {
   printf(" ! \tGL_CONTEXT_FLAGS  : %s\n", "shaders potentionally can access out of bounds memory.");
  }
  printf(" ! \t\tGL_DOUBLEBUFFER                            : %hhu\n", b);
  glGetIntegerv(GL_MAX_ELEMENTS_INDICES, &v);
  printf(" ! \t\tGL_MAX_ELEMENTS_INDICES                    : %d /* %s */ \n", v, "the recommended maximum number of vertex array indices");
  glGetIntegerv(GL_MAX_ELEMENTS_VERTICES, &v);
  printf(" ! \t\tGL_MAX_ELEMENTS_VERTICES                   : %d /* %s */ \n", v, "recommended maximum number of vertex array vertices");
  glGetIntegerv(GL_MAX_FRAMEBUFFER_WIDTH, &v);
  printf(" ! \t\tGL_MAX_FRAMEBUFFER_WIDTH                   : %d\n", v);
  glGetIntegerv(GL_MAX_FRAMEBUFFER_HEIGHT, &v);
  printf(" ! \t\tGL_MAX_FRAMEBUFFER_HEIGHT                  : %d\n", v);
  glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &v);
  printf(" ! \t\tGL_MAX_RENDERBUFFER_SIZE                   : %d\n", v);
  glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &v);
  printf(" ! \t\tGL_MAX_TEXTURE_BUFFER_SIZE                 : %d\n", v);
  glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &v);
  printf(" ! \t\tGL_MAX_UNIFORM_BUFFER_BINDINGS             : %d\n", v);
  glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &v);
  printf(" ! \t\tGL_MAX_UNIFORM_BLOCK_SIZE                  : %d\n", v);
  glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &v);
  printf(" ! \t\tGL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT  : %d\n", v);

  printf("\n\n");
 }


l_end_creat_rctx:
 if((NULL != dpy) && (NULL != rctx)) {
  ctx->dpy  = dpy;
  ctx->rctx = rctx;
 }

 return status;
}


#define ERROR_DESC(...) fprintf(stderr, " * aux-egl: %s\n", __VA_ARGS__); break

/* the descriptions are taken from the eglGetError manual */
void aux_egl_print_error(void)
{
 switch(eglGetError()) {
  case(EGL_SUCCESS):
    ERROR_DESC("The last function succeeded without error.");
  case(EGL_NOT_INITIALIZED):
    ERROR_DESC("EGL is not initialized, or could not be initialized, for the specified EGL display connection.");
  case(EGL_BAD_ACCESS):
    ERROR_DESC("EGL cannot access a requested resource (for example a context is bound in another thread).");
  case(EGL_BAD_ALLOC):
    ERROR_DESC("EGL failed to allocate resources for the requested operation.");
  case(EGL_BAD_ATTRIBUTE):
    ERROR_DESC("An unrecognized attribute or attribute value was passed in the attribute list.");
  case(EGL_BAD_CONTEXT):
    ERROR_DESC("An EGLContext argument does not name a valid EGL rendering context.");
  case(EGL_BAD_CONFIG):
    ERROR_DESC("An EGLConfig argument does not name a valid EGL frame buffer configuration.");
  case(EGL_BAD_CURRENT_SURFACE):
    ERROR_DESC("The current surface of the calling thread is a window, pixel buffer or pixmap that is no longer valid.");
  case(EGL_BAD_DISPLAY):
    ERROR_DESC("An EGLDisplay argument does not name a valid EGL display connection.");
  case(EGL_BAD_SURFACE):
    ERROR_DESC("An EGLSurface argument does not name a valid surface (window, pixel buffer or pixmap) configured for GL rendering.");
  case(EGL_BAD_MATCH):
    ERROR_DESC("Arguments are inconsistent (for example, a valid context requires buffers not supplied by a valid surface).");
  case(EGL_BAD_PARAMETER):
    ERROR_DESC("One or more argument values are invalid.");
  case(EGL_BAD_NATIVE_PIXMAP):
    ERROR_DESC("A NativePixmapType argument does not refer to a valid native pixmap.");
  case(EGL_BAD_NATIVE_WINDOW):
    ERROR_DESC("A NativeWindowType argument does not refer to a valid native window.");
  case(EGL_CONTEXT_LOST):
    ERROR_DESC("A power management event has occurred. The application must destroy all contexts and reinitialise OpenGL ES state and objects to continue rendering. ");
  default:
   ERROR_DESC("not defined");
  }
}
