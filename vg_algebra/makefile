CC       = cc
CPP      = g++
AR       = ar
LD       = ld
PFLAGS   = -D_XOPEN_SOURCE=700 -DGL_GLEXT_PROTOTYPES
CPPFLAGS = -std=c++2a -Wall -Wpedantic -Wextra -fpic -fopenmp -DDEBUG -ggdb -O0
TARGET   = libyma_geometry.a
INC      = -I./include

INC_H = yma_geometry.h\
        yma_fequals.h\
        yma_geometry_frwd.h\
        yma_geometry_pt2.h\
        yma_geometry_pt3.h\
        yma_geometry_vec2.h\
        yma_geometry_vec3.h\
        yma_geometry_vec4.h\
        yma_geometry_segment.h\
        yma_geometry_mat4x4.h\
        yma_geometry_triangle.h\
        yma_glsl_program.h\
        yma_glsl.h

DEPS  = $(INC_H) makefile

PREFIX     = /usr/local
PREFIX_INC = $(PREFIX)/include/yma
PREFIX_LIB = $(PREFIX)/lib/yma

LIBS_TEST = -Wl,-Bstatic -lyma_geometry\
            -Wl,-Bdynamic  -lm\
                           -lGL\
                           -lGLU\
                           -lyma_raster\
                           -lyma_xcb

BIN_LIBS  = -dy -lm 

SRC     = yma_fequals.cpp

INL_SRC =
OBJ  = ./build/yma_fequals.o\
       ./build/yma_glsl_program.o

BIN  = ./bin/$(TARGET)
TEST_PT2      = bin/test-pt2
TEST_PT3      = bin/test-pt3
TEST_VEC2     = bin/test-vec2
TEST_VEC3     = bin/test-vec3
TEST_VEC4     = bin/test-vec4
TEST_MAT4x4   = bin/test-mat4x4
TEST_MAIN     = bin/test-main
BIN_TEST = $(TEST_PT2) $(TEST_PT3) $(TEST_VEC2) $(TEST_VEC3) $(TEST_MAT4x4) $(TEST_MAIN)

HDR  = $(INC_H:%.h=$(PREFIX_INC)/%.h)

all: build install

build: mk_dirs copy_headers $(BIN)

clean: FORCE
	rm -f build/* bin/*

rebuild: FORCE
	rm -f build/* bin/*
	$(MAKE)

mk_dirs: FORCE
	@echo ' - make build directories'
	@mkdir -p bin build
    
	@if [ ! -d $(PREFIX_LIB) ]; then \
	 @echo ' - make lib directories'; \
	 @sudo mkdir -p $(PREFIX_LIB); \
	 @sudo chown yv:yv $(PREFIX_LIB); \
	 @chmod 750 $(PREFIX_LIB); \
	 @echo ' - make $(PREFIX_LIB) directory'; \
	fi
	
	@if [ ! -d $(PREFIX_INC) ]; then \
	 @echo ' - make $(PREFIX_INC) directory'; \
	 @echo ' - make lib directories'; \
	 @sudo mkdir -p $(PREFIX_INC); \
	 @sudo chown yv:yv $(PREFIX_INC); \
	 @chmod 750 $(PREFIX_INC); \
	fi

copy_headers: FORCE
	@echo ' - copy library headers'
	@cp $(INC_H) $(PREFIX_INC)
	@echo ' - $(INC_H) to: $(PREFIX_INC)'
	@echo ''

install: mk_dirs copy_headers $(BIN)
	@echo ' - install static library $(BIN) to $(PREFIX_LIB) '
	@cp $(BIN) $(PREFIX_LIB)

test: $(BIN_TEST)

# dynamic version
#$(BIN): $(OBJ)
#	$(LD) -L $(PREFIX_LIB) -shared $(BIN_LIBS) -rpath=$(PREFIX_LIB) -o $(BIN) --start-group $(OBJ) --end-group

# static library
$(BIN): $(OBJ)
	$(AR) rcs $(BIN) $(OBJ)

build/yma_fequals.o: $(DEPS) yma_fequals.cpp
	$(CPP) $(PFLAGS) $(CPPFLAGS) $(INC) -I./usr/local/include -c yma_fequals.cpp -o $@ 

build/yma_glsl_program.o: $(DEPS) yma_glsl_program.cpp
	$(CPP) $(PFLAGS) $(CPPFLAGS) $(INC) -c yma_glsl_program.cpp -o $@

./bin/test-main: $(DEPS) test/test-main.cpp $(BIN)
	$(CPP) -L$(PREFIX_LIB) $(PFLAGS) $(CPPFLAGS) ./test/test-main.cpp -o $@ -Wl,-rpath=$(PREFIX_LIB) $(LIBS_TEST)

bin/test-vec2: $(DEPS) test/test-vec2.cpp $(BIN)
	$(CPP) -L$(PREFIX_LIB) $(PFLAGS) $(CPPFLAGS) ./test/test-vec2.cpp -o ./bin/test-vec2 $(LIBS_TEST)

bin/test-vec3: $(DEPS) test/test-vec3.cpp $(BIN)
	$(CPP) -L$(PREFIX_LIB) $(PFLAGS) $(CPPFLAGS) ./test/test-vec3.cpp -o ./bin/test-vec3 $(LIBS_TEST)

bin/test-vec4: $(DEPS) test/test-vec4.cpp $(BIN)
	$(CPP) -L$(PREFIX_LIB) $(PFLAGS) $(CPPFLAGS) ./test/test-vec4.cpp -o ./bin/test-vec4 $(LIBS_TEST)

bin/test-pt2: $(DEPS) test/test-pt2.cpp $(BIN)
	$(CPP) -L$(PREFIX_LIB) $(PFLAGS) $(CPPFLAGS) -o ./bin/test-pt2 ./test/test-pt2.cpp $(LIBS_TEST)

bin/test-pt3: $(DEPS) test/test-pt3.cpp $(BIN)
	$(CPP) -L$(PREFIX_LIB) $(PFLAGS) $(CPPFLAGS) ./test/test-pt3.cpp -o ./bin/test-pt3 $(LIBS_TEST)

bin/test-mat4x4: $(DEPS) test/test-mat4x4.cpp $(BIN)
	$(CPP) -L$(PREFIX_LIB) $(PFLAGS) $(CPPFLAGS) ./test/test-mat4x4.cpp -o ./bin/test-mat4x4 $(LIBS_TEST)

FORCE:
