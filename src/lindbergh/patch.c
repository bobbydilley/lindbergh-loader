#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <unistd.h>

#include "patch.h"
#include "config.h"
#include "hook.h"
#include "securityboard.h"

extern cpuvendor cpu_vendor;

static void setVariable(uint32_t address, uint32_t value)
{
    int pagesize = sysconf(_SC_PAGE_SIZE);

    uint32_t *variable = (uint32_t *)address;

    void *toModify = (void *)(address - (address % pagesize));

    int prot = mprotect(toModify, pagesize, PROT_EXEC | PROT_WRITE);
    if (prot != 0)
    {
        printf("Error: Cannot unprotect memory region to change variable (%d)\n", prot);
        return;
    }
    *variable = value;
}

static void patchMemory(uint32_t address, char *value)
{

    size_t size = strlen((void *)value);
    if (size % 2 != 0)
    {
        printf("Patch value should be even.\n");
        exit(1);
    }

    char buf[size / 2];
    char tmpchr[3];
    char *p = value;
    for (int i = 0; i < size; i++)
    {
        memcpy(tmpchr, p, 2);
        tmpchr[2] = '\0';
        buf[i] = (int)strtol(tmpchr, NULL, 16);
        p += 2;
    }

    int pagesize = sysconf(_SC_PAGE_SIZE);

    void *toModify = (void *)(address - (address % pagesize));

    int prot = mprotect(toModify, pagesize, PROT_EXEC | PROT_WRITE);
    if (prot != 0)
    {
        printf("Error: Cannot unprotect memory region to change variable (%d)\n", prot);
        return;
    }

    memcpy((uint32_t *)address, buf, size / 2);
}

static void detourFunction(uint32_t address, void *function)
{
    int pagesize = sysconf(_SC_PAGE_SIZE);

    void *toModify = (void *)(address - (address % pagesize));

    int prot = mprotect(toModify, pagesize, PROT_EXEC | PROT_WRITE);
    if (prot != 0)
    {
        printf("Error: Cannot detour memory region to change variable (%d)\n", prot);
        return;
    }

    uint32_t jumpAddress = ((uint32_t)function - address) - 5;

    // Build the assembly to make the function jump
    char cave[5] = {0xE9, 0x00, 0x00, 0x00, 0x00};
    cave[4] = (jumpAddress >> (8 * 3)) & 0xFF;
    cave[3] = (jumpAddress >> (8 * 2)) & 0xFF;
    cave[2] = (jumpAddress >> (8 * 1)) & 0xFF;
    cave[1] = (jumpAddress) & 0xFF;

    memcpy((void *)address, cave, 5);
}

int stubRetZero()
{
    return 0;
}

int stubRetOne()
{
    return 1;
}

int stubRetMinusOne()
{
    return -1;
}

char stubRetZeroChar()
{
    return 0x00;
}

int amDongleInit()
{
    return 0;
}

int amDongleIsAvailable()
{
    return 1;
}

int amDongleUpdate()
{
    return 0;
}

int amDongleUserInfoEx(int a, int b, char *_arcadeContext)
{
    if (getConfig()->crc32 == INITIALD_4_REVE)
        memcpy(_arcadeContext, "SBNK", 4);
    else if (getConfig()->crc32 == INITIALD_5_EXP_20)
        memcpy(_arcadeContext, "SBPF", 4);
    else if (getConfig()->crc32 == HUMMER_EXTREME)
        memcpy(_arcadeContext, "SBST", 4);
    return 0;
}

int amDipswInit()
{
    return 0;
}

int amDipswSetLed()
{
    return 0;
}

int amDongleIsDevelop()
{
    return 1;
}

void print_binary(unsigned int number)
{
    if (number >> 1)
    {
        print_binary(number >> 1);
    }
    putc((number & 1) ? '1' : '0', stdout);
}

int amDipswGetData(uint8_t *dip)
{
    uint8_t result;
    uint32_t data;

    securityBoardIn(0x38, &data);

    result = (~data & 4) != 0; // Test Button
    if ((~data & 8) != 0)
        result |= 2; // Service Button
    if ((~data & 0x10) != 0)
        result |= 4; // ??
    if ((char)data >= 0)
        result |= 8; // ??
    if ((~data & 0x100) != 0)
        result |= 0x10; // Rotation
    if ((~data & 0x200) != 0)
        result |= 0x20; // Resolution Dip 4
    if ((~data & 0x400) != 0)
        result |= 0x40; // Resolution Dip 5
    if ((~data & 0x800) != 0)
        result |= 0x80; // Resolution Dip 6
    *dip = result;
    return 0;
}

void _putConsole(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    while (*format)
    {
        if (*format == '%')
        {
            format++;
            if ((*format == 'd') || (*format == 'n'))
            {
                printf("%d", va_arg(args, int));
            }
            else if (*format == 's')
            {
                printf("%s", va_arg(args, char *));
            }
            else if (*format == 'u')
            {
                printf("%u", va_arg(args, unsigned int));
            }
            else if (*format == '0')
            {
                format++;
                if (*format == '2')
                {
                    format++;
                    printf("%02X", va_arg(args, int));
                }
                else if (*format == '4')
                {
                    format++;
                    printf("%04X", va_arg(args, unsigned int));
                }
            }
            else
            {
                printf("\nFormat: %c.\n", *format);
            }
        }
        else
        {
            putchar(*format);
        }
        format++;
    }
    va_end(args);
    printf("\n");
}

int initPatch()
{
    EmulatorConfig *config = getConfig();

    switch (config->crc32)
    {
    case R_TUNED:
    {
        // Security
        detourFunction(0x08366846, amDongleInit);
        detourFunction(0x08365301, amDongleIsAvailable);
        detourFunction(0x08365cf7, amDongleUpdate);
    }
    break;

    case SEGA_RACE_TV:
    {
        // Security
        detourFunction(0x084d5b40, amDongleInit);
        detourFunction(0x084d45f9, amDongleIsAvailable);
        detourFunction(0x084d4fef, amDongleUpdate);
        // Fixes
        detourFunction(0x084d44fc, amDipswSetLed);
        detourFunction(0x084d4485, amDipswGetData);
    }
    break;
    case AFTER_BURNER_CLIMAX_REVA:
    {
        if (config->showDebugMessages == 1)
        {
            // Debug Messages
            setVariable(0x0a0a37e4, 2); // amBackupDebugLevel
            setVariable(0x0a0a3800, 2); // amCreditDebugLevel
            setVariable(0x0a0a3a58, 2); // amDipswDebugLevel
            setVariable(0x0a0a3a5c, 2); // amDongleDebugLevel
            setVariable(0x0a0a3a60, 2); // amEepromDebugLevel
            setVariable(0x0a0a3a64, 2); // amHwmonitorDebugLevel
            setVariable(0x0a0a3a68, 2); // amJvsDebugLevel
            setVariable(0x0a0a3a6c, 2); // amLibDebugLevel
            setVariable(0x0a0a3a70, 2); // amMiscDebugLevel
            setVariable(0x0a0a3a74, 2); // amOsinfoDebugLevel
            setVariable(0x0a0a3a78, 2); // amSysDataDebugLevel
            setVariable(0x0a0a3a80, 2); // bcLibDebugLevel
        }
        // Security
        detourFunction(0x081e4980, amDongleInit);
        detourFunction(0x081e4cce, amDongleIsAvailable);
        detourFunction(0x081e4bfa, amDongleUpdate);
        // Fixes
        detourFunction(0x081e48b6, amDipswGetData);
        detourFunction(0x081e492e, amDipswSetLed);
    }
    break;
    case AFTER_BURNER_CLIMAX_REVB:
    {
        if (config->showDebugMessages == 1)
        {
            // Debug Messages
            setVariable(0x0a0a0d24, 2); // amBackupDebugLevel
            setVariable(0x0a0a0d40, 2); // amCreditDebugLevel
            setVariable(0x0a0a0f98, 2); // amDipswDebugLevel
            setVariable(0x0a0a0f9c, 2); // amDongleDebugLevel
            setVariable(0x0a0a0fa0, 2); // amEepromDebugLevel
            setVariable(0x0a0a0fa4, 2); // amHwmonitorDebugLevel
            setVariable(0x0a0a0fa8, 2); // amJvsDebugLevel
            setVariable(0x0a0a0fac, 2); // amLibDebugLevel
            setVariable(0x0a0a0fb0, 2); // amMiscDebugLevel
            setVariable(0x0a0a0fb4, 2); // amOsinfoDebugLevel
            setVariable(0x0a0a0fb8, 2); // amSysDataDebugLevel
            setVariable(0x0a0a0fc0, 2); // bcLibDebugLevel
        }
        // Security
        detourFunction(0x081e3424, amDongleInit);
        detourFunction(0x081e3772, amDongleIsAvailable);
        detourFunction(0x081e369e, amDongleUpdate);
        // Fixes
        detourFunction(0x081e335a, amDipswGetData);
        detourFunction(0x081e33d2, amDipswSetLed);
    }
    break;
    case AFTER_BURNER_CLIMAX_SDX:
    {
        // Security
        detourFunction(0x081e4968, amDongleInit);
        detourFunction(0x081e4cb6, amDongleIsAvailable);
        detourFunction(0x081e4be2, amDongleUpdate);
        patchMemory(0x08064dc4, "9090909090");
        patchMemory(0x08064dca, "00");
        // Fixes
        detourFunction(0x081e489e, amDipswGetData);
        detourFunction(0x081e4916, amDipswSetLed);
    }
    break;
    case AFTER_BURNER_CLIMAX_CE:
    {
        // Security
        detourFunction(0x081e3458, amDongleInit);
        detourFunction(0x081e37a6, amDongleIsAvailable);
        detourFunction(0x081e36d2, amDongleUpdate);
        patchMemory(0x08064cfe, "9090909090");
        patchMemory(0x08064d04, "00");
        // Fixes
        detourFunction(0x081e338e, amDipswGetData);
        detourFunction(0x081e3406, amDipswSetLed);
    }
    break;
    case OUTRUN_2_SP_SDX_REVA:
    {
        if (config->showDebugMessages == 1)
        {
            // Debug Messages
            setVariable(0x0893a24c, 2); // amBackupDebugLevel
            setVariable(0x0893a260, 2); // amCreditDebugLevel
            setVariable(0x0893a4b8, 2); // amDipswDebugLevel
            setVariable(0x0893a4bc, 2); // amDongleDebugLevel
            setVariable(0x0893a4c0, 2); // amEepromDebugLevel
            setVariable(0x0893a4c4, 2); // amHwmonitorDebugLevel
            setVariable(0x0893a4c8, 2); // amJvsDebugLevel
            setVariable(0x0893a4cc, 2); // amLibDebugLevel
            setVariable(0x0893a4d0, 2); // amMiscDebugLevel
            setVariable(0x0893a4d4, 2); // amOsinfoDebugLevel
            setVariable(0x0893a4d8, 2); // amSysDataDebugLevel
            setVariable(0x0893a4e0, 2); // bcLibDebugLevel
        }
        // Security
        detourFunction(0x08190e80, amDongleInit);
        detourFunction(0x08191201, amDongleIsAvailable);
        detourFunction(0x08191125, amDongleUpdate);
        detourFunction(0x08191221, amDongleIsDevelop);
        // Fixes
        detourFunction(0x08190ca4, amDipswInit);
        detourFunction(0x08190db6, amDipswGetData);
        detourFunction(0x08190e2e, amDipswSetLed);

        // Taken from original patched OUTRUN 2 SP SDX by Android
        patchMemory(0x8105317, "909090909090909090909090909090909090909090909090909090909090909090909090");
        patchMemory(0x8048000 + 0x000C1593, "9090");
        patchMemory(0x8048000 + 0x000C1597, "9090");
        patchMemory(0x8048000 + 0x000C159D, "77");
    }
    break;
    case OUTRUN_2_SP_SDX_REVA_TEST:
    {
        // Security
        detourFunction(0x08066220, amDongleInit);
        detourFunction(0x080665a1, amDongleIsAvailable);
        detourFunction(0x080664c5, amDongleUpdate);
        // Fixes
        detourFunction(0x08066156, amDipswGetData);
        detourFunction(0x080661ce, amDipswSetLed); // Stub amDipswSetLed
        detourFunction(0x08066044, amDipswInit);
    }
    break;
    case THE_HOUSE_OF_THE_DEAD_4_REVA:
    {
        if (config->showDebugMessages == 1)
        {
            // Debug Messages
            setVariable(0x0a737c60, 2);          // amBackupDebugLevel
            setVariable(0x0a737c64, 2);          // amChunkDataDebugLevel
            setVariable(0x0a737c80, 2);          // amCreditDebugLevel
            setVariable(0x0a737ed8, 2);          // amDipswDebugLevel
            setVariable(0x0a737edc, 2);          // amDiskDebugLevel
            setVariable(0x0a737ee0, 2);          // amDongleDebugLevel
            setVariable(0x0a737ee4, 2);          // amEepromDebugLevel
            setVariable(0x0a737ee8, 2);          // amHmDebugLevel
            setVariable(0x0a737ef0, 2);          // amJvsDebugLevel
            setVariable(0x0a737f14, 2);          // amLibDebugLevel
            setVariable(0x0a737f18, 2);          // amMiscDebugLevel
            setVariable(0x0a737f1c, 2);          // amSysDataDebugLevel
            setVariable(0x0a737f20, 2);          // bcLibDebugLevel
            setVariable(0x0a737f24, 0x0FFFFFFF); // s_logMask
        }
        // Security
        detourFunction(0x08320178, amDongleInit);
        detourFunction(0x08320459, amDongleIsAvailable);
        detourFunction(0x083203c0, amDongleUpdate);
        // Fixes
        detourFunction(0x0831ddd7, amDipswGetData);
        detourFunction(0x0831de4f, amDipswSetLed);

        // Patch comparison check
        patchMemory(0x081b502a, "c7420400000001");

        // Patch fullscreen
        detourFunction(0x08376405, stubRetZero);

        // CPU patch to support AMD processors
        if (strcmp("AuthenticAMD", cpu_vendor.cpuid) == 0)
        {
            setVariable(0x0837d6aa, cpu_vendor.ebx);
            setVariable(0x0837d6ba, cpu_vendor.edx);
            setVariable(0x0837d6c5, cpu_vendor.ecx);
        }
    }
    break;
    case THE_HOUSE_OF_THE_DEAD_4_REVC:
    {
        // Security
        detourFunction(0x0831ad04, amDongleInit);
        detourFunction(0x0831b017, amDongleIsAvailable);
        detourFunction(0x0831af7e, amDongleUpdate);
        // Fixes
        detourFunction(0x0831875f, amDipswGetData);
        detourFunction(0x083187d7, amDipswSetLed);

        // Patch comparison check
        patchMemory(0x081b58be, "c7420400000001");

        // Patch fullscreen
        detourFunction(0x08372391, stubRetZero);

        // CPU patch to support AMD processors
        if (strcmp("AuthenticAMD", cpu_vendor.cpuid) == 0)
        {
            setVariable(0x0837963a, cpu_vendor.ebx);
            setVariable(0x0837964a, cpu_vendor.edx);
            setVariable(0x08379655, cpu_vendor.ecx);
        }
    }
    break;
    case THE_HOUSE_OF_THE_DEAD_4_REVA_TEST:
    {
        detourFunction(0x080677a0, amDongleInit);
        detourFunction(0x08067a81, amDongleIsAvailable);
        detourFunction(0x080679e8, amDongleUpdate);
        // Fixes
        detourFunction(0x08067653, amDipswGetData);
        detourFunction(0x080676cb, amDipswSetLed);

        // CPU patch to support AMD processors
        if (strcmp("AuthenticAMD", cpu_vendor.cpuid) == 0)
        {
            setVariable(0x0807217a, cpu_vendor.ebx);
            setVariable(0x0807218a, cpu_vendor.edx);
            setVariable(0x08072195, cpu_vendor.ecx);
        }
    }
    break;
    case THE_HOUSE_OF_THE_DEAD_4_SPECIAL:
    {
        detourFunction(0x08363438, amDongleInit);
        detourFunction(0x0836374b, amDongleIsAvailable);
        detourFunction(0x083636b2, amDongleUpdate);
        patchMemory(0x081f9491, "9090");
        patchMemory(0x081f9499, "01");
        // Fixes
        detourFunction(0x08360e93, amDipswGetData);
        detourFunction(0x08360f0b, amDipswSetLed);

        // CPU patch to support AMD processors
        if (strcmp("AuthenticAMD", cpu_vendor.cpuid) == 0)
        {
            setVariable(0x083cef0a, cpu_vendor.ebx);
            setVariable(0x083cef1a, cpu_vendor.edx);
            setVariable(0x083cef25, cpu_vendor.ecx);
        }
    }
    break;
    case THE_HOUSE_OF_THE_DEAD_4_SPECIAL_TEST:
    {
        detourFunction(0x0806e914, amDongleInit);
        detourFunction(0x0806ec27, amDongleIsAvailable);
        detourFunction(0x0806eb8e, amDongleUpdate);
        // Fixes
        detourFunction(0x0806e7c7, amDipswGetData);
        detourFunction(0x0806e83f, amDipswSetLed);

        // CPU patch to support AMD processors
        if (strcmp("AuthenticAMD", cpu_vendor.cpuid) == 0)
        {
            setVariable(0x0807a3ba, cpu_vendor.ebx);
            setVariable(0x0807a3ca, cpu_vendor.edx);
            setVariable(0x0807a3d5, cpu_vendor.ecx);
        }
    }
    break;
    case THE_HOUSE_OF_THE_DEAD_EX:
    {
        detourFunction(0x084ba886, amDongleInit);
        detourFunction(0x084b9341, amDongleIsAvailable);
        detourFunction(0x084b9d37, amDongleUpdate);
        // Fixes
        detourFunction(0x084b6a69, amDipswGetData);
        detourFunction(0x084b6adf, amDipswSetLed);

        // CPU patch to support AMD processors
        if (strcmp("AuthenticAMD", cpu_vendor.cpuid) == 0)
        {
            setVariable(0x0849E2AD, cpu_vendor.ebx);
            setVariable(0x0849E2B7, cpu_vendor.edx);
            setVariable(0x0849E2C1, cpu_vendor.ecx);
        }
    }
    break;
    case THE_HOUSE_OF_THE_DEAD_EX_TEST:
    {
        detourFunction(0x08078996, amDongleInit);
        detourFunction(0x08077451, amDongleIsAvailable);
        detourFunction(0x08077e47, amDongleUpdate);
        // Fixes
        detourFunction(0x080772dd, amDipswGetData);
        detourFunction(0x08077353, amDipswSetLed);

        // CPU patch to support AMD processors
        if (strcmp("AuthenticAMD", cpu_vendor.cpuid) == 0)
        {
            setVariable(0x080847BD, cpu_vendor.ebx);
            setVariable(0x080847C7, cpu_vendor.edx);
            setVariable(0x080847D1, cpu_vendor.ecx);
        }
    }
    break;
    case VIRTUA_FIGHTER_5_REVC:
    {
        // Security
        detourFunction(0x085c6010, amDongleInit);
        detourFunction(0x085c63cc, amDongleIsAvailable);
        detourFunction(0x085c62f0, amDongleUpdate);

        // Fixes and patches to bypss network check
        detourFunction(0x085c5f46, amDipswGetData);
        detourFunction(0x085c5fbe, amDipswSetLed); // Stub amDipswSetLed
        detourFunction(0x080b3426, stubRetZero);   // Stub returns 0
        detourFunction(0x080cb6d4, stubRetZero);   // Stub returns 0
        detourFunction(0x0840889e, stubRetZero);   // Stub returns 0
        detourFunction(0x0840ab90, stubRetZero);   // Stub returns 0
        patchMemory(0x080e17af, "b800000000");     // Patch IDK what
    }
    break;

    case VIRTUA_FIGHTER_5_EXPORT:
    {
        // Security
        detourFunction(0x084fca4c, amDongleInit);
        detourFunction(0x084fcd2c, amDongleUpdate);
        detourFunction(0x084fce08, amDongleIsAvailable);
        detourFunction(0x084fce28, amDongleIsDevelop);

        // Fixes and patches to bypss network check
        detourFunction(0x084fc982, amDipswGetData);
        detourFunction(0x084fc9fa, amDipswSetLed);
    }
    break;

    case LETS_GO_JUNGLE_REVA:
    {
        if (config->showDebugMessages == 1)
        {
            setVariable(0x08c10604, 2);          // amBackupDebugLevel
            setVariable(0x08c10620, 2);          // amCreditDebugLevel
            setVariable(0x08c10878, 2);          // amDipswDebugLevel
            setVariable(0x08c1087c, 2);          // amDongleDebugLevel
            setVariable(0x08c10880, 2);          // amEepromDebugLevel
            setVariable(0x08c10884, 2);          // amHwmonitorDebugLevel
            setVariable(0x08c10888, 2);          // amJvsDebugLevel
            setVariable(0x08c1088c, 2);          // amLibDebugLevel
            setVariable(0x08c10890, 2);          // amMiscDebugLevel
            setVariable(0x08c10898, 2);          // amSysDataDebugLevel
            setVariable(0x08c108a0, 2);          // bcLibDebugLevel
            setVariable(0x08c10894, 2);          // amOsinfoDebugLevel
            setVariable(0x08c108a4, 0x0FFFFFFF); // s_logMask
            // detourFunction(0x08074a8c, _putConsole); // Debug Messages
        }
        // Security
        detourFunction(0x084e9fbc, amDongleInit);
        detourFunction(0x084ea378, amDongleIsAvailable);
        detourFunction(0x084ea29c, amDongleUpdate);
        patchMemory(0x0807b86a, "9090"); // Patch initializeArcadeBackup
        // Fixes
        detourFunction(0x084e9ef2, amDipswGetData);
        detourFunction(0x084e9f6a, amDipswSetLed);
        patchMemory(0x084125f0, "9090"); // No full screen
        // Set Resolution
        // setVariable(0x082e1323, 0x00000550); // Set ResX
        // setVariable(0x082e1330, 0x00000300); // Set ResY
    }

    case LETS_GO_JUNGLE:
    {
        if (config->showDebugMessages == 1)
        {
            setVariable(0x08c083a4, 2);          // amBackupDebugLevel
            setVariable(0x08c083c0, 2);          // amCreditDebugLevel
            setVariable(0x08c08618, 2);          // amDipswDebugLevel
            setVariable(0x08c0861c, 2);          // amDongleDebugLevel
            setVariable(0x08c08620, 2);          // amEepromDebugLevel
            setVariable(0x08c08624, 2);          // amHwmonitorDebugLevel
            setVariable(0x08c08628, 2);          // amJvsDebugLevel
            setVariable(0x08c0862c, 2);          // amLibDebugLevel
            setVariable(0x08c08630, 2);          // amMiscDebugLevel
            setVariable(0x08c08638, 2);          // amSysDataDebugLevel
            setVariable(0x08c08640, 2);          // bcLibDebugLevel
            setVariable(0x08c08634, 2);          // amOsinfoDebugLevel
            setVariable(0x08c08644, 0x0FFFFFFF); // s_logMask
            // detourFunction(0x08074a8c, _putConsole); // Debug Messages
        }
        // Security
        detourFunction(0x084e50d8, amDongleInit);
        detourFunction(0x084e5459, amDongleIsAvailable);
        detourFunction(0x084e537d, amDongleUpdate);
        patchMemory(0x0807b76a, "9090"); // Patch initializeArcadeBackup
        // Fixes
        detourFunction(0x084e500e, amDipswGetData);
        detourFunction(0x084e5086, amDipswSetLed);
        patchMemory(0x0840d858, "9090"); // No full screen
        // Set Resolution
        setVariable(0x082E006b, 0x00000550); // Set ResX
        setVariable(0x082E0078, 0x00000300); // Set ResY
        // Test
        // patchMemory(0x084032e0, "33c990");
        // 08523950 return ADXM_SetupFramework
    }
    break;

    case LETS_GO_JUNGLE_SPECIAL:
    {
        if (config->showDebugMessages == 1)
        {
            setVariable(0x08c453e4, 2);          // amBackupDebugLevel
            setVariable(0x08c45400, 2);          // amCreditDebugLevel
            setVariable(0x08c45658, 2);          // amDipswDebugLevel
            setVariable(0x08c4565c, 2);          // amDongleDebugLevel
            setVariable(0x08c45660, 2);          // amEepromDebugLevel
            setVariable(0x08c45664, 2);          // amHwmonitorDebugLevel
            setVariable(0x08c45668, 2);          // amJvsDebugLevel
            setVariable(0x08c4566c, 2);          // amLibDebugLevel
            setVariable(0x08c45670, 2);          // amMiscDebugLevel
            setVariable(0x08c45678, 2);          // amSysDataDebugLevel
            setVariable(0x08c45680, 2);          // bcLibDebugLevel
            setVariable(0x08c45674, 2);          // amOsinfoDebugLevel
            setVariable(0x08c45684, 0x0FFFFFFF); // s_logMask
            detourFunction(0x08075012, _putConsole);
        }
        // Security
        detourFunction(0x08510320, amDongleInit);
        detourFunction(0x085106dc, amDongleIsAvailable);
        detourFunction(0x08510600, amDongleUpdate);
        patchMemory(0x0807e609, "909090909090");
        // Fixes
        detourFunction(0x08510256, amDipswGetData);
        detourFunction(0x085102ce, amDipswSetLed);
        patchMemory(0x08438954, "9090"); // No full screen
        // Set Resolution
        // setVariable(0x08303C4B, 0x00000780); // Set ResX
        // setVariable(0x08303C58, 0x00000438); // Set ResY
    }
    break;

    case INITIALD_4:
    {
        if (config->showDebugMessages == 1)
        {
            setVariable(0x08d71750, 2);          // amBackupDebugLevel
            setVariable(0x08d71760, 2);          // amCreditDebugLevel
            setVariable(0x08d719b8, 2);          // amDipswDebugLevel
            setVariable(0x08d719bc, 2);          // amDongleDebugLevel
            setVariable(0x08d719c0, 2);          // amEepromDebugLevel
            setVariable(0x08d719c4, 2);          // amHwmonitorDebugLevel
            setVariable(0x08d719c8, 2);          // amJvsDebugLevel
            setVariable(0x08d719cc, 2);          // amLibDebugLevel
            setVariable(0x08d719d0, 2);          // amMiscDebugLevel
            setVariable(0x08d719d8, 2);          // amSysDataDebugLevel
            setVariable(0x08d719e0, 2);          // bcLibDebugLevel
            setVariable(0x08d719d4, 2);          // amOsinfoDebugLevel
            setVariable(0x08d719e4, 0x0FFFFFFF); // s_logMask
            // detourFunction(0x0808f9a8, _putConsole); // Crashes the game sometimes.
        }
        // Security
        detourFunction(0x086e2336, amDongleInit);
        detourFunction(0x086e0d81, amDongleIsAvailable);
        detourFunction(0x086e17e5, amDongleUpdate);
        // Fixes
        detourFunction(0x086e0c0d, amDipswGetData);
        detourFunction(0x086e0c84, amDipswSetLed); // amDipswSetLED

        detourFunction(0x0821e6dc, stubRetOne); // isEthLinkUp
        patchMemory(0x082cb412, "c0270900");    // tickInitStoreNetwork
        patchMemory(0x082cb6d9, "e950010000");  // tickWaitDHCP
    }
    break;
    case INITIALD_4_REVE:
    {
        if (config->showDebugMessages == 1)
        {
            setVariable(0x08d972d0, 2);          // amBackupDebugLevel
            setVariable(0x08d972e0, 2);          // amCreditDebugLevel
            setVariable(0x08d97538, 2);          // amDipswDebugLevel
            setVariable(0x08d9753c, 2);          // amDongleDebugLevel
            setVariable(0x08d97540, 2);          // amEepromDebugLevel
            setVariable(0x08d97544, 2);          // amHwmonitorDebugLevel
            setVariable(0x08d97548, 2);          // amJvsDebugLevel
            setVariable(0x08d9754c, 2);          // amLibDebugLevel
            setVariable(0x08d97550, 2);          // amMiscDebugLevel
            setVariable(0x08d97554, 2);          // amSysDataDebugLevel
            setVariable(0x08d97558, 2);          // bcLibDebugLevel
            setVariable(0x08d97560, 2);          // amOsinfoDebugLevel
            setVariable(0x08d97564, 0x0FFFFFFF); // s_logMask
            // detourFunction(0x08090478, _putConsole); // Crashes the game sometimes.
        }
        // Security
        detourFunction(0x087106e6, amDongleInit);
        detourFunction(0x0870f131, amDongleIsAvailable);
        detourFunction(0x0870fb95, amDongleUpdate);
        detourFunction(0x087105ad, amDongleUserInfoEx);

        // Fixes
        detourFunction(0x0870efbd, amDipswGetData);
        detourFunction(0x0870f034, stubRetZero); // amDipswSetLed
        detourFunction(0x08230fde, stubRetOne);  // isEthLinkUp
        patchMemory(0x082df87d, "e954010000");   // tickWaitDHCP
        patchMemory(0x082e0ec9, "eb60");         // tickInitAddress
        setVariable(0x08580979, 0x000126e9);     // Avoid Full Screen set from Game
        // Set Resolution
        setVariable(0x0837b12d, 0x0000f0e9); // Force set resolution
        setVariable(0x0837b223, 0x00000550); // Set ResX
        setVariable(0x0837b228, 0x00000300); // Set ResY
    }
    break;
    case INITIALD_5_EXP_20:
    {

        if (config->showDebugMessages == 1)
        {
            setVariable(0x093f6fa0, 2);          // amBackupDebugLevel
            setVariable(0x093f6fc0, 2);          // amCreditDebugLevel
            setVariable(0x093f7218, 2);          // amDipswDebugLevel
            setVariable(0x093f721c, 2);          // amDongleDebugLevel
            setVariable(0x093f7220, 2);          // amEepromDebugLevel
            setVariable(0x093f7224, 2);          // amHwmonitorDebugLevel
            setVariable(0x093f7228, 2);          // amJvsDebugLevel
            setVariable(0x093f722c, 2);          // amLibDebugLevel
            setVariable(0x093f7230, 2);          // amMiscDebugLevel
            setVariable(0x093f7238, 2);          // amSysDataDebugLevel
            setVariable(0x093f7240, 2);          // bcLibDebugLevel
            setVariable(0x093f7234, 2);          // amOsinfoDebugLevel
            setVariable(0x093f7244, 0x0FFFFFFF); // s_logMask
                                                 //    detourFunction(0x0808f9a8, _putConsole); // Crashes the game sometimes.
            patchMemory(0x08942d91, "E912ffffff");
            patchMemory(0x0894375b, "EB29");
        }
        // Security
        detourFunction(0x0893dd25, amDongleInit);
        detourFunction(0x0893c5b1, amDongleIsAvailable);
        detourFunction(0x0893d0d2, amDongleUpdate);
        // Fixes
        detourFunction(0x0893c43d, amDipswGetData);
        detourFunction(0x0893c4b3, amDipswSetLed); // amDipswSetLed
        detourFunction(0x0832fca6, stubRetOne);    // isEthLinkUp
        patchMemory(0x08456348, "e954010000");     // tickWaitDHCP
        patchMemory(0x0845843b, "eb60");           // tickInitAddress
        patchMemory(0x08455584, "C0270900");       // tickInitStoreNetwork
        detourFunction(0x08943eb6, stubRetZero);   // amOsinfoExecDhcpNic
        detourFunction(0x085135e0, stubRetZero);   // isUseServerBox
        // Set Resolution
        patchMemory(0x0855a48d, "E9f000");   // Accept different Resolutions
        setVariable(0x0855a583, 0x00000550); // Set ResX
        setVariable(0x0855a588, 0x00000300); // Set ResY

        // amsInit
        patchMemory(0x08938437, "00");         // Avoids strBBBlackList
        patchMemory(0x089385cb, "e9e7000000"); // Eliminate Dongle Challenges
        patchMemory(0x0893871e, "74");         //      // Returns 1
        patchMemory(0x0893871a, "01000000");   // Returns 1
        // amsCheckKeyDataVerify
        patchMemory(0x08939696, "00000000"); // amsCheckKeyDataVerify
        patchMemory(0x0893962b, "00000000"); // amsCheckKeyDataVerify

        detourFunction(0x0893dbeb, amDongleUserInfoEx);
    }
    break;
    case INITIALD_ARCADE_STAGE_5:
    {
        if (config->showDebugMessages == 1)
        {
            setVariable(0x093f6fa0, 2);            // amBackupDebugLevel
            setVariable(0x093f6fc0, 2);            // amCreditDebugLevel
            setVariable(0x093f7218, 2);            // amDipswDebugLevel
            setVariable(0x093f721c, 2);            // amDongleDebugLevel
            setVariable(0x093f7220, 2);            // amEepromDebugLevel
            setVariable(0x093f7224, 2);            // amHwmonitorDebugLevel
            setVariable(0x093f7228, 2);            // amJvsDebugLevel
            setVariable(0x093f722c, 2);            // amLibDebugLevel
            setVariable(0x093f7230, 2);            // amMiscDebugLevel
            setVariable(0x093f7238, 2);            // amSysDataDebugLevel
            setVariable(0x093f7240, 2);            // bcLibDebugLevel
            setVariable(0x093f7234, 2);            // amOsinfoDebugLevel
            setVariable(0x093f7244, 0x0FFFFFFF);   // s_logMask
                                                   //    detourFunction(0x0808f9a8, _putConsole); // Crashes the game sometimes.
            patchMemory(0x08942fe1, "E912ffffff"); // Removes grep messages
            patchMemory(0x089439ab, "EB29");       // Removes grep messages
        }
        // Security
        detourFunction(0x0893df75, amDongleInit);
        detourFunction(0x0893c801, amDongleIsAvailable);
        detourFunction(0x0893d322, amDongleUpdate);
        detourFunction(0x0893de3b, amDongleUserInfoEx);
        patchMemory(0x0845beff, "00");
        patchMemory(0x0845bf33, "00");
        patchMemory(0x0845bf51, "00");
        detourFunction(0x084fcbba, stubRetOne);
        patchMemory(0x084fde5a, "E984010000");
        patchMemory(0x084fe12a, "00");

        // Fixes
        detourFunction(0x0893c68d, amDipswGetData);
        detourFunction(0x0893c703, amDipswSetLed); // amDipswSetLed
        detourFunction(0x0832fe46, stubRetOne);    // isEthLinkUp
        patchMemory(0x084566d8, "e954010000");     // tickWaitDHCP
        patchMemory(0x084587cb, "eb60");           // tickInitAddress
        patchMemory(0x08455914, "C0270900");       // tickInitStoreNetwork
        detourFunction(0x08944106, stubRetZero);   // amOsinfoExecDhcpNic
        detourFunction(0x08513810, stubRetZero);   // isUseServerBox
        // Set Resolution
        patchMemory(0x0855a6dd, "E9f000");   // Accept different Resolutions
        setVariable(0x0855a7d3, 0x00000550); // Set ResX
        setVariable(0x0855a7d8, 0x00000300); // Set ResY
        // amsInit
        patchMemory(0x08938437, "00");         // Avoids strBBBlackList
        patchMemory(0x089385cb, "e9e7000000"); // Eliminate Dongle Challenges
        patchMemory(0x0893871e, "74");         //           // Returns 1
        patchMemory(0x0893871a, "01000000");   // Returns 1
        // amsCheckKeyDataVerify
        patchMemory(0x08939696, "00000000"); // amsCheckKeyDataVerify
        patchMemory(0x0893962b, "00000000"); // amsCheckKeyDataVerify
    }
    break;
    case SEGABOOT_2_4_SYM:
    {
        if (config->showDebugMessages == 1)
        {
            setVariable(0x0808da48, 2);          // amAdtecDebugLevel
            setVariable(0x0808cf8c, 2);          // amAtaDebugLevel
            setVariable(0x0808cf90, 2);          // amBackupDebugLevel
            setVariable(0x0808cf94, 2);          // amChunkDataDebugLevel
            setVariable(0x0808cfa0, 2);          // amCreditDebugLevel
            setVariable(0x0808d1f8, 2);          // amDipswDebugLevel
            setVariable(0x0808d1fc, 2);          // amDiskDebugLevel
            setVariable(0x0808d200, 2);          // amDongleDebugLevel
            setVariable(0x0808d204, 2);          // amEepromDebugLevel
            setVariable(0x0808d208, 2);          // amHmDebugLevel
            setVariable(0x0808d210, 2);          // amJvsDebugLevel
            setVariable(0x0808d214, 2);          // amLibDebugLevel
            setVariable(0x0808d218, 2);          // amMiscDebugLevel
            setVariable(0x0808d21c, 2);          // amSysDataDebugLevel
            setVariable(0x0808d220, 2);          // bcLibDebugLevel
            setVariable(0x0808cf58, 2);          // g_DebugLevel
            setVariable(0x0808d224, 0x0FFFFFFF); // logmask
        }

        detourFunction(0x0805e8b0, amDongleInit);
        detourFunction(0x0805ebc3, amDongleIsAvailable);
        detourFunction(0x0805eb2a, amDongleUpdate);
        detourFunction(0x0805c30b, amDipswGetData);
    }
    break;
    case VIRTUA_TENNIS_3:
    {
        // Security
        detourFunction(0x0831c724, amDongleInit);
        detourFunction(0x0831ca37, amDongleIsAvailable);
        detourFunction(0x0831c99e, amDongleUpdate);
        // Fixes
        detourFunction(0x0831c5d7, amDipswGetData);
        detourFunction(0x0831c64f, stubRetZero);
        setVariable(0x0827ae1b, 0x34891beb); // Disable Fullscreen set from the game
        // Resolution
        patchMemory(0x08175b03, "9090");
        setVariable(0x08175b0b, 0x00000550);
        setVariable(0x08175b15, 0x00000300);
        detourFunction(0x08177c1c, stubRetOne); // Patch seterror
    }
    break;
    case VIRTUA_TENNIS_3_TEST:
    {
        if (config->showDebugMessages == 1)
        {
            // Debug
            detourFunction(0x08054d14, _putConsole); // Crashes the game sometimes.
        }
        // Security
        detourFunction(0x0815f610, amDongleInit);
        detourFunction(0x0815f923, amDongleIsAvailable);
        detourFunction(0x0815f88a, amDongleUpdate);
        // Fixes
        detourFunction(0x0815d06b, amDipswGetData);
        detourFunction(0x0815d0e3, amDipswSetLed);
    }
    break;
    case RAMBO:
    {
        // Security
        detourFunction(0x082c4746, amDongleInit);
        detourFunction(0x082c3201, amDongleIsAvailable);
        detourFunction(0x082c3bf7, amDongleUpdate);
        // Fixes
        detourFunction(0x082c308d, amDipswGetData);
        detourFunction(0x082c3103, amDipswSetLed);
    }
    break;
    case TOO_SPICY:
    {
        // Security
        detourFunction(0x0831cf02, amDongleInit);
        detourFunction(0x0831b94d, amDongleIsAvailable);
        detourFunction(0x0831c3b1, amDongleUpdate);
        // Fixes
        detourFunction(0x0831907d, amDipswGetData);
        detourFunction(0x083190f4, amDipswSetLed);
        // CPU patch to support AMD processors
        if (strcmp("AuthenticAMD", cpu_vendor.cpuid) == 0)
        {
            setVariable(0x08399ADA, cpu_vendor.ebx);
            setVariable(0x08399AEA, cpu_vendor.edx);
            setVariable(0x08399AF5, cpu_vendor.ecx);
        }
    }
    break;
    case PRIMEVAL_HUNT:
    {
        // Security
        detourFunction(0x08141770, amDongleInit);
        detourFunction(0x08140229, amDongleIsAvailable);
        detourFunction(0x08140c1f, amDongleUpdate);
        patchMemory(0x08055264, "EB");
        // Fixes
        detourFunction(0x081400b5, amDipswGetData);
        detourFunction(0x0814012c, stubRetZero);
        patchMemory(0x08052cb2, "9090909090");
        patchMemory(0x0805206a, "00");
        // test
        // patchMemory(0x0804d8be, "909090909090909090909090909090");
        // patchMemory(0x0804e5cf, "909090909090909090909090909090");
        // Res
        // setVariable(0x0805af8b, 0x00000438);
        // setVariable(0x0805af93, 0x000003c0);
        // setVariable(0x0805afa2, 0x000003c0);
        // setVariable(0x0805b0ed, 0x00000438);
        // setVariable(0x0805aff5, 0x000003c0);
    }
    break;
    case GHOST_SQUAD_EVOLUTION:
    {
        if (config->showDebugMessages == 1)
        {
            // Debug
            detourFunction(0x080984fe, _putConsole); // Crashes the game sometimes.
        }
        // Security
        detourFunction(0x08183046, amDongleInit);
        detourFunction(0x08181a91, amDongleIsAvailable);
        detourFunction(0x081824f5, amDongleUpdate);
        // Fixes
        detourFunction(0x0818191d, amDipswGetData);
        detourFunction(0x08181994, stubRetZero);
        // patchMemory(0x807c9ec, "01");
        // detourFunction(0x080f3f94, stubRetZero); //eliminates init_heap function.
    }
    break;
    case HUMMER_EXTREME:
    {
        if (config->showDebugMessages == 1)
        {
            setVariable(0x0a79a834, 2); // amAdtecDebugLevel
            setVariable(0x0a79a838, 2); // amAtaDebugLevel
            setVariable(0x0a79a83c, 2); // amBackupDebugLevel
            setVariable(0x0a79a840, 2); // amCreditDebugLevel
            setVariable(0x0a79aa98, 2); // amDipswDebugLevel
            setVariable(0x0a79aa9c, 2); // amDongleDebugLevel
            setVariable(0x0a79aaa0, 2); // amEepromDebugLevel
            setVariable(0x0a79aaa4, 2); // amHwmonitorDebugLevel
            setVariable(0x0a79aaa8, 2); // amJvsDebugLevel
            setVariable(0x0a79aaac, 2); // amLibDebugLevel
            setVariable(0x0a79aab0, 2); // amMiscDebugLevel
            setVariable(0x0a79aab4, 2); // amOsinfoDebugLevel
            setVariable(0x0a79a830, 2); // amsLibDebugLevel
            setVariable(0x0a79aab8, 2); // amSysDataDebugLevel
            setVariable(0x0a79aac0, 2); // bcLibDebugLevel
        }

        // Security
        detourFunction(0x0831c0d1, amDongleInit);
        detourFunction(0x0831a95d, amDongleIsAvailable);
        detourFunction(0x0831b47e, amDongleUpdate);
        detourFunction(0x0831a97a, amDongleIsDevelop);
        detourFunction(0x0831bf97, amDongleUserInfoEx);
        detourFunction(0x0831668c, stubRetZero);     // amsInit
        detourFunction(0x08170654, stubRetMinusOne); // checkError

        // Security Board
        detourFunction(0x0831a7e9, amDipswGetData);
        detourFunction(0x0831a85f, amDipswSetLed);

        // Networking
        detourFunction(0x08323886, stubRetZero); // amOsinfoModifyNetworkAdr
        detourFunction(0x0832386b, stubRetZero); // amOsinfoModifyNetworkProperty

        // While we can't get into the test menu, immidiately return from the call to clSteerErrorDisp::run that complains about the calibration values.
        patchMemory(0x080e8b40, "C3");
    }
    break;
    default:
        break;
    }
    return 0;
}
