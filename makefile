# to search for dependencies
#     (apt search)|(pkg search) <pattern>
#     example: pkg search xcb-util-*
# to see optimization reports, research into
#     https://clang.llvm.org/docs/UsersManual.html#options-to-emit-optimization-reports

CC     = clang19
CXX    = clang++19
AR     = ar
# lld prefers static libs first then dynamic ones
#     to use static versions use a full filesystem path(/path/to/lib/lib.a), or relative filesystem path(./lib.a)
#     there is no Wl,Bstatic and Wl,Bdynamic for lld
PFLAGS       = -D_XOPEN_SOURCE=700 -D_POSIX_C_SOURCE=202406L -D__BSD_VISIBLE=1
SFLAGS       = -fsanitize=leak,signed-integer-overflow,bounds,float-cast-overflow,pointer-overflow,undefined
CFLAGS_WARN    = -Weverything -Wno-c++98-compat   -Wno-pre-c23-compat      -Wno-unsafe-buffer-usage -Wno-old-style-cast
CXXFLAGS_WARN  = -Weverything -Wno-c++98-compat   -Wno-unsafe-buffer-usage -Wno-old-style-cast
CFLAGS       = -std=c23   -Wall -Wpedantic -Wextra $(CFLAGS_WARN)   -fpic
CXXFLAGS     = -std=c++23 -Wall -Wpedantic -Wextra $(CXXFLAGS_WARN) -fpic
CFLAGS_DBG   = -O0 -ggdb3
CXXFLAGS_DBG = -O0 -ggdb3
CFLAGS_REL   = -O2
CXXFLAGS_REL = -O2
CFLAGS_DRM     != pkgconf --cflags libdrm
CFLAGS_EGL     != pkgconf --cflags egl glu
CFLAGS_GL      != pkgconf --cflags gl glu
CFLAGS_CPU0    !=

# default build type
BUILD    = DEBUG
SANITIZE = ADDRESS

# sanitize flags {address, thread} are incompatible for clang within same module
.if $(SANITIZE) == ADDRESS
SFLAGS   += -fsanitize=address
.else
SFLAGS   += -fsanitize=thread
.endif

# flags based on build type
.if $(BUILD) == DEBUG
    CFLAGS   += $(CFLAGS_DBG) $(SFLAGS)
    CXXFLAGS += $(CXXFLAGS_DBG) $(SFLAGS)
.else
    SFLAGS   = ''
    CFLAGS   += $(CFLAGS_REL)
    CXXFLAGS += $(CXXFLAGS_REL)
.endif

INC_H = aux_xcb_keymap.h\
        aux_xcb.h\
        aux_raster.h\
        aux_raster_color_rgb.h\
        aux_drm.h\
        aux_egl.h\
        vg_algebra/vec2.h\
        vg_algebra/vec3.h\
        vg_algebra/vec4.h\
        vg_algebra/mat4x4.h\
        vg_algebra/pt2.h\
        vg_algebra/pt3.h\
        vg_algebra/frwd.h\
        vg_algebra/fequals.h\
        vg_algebra/segment.h\
        vg_algebra/geometry.h\
        vg_algebra/cpu_glsl.h

DEPS  = $(INC_H) makefile

XCB_PROB = prob_xcb
DRI_PROB = prob_dri
EGL_PROB = prob_egl
CPU_COMPUTE0 = cpu_compute0

INC  !=  pkgconf --cflags xcb xcb-keysyms xcb-errors xcb-image

LIBS_DY_XCB  != pkgconf --libs x11 x11-xcb xcb-present xcb xcb-keysyms xcb-errors xcb-image xcb-randr xcb-dri2 xcb-dri3
LIBS_DY_DRM  != pkgconf --libs libdrm
LIBS_DY_EGL  != pkgconf --libs egl glu
LIBS_DY_GL   != pkgconf --libs gl glu
LIBS_DY_CPU0 !=
LIBS_STA_LEX  = -lfl

INL_SRC =

LEX_YACC_PRODUCED = ./svg/yy.svg_docu.tab.c\
                    ./svg/lex.svg_docu.c

BIN  = $(LEX_YACC_PRODUCED)\
       bin/$(XCB_PROB)\
       bin/$(DRI_PROB)\
       bin/$(EGL_PROB)\
       bin/$(CPU_COMPUTE0)\

OBJ_XCB  = ./build/aux_xcb.o\
	       ./build/aux_raster.o

OBJ_DRM  = ./build/aux_drm.o

OBJ_VG   = ./build/fequals.o

OBJ_EGL  = ./build/aux_egl.o\
           ./build/aux_gl.o

OBJ_CPU_COMPUTE0 = ./build/cpu_compute0.o

OBJ_PARSER = ./build/lex.svg_docu.o\
             ./build/yy.svg_docu.tab.o\
             ./build/aux_svg.o

all: build

build: mk_dirs $(BIN)

clean: FORCE
	rm -f build/* bin/*
	rm ./svg/*.c ./svg/*.output ./svg/*.h

rebuild: FORCE
	rm -f build/* bin/*
	$(MAKE)

mk_dirs: FORCE
	@echo ' - make "build" and "bin" directories'
	@mkdir -p bin build
	@echo ' - make "out" directory'
	@mkdir -p out

# binaries
# link step
bin/prob_xcb: $(OBJ_XCB) ./build/prob_xcb.o
	$(CC) $(SFLAGS) ./build/prob_xcb.o $(OBJ_XCB) -o ./bin/prob_xcb $(LIBS_DY_XCB)

bin/prob_dri: $(OBJ_XCB) $(OBJ_DRM) ./build/prob_xcb_dri.o
	$(CC) $(SFLAGS) ./build/prob_xcb_dri.o $(OBJ_XCB) $(OBJ_DRM) -o ./bin/prob_dri $(LIBS_DY_DRM) $(LIBS_DY_XCB)

bin/prob_egl: $(OBJ_XCB) $(OBJ_DRM) $(OBJ_EGL) ./build/prob_egl.o
	$(CXX) $(SFLAGS) ./build/prob_egl.o $(OBJ_XCB) $(OBJ_DRM) $(OBJ_EGL) -o ./bin/prob_egl $(LIBS_DY_DRM) $(LIBS_DY_XCB) $(LIBS_DY_EGL) $(LIBS_DY_GL)

bin/cpu_compute0: $(OBJ_PARSER) $(OBJ_XCB) $(OBJ_DRM) $(OBJ_VG) $(OBJ_CPU_COMPUTE0)
	$(CXX) $(SFLAGS) $(OBJ_CPU_COMPUTE0) $(OBJ_PARSER) $(OBJ_XCB) $(OBJ_DRM) $(OBJ_VG) -o ./bin/cpu_compute0 $(LIBS_STA_LEX) $(LIBS_DY_DRM) $(LIBS_DY_XCB) $(LIBS_DY_CPU0)

# lexer and parser files
./svg/yy.svg_docu.tab.c: makefile ./svg/svg.y
	@printf "\nlexing && yaccing\n"
	yacc -v -d -t -p yy_svg_docu_ -b yy_svg_docu -o ./svg/yy.svg_docu.tab.c ./svg/svg.y

./svg/lex.svg_docu.c: makefile ./svg/svg.l
	lex --prefix=yy_svg_docu_ -o./svg/lex.svg_docu.c ./svg/svg.l
	@printf "\n"

# object files, lex && yacc produced and SVG related

LEX_YACC_NO_WARN=-Wno-missing-variable-declarations -Wno-unused-macros -Wno-switch-default\
                 -Wno-unused-function -Wno-implicit-fallthrough -Wno-extra-semi-stmt -Wno-reserved-macro-identifier\
                 -Wno-unreachable-code -Wno-padded -Wno-switch-enum

./build/aux_svg.o: aux_svg.h aux_svg.c
	$(CC) $(PFLAGS) -c aux_svg.c -o ./build/aux_svg.o $(CFLAGS)

./build/lex.svg_docu.o: ./svg/lex.svg_docu.c ./svg/yy.svg_docu.tab.h
	$(CC) $(PFLAGS) -DYYDEBUG=1 -c ./svg/lex.svg_docu.c -o ./build/lex.svg_docu.o $(CFLAGS) $(LEX_YACC_NO_WARN)

./build/yy.svg_docu.tab.o: ./svg/yy.svg_docu.tab.c
	$(CC) $(PFLAGS) -DYYDEBUG=1 -c ./svg/yy.svg_docu.tab.c -o ./build/yy.svg_docu.tab.o $(CFLAGS) $(LEX_YACC_NO_WARN)

# object files, "main" files
./build/prob_xcb.o: prob_xcb.c $(DEPS)
	$(CC) $(PFLAGS) $(INC) -c prob_xcb.c -o ./build/prob_xcb.o  $(CFLAGS)

./build/prob_xcb_dri.o: prob_xcb_dri.c $(DEPS)
	$(CC) $(PFLAGS) $(INC) -c prob_xcb_dri.c -o ./build/prob_xcb_dri.o  $(CFLAGS_DRM) $(CFLAGS)

./build/prob_egl.o: prob_egl.cpp $(OBJ_EGL) $(DEPS)
	$(CXX) $(PFLAGS) $(INC) -c prob_egl.cpp -o ./build/prob_egl.o  $(CFLAGS_DRM) $(CXXFLAGS) $(CFLAGS_EGL) $(CFLAGS_GL)

./build/cpu_compute0.o: cpu_compute0.cpp $(DEPS)
	$(CXX) $(PFLAGS) $(INC) -c cpu_compute0.cpp -o ./build/cpu_compute0.o  $(CFLAGS_DRM) $(CFLAGS_CPU0) $(CXXFLAGS)

# object files, supplementary
./build/aux_xcb.o: aux_xcb.c $(DEPS)
	$(CC) $(PFLAGS) $(INC) -c aux_xcb.c -o ./build/aux_xcb.o  $(CFLAGS)

./build/aux_drm.o: aux_drm.c $(DEPS)
	$(CC) $(PFLAGS) $(INC) -c aux_drm.c -o ./build/aux_drm.o  $(CFLAGS) $(CFLAGS_DRM)

./build/aux_raster.o: aux_raster.c $(DEPS)
	$(CC) $(PFLAGS) $(INC) -c aux_raster.c -o ./build/aux_raster.o  $(CFLAGS)

./build/aux_egl.o: aux_egl.c $(DEPS)
	$(CC) $(PFLAGS) $(INC) -c aux_egl.c -o ./build/aux_egl.o  $(CFLAGS) $(CFLAGS_EGL)

./build/fequals.o: vg_algebra/fequals.cpp $(DEPS)
	$(CXX) $(PFLAGS) $(INC) -c vg_algebra/fequals.cpp -o ./build/fequals.o  $(CXXFLAGS) $(CFLAGS_EGL)

./build/aux_gl.o: aux_gl.c $(DEPS)
	$(CC) $(PFLAGS) $(INC) -c aux_gl.c -o ./build/aux_gl.o  $(CFLAGS) $(CFLAGS_GL)

FORCE:
