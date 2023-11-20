CC=gcc -m32
CFLAGS = -g -O0 -fPIC -m32 -Wall -Werror -Wno-unused-variable -Wno-unused-function
LD = g++ -m32
LDFLAGS = -Wl,-z,defs -rdynamic -static-libstdc++ -static-libgcc -lc -ldl -lGL -lglut -lX11 -lm -lpthread -shared -nostdlib

BUILD = build

OBJS := $(patsubst %.c,%.o,$(wildcard *.c))

all: hookcrc32.so

hookcrc32.so: $(OBJS)
	$(LD) $(OBJS) $(LDFLAGS) $(CFLAGS) -o hookcrc32.so
	rm -f *.o

clean:
	rm -f hookcrc32.so
