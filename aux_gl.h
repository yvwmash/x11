#ifndef AUX_GL_H
#define AUX_GL_H

#define AUX_GL_PRINT_ERROR  {int e = glGetError();\
                             if(e){\
                              fprintf(stderr, " * aux-egl: %s:%s:%d\n", __FILE__, __func__, __LINE__);\
                              fprintf(stderr, " * aux-egl: GL: %s\n",gluErrorString(e));\
                             }\
}

#define AUX_GL_RETURN_IF_ERROR {int e = glGetError();\
                                if(e){\
                                 fprintf(stderr, " * aux-egl: %s:%s:%d\n", __FILE__, __func__, __LINE__);\
                                 fprintf(stderr, " * aux-egl: GL: %s\n",gluErrorString(e));\
                                 return -1;\
                               }\
}

typedef struct aux_gl_ctx {
 unsigned fbo;
}aux_gl_ctx;

/* */
void aux_zero_gl_ctx(aux_gl_ctx *ctx);

/* */
int aux_gl_destroy_fbo(unsigned *fb);

/* */
int   aux_gl_debug_error_cb(int         source,
                            int         type,
			                unsigned    id,
			                int         severity,
			                int         length,
			                const char *message,
			                const void *userParam);

#endif
