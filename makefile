CC     = cc
AR     = ar
PFLAGS = -D_XOPEN_SOURCE=700 -D_GNU_SOURCE
CFLAGS = -std=c2x -Wall -Wpedantic -Wextra -O0 -ggdb3 -fpic
CFLAGS_DRM = $(shell pkg-config --cflags libdrm)
#CFLAGS_DRM = -I/home/yv/src/git/drm/ -I/home/yv/src/git/drm/include/drm/

# -fsanitize=address,leak,signed-integer-overflow,bounds,bounds-strict,float-cast-overflow,pointer-overflow\

INC_H = aux_xcb_keymap.h aux_xcb.h aux_raster.h aux_raster_color_rgb.h aux_drm.h

DEPS  = $(INC_H) makefile

XCB_PROB = prob_xcb
DRI_PROB = prob_dri

INC     = -I./include $(shell pkg-config --cflags xcb xcb-keysyms xcb-errors xcb-image libdrm)
LIBS_DYNAMIC = -Wl,-Bdynamic $(shell pkg-config --libs x11 x11-xcb xcb-present xcb xcb-keysyms xcb-errors xcb-image xcb-randr)
LIBS_DY_DRM = -Wl,-Bdynamic $(shell pkg-config --libs  libdrm)
#LPATH_DRM   = -L//home/yv/src/git/third-party/lib/x86_64-linux-gnu
#LIBS_DY_DRM = -Wl,-rpath=/home/yv/src/git/third-party/lib/x86_64-linux-gnu -Wl,-Bdynamic -ldrm
INL_SRC =
BIN  = bin/$(XCB_PROB) bin/$(DRI_PROB)
OBJ  = ./build/aux_xcb.o\
	   ./build/aux_raster.o

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

bin/prob_xcb: ./build/aux_raster.o ./build/aux_xcb.o ./build/prob_xcb.o $(OBJ)
	$(CC) $(PFLAGS) $(INC) ./build/prob_xcb.o $(OBJ) -o ./bin/prob_xcb $(CFLAGS) $(LIBS_DYNAMIC)

bin/prob_dri: ./build/aux_raster.o ./build/aux_xcb.o ./build/aux_drm.o ./build/prob_xcb_dri.o $(OBJ)
	$(CC) $(PFLAGS) $(INC) ./build/prob_xcb_dri.o $(OBJ) ./build/aux_drm.o -o ./bin/prob_dri $(CFLAGS) $(LIBS_DY_DRM) $(LIBS_DYNAMIC)

build/prob_xcb.o: prob_xcb.c $(DEPS)
	$(CC) $(PFLAGS) $(INC) -c prob_xcb.c -o ./build/prob_xcb.o  $(CFLAGS)

build/prob_xcb_dri.o: prob_xcb_dri.c $(DEPS)
	$(CC) $(PFLAGS) $(INC) -c prob_xcb_dri.c -o ./build/prob_xcb_dri.o  $(CFLAGS_DRM) $(CFLAGS)

build/aux_xcb.o: aux_xcb.c $(DEPS)
	$(CC) $(PFLAGS) $(INC) -c aux_xcb.c -o ./build/aux_xcb.o  $(CFLAGS)

build/aux_drm.o: aux_drm.c $(DEPS)
	$(CC) $(PFLAGS) $(INC) -c aux_drm.c -o ./build/aux_drm.o  $(CFLAGS)

build/aux_raster.o: aux_raster.c $(DEPS)
	$(CC) $(PFLAGS) $(INC) -c aux_raster.c -o ./build/aux_raster.o  $(CFLAGS)

FORCE:
