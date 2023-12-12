#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <unistd.h>

#include "patch.h"
#include "config.h"
#include "hook.h"

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

static void setMem(uint32_t address, uint32_t value, int size)
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
    memccpy((void *)variable, (void *)value, 1, size);
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

int stub0()
{
    return 0;
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
        detourFunction(0x08366846, amDongleInit);
        detourFunction(0x08365301, amDongleIsAvailable);
        detourFunction(0x08365cf7, amDongleUpdate);
    }
    break;

    case SEGA_RACE_TV:
    {
        detourFunction(0x084d5b40, amDongleInit);
        detourFunction(0x084d45f9, amDongleIsAvailable);
        detourFunction(0x084d4fef, amDongleUpdate);
    }
    break;
    case AFTER_BURNER_CLIMAX_REVA:
    {
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

        detourFunction(0x081e4980, amDongleInit);
        detourFunction(0x081e4cce, amDongleIsAvailable);
        detourFunction(0x081e4bfa, amDongleUpdate);
    }
    break;
    case AFTER_BURNER_CLIMAX_REVB:
    {
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

        detourFunction(0x081e3424, amDongleInit);
        detourFunction(0x081e3772, amDongleIsAvailable);
        detourFunction(0x081e369e, amDongleUpdate);
        setVariable(0x081e7945, 0x00000001); // Test
    }
    break;
    case OUTRUN_2_SP_SDX_REVA:
    {
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

        detourFunction(0x08190e80, amDongleInit);
        detourFunction(0x08191201, amDongleIsAvailable);
        detourFunction(0x08191125, amDongleUpdate);
    }
    break;

    case THE_HOUSE_OF_THE_DEAD_4:
    {
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

        detourFunction(0x08320178, amDongleInit);
        detourFunction(0x08320459, amDongleIsAvailable);
        detourFunction(0x083203c0, amDongleUpdate);

        if (config->amdFix)
        {
            setVariable(0x0837d6aa, cpu_vendor.ebx);
            setVariable(0x0837d6ba, cpu_vendor.edx);
            setVariable(0x0837d6c5, cpu_vendor.ecx);
        }
    }
    break;
    case THE_HOUSE_OF_THE_DEAD_4_TEST:
    {
        detourFunction(0x080677a0, amDongleInit);
        detourFunction(0x08067a81, amDongleIsAvailable);
        detourFunction(0x080679e8, amDongleUpdate);

        if (config->amdFix)
        {
            setVariable(0x0807217a, cpu_vendor.ebx);
            setVariable(0x0807218a, cpu_vendor.edx);
            setVariable(0x08072195, cpu_vendor.ecx);
        }
    }
    break;
    case VIRTUA_FIGHTER_5_REVC:
    {
        detourFunction(0x085c6010, amDongleInit);
        detourFunction(0x085c63cc, amDongleIsAvailable);
        detourFunction(0x085c62f0, amDongleUpdate);
        detourFunction(0x080b3426, stub0);   // Stub returns 0
        detourFunction(0x080cb6d4, stub0);   // Stub returns 0
        detourFunction(0x0840889e, stub0);   // Stub returns 0
        detourFunction(0x0840ab90, stub0);   // Stub returns 0
        setVariable(0x080e17af, 0x000000b8); // Patch IDK what
        setVariable(0x080e17b3, 0x01e88300); // Patch IDK what
    }
    break;

    case LETS_GO_JUNGLE:
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
        detourFunction(0x084e50d8, amDongleInit);
        detourFunction(0x084e5459, amDongleIsAvailable);
        detourFunction(0x084e537d, amDongleUpdate);
        detourFunction(0x08074a8c, _putConsole);
        setVariable(0x080d1f02, 0x90909090); // Patch acpSystem::checkDongle
        setVariable(0x080d1f06, 0xE8C3C990); // Patch acpSystem::checkDongle
        setVariable(0x0807b76a, 0xc2839090); // Patch initializeArcadeBackup
        //        setVariable(0x082E006b, 0x00000280); // Set ResX
        //        setVariable(0x082E0078, 0x000001E0); // Set ResY

        detourFunction(0x084e4efc, stub0); // Stub amDipswInit
        detourFunction(0x084e500e, stub0); // Stub amDipswGetData
        detourFunction(0x084e5086, stub0); // Stub amDipswSetLed
        detourFunction(0x084e4f98, stub0); // Stub amDipswExit

        setVariable(0x0840d858, 0x1c899090); // No more Full Screen from the Game

        if (config->amdFix)
        {
            setVariable(0x083ef701, 0x00036ee9); // AMDFIX
            setVariable(0x084032e0, 0x8b90c933); // fix shader compilation with AMD GPUs
            setVariable(0x08523950, 0x000000c3); // Remove ADXM_SetupFramework (Not necessary)
        }
    }
    break;

    case LETS_GO_JUNGLE_SPECIAL:
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
        detourFunction(0x08510320, amDongleInit);
        detourFunction(0x085106dc, amDongleIsAvailable);
        detourFunction(0x08510600, amDongleUpdate);
        detourFunction(0x08075012, _putConsole);
        // setVariable(0x08303C4B, 0x00000780); // Set ResX
        // setVariable(0x08303C58, 0x00000438); // Set ResY
        setVariable(0x080dad63, 0x90909090); // Patch acpSystem::checkDongle
        setVariable(0x080dad67, 0xE8C3C990); // Patch acpSystem::checkDongle
        setVariable(0x0807e609, 0x90909090); // Patch initializeArcadeBackup
        setVariable(0x0807e60D, 0xC2839090); // Patch initializeArcadeBackup
        setVariable(0x087d47f7, 0x62ab8500); // Seat Test??
        setVariable(0x08438954, 0x1c899090); // No more Full Screen from the Game
    }
    break;

    case INITIALD_4:
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
        
        detourFunction(0x086e2336, amDongleInit);
        detourFunction(0x086e0d81, amDongleIsAvailable);
        detourFunction(0x086e17e5, amDongleUpdate);
        detourFunction(0x0808f9a8, _putConsole);

        setVariable(0x080dad63, 0x90909090); // Patch acpSystem::checkDongle
        setVariable(0x080dad67, 0xE8C3C990); // Patch acpSystem::checkDongle
        setVariable(0x0807e609, 0x90909090); // Patch initializeArcadeBackup
        setVariable(0x0807e60D, 0xC2839090); // Patch initializeArcadeBackup
    }
    default:
        // Don't do any patches for random games
        break;
    }
    return 0;
}
