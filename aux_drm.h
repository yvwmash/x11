#ifndef AUX_DRM_H
#define AUX_DRM_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
/* *********************************************************************************** */
#define AUX_DRM_CONNECTOR_STATUS_CONNECTED      1
#define AUX_DRM_CONNECTOR_STATUS_DISCONNECTED   2
#define AUX_DRM_CONNECTOR_STATUS_UNKNOWN        3

#define AUX_DRM_CONNECTOR_DPMS_MODE_ON          0
#define AUX_DRM_CONNECTOR_DPMS_MODE_STANDBY     1
#define AUX_DRM_CONNECTOR_DPMS_MODE_SUSPEND     2
#define AUX_DRM_CONNECTOR_DPMS_MODE_OFF         3

#define AUX_DRM_MODE_ENCODER_NONE	  0
#define AUX_DRM_MODE_ENCODER_DAC	  1
#define AUX_DRM_MODE_ENCODER_TMDS	  2
#define AUX_DRM_MODE_ENCODER_LVDS	  3
#define AUX_DRM_MODE_ENCODER_TVDAC	  4
#define AUX_DRM_MODE_ENCODER_VIRTUAL  5
#define AUX_DRM_MODE_ENCODER_DSI	  6
#define AUX_DRM_MODE_ENCODER_DPMST	  7
#define AUX_DRM_MODE_ENCODER_DPI	  8

/* queue event to be delivered at specified sequence. time stamp marks
 * when the first pixel of the refresh cycle leaves the display engine
 * for the display
 */
#define AUX_DRM_CRTC_SEQUENCE_RELATIVE		    0x00000001	/* sequence is relative to current   */
#define AUX_DRM_CRTC_SEQUENCE_NEXT_ON_MISS		0x00000002	/* use next sequence if we've missed */
/* *********************************************************************************** */

/* same as seen in drm.h */
#define AUX_DRM_EVENT_VBLANK         0x01
#define AUX_DRM_EVENT_FLIP_COMPLETE  0x02
#define AUX_DRM_EVENT_CRTC_SEQUENCE	 0x03

/* same as seen in drm.h */
struct aux_drm_event_base {
 uint32_t  typ; /* event type */
 uint32_t  len; /* total number of payload bytes (including header) */
};

/* same as seen in drm.h
 * event delivered at sequence. time stamp marks when the first pixel
 * of the refresh cycle leaves the display engine for the display
 */
struct aux_drm_event_crtc_sq {
	struct aux_drm_event_base  base;
	uint64_t                   user_data;
	int64_t                    time_ns;
	uint64_t                   sequence;
};

typedef struct aux_drm_ctx {
 int             fd;
 bool            b_cap_plains, b_cap_aapis, b_cap_dbuf, b_cap_crtc_evblank;
 unsigned        n_con, n_enc, n_crtc, n_pln, n_fb;
 void           *venc, *vcon, *vcrtc, *vpln, *vfb;
 unsigned       *vdpms;
 volatile  bool *vqq; /* NB: what was a reason to add vqq? what is it for?  */
}aux_drm_ctx;
/* *********************************************************************************** */

/* */
int   aux_drm_open_fd(const char *nm, aux_drm_ctx *ctx);
void  aux_drm_zero_ctx(aux_drm_ctx *ctx);
bool  aux_drm_init_ctx(aux_drm_ctx *ctx);
void  aux_drm_destroy_ctx(aux_drm_ctx *ctx);

bool aux_drm_is_crtc_active_idx(aux_drm_ctx *ctx, unsigned idx);
bool aux_drm_is_crtc_active_id (aux_drm_ctx *ctx, unsigned id);
bool aux_drm_is_connector_connected_idx(aux_drm_ctx *ctx, unsigned idx);
bool aux_drm_is_connector_connected_id (aux_drm_ctx *ctx, unsigned id);
int  aux_drm_connector_DPMS_idx(aux_drm_ctx *ctx, unsigned idx);
int  aux_drm_connector_DPMS_id (aux_drm_ctx *ctx, unsigned id);

void aux_drm_crtcs_freq(aux_drm_ctx *ctx, unsigned size, int *vrefresh);

int  aux_drm_queue_sq_id(aux_drm_ctx *ctx,
                         uint32_t     crtc_id,
                         uint32_t     flags,
                         uint64_t     sequence,        /* requested sequence to enqueue   */
                         uint64_t    *sequence_queued, /* actual sequence that was queued */
                         uint64_t     user_data);
int  aux_drm_queue_sq_idx(aux_drm_ctx *ctx,
                          uint32_t     crtc_idx,
                          uint32_t     flags,
                          uint64_t     sequence,        /* requested sequence to enqueue   */
                          uint64_t    *sequence_queued, /* actual sequence that was queued */
                          uint64_t     user_data);

void aux_drm_print_ctx(aux_drm_ctx *ctx);

#ifdef __cplusplus
}
#endif

#endif