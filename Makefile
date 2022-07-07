CC=gcc -m32
CFLAGS = -g -O0 -fPIC -m32 -D_GNU_SOURCE -Wall -Werror -Wno-unused-variable -Wno-unused-function
LD = g++ -m32
LDFLAGS = -Wl,-z,defs -rdynamic -static-libstdc++ -static-libgcc -lc -ldl -lGL -lglut -lX11 -lm -lpthread -shared -nostdlib

BUILD = build

OBJS := $(patsubst %.c,%.o,$(wildcard src/lindbergh/*.c))

all: lindbergh.so

lindbergh.so: $(OBJS)
	mkdir -p $(BUILD)
	$(LD) $(OBJS) $(LDFLAGS) $(CFLAGS) -o $(BUILD)/lindbergh.so
	rm -f src/lindbergh/*.o

clean:
	rm -rf $(BUILD)
