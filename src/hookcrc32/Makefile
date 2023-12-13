CC=gcc -m32
CFLAGS = -g -O0 -fPIC -m32 -Wall -Werror -Wno-unused-variable -Wno-unused-function

BUILD = build

OBJS := $(patsubst %.c,%.o,$(wildcard *.c))

all: getcrc

hookcrc32.so: $(OBJS)
	$(LD) $(OBJS) $(LDFLAGS) $(CFLAGS) -o getcrc
	rm -f *.o

clean:
	rm -f getcrc
