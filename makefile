CC     = cc
AR     = ar
PFLAGS = -D_XOPEN_SOURCE=700 -D_GNU_SOURCE
CFLAGS = -std=c2x -Wall -Wpedantic -Wextra -Wmissing-prototypes -Wmissing-declarations -O0 -ggdb3 -fpic
CFLAGS_DRM = $(shell pkg-config --cflags libdrm)
CFLAGS_EGL = $(shell pkg-config --cflags egl glu)
CFLAGS_GL  = $(shell pkg-config --cflags gl glu)

# -fsanitize=address,leak,signed-integer-overflow,bounds,bounds-strict,float-cast-overflow,pointer-overflow\

INC_H = aux_xcb_keymap.h aux_xcb.h aux_raster.h aux_raster_color_rgb.h aux_drm.h aux_egl.h
DEPS  = $(INC_H) makefile

XCB_PROB = prob_xcb
DRI_PROB = prob_dri
EGL_PROB = prob_egl

INC     = -I./include $(shell pkg-config --cflags xcb xcb-keysyms xcb-errors xcb-image libdrm)
LIBS_DY_XCB = -Wl,-Bdynamic $(shell pkg-config --libs x11 x11-xcb xcb-present xcb xcb-keysyms xcb-errors xcb-image xcb-randr)
LIBS_DY_DRM = -Wl,-Bdynamic $(shell pkg-config --libs  libdrm)
LIBS_DY_EGL = -Wl,-Bdynamic $(shell pkg-config --libs  egl glu)
LIBS_DY_GL  = -Wl,-Bdynamic $(shell pkg-config --libs  gl glu)

INL_SRC =
BIN  = bin/$(XCB_PROB) bin/$(DRI_PROB) bin/$(EGL_PROB)
OBJ_XCB  = ./build/aux_xcb.o\
	       ./build/aux_raster.o

OBJ_DRM  = ./build/aux_drm.o

OBJ_EGL  = ./build/aux_egl.o\
           ./build/aux_gl.o

all: build

build: mk_dirs $(BIN)

clean: FORCE
	rm -f build/* bin/*

rebuild: FORCE
	rm -f build/* bin/*
	$(MAKE)

mk_dirs: FORCE
	@echo ' - make build directories'
	@mkdir -p bin build
	@echo ' - make "build" and "bin" directories'

# binaries
bin/prob_xcb: $(OBJ_XCB) ./build/prob_xcb.o 
	$(CC) $(PFLAGS) $(INC) ./build/prob_xcb.o $(OBJ_XCB) -o ./bin/prob_xcb $(CFLAGS) $(LIBS_DY_XCB)

bin/prob_dri: $(OBJ_XCB) $(OBJ_DRM) ./build/prob_xcb_dri.o 
	$(CC) $(PFLAGS) $(INC) ./build/prob_xcb_dri.o $(OBJ_XCB) $(OBJ_DRM) -o ./bin/prob_dri $(CFLAGS) $(LIBS_DY_DRM) $(LIBS_DY_XCB)

bin/prob_egl: $(OBJ_XCB) $(OBJ_DRM) $(OBJ_EGL) ./build/prob_egl.o 
	$(CC) $(PFLAGS) $(INC) ./build/prob_egl.o $(OBJ_XCB) $(OBJ_DRM) $(OBJ_EGL) -o ./bin/prob_egl $(CFLAGS) $(CFLAGS_GL) $(CFLAGS_EGL) $(LIBS_DY_DRM) $(LIBS_DY_XCB) $(LIBS_DY_EGL) $(LIBS_DY_GL)

# object files
build/prob_xcb.o: prob_xcb.c $(DEPS)
	$(CC) $(PFLAGS) $(INC) -c prob_xcb.c -o ./build/prob_xcb.o  $(CFLAGS)

build/prob_xcb_dri.o: prob_xcb_dri.c $(DEPS)
	$(CC) $(PFLAGS) $(INC) -c prob_xcb_dri.c -o ./build/prob_xcb_dri.o  $(CFLAGS_DRM) $(CFLAGS)

build/prob_egl.o: prob_egl.c $(OBJ_EGL) $(DEPS)
	$(CC) $(PFLAGS) $(INC) -c prob_egl.c -o ./build/prob_egl.o  $(CFLAGS_DRM) $(CFLAGS) $(CFLAGS_EGL) $(CFLAGS_GL)

build/aux_xcb.o: aux_xcb.c $(DEPS)
	$(CC) $(PFLAGS) $(INC) -c aux_xcb.c -o ./build/aux_xcb.o  $(CFLAGS)

build/aux_drm.o: aux_drm.c $(DEPS)
	$(CC) $(PFLAGS) $(INC) -c aux_drm.c -o ./build/aux_drm.o  $(CFLAGS)

build/aux_raster.o: aux_raster.c $(DEPS)
	$(CC) $(PFLAGS) $(INC) -c aux_raster.c -o ./build/aux_raster.o  $(CFLAGS)

build/aux_egl.o: aux_egl.c $(DEPS)
	$(CC) $(PFLAGS) $(INC) -c aux_egl.c -o ./build/aux_egl.o  $(CFLAGS) $(CFLAGS_EGL)

build/aux_gl.o: aux_gl.c $(DEPS)
	$(CC) $(PFLAGS) $(INC) -c aux_gl.c -o ./build/aux_gl.o  $(CFLAGS) $(CFLAGS_GL)

FORCE:
