#define _GNU_SOURCE
#include <link.h>
#include <dlfcn.h>
#include <errno.h>
#include <linux/sockios.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ucontext.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cpuid.h>
#include <unistd.h>

#include "hook.h"

#include "baseboard.h"
#include "config.h"
#include "driveboard.h"
#include "eeprom.h"
#include "jvs.h"
#include "rideboard.h"
#include "securityboard.h"
#include "patch.h"
#include "pcidata.h"
#include "input.h"
#include "soundcard.h"

#include "alsa/global.h"
#include "alsa/input.h"
#include "alsa/output.h" 
#include "alsa/conf.h"
#include "alsa/pcm.h"
#include "alsa/control.h"
#include "alsa/error.h"

#define HOOK_FILE_NAME "/dev/zero"

#define BASEBOARD 0
#define EEPROM 1
#define SERIAL0 2
#define SERIAL1 3
#define PCI_CARD_000 4

#define CPUINFO 0
#define OSRELEASE 1
#define PCI_CARD_1F0 2

int hooks[5] = {-1, -1, -1, -1, -1};
FILE *fileHooks[3] = {NULL, NULL, NULL};
int fileRead[3] = {0, 0, 0};
char envpath[100];
uint32_t elf_crc = 0;

cpuvendor cpu_vendor = {0};

static int callback(struct dl_phdr_info *info, size_t size, void *data);

uint16_t basePortAddress = 0xFFFF;

/**
 * Signal handler for the SIGSEGV signal, which is triggered when a process tries to access an illegal memory location.
 * @param signal
 * @param info
 * @param ptr
 */
static void handleSegfault(int signal, siginfo_t *info, void *ptr)
{
    ucontext_t *ctx = ptr;

    // Get the address of the instruction causing the segfault
    uint8_t *code = (uint8_t *)ctx->uc_mcontext.gregs[REG_EIP];

    switch (*code)
    {
    case 0xED:
    {
        // Get the port number from the EDX register
        uint16_t port = ctx->uc_mcontext.gregs[REG_EDX] & 0xFFFF;

        // The first port called is usually random, but everything after that
        // is a constant offset, so this is a hack to fix that.
        // When run as sudo it works fine!?

        if (basePortAddress == 0xFFFF)
            basePortAddress = port;

        // Adjust the port number if necessary
        if (port > 0x38)
            port = port - basePortAddress;

        // Call the security board input function with the port number and data
        securityBoardIn(port, (uint32_t *)&(ctx->uc_mcontext.gregs[REG_EAX]));

        ctx->uc_mcontext.gregs[REG_EIP]++;
        return;
    }
    break;

    case 0xE7: // OUT IMMEDIATE
    {
        // Increment the instruction pointer by two to skip over this instruction
        ctx->uc_mcontext.gregs[REG_EIP] += 2;
        return;
    }
    break;

    case 0xE6: // OUT IMMEDIATE
    {
        // Increment the instruction pointer by two to skip over this instruction
        ctx->uc_mcontext.gregs[REG_EIP] += 2;
        return;
    }
    break;

    case 0xEE: // OUT
    {
        uint16_t port = ctx->uc_mcontext.gregs[REG_EDX] & 0xFFFF;
        uint8_t data = ctx->uc_mcontext.gregs[REG_EAX] & 0xFF;
        ctx->uc_mcontext.gregs[REG_EIP]++;
        return;
    }
    break;

    case 0xEF: // OUT
    {
        uint16_t port = ctx->uc_mcontext.gregs[REG_EDX] & 0xFFFF;
        ctx->uc_mcontext.gregs[REG_EIP]++;
        return;
    }
    break;

    default:
        printf("Warning: Skipping SEGFAULT %X\n", *code);
        ctx->uc_mcontext.gregs[REG_EIP]++;
        // abort();
    }
}

void __attribute__((constructor)) hook_init()
{

    // Get offsets of the Game's ELF and calculate CRC32.
    dl_iterate_phdr(callback, NULL);

    // Get CPU ID
    getCPUID();

    // Implement SIGSEGV handler
    struct sigaction act;
    act.sa_sigaction = handleSegfault;
    act.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &act, NULL);

    initConfig();

    if (initPatch() != 0)
        exit(1);

    if (initEeprom() != 0)
        exit(1);

    if (initBaseboard() != 0)
        exit(1);

    if (initJVS() != 0)
        exit(1);

    if (initSecurityBoard() != 0)
        exit(1);

    if (getConfig()->emulateDriveboard)
    {
        if (initDriveboard() != 0)
            exit(1);
    }

    if (getConfig()->emulateRideboard)
    {
        if (initRideboard() != 0)
            exit(1);
    }

    if (initInput() != 0)
        exit(1);

    securityBoardSetDipResolution(getConfig()->width, getConfig()->height);

    printf("\nSEGA Lindbergh Emulator\nRobert Dilley 2023\n\n");
    printf("  GAME:       %s\n", getGameName());
    printf("  GAME ID:    %s\n", getGameID());
    printf("  DVP:        %s\n", getDVPName());
    printf("  STATUS:     %s\n", getConfig()->gameStatus == WORKING ? "WORKING" : "NOT WORKING");
}

int open(const char *pathname, int flags)
{
    int (*_open)(const char *pathname, int flags) = dlsym(RTLD_NEXT, "open");

    // printf("Open %s\n", pathname);

    if (strcmp(pathname, "/dev/lbb") == 0)
    {
        hooks[BASEBOARD] = _open(HOOK_FILE_NAME, flags);
        return hooks[BASEBOARD];
    }

    if (strcmp(pathname, "/dev/i2c/0") == 0)
    {
        hooks[EEPROM] = _open(HOOK_FILE_NAME, flags);
        return hooks[EEPROM];
    }

    if (strcmp(pathname, "/dev/ttyS0") == 0 || strcmp(pathname, "/dev/tts/0") == 0)
    {
        if (getConfig()->emulateDriveboard == 0 && getConfig()->emulateRideboard == 0)
            return _open(getConfig()->serial1Path, flags);

        if (hooks[SERIAL0] != -1)
            return -1;

        hooks[SERIAL0] = _open(HOOK_FILE_NAME, flags);
        printf("Warning: SERIAL0 Opened %d\n", hooks[SERIAL0]);
        return hooks[SERIAL0];
    }

    if (strcmp(pathname, "/dev/ttyS1") == 0 || strcmp(pathname, "/dev/tts/1") == 0)
    {
        if (getConfig()->emulateMotionboard == 0)
            return _open(getConfig()->serial2Path, flags);

        if (hooks[SERIAL1] != -1)
            return -1;

        hooks[SERIAL1] = _open(HOOK_FILE_NAME, flags);
        printf("Warning: SERIAL1 opened %d\n", hooks[SERIAL1]);
        return hooks[SERIAL1];
    }

    if (strncmp(pathname, "/tmp/", 5) == 0)
    {
        mkdir("tmp", 0777);
        return _open(pathname + 1, flags);
    }

    if (strcmp(pathname, "/proc/bus/pci/01/00.0") == 0)
    {
        hooks[PCI_CARD_000] = _open(HOOK_FILE_NAME, flags);
        return hooks[PCI_CARD_000];
    }

    return _open(pathname, flags);
}

int open64(const char *pathname, int flags)
{
    return open(pathname, flags);
}

FILE *fopen(const char *restrict pathname, const char *restrict mode)
{
    FILE *(*_fopen)(const char *restrict pathname, const char *restrict mode) = dlsym(RTLD_NEXT, "fopen");

    if (strcmp(pathname, "/root/lindbergrc") == 0)
    {
        return _fopen("lindbergrc", mode);
    }

    if ((strcmp(pathname, "/usr/lib/boot/logo.tga") == 0) || (strcmp(pathname, "/usr/lib/boot/logo.tga") == 0))
    {
        return _fopen("logo.tga", mode);
    }

    if (strcmp(pathname, "/usr/lib/boot/LucidaConsole_12.tga") == 0)
    {
        return _fopen("LucidaConsole_12.tga", mode);
    }

    if (strcmp(pathname, "/usr/lib/boot/LucidaConsole_12.abc") == 0)
    {
        return _fopen("LucidaConsole_12.abc", mode);
    }

    if (strcmp(pathname, "/proc/cpuinfo") == 0)
    {
        fileRead[CPUINFO] = 0;
        fileHooks[CPUINFO] = _fopen(HOOK_FILE_NAME, mode);
        return fileHooks[CPUINFO];
    }

    if (strcmp(pathname, "/usr/lib/boot/logo_red.tga") == 0)
    {
        return _fopen("logo_red.tga", mode);
    }
    if (strcmp(pathname, "/usr/lib/boot/SEGA_KakuGothic-DB-Roman_12.tga") == 0)
    {
        return _fopen("SEGA_KakuGothic-DB-Roman_12.tga", mode);
    }

    if (strcmp(pathname, "/usr/lib/boot/SEGA_KakuGothic-DB-Roman_12.abc") == 0)
    {
        return _fopen("SEGA_KakuGothic-DB-Roman_12.abc", mode);
    }

    if (strcmp(pathname, "/proc/bus/pci/00/1f.0") == 0)
    {
        fileRead[PCI_CARD_1F0] = 0;
        fileHooks[PCI_CARD_1F0] = _fopen(HOOK_FILE_NAME, mode);
        return fileHooks[PCI_CARD_1F0];
    }

    char *result;
    if ((result = strstr(pathname, "/home/disk0")) != NULL)
    {
        memmove(result + 2, result + 11, strlen(result + 11) + 1);
        memcpy(result, "..", 2);
        return _fopen(result, mode);
    }

    // printf("Path= %s\n", pathname);

    return _fopen(pathname, mode);
}

FILE *fopen64(const char *pathname, const char *mode)
{
    FILE *(*_fopen64)(const char *restrict pathname, const char *restrict mode) = dlsym(RTLD_NEXT, "fopen64");
    // printf("fopen64 %s\n", pathname);

    if (strcmp(pathname, "/proc/sys/kernel/osrelease") == 0)
    {
        EmulatorConfig *config = getConfig();
        fileRead[OSRELEASE] = 0;
        fileHooks[OSRELEASE] = _fopen64(HOOK_FILE_NAME, mode);
        return fileHooks[OSRELEASE];
    }

    if (strcmp(pathname, "/usr/lib/boot/logo_red.tga") == 0)
    {
        return _fopen64("logo_red.tga", mode);
    }

    if (strcmp(pathname, "/usr/lib/boot/logo.tga") == 0)
    {
        return _fopen64("logo.tga", mode);
    }

    if (strcmp(pathname, "/usr/lib/boot/SEGA_KakuGothic-DB-Roman_12.tga") == 0)
    {
        return _fopen64("SEGA_KakuGothic-DB-Roman_12.tga", mode);
    }

    if (strcmp(pathname, "/usr/lib/boot/SEGA_KakuGothic-DB-Roman_12.abc") == 0)
    {
        return _fopen64("SEGA_KakuGothic-DB-Roman_12.abc", mode);
    }
    return _fopen64(pathname, mode);
}

int fclose(FILE *stream)
{
    int (*_fclose)(FILE *stream) = dlsym(RTLD_NEXT, "fclose");
    for (int i = 0; i < 3; i++)
    {
        if (fileHooks[i] == stream)
        {
            int r = _fclose(stream);
            fileHooks[i] = NULL;
            return r;
        }
    }
    return _fclose(stream);
}
int openat(int dirfd, const char *pathname, int flags)
{
    int (*_openat)(int dirfd, const char *pathname, int flags) = dlsym(RTLD_NEXT, "openat");
    // printf("openat %s\n", pathname);

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

char *fgets(char *str, int n, FILE *stream)
{
    char *(*_fgets)(char *str, int n, FILE *stream) = dlsym(RTLD_NEXT, "fgets");

    if (stream == fileHooks[OSRELEASE])
    {
        char *contents = "mvl";
        strcpy(str, contents);
        return str;
    }

    // This currently doesn't work
    if (stream == fileHooks[CPUINFO])
    {
        char contents[4][256];

        strcpy(contents[0], "processor	: 0");
        strcpy(contents[1], "vendor_id	: GenuineIntel");
        strcpy(contents[2], "model		: 142");
        strcpy(contents[3], "model name	: Intel(R) Pentium(R) CPU 3.00GHz");

        if (getConfig()->lindberghColour == RED)
            strcpy(contents[3], "model name	: Intel(R) Celeron(R) CPU 3.00GHz");

        if (fileRead[CPUINFO] == 4)
            return NULL;

        strcpy(str, contents[fileRead[CPUINFO]++]);
        return str;
    }

    return _fgets(str, n, stream);
}

ssize_t read(int fd, void *buf, size_t count)
{
    int (*_read)(int fd, void *buf, size_t count) = dlsym(RTLD_NEXT, "read");

    if (fd == hooks[BASEBOARD])
    {
        return baseboardRead(fd, buf, count);
    }

    if (fd == hooks[SERIAL0] && getConfig()->emulateRideboard)
    {
        return rideboardRead(fd, buf, count);
    }

    if (fd == hooks[SERIAL0] && getConfig()->emulateDriveboard)
    {
        return driveboardRead(fd, buf, count);
    }

    // If we don't hook the serial just reply with nothing
    if (fd == hooks[SERIAL0] || fd == hooks[SERIAL1])
    {
        return -1;
    }

    if (fd == hooks[PCI_CARD_000])
    {
        memcpy(buf, pci_000, count);
        return count;
    }

    return _read(fd, buf, count);
}

size_t fread(void *buf, size_t size, size_t count, FILE *stream)
{
    size_t (*_fread)(void *buf, size_t size, size_t count, FILE *stream) = dlsym(RTLD_NEXT, "fread");

    if (stream == fileHooks[PCI_CARD_1F0])
    {
        memcpy(buf, pci_1f0, size * count);
        return size * count;
    }
    return _fread(buf, size, count, stream);
}

ssize_t write(int fd, const void *buf, size_t count)
{
    int (*_write)(int fd, const void *buf, size_t count) = dlsym(RTLD_NEXT, "write");

    if (fd == hooks[BASEBOARD])
    {
        return baseboardWrite(fd, buf, count);
    }

    if (fd == hooks[SERIAL0] && getConfig()->emulateRideboard)
    {
        return rideboardWrite(fd, buf, count);
    }

    if (fd == hooks[SERIAL0] && getConfig()->emulateDriveboard)
    {
        return driveboardWrite(fd, buf, count);
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
        if (request == 0xC04064A0)
            return _ioctl(fd, request, data);
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

int system(const char *command)
{
    int (*_system)(const char *command) = dlsym(RTLD_NEXT, "system");

    if (strcmp(command, "lsmod | grep basebd > /dev/null") == 0)
        return 0;

    if (strcmp(command, "cd /tmp/segaboot > /dev/null") == 0)
        return system("cd tmp/segaboot > /dev/null");

    if (strcmp(command, "mkdir /tmp/segaboot > /dev/null") == 0)
        return system("mkdir tmp/segaboot > /dev/null");

    if (strcmp(command, "lspci | grep \"Multimedia audio controller: %Creative\" > /dev/null") == 0)
        return 0;

    if (strcmp(command, "lsmod | grep ctaud") == 0)
        return 0;

    if (strcmp(command, "lspci | grep MPC8272 > /dev/null") == 0)
        return 0;

    if (strcmp(command, "uname -r | grep mvl") == 0)
        return 0;

    if (strstr(command, "hwclock") != NULL)
        return 0;

    if (strstr(command, "losetup") != NULL)
        return 0;

    return _system(command);
}

int iopl(int level)
{
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

/*
int sem_wait(sem_t *sem)
{
    int (*original_sem_wait)(sem_t * sem) = dlsym(RTLD_NEXT, "sem_wait");
    return 0;
}
*/

/**
 * Hook function used by Harley Davidson to change IPs to localhost
 * Currently does nothing.
 * @param sockfd
 * @param addr
 * @param addrlen
 * @return
 */
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    int (*_connect)(int sockfd, const struct sockaddr *addr, socklen_t addrlen) = dlsym(RTLD_NEXT, "connect");

    struct sockaddr_in *in_pointer = (struct sockaddr_in *)addr;

    // Change the IP to connect to 127.0.0.1
    // in_pointer->sin_addr.s_addr = inet_addr("127.0.0.1");
    char *some_addr = inet_ntoa(in_pointer->sin_addr);
    if (getConfig()->showDebugMessages)
    {
        printf("Connecting to %s\n", some_addr);
    }

    return _connect(sockfd, addr, addrlen);
}

/**
 * Function to calculate CRC32 checksum in memory.
 */
uint32_t get_crc32(const char *s, size_t n)
{
    uint32_t crc = 0xFFFFFFFF;

    for (size_t i = 0; i < n; i++)
    {
        char ch = s[i];
        for (size_t j = 0; j < 8; j++)
        {
            uint32_t b = (ch ^ crc) & 1;
            crc >>= 1;
            if (b)
                crc = crc ^ 0xEDB88320;
            ch >>= 1;
        }
    }
    return ~crc;
}

/**
 * Callback function to get the offset and size of the execution program in memory of the ELF we hook to.
 */
static int callback(struct dl_phdr_info *info, size_t size, void *data)
{
    if ((info->dlpi_phnum >= 3) && (info->dlpi_phdr[2].p_type == PT_LOAD) && (info->dlpi_phdr[2].p_flags == 5))
    {
        elf_crc = get_crc32((void *)(info->dlpi_addr + info->dlpi_phdr[2].p_vaddr + 10), 128);
    }
    return 1;
}

void getCPUID()
{
    unsigned eax;
    eax = 0;
    __get_cpuid(0, &eax, &cpu_vendor.ebx, &cpu_vendor.ecx, &cpu_vendor.edx);
    sprintf(cpu_vendor.cpuid, "%.4s%.4s%.4s", (const char *)&cpu_vendor.ebx, (const char *)&cpu_vendor.edx, (const char *)&cpu_vendor.ecx);
    if (getConfig()->showDebugMessages)
    {
        printf("Detected CPU Vendor: %s\n", cpu_vendor.cpuid);
    }
}

/**
 * Stop the game changing the DISPLAY environment variable
 */
int setenv(const char *name, const char *value, int overwrite)
{
    int (*_setenv)(const char *name, const char *value, int overwrite) = dlsym(RTLD_NEXT, "setenv");

    if (strcmp(name, "DISPLAY") == 0)
    {
        return 0;
    }

    return _setenv(name, value, overwrite);
}

/**
 * Fake the TEA_DIR environment variable to games that require it to run
 */
char *getenv(const char *name)
{
    char *(*_getenv)(const char *name) = dlsym(RTLD_NEXT, "getenv");

    if ((strcmp(name, "TEA_DIR") == 0) && ((getConfig()->crc32 == VIRTUA_TENNIS_3) || (getConfig()->crc32 == VIRTUA_TENNIS_3_TEST) ||
                                           ((getConfig()->crc32 == RAMBO)) || (getConfig()->crc32 == TOO_SPICY)))
    {
        if (getcwd(envpath, 100) == NULL)
            return "";
        char *ptr = strrchr(envpath, '/');
        if (ptr == NULL)
            return "";
        *ptr = '\0';
        return envpath;
    }
    else if (strcmp(name, "TEA_DIR") == 0)
    {
        if (getcwd(envpath, 100) == NULL)
            return "";
        return envpath;
    }

    return _getenv(name);
}

/**
 * Stop the game unsetting the DISPLAY environment variable
 */
int unsetenv(const char *name)
{
    int (*_unsetenv)(const char *name) = dlsym(RTLD_NEXT, "unsetenv");

    if (strcmp(name, "DISPLAY") == 0)
    {
        return 0;
    }

    return _unsetenv(name);
}

int snd_pcm_open(snd_pcm_t **pcmp, const char *name, snd_pcm_stream_t stream, int mode)
{
    int (*_snd_pcm_open)(snd_pcm_t **pcmp, const char *name, snd_pcm_stream_t stream, int mode) = dlsym(RTLD_NEXT, "snd_pcm_open");
    return _snd_pcm_open(pcmp, get_sndcard(), stream, mode);
}
