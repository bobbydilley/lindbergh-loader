LD=gcc #clang
CC=gcc
CFLAGS=-m32 -O0 -g
LDFLAGS=$(CFLAGS)

libsegaapi.so: segaapi.o
	$(LD) $(LDFLAGS) segaapi.o -L/usr/lib/i386-linux-gnu -lalut -fPIC -shared -o libsegaapi.so
clean:
	rm -f test libsegaapi.so

force:
