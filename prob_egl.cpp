#include <stdio.h>
#include <stdint.h>

extern "C" {
#include "aux_egl.h"
#include "aux_gl.h"
}

/* ============================================================================================== */

int main(int argc, char *argv[])
{
 int          status = 0;
 int          r      = 0;
 aux_egl_ctx  egl_ctx;

 (void)argc;
 (void)argv;

 r = aux_egl_connect(&egl_ctx);
 if((r != 0) && (r != 3)) {
  status = 1;
  goto l_end_main;
 }

 r = aux_egl_creat_rctx(&egl_ctx, "/dev/dri/card0", NULL);
 if(r != 0) {
  status = 1;
  goto l_end_main;
 }

l_end_main:
 aux_egl_disconnect(&egl_ctx);

 return status;
}
