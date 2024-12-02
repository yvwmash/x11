#include <string.h>
#include <stdio.h>

#define GL_GLEXT_PROTOTYPES

#include <GL/gl.h>

#include "aux_gl.h"

/* */
void aux_zero_gl_ctx(aux_gl_ctx *ctx)
{
 memset(ctx, 0, sizeof(aux_gl_ctx));
}

/* */
int aux_gl_destroy_fbo(unsigned *fb)
{
 if(*fb != 0) {
  glDeleteFramebuffers(1, fb);
  *fb = 0;
 }

 return 0;
}

/*
   https://www.khronos.org/registry/OpenGL/extensions/KHR/KHR_debug.txt
*/
void  aux_gl_debug_error_cb(int         source,
                            int         type,
			                unsigned    id,
			                int         severity,
			                int         length,
			                const char *message,
			                const void *userParam )
{
 const char *ps_sour = NULL;
 const char *ps_type = NULL;
 const char *ps_seve = NULL;

 (void)id;
 (void)length;
 (void)userParam;

 switch(source){
  case GL_DEBUG_SOURCE_API:
   ps_sour = "SOURCE_API";
   break;
  case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
   ps_sour = "WINDOW_SYSTEM";
   break;
  case GL_DEBUG_SOURCE_SHADER_COMPILER:
   ps_sour = "SHADER_COMPILER";
   break;
  case GL_DEBUG_SOURCE_THIRD_PARTY:
   ps_sour = "THIRD_PARTY";
   break;
  case GL_DEBUG_SOURCE_APPLICATION:
   ps_sour = "APPLICATION";
   break;
  case GL_DEBUG_SOURCE_OTHER:
   ps_sour = "OTHER";
   break;
 }
 switch(type){
    case GL_DEBUG_TYPE_ERROR:
        ps_type = "ERROR";
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        ps_type = "DEPRECATED_BEHAVIOR";
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        ps_type = "UNDEFINED_BEHAVIOR";
        break;
    case GL_DEBUG_TYPE_PORTABILITY:
        ps_type = "PORTABILITY";
        break;
    case GL_DEBUG_TYPE_PERFORMANCE:
        ps_type = "PERFORMANCE";
        break;
    case GL_DEBUG_TYPE_OTHER:
        ps_type = "OTHER";
        break;
    case GL_DEBUG_TYPE_MARKER:
        ps_type = "MARKER";
        break;
 }
 switch(severity){
    case GL_DEBUG_SEVERITY_LOW:
        ps_seve = "LOW";
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        ps_seve = "MEDIUM";
        break;
    case GL_DEBUG_SEVERITY_HIGH:
        ps_seve = "HIGH";
        break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        ps_seve = "NOTIFICATION";
        break;
 }

 fprintf(stderr, " !* GL(debug callback):\n\t source:%s\n\t type:%s\n\t severity:%s\n\t message:%s\n\n", ps_sour, ps_type, ps_seve, message);
}
