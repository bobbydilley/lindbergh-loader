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
    char cave[5] = {0xE9, 0x0, 0x00, 0x00, 0x00};
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
    if (getConfig()->game == INITIALD_4_REVE)
        memcpy(_arcadeContext, "SBNK",4);
    else if (getConfig()->game == INITIALD_5)
        memcpy(_arcadeContext, "SBPF",4);
    else if (getConfig()->game == HUMMER_EXTREME)
        memcpy(_arcadeContext, "SBST",4);
    return 0;
}

int amLibInit()
{
    uint32_t *amLibContext = (uint32_t *)0x080986c0; // 0x0809cb00;
    *amLibContext = 1;
    uint32_t *amLibInitialized = (uint32_t *)0x080986c4;       // 0x0809cb04;
    uint16_t *amLibPort1 = (uint16_t *)(0x080986c4 + 4);       //(0x0809cb04 + 4);
    uint16_t *amLibPort2 = (uint16_t *)(0x080986c4 + 6);       //(0x0809cb04 + 6);
    uint32_t *bcLibInitialized = (uint32_t *)(0x080986c4 + 8); // 0x0809cb0c;
    *amLibInitialized = 1;
    *amLibPort1 = 0xd000;
    *amLibPort2 = 0x0004;
    *bcLibInitialized = 0;
    int res = ((int (*)(void))0x084dedc4)(); // 0x08065d80)(); IDK what it was.
    if (res == 1)
        *bcLibInitialized = 1;
    return 0;
}

int amDipswInit()
{
    uint32_t *amDipswContext = (uint32_t *)0x080980e8;         // 0x0809c12c;
    uint32_t *amDipswContext1 = (uint32_t *)(0x080980e8 + 4);  //(0x0809c12c + 4);
    uint32_t *amDipswContext2 = (uint32_t *)(0x080980e8 + 8);  //(0x0809c12c + 8);
    uint32_t *amDipswContext3 = (uint32_t *)(0x080980e8 + 12); //(0x0809c12c + 12);
    // typedef void *(*___constant_c_and_count_memset)(uint32_t *, int, size_t);
    //___constant_c_and_count_memset func = (___constant_c_and_count_memset)//0x0805c3d5;
    // func(amDipswContext, 0, 4);
    *amDipswContext = 1;
    *amDipswContext1 = 1;
    *amDipswContext2 = 1;
    *amDipswContext3 = 1;
    return 0;
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

    switch (config->game)
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
        detourFunction(0x084d44fc, stubRetZero); // Stub amDipswSetLed
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
        detourFunction(0x081e492e, stubRetZero); // Stub amDipswSetLed
        // Does not work
        // setVariable(0x08061c31, 0x0000000c); // Force HD resolution
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
        detourFunction(0x081e33d2, stubRetZero); // Stub amDipswSetLed
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
        detourFunction(0x081e4916, stubRetZero); // Stub amDipswSetLed
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
        detourFunction(0x081e3406, stubRetZero); // Stub amDipswSetLed
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
        // Fixes
        detourFunction(0x08190db6, amDipswGetData);
        detourFunction(0x08190e2e, stubRetZero); // Stub amDipswSetLed
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
        detourFunction(0x080661ce, stubRetZero); // Stub amDipswSetLed
        detourFunction(0x08066044, amDipswInit);
    }
    break;
    case THE_HOUSE_OF_THE_DEAD_4:
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
        detourFunction(0x0831de4f, stubRetZero); // Stub amDipswSetLed
        // CPU patch to support AMD processors
        if (strcmp("AuthenticAMD", cpu_vendor.cpuid) == 0)
        {
            setVariable(0x0837d6aa, cpu_vendor.ebx);
            setVariable(0x0837d6ba, cpu_vendor.edx);
            setVariable(0x0837d6c5, cpu_vendor.ecx);
        }
    }
    break;
    case THE_HOUSE_OF_THE_DEAD_4_STRIPPED:
    {
        // Security
        detourFunction(0x0831ad04, amDongleInit);
        detourFunction(0x0831b017, amDongleIsAvailable);
        detourFunction(0x0831af7e, amDongleUpdate);
        // Fixes
        detourFunction(0x0831875f, amDipswGetData);
        detourFunction(0x083187d7, stubRetZero); // Stub amDipswSetLed
        // CPU patch to support AMD processors
        if (strcmp("AuthenticAMD", cpu_vendor.cpuid) == 0)
        {
            setVariable(0x0837963a, cpu_vendor.ebx);
            setVariable(0x0837964a, cpu_vendor.edx);
            setVariable(0x08379655, cpu_vendor.ecx);
        }
    }
    break;
    case THE_HOUSE_OF_THE_DEAD_4_TEST:
    {
        detourFunction(0x080677a0, amDongleInit);
        detourFunction(0x08067a81, amDongleIsAvailable);
        detourFunction(0x080679e8, amDongleUpdate);
        // Fixes
        detourFunction(0x08067653, amDipswGetData);
        detourFunction(0x080676cb, stubRetZero); // Stub amDipswSetLed
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
        detourFunction(0x08360f0b, stubRetZero); // Stub amDipswSetLed
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
        detourFunction(0x0806e83f, stubRetZero); // Stub amDipswSetLed
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
        detourFunction(0x084b6adf, stubRetZero); // Stub amDipswSetLed
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
        detourFunction(0x08077353, stubRetZero); // Stub amDipswSetLed
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
        detourFunction(0x085c5fbe, stubRetZero); // Stub amDipswSetLed
        detourFunction(0x080b3426, stubRetZero); // Stub returns 0
        detourFunction(0x080cb6d4, stubRetZero); // Stub returns 0
        detourFunction(0x0840889e, stubRetZero); // Stub returns 0
        detourFunction(0x0840ab90, stubRetZero); // Stub returns 0
        patchMemory(0x080e17af, "b800000000");   // Patch IDK what
    }
    break;

    case LETS_GO_JUNGLE:
    {
        if (config->showDebugMessages == 1)
        {
            setVariable(0x08c083a4, 2);              // amBackupDebugLevel
            setVariable(0x08c083c0, 2);              // amCreditDebugLevel
            setVariable(0x08c08618, 2);              // amDipswDebugLevel
            setVariable(0x08c0861c, 2);              // amDongleDebugLevel
            setVariable(0x08c08620, 2);              // amEepromDebugLevel
            setVariable(0x08c08624, 2);              // amHwmonitorDebugLevel
            setVariable(0x08c08628, 2);              // amJvsDebugLevel
            setVariable(0x08c0862c, 2);              // amLibDebugLevel
            setVariable(0x08c08630, 2);              // amMiscDebugLevel
            setVariable(0x08c08638, 2);              // amSysDataDebugLevel
            setVariable(0x08c08640, 2);              // bcLibDebugLevel
            setVariable(0x08c08634, 2);              // amOsinfoDebugLevel
            setVariable(0x08c08644, 0x0FFFFFFF);     // s_logMask
            detourFunction(0x08074a8c, _putConsole); // Debug Messages
        }
        // Security
        detourFunction(0x084e50d8, amDongleInit);
        detourFunction(0x084e5459, amDongleIsAvailable);
        detourFunction(0x084e537d, amDongleUpdate);
        patchMemory(0x0807b76a, "9090"); // Patch initializeArcadeBackup
        // Fixes
        detourFunction(0x084e500e, amDipswGetData);
        detourFunction(0x084e5086, stubRetZero); // Stub amDipswSetLed
        patchMemory(0x0840d858, "9090");         // No more Full Screen from the Game
        // Set Resolution
        // setVariable(0x082E006b, 0x00000780); // Set ResX
        // setVariable(0x082E0078, 0x00000438); // Set ResY

        // From Teknoparrot AMDFIX
        // setVariable(0x083ef701, 0x00036ee9); // AMDFIX
        // setVariable(0x084032e0, 0x8b90c933); // fix shader compilation with AMD GPUs
        // setVariable(0x08523950, 0x000000c3); // Remove ADXM_SetupFramework (Not necessary)
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
        detourFunction(0x085102ce, stubRetZero); // Stub amDipswSetLed
        patchMemory(0x08438954, "9090");         // No more Full Screen from the Game
        // Set Resolution
        // setVariable(0x08303C4B, 0x00000780); // Set ResX
        // setVariable(0x08303C58, 0x00000438); // Set ResY
    }
    break;

    case INITIALD_4:
    {
        if (config->showDebugMessages == 1)
        {
            setVariable(0x08d71750, 2);              // amBackupDebugLevel
            setVariable(0x08d71760, 2);              // amCreditDebugLevel
            setVariable(0x08d719b8, 2);              // amDipswDebugLevel
            setVariable(0x08d719bc, 2);              // amDongleDebugLevel
            setVariable(0x08d719c0, 2);              // amEepromDebugLevel
            setVariable(0x08d719c4, 2);              // amHwmonitorDebugLevel
            setVariable(0x08d719c8, 2);              // amJvsDebugLevel
            setVariable(0x08d719cc, 2);              // amLibDebugLevel
            setVariable(0x08d719d0, 2);              // amMiscDebugLevel
            setVariable(0x08d719d8, 2);              // amSysDataDebugLevel
            setVariable(0x08d719e0, 2);              // bcLibDebugLevel
            setVariable(0x08d719d4, 2);              // amOsinfoDebugLevel
            setVariable(0x08d719e4, 0x0FFFFFFF);     // s_logMask
            //detourFunction(0x0808f9a8, _putConsole); // Crashes the game sometimes.
        }
        // Security
        detourFunction(0x086e2336, amDongleInit);
        detourFunction(0x086e0d81, amDongleIsAvailable);
        detourFunction(0x086e17e5, amDongleUpdate);
        // Fixes
        detourFunction(0x086e0c0d, amDipswGetData);
        detourFunction(0x086e0c84, stubRetZero); // amDipswSetLED
        detourFunction(0x0821e6dc, stubRetOne);  // isEthLinkUp
        patchMemory(0x082cb412, "c0270900");     // tickInitStoreNetwork
        patchMemory(0x082cb6d9, "e950010000");   // tickWaitDHCP
        // Set Resolution
        patchMemory(0x0835664d, "e9f000");   // Force resolution set
        //setVariable(0x08356743, 0x00000780); // Set ResX
        //setVariable(0x08356748, 0x00000438); // Set ResY

        // AMDFIX
        //uint32_t a = (uint32_t)glProgramEnvParameters4fvEXT;
        //uint32_t b = ((a & 0xFF) << 24) | (((a >> 8) & 0xFF) << 16) | (((a >> 16) & 0xFF) << 8) | ((a >> 24) & 0xFF);

        //printf("Address Orig = %p - Address in a = %08x\n", (void*)glProgramEnvParameters4fvEXT, b);
        //printf("Address %08x\n", (uint32_t)glProgramEnvParameters4fvEXT);

        //const char *glProgramEnvParameters4fvEXT = "glProgramEnvParameters4fvEXT";
        //memset((void*)0x08813035,0, 28);
        patchMemory(0x08812fcc, "676C50726F6772616D456E76506172616D657465727334667645585400");
        //setVariable(0x08524247, 0x08813035);
        patchMemory(0x08524247, "CC2F8108");    // glProgramParameters4fvNV
        patchMemory(0x08524258, "9090");        // jnz
        patchMemory(0x0852443a, "CC2F8108");    // glProgramParameters4fvNV
        //setVariable(0x0852443a, a);
        patchMemory(0x0852444a, "CC2F8108");    // glProgramParameters4fvNV
        //setVariable(0x0852444a, a);

        //FILE *f = fopen("dump.elf", "w+b");
        //fwrite((void*)0x8048000, 0xc23632, 1, f);
        //fclose(f);

        // FrameBuffer Resolution (No effect that I know)
        /*
        setVariable(0x08248037, 0x00000780);       // Set ResX
        setVariable(0x0824802f, 0x00000438);       // Set ResY
        setVariable(0x082480f7, 0x00000780);       // Set ResX
        setVariable(0x082480ef, 0x00000438);       // Set ResY
        setVariable(0x082481b7, 0x00000780);       // Set ResX
        setVariable(0x082481af, 0x00000438);       // Set ResY
        setVariable(0x08248216, 0x00000780);       // Set ResX
        setVariable(0x0824820e, 0x00000438);       // Set ResY

        setVariable(0x082489a7, 0x00000780);       // Set ResX
        setVariable(0x0824899f, 0x00000438);       // Set ResY
        setVariable(0x08248a32, 0x00000780);       // Set ResX
        setVariable(0x08248a2a, 0x00000438);       // Set ResY
        */

        // IDK if the following work (taken from TP)
        // setVariable(0x08548ef3, 0x8990c031);       // Shader Compiler
        // setVariable(0x08799d8c, 0x082c9f52);       // childTerminationHanlder
    }
    break;
    case INITIALD_4_REVE:
    {
        if (config->showDebugMessages == 1)
        {
            setVariable(0x08d972d0, 2);              // amBackupDebugLevel
            setVariable(0x08d972e0, 2);              // amCreditDebugLevel
            setVariable(0x08d97538, 2);              // amDipswDebugLevel
            setVariable(0x08d9753c, 2);              // amDongleDebugLevel
            setVariable(0x08d97540, 2);              // amEepromDebugLevel
            setVariable(0x08d97544, 2);              // amHwmonitorDebugLevel
            setVariable(0x08d97548, 2);              // amJvsDebugLevel
            setVariable(0x08d9754c, 2);              // amLibDebugLevel
            setVariable(0x08d97550, 2);              // amMiscDebugLevel
            setVariable(0x08d97554, 2);              // amSysDataDebugLevel
            setVariable(0x08d97558, 2);              // bcLibDebugLevel
            setVariable(0x08d97560, 2);              // amOsinfoDebugLevel
            setVariable(0x08d97564, 0x0FFFFFFF);     // s_logMask
            //detourFunction(0x08090478, _putConsole); // Crashes the game sometimes.
        }
        // Security
        detourFunction(0x087106e6, amDongleInit);
        detourFunction(0x0870f131, amDongleIsAvailable);
        detourFunction(0x0870fb95, amDongleUpdate);
        // Fixes
        detourFunction(0x0870efbd, amDipswGetData);
        detourFunction(0x0870f034, stubRetZero); // amDipswSetLed
        detourFunction(0x08230fde, stubRetOne);  // isEthLinkUp
        patchMemory(0x082df87d, "e954010000");   // tickWaitDHCP
        patchMemory(0x082e0ec9, "eb60");         // tickInitAddress
        setVariable(0x08580979, 0x000126e9);         // Avoid Full Screen set from Game
        // Set Resolution
        setVariable(0x0837b12d, 0x0000f0e9);         // Force set resolution
        setVariable(0x0837b223, 0x00000550);         // Set ResX
        setVariable(0x0837b228, 0x00000300);         // Set ResY
        // setVariable(0x085700d3, 0x8990c031);         // Fix something with the Shaders??
        //Tests
        patchMemory(0x081944e7, "9090909090");          // Closedir
        patchMemory(0x082082e8, "88fa79");              // ~cRealCardIF
        patchMemory(0x087beb6c, "5b");                  // seqInitCard::typeinfo
        patchMemory(0x087beb7c, "d4");                  // seqInitCard::typeinfo

        detourFunction(0x087105ad, amDongleUserInfoEx);
    }
    break;
    case INITIALD_5:
    {
        
        if (config->showDebugMessages == 1)
        {
            setVariable(0x093f6fa0, 2);              // amBackupDebugLevel
            setVariable(0x093f6fc0, 2);              // amCreditDebugLevel
            setVariable(0x093f7218, 2);              // amDipswDebugLevel
            setVariable(0x093f721c, 2);              // amDongleDebugLevel
            setVariable(0x093f7220, 2);              // amEepromDebugLevel
            setVariable(0x093f7224, 2);              // amHwmonitorDebugLevel
            setVariable(0x093f7228, 2);              // amJvsDebugLevel
            setVariable(0x093f722c, 2);              // amLibDebugLevel
            setVariable(0x093f7230, 2);              // amMiscDebugLevel
            setVariable(0x093f7238, 2);              // amSysDataDebugLevel
            setVariable(0x093f7240, 2);              // bcLibDebugLevel
            setVariable(0x093f7234, 2);              // amOsinfoDebugLevel
            setVariable(0x093f7244, 0x0FFFFFFF);     // s_logMask
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
        detourFunction(0x0893c4b3, stubRetZero); // amDipswSetLed
        detourFunction(0x0832fca6, stubRetOne);  // isEthLinkUp
        patchMemory(0x08456348, "e954010000");   // tickWaitDHCP
        patchMemory(0x0845843b, "eb60");         // tickInitAddress
        patchMemory(0x08455584, "C0270900");     // tickInitStoreNetwork
        detourFunction(0x08943eb6, stubRetZero); // amOsinfoExecDhcpNic
        detourFunction(0x085135e0, stubRetZero); // isUseServerBox
        // Set Resolution
        patchMemory(0x0855a48d, "E9f000");       // Accept different Resolutions
        setVariable(0x0855a583, 0x00000550);         // Set ResX
        setVariable(0x0855a588, 0x00000300);         // Set ResY
        // Tests
        patchMemory(0x08a0f78c, "95");           // seqInitialize::childTerminationHandler
        patchMemory(0x082923ad, "4851");         // ~cRealCardIF 
        patchMemory(0x081d7f69, "9090909090");   // closedir function skiped????

        // amsInit
        patchMemory(0x08938437, "00");         // Avoids strBBBlackList
        patchMemory(0x089385cb, "e9e7000000"); // Eliminate Dongle Challenges
        patchMemory(0x0893871e, "74"); //      // Returns 1
        patchMemory(0x0893871a, "01000000");   // Returns 1
        // amsCheckKeyDataVerify
        patchMemory(0x08939696, "00000000");   // amsCheckKeyDataVerify
        patchMemory(0x0893962b, "00000000");   // amsCheckKeyDataVerify

        detourFunction(0x0893dbeb, amDongleUserInfoEx);

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
        detourFunction(0x0815d0e3, stubRetZero);
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
        detourFunction(0x082c3103, stubRetZero);
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
        detourFunction(0x083190f4, stubRetZero);
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
        //test
        //patchMemory(0x0804d8be, "909090909090909090909090909090");
        //patchMemory(0x0804e5cf, "909090909090909090909090909090");
        //Res
        //setVariable(0x0805af8b, 0x00000438);
        //setVariable(0x0805af93, 0x000003c0);
        //setVariable(0x0805afa2, 0x000003c0);
        //setVariable(0x0805b0ed, 0x00000438);
        //setVariable(0x0805aff5, 0x000003c0);
        
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
        //patchMemory(0x807c9ec, "01");
        //detourFunction(0x080f3f94, stubRetZero); //eliminates init_heap function.
    }
    break;
    case HUMMER_EXTREME:
    {
    /*    if (config->showDebugMessages == 1)
        {
            setVariable(0x093f6fa0, 2);              // amBackupDebugLevel
            setVariable(0x093f6fc0, 2);              // amCreditDebugLevel
            setVariable(0x093f7218, 2);              // amDipswDebugLevel
            setVariable(0x093f721c, 2);              // amDongleDebugLevel
            setVariable(0x093f7220, 2);              // amEepromDebugLevel
            setVariable(0x093f7224, 2);              // amHwmonitorDebugLevel
            setVariable(0x093f7228, 2);              // amJvsDebugLevel
            setVariable(0x093f722c, 2);              // amLibDebugLevel
            setVariable(0x093f7230, 2);              // amMiscDebugLevel
            setVariable(0x093f7238, 2);              // amSysDataDebugLevel
            setVariable(0x093f7240, 2);              // bcLibDebugLevel
            setVariable(0x093f7234, 2);              // amOsinfoDebugLevel
            setVariable(0x093f7244, 0x0FFFFFFF);     // s_logMask
        //    detourFunction(0x0808f9a8, _putConsole); // Crashes the game sometimes.
            patchMemory(0x08942d91, "E912ffffff");
            patchMemory(0x0894375b, "EB29");
        }*/
        // Security
        detourFunction(0x0831c0d1, amDongleInit);
        detourFunction(0x0831a95d, amDongleIsAvailable);
        detourFunction(0x0831b47e, amDongleUpdate);
        // Fixes
        detourFunction(0x0831a7e9, amDipswGetData);
        detourFunction(0x0831a85f, stubRetZero); // amDipswSetLed
        
        // from TP
        //patchMemory(0x080a0ef8, "C3000000");                       // smpGlxSetCursos
        //patchMemory(0x080a872b, "909090909090909090909090909090"); // glShaderSource
        patchMemory(0x080cf7b8, "C3000000");                       // iserror
        patchMemory(0x080e8b40, "C3000000");                       // clSteerErrorDisp::run
        patchMemory(0x08171396, "C3000000");                       // clErrorDisp::update_
        detourFunction(0x08322dec, stubRetZero);                   // amOsinfoModifyNetworkAdrNic
        detourFunction(0x083238a8, stubRetZero);                   // amOsinfoGetNetworkPropertyNicEx
        patchMemory(0x08361aff, "C3000000");                       // ADXM_SetupFramework
        patchMemory(0x0836fa92, "C3000000");                       // LXSYNC_Init

        detourFunction(0x0831bf97, amDongleUserInfoEx);

        //Force Test for debug
        patchMemory(0x0807d4cf, "01");
    }
    break;
    default:
        // Don't do any patches for random games
        break;
    }
    return 0;
}
