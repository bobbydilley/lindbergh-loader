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
    // printf("Variable: %8X , Value: %8X\n",(uint32_t)variable, value);
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
    // printf("Variable: %8X , Value: %8X\n",(uint32_t)variable, value);
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

int amLibInit()
{
    uint32_t *amLibContext = (uint32_t *)0x08dfa2c0; // 0x0809cb00;
    *amLibContext = 1;
    uint32_t *amLibInitializad = (uint32_t *)0x08dfa2c4;       // 0x0809cb04;
    uint16_t *amLibPort1 = (uint16_t *)(0x08dfa2c4 + 4);       //(0x0809cb04 + 4);
    uint16_t *amLibPort2 = (uint16_t *)(0x08dfa2c4 + 4);       //(0x0809cb04 + 6);
    uint32_t *bcLibInitialized = (uint32_t *)(0x08dfa2c4 + 8); // 0x0809cb0c;
    *amLibInitializad = 1;
    *amLibPort1 = 0xd000;
    *amLibPort2 = 0x0004;
    *bcLibInitialized = 0;
    int res = ((int (*)(void))0x084dedc4)(); // 0x08065d80)();
    if (res == 1)
        *bcLibInitialized = 1;
    return 0;
}

int amDipswInit()
{
    uint32_t *amDipswContext = (uint32_t *)0x08df9cec;         // 0x0809c12c;
    uint32_t *amDipswContext1 = (uint32_t *)(0x08df9cec + 4);  //(0x0809c12c + 4);
    uint32_t *amDipswContext2 = (uint32_t *)(0x08df9cec + 8);  //(0x0809c12c + 8);
    uint32_t *amDipswContext3 = (uint32_t *)(0x08df9cec + 12); //(0x0809c12c + 12);
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
    // printf("amDipswGetData Called!!!!!\n");
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
    Game game = getConfig()->game;

    switch (game)
    {
    case RTUNED:
    {
        // Security
        detourFunction(0x08366846, amDongleInit);
        detourFunction(0x08365301, amDongleIsAvailable);
        detourFunction(0x08365cf7, amDongleUpdate);
    }
    break;

    case SRTV:
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
    case ABC_2006:
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
        // Security
        detourFunction(0x081e4980, amDongleInit);
        detourFunction(0x081e4cce, amDongleIsAvailable);
        detourFunction(0x081e4bfa, amDongleUpdate);
        // Fixes
        detourFunction(0x081e48b6, amDipswGetData);
        detourFunction(0x081e492e, stubRetZero); // Stub amDipswSetLed
        // Does not work
        setVariable(0x08061c31, 0x0000000c); // Force HD resolution
    }
    break;
    case ABC_2007:
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
        // Security
        detourFunction(0x081e3424, amDongleInit);
        detourFunction(0x081e3772, amDongleIsAvailable);
        detourFunction(0x081e369e, amDongleUpdate);
        // Fixes
        detourFunction(0x081e335a, amDipswGetData);
        detourFunction(0x081e33d2, stubRetZero); // Stub amDipswSetLed
    }
    break;
    case OUTRUN:
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
        // Security
        detourFunction(0x08190e80, amDongleInit);
        detourFunction(0x08191201, amDongleIsAvailable);
        detourFunction(0x08191125, amDongleUpdate);
        // Fixes
        detourFunction(0x08190db6, amDipswGetData);
        detourFunction(0x08190e2e, stubRetZero); // Stub amDipswSetLed
    }
    break;

    case THE_HOUSE_OF_THE_DEAD_4:
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
        // Security
        detourFunction(0x08320178, amDongleInit);
        detourFunction(0x08320459, amDongleIsAvailable);
        detourFunction(0x083203c0, amDongleUpdate);
        // Fixes
        detourFunction(0x0831ddd7, amDipswGetData);
        detourFunction(0x0831de4f, stubRetZero); // Stub amDipswSetLed
        // CPU patch to support AMD processors
        setVariable(0x0837d6aa, cpu_vendor.ebx);
        setVariable(0x0837d6ba, cpu_vendor.edx);
        setVariable(0x0837d6c5, cpu_vendor.ecx);
    }
    break;
    case THE_HOUSE_OF_THE_DEAD_4_STRIPPED:
    {
        //// Security
        detourFunction(0x0831ad04, amDongleInit);
        detourFunction(0x0831b017, amDongleIsAvailable);
        detourFunction(0x0831af7e, amDongleUpdate);
        //// Fixes
        detourFunction(0x0831875f, amDipswGetData);
        detourFunction(0x083187d7, stubRetZero); // Stub amDipswSetLed
        //// CPU patch to support AMD processors
        setVariable(0x0837963a, cpu_vendor.ebx);
        setVariable(0x0837964a, cpu_vendor.edx);
        setVariable(0x08379655, cpu_vendor.ecx);
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
        setVariable(0x0807217a, cpu_vendor.ebx);
        setVariable(0x0807218a, cpu_vendor.edx);
        setVariable(0x08072195, cpu_vendor.ecx);
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
        setVariable(0x0849E2AD, cpu_vendor.ebx);
        setVariable(0x0849E2B7, cpu_vendor.edx);
        setVariable(0x0849E2C1, cpu_vendor.ecx);
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
        setVariable(0x080847BD, cpu_vendor.ebx);
        setVariable(0x080847C7, cpu_vendor.edx);
        setVariable(0x080847D1, cpu_vendor.ecx);
    }
    break;
    case VF5_REVC:
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
        setVariable(0x080e17af, 0x000000b8);     // Patch IDK what
        setVariable(0x080e17b3, 0x01e88300);     // Patch IDK what
    }
    break;
    case LETS_GO_JUNGLE:
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
        // Security
        detourFunction(0x084e50d8, amDongleInit);
        detourFunction(0x084e5459, amDongleIsAvailable);
        detourFunction(0x084e537d, amDongleUpdate);
        setVariable(0x080d1f02, 0x90909090); // Patch acpSystem::checkDongle
        setVariable(0x080d1f06, 0xE8C3C990); // Patch acpSystem::checkDongle
        setVariable(0x0807b76a, 0xc2839090); // Patch initializeArcadeBackup
        // Fixes
        detourFunction(0x084e500e, amDipswGetData);
        detourFunction(0x084e5086, stubRetZero); // Stub amDipswSetLed
        setVariable(0x0840d858, 0x1c899090);     // No more Full Screen from the Game
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
        // Security
        detourFunction(0x08510320, amDongleInit);
        detourFunction(0x085106dc, amDongleIsAvailable);
        detourFunction(0x08510600, amDongleUpdate);
        setVariable(0x080dad63, 0x90909090); // Patch acpSystem::checkDongle
        setVariable(0x080dad67, 0xE8C3C990); // Patch acpSystem::checkDongle
        setVariable(0x0807e609, 0xc2839090); // Patch initializeArcadeBackup
        // Fixes
        detourFunction(0x08510256, amDipswGetData);
        detourFunction(0x085102ce, stubRetZero); // Stub amDipswSetLed
        setVariable(0x08438954, 0x1c899090);     // No more Full Screen from the Game
        // Set Resolution
        // setVariable(0x08303C4B, 0x00000780); // Set ResX
        // setVariable(0x08303C58, 0x00000438); // Set ResY

        // setVariable(0x087d47f7, 0x62ab8500); // Seat Test??
    }
    break;
    case ID4:
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
        // Security
        detourFunction(0x086e2336, amDongleInit);
        detourFunction(0x086e0d81, amDongleIsAvailable);
        detourFunction(0x086e17e5, amDongleUpdate);
        // Fixes
        detourFunction(0x086e0c0d, amDipswGetData);
        detourFunction(0x086e0c84, stubRetZero); // amDipswSetLED
        detourFunction(0x0821e6dc, stubRetOne);  // isEthLinkUp
        setVariable(0x082cb411, 0x0927c020);     // tickInitStoreNetwork
        setVariable(0x082cb6d9, 0x000150e9);     // tickWaitDHCP
        setVariable(0x082cb6dd, 0x448b5100);     // tickWaitDHCP
        // Set Resolution
        setVariable(0x0835664d, 0x0000f0e9); // Force resolution set
        setVariable(0x08356743, 0x00000780); // Set ResX
        setVariable(0x08356748, 0x00000438); // Set ResY
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

        
        // Hooked in graphics.c
        //setVariable(0x085599f2, 0x0001d2e9); // Force not supported resolutions
        //setVariable(0x085599f6, 0x01bb0000); // Force not supported resolutions

        // IDK if the following work (taken from TP)
        // setVariable(0x08548ef3, 0x8990c031);       // Shader Compiler
        // setVariable(0x08799d8c, 0x082c9f52);       // childTerminationHanlder
    }
    break;
    case ID4_E:
    {
        // Debug
        // detourFunction(0x08090478, _putConsole);  // Crashes the game sometimes.
        // Security
        detourFunction(0x087106e6, amDongleInit);
        detourFunction(0x0870f131, amDongleIsAvailable);
        detourFunction(0x0870fb95, amDongleUpdate);
        // Fixes
        detourFunction(0x0870efbd, amDipswGetData);
        detourFunction(0x0870f034, stubRetZero); // amDipswSetLed
        setVariable(0x087a05e8, 0x08194748);     // PTR_~cRealCardIF SIGSEV
        detourFunction(0x08230fde, stubRetOne);  // isEthLinkUp
        setVariable(0x082df87d, 0x000154e9);     // tickWaitDHCP
        setVariable(0x082df881, 0x448b5100);     // tickWaitDHCP
        setVariable(0x082e0ec9, 0x3d8960eb);     // tickInitAddress
        // setVariable(0x08580979, 0x000126e9);         // Avoid Full Screen set from Game
        // Set Resolution
        // setVariable(0x0837b12d, 0x0000f0e9);         // Force set resolution
        // setVariable(0x0837b223, 0x00000550);         // Set ResX
        // setVariable(0x0837b228, 0x00000300);         // Set ResY
        // setVariable(0x085700d3, 0x8990c031);         // Fix something with the Shaders??
    }
    break;
    case SEGABOOT_2_4_SYM:
    {
        detourFunction(0x0805e8b0, amDongleInit);
        detourFunction(0x0805ebc3, amDongleIsAvailable);
        detourFunction(0x0805eb2a, amDongleUpdate);
        detourFunction(0x0805c30b, amDipswGetData);
    }
    break;
    case VT3:
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
    case VT3_TESTMODE:
    {
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
        setVariable(0x08399ADA, cpu_vendor.ebx);
        setVariable(0x08399AEA, cpu_vendor.edx);
        setVariable(0x08399AF5, cpu_vendor.ecx);
    }
    break;
    default:
        // Don't do any patches for random games
        break;
    }
    return 0;
}