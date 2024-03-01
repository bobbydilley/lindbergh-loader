CC=gcc -m32
CFLAGS = -g -O0 -fPIC -m32 -Wall -Werror -Wno-unused-variable -Wno-unused-function
LD = g++ -m32
LDFLAGS = -Wl,-z,defs -rdynamic -static-libstdc++ -static-libgcc -lc -ldl -lGL -lglut -lX11 -lm -lpthread -shared -nostdlib -lasound

BUILD = build

OBJS := $(patsubst %.c,%.o,$(wildcard src/lindbergh/*.c))

all: lindbergh lindbergh.so libsegaapi.so libkswapapi.so

lindbergh: src/lindbergh/lindbergh.c
	mkdir -p $(BUILD)
	$(CC) src/lindbergh/lindbergh.c -o $(BUILD)/lindbergh

lindbergh.so: $(OBJS)
	mkdir -p $(BUILD)
	$(LD) $(OBJS) $(LDFLAGS) $(CFLAGS) -o $(BUILD)/lindbergh.so
	rm -f src/lindbergh/*.o

LIBSEGA_LD=gcc #clang
LIBSEGA_LDFLAGS=-m32 -O0 -g

libsegaapi.so: src/libsegaapi/segaapi.o
	$(LIBSEGA_LD) $(LIBSEGA_LDFLAGS) src/libsegaapi/segaapi.o -L/usr/lib/i386-linux-gnu -lalut -fPIC -shared -o $(BUILD)/libsegaapi.so
	rm -f src/libsegaapi/*.o

libkswapapi.so: src/libkswapapi/libkswapapi.o
	$(LIBSEGA_LD) $(LIBSEGA_LDFLAGS) src/libkswapapi/libkswapapi.o -L/usr/lib/i386-linux-gnu -fPIC -shared -o $(BUILD)/libkswapapi.so
	rm -f src/libkswapapi/*.o

clean:
	rm -rf $(BUILD)
	rm -f src/lindbergh/*.o
	rm -f src/libsegaapi/*.o
	rm -f src/libkswapapi/*.o
