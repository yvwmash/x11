#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm/drm.h>
#include <drm/drm_fourcc.h>

#include "aux_drm.h"

/* *********************************************************************************** */

/* */
static bool  drm_available() {
 if(drmAvailable() == 0) {
  fprintf(stderr, " ! aux-drm: no DRM available\n");
  return false;
 }
 return true;
}

/* print DRM version */
static void print_version(int fd) {
 drmVersionPtr version = drmGetVersion(fd);

 if (version) {
  printf(" ! aux-drm: DRM version    : %d.%d.%d\n", version->version_major, version->version_minor, version->version_patchlevel);
  printf(" ! aux-drm: DRM name       : %s\n",       version->name);
  printf(" ! aux-drm: DRM description: %s\n",       version->desc); 
  drmFreeVersion(version);
 } else {
  fprintf(stderr, " ! aux-drm: DRM version\n");
 }
}

/* set/get caps */
static void init_caps(aux_drm_ctx *ctx) {
 int       res;
 uint64_t  cap;
 int       fd = ctx->fd;

 ctx->b_cap_plains       = true;
 ctx->b_cap_aapis        = true;
 ctx->b_cap_dbuf         = true;
 ctx->b_cap_crtc_evblank = true;

 res = drmSetClientCap(fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
 if(res) {
  ctx->b_cap_plains = false;
 }
 res = drmSetClientCap(fd, DRM_CLIENT_CAP_ATOMIC, 1);
 if(res) {
  ctx->b_cap_aapis = false;
 }
 if (drmGetCap(fd, DRM_CAP_DUMB_BUFFER, &cap) < 0 || !cap) {
  ctx->b_cap_dbuf = false;
 }
 if (drmGetCap(fd, DRM_CAP_CRTC_IN_VBLANK_EVENT, &cap) < 0 || !cap) {
  ctx->b_cap_crtc_evblank = false;
 }

 printf(" ! aux-drm:  DRM_CLIENT_CAP_UNIVERSAL_PLANES: %d\n", ctx->b_cap_plains);
 printf(" ! aux-drm:  DRM_CLIENT_CAP_ATOMIC:           %d\n", ctx->b_cap_aapis);
 printf(" ! aux-drm:  DRM_CAP_DUMB_BUFFER:             %d\n", ctx->b_cap_dbuf);
 printf(" ! aux-drm:  DRM_CAP_CRTC_IN_VBLANK_EVENT:    %d\n", ctx->b_cap_crtc_evblank);
}

static bool alloc_mem(aux_drm_ctx *ctx, drmModeRes const * const vres, drmModePlaneRes const * const vplanes) {
 bool                 ret      = true;
 unsigned             save_n_crtcs = ctx->n_crtc;
 unsigned             save_n_pln   = ctx->n_pln;
 unsigned             save_n_enc   = ctx->n_enc;
 unsigned             save_n_con   = ctx->n_con;

 ctx->n_crtc = vres->count_crtcs;
 ctx->n_con  = vres->count_connectors;
 ctx->n_enc  = vres->count_encoders;
 ctx->n_pln  = vplanes->count_planes;
 ctx->n_fb   = vplanes->count_planes;

 if(NULL != ctx->vcrtc) {
  memset(ctx->vcrtc, 0, ctx->n_crtc * sizeof(drmModeCrtc));
 }
 if(NULL != ctx->vqq) {
  memset((void*)ctx->vqq, 0, ctx->n_crtc * sizeof(bool));
 }
 if(NULL != ctx->vcon) {
  memset(ctx->vcon , 0, ctx->n_con  * sizeof(drmModeConnector));
 }
 if(NULL != ctx->vdpms) {
  memset(ctx->vdpms, 0, ctx->n_con  * sizeof(unsigned));
 }
 if(NULL != ctx->venc) {
  memset(ctx->venc,  0, ctx->n_enc  * sizeof(drmModeEncoder));
 }
 if(NULL != ctx->vpln) {
  memset(ctx->vpln,  0, ctx->n_pln  * sizeof(drmModePlane));
 }
 if(NULL != ctx->vfb) {
  memset(ctx->vfb,   0, ctx->n_pln  * sizeof(drmModeFB2));
 }

 if((ctx->n_crtc < save_n_crtcs) 
     && (ctx->n_con < save_n_con) 
     && (ctx->n_enc < save_n_enc) 
     && (ctx->n_pln < save_n_pln))
 {
  goto end_alloc_mem;
 }

 if(ctx->n_con > save_n_con) {
  free(ctx->vcon);  ctx->vcon  = NULL;
  free(ctx->vdpms); ctx->vdpms = NULL;
  ctx->vcon = malloc(ctx->n_con * sizeof(drmModeConnector));
  if(NULL == ctx->vcon) {
   ret = false;
   goto end_alloc_mem;
  }
  ctx->vdpms = malloc(ctx->n_con * sizeof(int));
  if(NULL == ctx->vdpms) {
   ret = false;
   goto end_alloc_mem;
  }
 }

 if(ctx->n_enc > save_n_enc) {
  free(ctx->venc); ctx->venc = NULL;
  ctx->venc = malloc(ctx->n_enc * sizeof(drmModeEncoder));
  if(NULL == ctx->venc) {
   ret = false;
   goto end_alloc_mem;
  }
 }

 if(ctx->n_crtc > save_n_crtcs) {
  free(ctx->vcrtc); ctx->vcrtc = NULL;
  free((void*)ctx->vqq);   ctx->vqq   = NULL;
  ctx->vcrtc = malloc(ctx->n_crtc * sizeof(drmModeCrtc));
  if(NULL == ctx->vcrtc) {
   ret = false;
   goto end_alloc_mem;
  }
  ctx->vqq = malloc(ctx->n_crtc * sizeof(bool));
  if(NULL == ctx->vqq) {
   ret = false;
   goto end_alloc_mem;
  }
 }

 if(ctx->n_pln > save_n_pln) {
  free(ctx->vpln); ctx->vpln = NULL;
  ctx->vpln = malloc(ctx->n_pln * sizeof(drmModePlane));
  if(NULL == ctx->vpln) {
   ret = false;
   goto end_alloc_mem;
  }
  free(ctx->vfb); ctx->vfb = NULL;
  ctx->vfb = malloc(ctx->n_pln * sizeof(drmModeFB2));
  if(NULL == ctx->vfb) {
   ret = false;
   goto end_alloc_mem;
  }
 }

end_alloc_mem:
 if(false == ret) {
  ctx->n_crtc = 0;
  ctx->n_con  = 0;
  ctx->n_enc  = 0;
  ctx->n_pln  = 0;
  ctx->n_fb   = 0;
  free(ctx->vcon);
  free(ctx->venc);
  free(ctx->vcrtc);
  free((void*)ctx->vqq);
  free(ctx->vpln);
  free(ctx->vfb);
  free(ctx->vdpms);
  ctx->vcon  = NULL;
  ctx->venc  = NULL;
  ctx->vcrtc = NULL;
  ctx->vqq   = NULL;
  ctx->vpln  = NULL;
  ctx->vfb   = NULL;
  ctx->vdpms = NULL;
  fprintf(stderr,  " * DRM malloc data: %s:%s:%d", __FILE__, __func__, __LINE__);
 }

 return ret;
}

/* */
void  aux_drm_destroy_ctx(aux_drm_ctx *ctx)
{
 if(NULL == ctx) {
  return;
 }

 
}

/* */
static bool do_crtcs(aux_drm_ctx *ctx, drmModeRes const * const vres) {
 bool          ret      = true;   
 int           fd       = ctx->fd;   
 drmModeCrtc  *crtc;
 
 /* loop through all CRTCs */
 for (unsigned i = 0; i < ctx->n_crtc; ++i) {
  crtc = drmModeGetCrtc(fd, vres->crtcs[i]);
  if (NULL == crtc) {
   fprintf(stderr, " * failed to get CRTC {%d:%u}\n", i, vres->crtcs[i]);
   perror(" * failed to get DRM CRTC");
   ctx->n_crtc = i;
   ret = false;
   goto end_do_crtcs;
  }
  *((drmModeCrtc*)ctx->vcrtc + i) = *crtc;
  drmModeFreeCrtc(crtc);
 }

end_do_crtcs:

 return ret;
}

static bool do_con(aux_drm_ctx *ctx, drmModeRes const * const vres) {
 bool                 ret      = true;   
 int                  fd       = ctx->fd;   
 drmModeConnector    *connector;
 drmModePropertyPtr   dpms_prop;
 uint64_t             dpms_value;

 /* loop through all connectors */
 for (unsigned i = 0; i < ctx->n_con; ++i) {
  connector = drmModeGetConnector(fd, vres->connectors[i]);
  if (NULL == connector) {
   fprintf(stderr, " * failed to get connector {%d:%u}\n", i, vres->connectors[i]);
   perror(" * failed to get DRM connector");
   ctx->n_con = i;
   ret = false;
   goto end_do_con;
  }
  *((drmModeConnector*)ctx->vcon + i) = *connector;

  /* check DPMS status */
  dpms_prop  = NULL;
  dpms_value = 0;
  for (int ci = 0; (dpms_value == 0) && (ci < connector->count_props); ci++) {
   dpms_prop = drmModeGetProperty(fd, connector->props[ci]);
   if(NULL == dpms_prop) {
   	continue;
   }
   if (strcmp(dpms_prop->name, "DPMS") == 0) {
    dpms_value = connector->prop_values[ci];
   }
   drmModeFreeProperty(dpms_prop);
  }
  ctx->vdpms[i] = dpms_value; /* on by default */

  drmModeFreeConnector(connector);
 }

end_do_con:
 return ret;
}

static bool do_enc(aux_drm_ctx *ctx, drmModeRes const * const vres) {
 bool                 ret      = true;
 int                  fd       = ctx->fd;
 drmModeEncoder      *encoder;

 /* loop through all encoders */
 for (int i = 0; i < vres->count_encoders; ++i) {
  encoder = drmModeGetEncoder(fd, vres->encoders[i]);
  if (NULL == encoder) {
   fprintf(stderr, " * failed to get encoder {%d:%u}\n", i, vres->encoders[i]);
   perror(" * failed to get DRM encoder");
   ctx->n_enc = i;
   ret = false;
   goto end_do_enc;
  }
  *((drmModeEncoder*)ctx->venc + i) = *encoder;
  drmModeFreeEncoder(encoder);
 }

end_do_enc:

 return ret;
}

static bool do_pln_and_fb(aux_drm_ctx *ctx, drmModePlaneRes const * const vplanes) {
 bool                 ret      = true;   
 int                  fd       = ctx->fd;
 drmModePlane        *plane;
 drmModeFB2          *fb;

 /* loop through all planes */
 for (unsigned i = 0; i < vplanes->count_planes; ++i) {
  plane = drmModeGetPlane(fd, vplanes->planes[i]);
  if (NULL == plane) {
   fprintf(stderr, " * failed to get plane {%d:%u}\n", i, vplanes->planes[i]);
   perror(" * failed to get DRM plane");
   ctx->n_pln = i;
   ctx->n_fb  = i;
   ret = false;
   goto end_do_pln;
  }
   *((drmModePlane*)ctx->vpln + i) = *plane;
  if(0 != plane->fb_id) {
   fb = drmModeGetFB2(fd, plane->fb_id);
   if (NULL == fb) {
    fprintf(stderr, " * failed to get FB {%d:%u}\n", i, plane->fb_id);
    perror(" * failed to get DRM FB");
    ret = false;
   }
   *((drmModeFB2*)ctx->vfb + i) = *fb;
   drmModeFreeFB2(fb);
  }
  drmModeFreePlane(plane);
 }

end_do_pln:

 return ret;
}

static drmModeEncoder      * get_encoder_id(aux_drm_ctx *ctx, unsigned id) {
 drmModeEncoder *p;

 for(unsigned i = 0; i < ctx->n_enc; ++i) {
  p = (drmModeEncoder*)ctx->venc + i;
  if(p->encoder_id == id) {
   return p;
  }
 }
 
 return NULL;
}

static drmModeConnector * get_connector_id(aux_drm_ctx *ctx, unsigned id) {
 drmModeConnector *p;

 for(unsigned i = 0; i < ctx->n_con; ++i) {
  p = (drmModeConnector*)ctx->vcon + i;
  if(p->connector_id == id) {
   return p;
  }
 }
 
 return NULL;
}

static drmModeCrtc      * get_crtc_id(aux_drm_ctx *ctx, unsigned id) {
 drmModeCrtc *p;

 for(unsigned i = 0; i < ctx->n_crtc; ++i) {
  p = (drmModeCrtc*)ctx->vcrtc + i;
  if(p->crtc_id == id) {
   return p;
  }
 }
 
 return NULL;
}

/* */
static int get_crtc_idx(aux_drm_ctx *ctx, drmModeCrtc *crtc) {
 drmModeCrtc *p;

 for(unsigned i = 0; i < ctx->n_crtc; ++i) {
  p = (drmModeCrtc*)ctx->vcrtc + i;
  if((0 != p->crtc_id) && (crtc->crtc_id == p->crtc_id)) {
   return i;
  }
 }

 return -1;
}

static int get_con_idx(aux_drm_ctx *ctx, drmModeConnector *con) {
 drmModeConnector *p;

 for(unsigned i = 0; i < ctx->n_con; ++i) {
  p = (drmModeConnector*)ctx->vcon + i;
  if((0 != p->connector_id) && (con->connector_id == p->connector_id)) {
   return i;
  }
 }

 return -1;
}

static int get_enc_idx(aux_drm_ctx *ctx, drmModeEncoder *enc) {
 drmModeEncoder *p;

 for(unsigned i = 0; i < ctx->n_enc; ++i) {
  p = (drmModeEncoder*)ctx->venc + i;
  if((0 != p->encoder_id) && (enc->encoder_id == p->encoder_id)) {
   return i;
  }
 }

 return -1;
}

/* *********************************************************************************** */

/* open DRM fd */
int  aux_drm_open_fd(const char *nm, aux_drm_ctx *ctx) {
 char cbuf[200];
 int  fd;

 (void)get_enc_idx;
 (void)get_con_idx;
 (void)get_crtc_idx;

 snprintf(cbuf, 200, "/dev/dri/%s", nm);

 fd = open(cbuf, O_RDWR);
 if(fd < 0) {
  fprintf(stderr, " * aux-drm: open(%s): %s", cbuf, strerror(errno));
  return fd;
 }

 ctx->fd = fd;
 return fd;
}

/* */
void  aux_drm_zero_ctx(aux_drm_ctx *ctx) {
 memset(ctx, 0, sizeof(aux_drm_ctx));
 ctx->fd            = -1;  /*  */
}

/* */
bool aux_drm_init_ctx(aux_drm_ctx *ctx) {
 bool              ret = true;
 int               fd  = ctx->fd;
 drmModeRes       *vres    = NULL;
 drmModePlaneRes  *vplanes = NULL;

 if(false == drm_available()) {
  return false;
 }

 print_version(fd);
 init_caps(ctx);

 vres = drmModeGetResources(fd);
 if(NULL == vres) {
  fprintf(stderr, " * failed to get DRM resources : %s:%s:%d\n", __FILE__, __func__, __LINE__);
  perror(" * failed to get DRM resources");
  ret = false;
  goto end_drm_init_ctx;
 }
 vplanes = drmModeGetPlaneResources(fd);
 if(NULL == vplanes) {
  fprintf(stderr, " * failed to get DRM plane resources : %s:%s:%d\n", __FILE__, __func__, __LINE__);
  perror(" * failed to get DRM plane resources");
  ret = false;
  goto end_drm_init_ctx;
 }

 ret = alloc_mem(ctx, vres, vplanes);
 if(false == ret) {
  goto end_drm_init_ctx;
 }

 /* set DPMS to defaults */
 for(unsigned i = 0; i < ctx->n_con; ++i) {
  ctx->vdpms[i] = AUX_DRM_CONNECTOR_DPMS_MODE_OFF;
 }

 if(false == do_crtcs(ctx, vres)) {
  fprintf(stderr, " ! aux-drm: CRTCs are possibly partially filled\n");
 }
 if(false == do_con(ctx, vres)) {
  fprintf(stderr, " ! aux-drm: connectors are possibly partially filled\n");
 }
 if(false == do_enc(ctx, vres)) {
  fprintf(stderr, " ! aux-drm: encoders are possibly partially filled\n");
 }
 if(false == do_pln_and_fb(ctx, vplanes)) {
  fprintf(stderr, " ! aux-drm: planes are possibly partially filled\n");
 }

end_drm_init_ctx:
 if(vres) {
  drmModeFreeResources(vres);
 }
 if(vplanes) {
  drmModeFreePlaneResources(vplanes);
 }

 return ret;
}

/* */
void aux_drm_print_ctx(aux_drm_ctx *ctx) {
 const char          *ctyp0, *ctyp1, *ctyp2;
 drmModeFB2          *fb2;
 drmModeCrtc         *crtc;
 drmModeConnector    *connector;
 drmModeEncoder      *encoder;
 drmModePlane        *plane;
 unsigned            ne;

 char *cstatus[] = {
  "not initialized",
  "connected",
  "disconnected",
  "unknown",
 };
 char *cDPMS[] = {
  "on",
  "standby",
  "suspend",
  "off",
 };
 char *cenc[] = {
 "none",
 "DAC",
 "TMDS",
 "LVDS",
 "TVDAC",
 "VIRTUAL",
 "DSI",
 "DPMST",
 "DPI",
 };

 (void)plane;
 (void)get_connector_id;

 putchar('\n');
 printf(" ! aux-drm statistics\n");
 putchar('\n');
 printf(" ! aux-drm: # of CRTCs:      %d\n", ctx->n_crtc);
 printf(" ! aux-drm: # of connectors: %d\n", ctx->n_con);
 printf(" ! aux-drm: # of encoders:   %d\n", ctx->n_enc);
 printf(" ! aux-drm: # of plains:     %d\n", ctx->n_pln);
 printf(" ! aux-drm: # of fb's:       %d\n", ctx->n_fb);
 putchar('\n');

#define AUX_DRM_S_NVL(s) if(NULL == (s)) s = "NULL"

 /* list connectors */
 for(unsigned i = 0; i < ctx->n_con; ++i) {
  connector = (drmModeConnector*)ctx->vcon + i;
  ctyp0     = drmModeGetConnectorTypeName(connector->connector_type);
  ctyp1     = cstatus[connector->connection];
  ctyp2     = cDPMS[ctx->vdpms[i]];
  ne        = connector->count_encoders;
  AUX_DRM_S_NVL(ctyp0);

  printf(" ! aux-drm:  connector {%03u}, type: %-10s, status: %-12s, DPMS: %-8s, # encoders: %02u\n", connector->connector_id, ctyp0, ctyp1, ctyp2, ne);
  if(connector->connection == AUX_DRM_CONNECTOR_STATUS_DISCONNECTED) {
   continue;
  }

  /* encoder connected to */
  encoder = get_encoder_id(ctx, connector->encoder_id);
  if(NULL == encoder) {
   printf(" ! \taux-drm:  no encoder connected\n");
   continue;
  }
  ctyp0   = cenc[encoder->encoder_type];
  printf(" ! \taux-drm:  encoder   {%03u}, type: %-8s, CRTC ID: %03u\n", encoder->encoder_id, ctyp0, encoder->crtc_id);

  /* CRTC used */
  crtc = get_crtc_id(ctx, encoder->crtc_id);
  if(NULL == crtc) {
   printf(" ! \t\taux-drm:  no CRTC used\n");
   continue;
  }
  printf(" ! \t\taux-drm:  CRTC {%03u}, mode_valid: %d, buffer_id: %03u, mode: %dx%d@%dHz\n", crtc->crtc_id, crtc->mode_valid, crtc->buffer_id, crtc->width, crtc->height, crtc->mode.vrefresh);
  printf(" ! \t\taux-drm:  CRTC {%03u}, position on the framebuffer x-y: {%u,%u}, WxH: {%u,%u}\n", crtc->crtc_id, crtc->x, crtc->y, crtc->width, crtc->height);
  
  /* FB used */
  if(0 == crtc->buffer_id) {
   printf(" ! \t\\ttaux-drm:  no FB used\n");
   continue;
  }
  for(unsigned i = 0; i < ctx->n_fb; ++i) {
   fb2 = (drmModeFB2*)ctx->vfb + i;
   if(fb2->fb_id == crtc->buffer_id) {
   	break;
   }
  }
  if(0 != fb2->fb_id) {
   printf(" ! \t\t\taux-drm:  FB {%03u}, WxH: {%u,%u}\n", fb2->fb_id, fb2->width, fb2->height);
  }else {
   printf(" ! \t\\ttaux-drm:  FB not initialized\n");
  }
 }
 putchar('\n'); /* END list connectors */

 /* list CRTCs */
 for(unsigned i = 0; i < ctx->n_crtc; ++i) {
  uint64_t seq, ns;
  int      r;

  crtc = (drmModeCrtc*)ctx->vcrtc + i;
  printf(" ! aux-drm:  CRTC {%03u}, mode_valid: %d, buffer_id: %03u, mode: %dx%d@%dHz\n", crtc->crtc_id, crtc->mode_valid, crtc->buffer_id, crtc->width, crtc->height, crtc->mode.vrefresh);
  if(crtc->mode_valid && crtc->buffer_id) {
   r = drmCrtcGetSequence(ctx->fd, crtc->crtc_id, &seq, &ns);
   if(r < 0) {
    perror(" * aux-drm: drmCrtcGetSequence(): ");
   }else {
    printf(" ! aux-drm: \t\tframe sequence: %lu, ns: %lu\n", seq, ns);
   }
  }
 }
 putchar('\n'); /* END list CRTCs */

 /* list encoders */
 for(unsigned i = 0; i < ctx->n_enc; ++i) {
  encoder = (drmModeEncoder*)ctx->venc + i;
  ctyp0 = cenc[encoder->encoder_type];
  printf(" ! aux-drm:  encoder   {%03u}, type: %-8s, CRTC ID: %03u\n", encoder->encoder_id, ctyp0, encoder->crtc_id);
 }
 putchar('\n'); /* END list encoders */
}

/* */
bool aux_drm_is_crtc_active_idx(aux_drm_ctx *ctx, unsigned idx) {
 drmModeCrtc *crtc;

 if(ctx->n_crtc <= idx) {
  return false;
 }
 crtc = (drmModeCrtc*)ctx->vcrtc + idx;
 return ((0 != crtc->mode_valid) && (crtc->buffer_id));
}

/* */
bool aux_drm_is_crtc_active_id(aux_drm_ctx *ctx, unsigned id) {
 drmModeCrtc *crtc;

 for(unsigned i = 0; i < ctx->n_crtc; ++i) {
  crtc = (drmModeCrtc*)ctx->vcrtc + i;
  if(crtc->crtc_id == id) {
   return true;
  }
 }

 return false;
}

/* */
bool aux_drm_is_connector_connected_idx(aux_drm_ctx *ctx, unsigned idx) {
 drmModeConnector    *connector;

 if(ctx->n_con <= idx) {
  return false;
 }

 connector = (drmModeConnector*)ctx->vcon + idx;
 return (DRM_MODE_DISCONNECTED != connector->connection);
}

/* */
bool aux_drm_is_connector_connected_id(aux_drm_ctx *ctx, unsigned id) {
 drmModeConnector    *connector;

 for(unsigned i = 0; i < ctx->n_con; ++i) {
  connector = (drmModeConnector*)ctx->vcon + i;
  if(connector->connector_id == id) {
   return (DRM_MODE_DISCONNECTED != connector->connection);
  }
 }

 return false;
}

/* */
int  aux_drm_connector_DPMS_idx(aux_drm_ctx *ctx, unsigned idx) {
 if(ctx->n_con <= idx) {
  return false;
 }

 return ctx->vdpms[idx];
}

/* */
int  aux_drm_connector_DPMS_id (aux_drm_ctx *ctx, unsigned id) {
 drmModeConnector    *connector;

 for(unsigned i = 0; i < ctx->n_con; ++i) {
  connector = (drmModeConnector*)ctx->vcon + i;
  if(connector->connector_id == id) {
   return ctx->vdpms[i];
  }
 }

 return false;
}

int  aux_drm_queue_sq_id(aux_drm_ctx *ctx,
                         uint32_t     crtc_id,
                         uint32_t     flags,
                         uint64_t     sequence,        /* requested sequence to enqueue   */
                         uint64_t    *sequence_queued, /* actual sequence that was queued */
                         uint64_t     user_data)
{
 int r;

 r = drmCrtcQueueSequence(ctx->fd, crtc_id, flags, sequence, sequence_queued, user_data);
 if(r) {
  perror(" * aux-drm: drmCrtcQueueSequence(): ");
 }

 return r;
}

int  aux_drm_queue_sq_idx(aux_drm_ctx *ctx,
                          uint32_t     crtc_idx,
                          uint32_t     flags,
                          uint64_t     sequence,        /* requested sequence to enqueue   */
                          uint64_t    *sequence_queued, /* actual sequence that was queued */
                          uint64_t     user_data)
{
 int r;

 r = drmCrtcQueueSequence(ctx->fd, ((drmModeCrtc*)ctx->vcrtc + crtc_idx)->crtc_id, flags, sequence, sequence_queued, user_data);
 if(r) {
  perror(" * aux-drm: drmCrtcQueueSequence(): ");
 }

 return r;
}

void aux_drm_crtcs_freq(aux_drm_ctx *ctx, unsigned size, int *vrefresh)
{
 drmModeCrtc *crtc;

 for(unsigned i = 0; (i < ctx->n_crtc) && (i < size); ++i) {
  crtc = (drmModeCrtc*)ctx->vcrtc + i;
  vrefresh[i] = crtc->mode.vrefresh;
 }
}

/* *********************************************************************************** */
