#include <stdio.h>

#include "aux_egl.h"
#include "aux_gl.h"

int main(int argc, char *argv[]) 
{
 int          status = 0;

 aux_egl_ctx  egl_ctx;

 (void)argc;
 (void)argv;

 if(aux_egl_connect(&egl_ctx, NULL) < 0) {
  status = 1;
  goto l_end_main;
 }

l_end_main:
 return status;
}