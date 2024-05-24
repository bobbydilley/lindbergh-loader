CC=gcc -m32 
#-fno-stack-protector
CFLAGS = -g -O0 -fPIC -m32 -Wall -Werror -Wno-unused-variable -Wno-unused-function #-fno-PIC
LD = g++ -m32
LDFLAGS = -Wl,-z,defs -rdynamic -static-libstdc++ -static-libgcc -lSDL2 -lGL -lc -ldl -lX11 -lm -lpthread -shared -nostdlib -lasound
#//-lSOIL2   -lglfw   -lRegal -lRegalGLUT

BUILD = build

OBJS := $(patsubst %.c,%.o,$(wildcard src/lindbergh/*.c))

all: lindbergh lindbergh.so libsegaapi.so libkswapapi.so

lindbergh: src/lindbergh/lindbergh.c
	mkdir -p $(BUILD)
	$(CC) src/lindbergh/lindbergh.c -o $(BUILD)/lindbergh
	#cp build/lindbergh ~/games/outrun-2-sp-sdx-rev-a/Jennifer/.

lindbergh.so: $(OBJS)
	mkdir -p $(BUILD)
	$(LD) $(OBJS) $(LDFLAGS) $(CFLAGS) -o $(BUILD)/lindbergh.so
	#rm -f src/lindbergh/*.o
	#cp build/lindbergh.so ~/games/outrun-2-sp-sdx-rev-a/Jennifer/.

LIBSEGA_LD=g++ #clang
LIBSEGA_LDFLAGS=-m32 -O0 -g
OBJS_SEGAAPI := $(patsubst %.cpp,%.c,%.o,$(wildcard src/libsegaapi/*.cpp))

libsegaapi.so: src/libsegaapi/opensegaapi.o
	$(LIBSEGA_LD) $(LIBSEGA_LDFLAGS) src/libsegaapi/opensegaapi.cpp src/libsegaapi/dqueue.c -lFAudio -fPIC -shared -o $(BUILD)/libsegaapi.so
	#rm -f src/libsegaapi/*.o
	#cp build/libsegaapi.so ~/games/outrun-2-sp-sdx-rev-a/Jennifer/.

libkswapapi.so: src/libkswapapi/libkswapapi.o
	$(LIBSEGA_LD) $(LIBSEGA_LDFLAGS) src/libkswapapi/libkswapapi.o -L/usr/lib/i386-linux-gnu -fPIC -shared -o $(BUILD)/libkswapapi.so
	#rm -f src/libkswapapi/*.o
	#cp build/libkswapapi.so ~/games/outrun-2-sp-sdx-rev-a/Jennifer/.

clean:
	rm -rf $(BUILD)
	rm -f src/lindbergh/*.o
	rm -f src/libsegaapi/*.o
	rm -f src/libkswapapi/*.o
