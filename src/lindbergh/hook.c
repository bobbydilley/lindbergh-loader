#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <linux/sockios.h>
#include <signal.h>

#include "hook.h"

#include "baseboard.h"
#include "config.h"
#include "rideboard.h"
#include "eeprom.h"
#include "jvs.h"

#define HOOK_FILE_NAME "/dev/zero"

#define BASEBOARD 0
#define EEPROM 1
#define SERIAL0 2
#define SERIAL1 3

int hooks[4] = {-1, -1, -1, -1};

void __attribute__((constructor)) hook_init()
{
    printf("SEGA Lindbergh Loader\nRobert Dilley 2022\nNot for public consumption\n\n");
    initConfig();

    if (initEeprom() != 0)
        exit(1);

    if (initBaseboard() != 0)
        exit(1);

    if (initJVS() != 0)
        exit(1);
}

int open(const char *pathname, int flags)
{
    int (*_open)(const char *pathname, int flags) = dlsym(RTLD_NEXT, "open");

    if (strcmp(pathname, "/dev/lbb") == 0)
    {
        hooks[BASEBOARD] = open(HOOK_FILE_NAME, flags);
        return hooks[BASEBOARD];
    }

    if (strcmp(pathname, "/dev/i2c/0") == 0)
    {
        hooks[EEPROM] = open(HOOK_FILE_NAME, flags);
        return hooks[EEPROM];
    }

    if (strcmp(pathname, "/dev/ttyS0") == 0 || strcmp(pathname, "/dev/tts/0") == 0)
    {
        hooks[SERIAL0] = open(HOOK_FILE_NAME, flags);
        return hooks[SERIAL0];
    }

    if (strcmp(pathname, "/dev/ttyS1") == 0 || strcmp(pathname, "/dev/tts/1") == 0)
    {
        hooks[SERIAL1] = open(HOOK_FILE_NAME, flags);
        return hooks[SERIAL1];
    }

    return _open(pathname, flags);
}

int openat(int dirfd, const char *pathname, int flags)
{
    int (*_openat)(int dirfd, const char *pathname, int flags) = dlsym(RTLD_NEXT, "openat");

    if (strcmp(pathname, "/dev/ttyS0") == 0 || strcmp(pathname, "/dev/ttyS1") == 0 || strcmp(pathname, "/dev/tts/0") == 0 || strcmp(pathname, "/dev/tts/1") == 0)
    {
        return open(pathname, flags);
    }

    return _openat(dirfd, pathname, flags);
}

int close(int fd)
{
    int (*_close)(int fd) = dlsym(RTLD_NEXT, "close");

    for (int i = 0; i < (sizeof hooks / sizeof hooks[0]); i++)
    {
        if (hooks[i] == fd)
        {
            hooks[i] = -1;
            return 0;
        }
    }

    return _close(fd);
}

ssize_t read(int fd, void *buf, size_t count)
{
    int (*_read)(int fd, void *buf, size_t count) = dlsym(RTLD_NEXT, "read");

    if (fd == hooks[BASEBOARD])
    {
        return baseboardRead(fd, buf, count);
    }

    if (fd == hooks[SERIAL1] && getConfig()->emulateRideboard)
    {
        return rideboardRead(fd, buf, count);
    }

    return _read(fd, buf, count);
}

ssize_t write(int fd, const void *buf, size_t count)
{
    int (*_write)(int fd, const void *buf, size_t count) = dlsym(RTLD_NEXT, "write");

    if (fd == hooks[BASEBOARD])
    {
        return baseboardWrite(fd, buf, count);
    }

    if (fd == hooks[SERIAL1] && getConfig()->emulateRideboard)
    {
        return rideboardWrite(fd, buf, count);
    }

    return _write(fd, buf, count);
}

int ioctl(int fd, unsigned int request, void *data)
{
    int (*_ioctl)(int fd, int request, void *data) = dlsym(RTLD_NEXT, "ioctl");

    // Attempt to stop access to the ethernet ports
    if ((request == SIOCSIFADDR) || (request == SIOCSIFFLAGS) || (request == SIOCSIFHWADDR) || (request == SIOCSIFHWBROADCAST) || (request == SIOCDELRT) || (request == SIOCADDRT) || (request == SIOCSIFNETMASK))
    {
        errno = ENXIO;
        return -1;
    }

    if (fd == hooks[EEPROM])
    {
        return eepromIoctl(fd, request, data);
    }

    if (fd == hooks[BASEBOARD])
    {
        return baseboardIoctl(fd, request, data);
    }

    // Just accept any IOCTL on serial ports and ignore it
    if (fd == hooks[SERIAL0] || fd == hooks[SERIAL1])
    {
        return 0;
    }

    return _ioctl(fd, request, data);
}

int select(int nfds, fd_set *restrict readfds, fd_set *restrict writefds, fd_set *restrict exceptfds, struct timeval *restrict timeout)
{
    int (*_select)(int nfds, fd_set *restrict readfds, fd_set *restrict writefds, fd_set *restrict exceptfds, struct timeval *restrict timeout) = dlsym(RTLD_NEXT, "select");

    if (readfds != NULL && FD_ISSET(hooks[BASEBOARD], readfds))
    {
        return baseboardSelect(nfds, readfds, writefds, exceptfds, timeout);
    }

    if (writefds != NULL && FD_ISSET(hooks[BASEBOARD], writefds))
    {
        return baseboardSelect(nfds, readfds, writefds, exceptfds, timeout);
    }

    return _select(nfds, readfds, writefds, exceptfds, timeout);
}

#include <ucontext.h>

static void exceptionTrampoline(int signum, siginfo_t *info, void *ptr)
{
    ucontext_t *ctx = ptr;

    uint8_t *code = (uint8_t *)ctx->uc_mcontext.gregs[REG_EIP];

    switch (*code)
    {
    case 0xED:
    {
        uint16_t port = ctx->uc_mcontext.gregs[REG_EDX] & 0xFFFF;

        switch (port)
        {
	case 0x38:
            ctx->uc_mcontext.gregs[REG_EAX] = 0xFFFFFFFF;
            break;
        default:
            break;
        }
        // printf("Warning: IO READ Port %X\n", port);
        ctx->uc_mcontext.gregs[REG_EIP]++;
        return;
    }
    break;

    case 0xE7:
    {
        //printf("Warning: IO WRITE IMMIDIATE %X\n", code[1]);
        ctx->uc_mcontext.gregs[REG_EIP] += 2;
        return;
    }
    break;

    case 0xE6:
    {
        //printf("Warning: IO WRITE IMMIDIATE %X\n", code[1]);
        ctx->uc_mcontext.gregs[REG_EIP] += 2;
        return;
    }
    break;

    case 0xEE:
    {
        uint16_t port = ctx->uc_mcontext.gregs[REG_EDX] & 0xFFFF;
        uint8_t data = ctx->uc_mcontext.gregs[REG_EAX] & 0xFF;
        //printf("Warning: IO WRITE Port %X Data %X\n", port, data);
        ctx->uc_mcontext.gregs[REG_EIP]++;
        return;
    }
    break;

    case 0xEF:
    {
        uint16_t port = ctx->uc_mcontext.gregs[REG_EDX] & 0xFFFF;
        //printf("Warning: IO WRITE Port %X\n", port);
        ctx->uc_mcontext.gregs[REG_EIP]++;
        return;
    }
    break;

    default:
        printf("Error: Unknown segfault %X\n", *code);
        abort();
    }
}

int iopl(int level)
{
    static int hasRunAlready = 0;

    if (hasRunAlready == 0)
    {
        struct sigaction act;

        act.sa_sigaction = exceptionTrampoline;
        act.sa_flags = SA_SIGINFO;

        sigaction(SIGSEGV, &act, NULL);

        hasRunAlready = 1;
    }

    return 0;
}

/**
 * Hook for the only function provided by kswapapi.so
 * @param p No idea this gets discarded
 */
void kswap_collect(void *p)
{
    return;
}

/**
 * Hook for function used by Primevil
 * @param base The number to raise to the exponent
 * @param exp The exponent to raise the number to
 * @return The result of raising the number to the exponent
 */
float powf(float base, float exponent)
{
    return (float)pow((double)base, (double)exponent);
}
