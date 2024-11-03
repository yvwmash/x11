CC     = cc
AR     = ar
PFLAGS = -D_XOPEN_SOURCE=700 -D_GNU_SOURCE
CFLAGS = -std=c2x -Wall -Wpedantic -Wextra -O0 -ggdb -fpic

# -fsanitize=address,leak,signed-integer-overflow,bounds,bounds-strict,float-cast-overflow,pointer-overflow\

INC_H = aux_xcb_keymap.h aux_xcb.h aux_raster.h

DEPS  = $(INC_H) makefile

TARGET  = prob_xcb
INC     = -I./include $(shell pkg-config --cflags xcb xcb-keysyms xcb-errors xcb-image)
LIBS_DYNAMIC    = -Wl,-Bdynamic $(shell pkg-config --libs x11 x11-xcb xcb-present xcb xcb-keysyms xcb-errors xcb-image)
INL_SRC =
BIN  = bin/$(TARGET)
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

bin/prob_xcb: ./build/prob_xcb.o $(OBJ)
	$(CC) $(PFLAGS) $(INC) ./build/prob_xcb.o $(OBJ) -o ./bin/prob_xcb $(CFLAGS) $(LIBS_DYNAMIC)

build/prob_xcb.o: prob_xcb.c $(DEPS)
	$(CC) $(PFLAGS) $(INC) -c prob_xcb.c -o ./build/prob_xcb.o  $(CFLAGS)

build/aux_xcb.o: aux_xcb.c $(DEPS)
	$(CC) $(PFLAGS) $(INC) -c aux_xcb.c -o ./build/aux_xcb.o  $(CFLAGS)

build/aux_raster.o: aux_raster.c $(DEPS)
	$(CC) $(PFLAGS) $(INC) -c aux_raster.c -o ./build/aux_raster.o  $(CFLAGS)

FORCE:
