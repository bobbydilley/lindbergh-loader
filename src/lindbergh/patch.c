#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#include "patch.h"
#include "config.h"

static void setVariable(uint32_t address, uint32_t value)
{
    int pagesize = sysconf(_SC_PAGE_SIZE);

    uint32_t *variable = (uint32_t *)address;

    void *toModify = (void *)(address - (address % pagesize));

    int prot = mprotect(toModify, pagesize, PROT_WRITE);
    if (prot != 0)
    {
        printf("Error: Cannot unprotect memory region to change variable (%d)\n", prot);
        return;
    }

    *variable = value;
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
    cave[1] = (jumpAddress)&0xFF;

    memcpy((void *)address, cave, 5);
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

int initPatch()
{
    Game game = getConfig()->game;

    switch (game)
    {
    case RTUNED:
    {
        detourFunction(0x08366846, amDongleInit);
        detourFunction(0x08365301, amDongleIsAvailable);
        detourFunction(0x08365cf7, amDongleUpdate);
    }
    break;
    
    case SRTV:
    {
        detourFunction(0x084d5b40, amDongleInit);
        detourFunction(0x084d45f9, amDongleIsAvailable);
        detourFunction(0x084d4fef, amDongleUpdate);
    }
    break;
       
    case ABC:
    {
        detourFunction(0x081e3424, amDongleInit);
        detourFunction(0x081e3772, amDongleIsAvailable);
        detourFunction(0x081e369e, amDongleUpdate);
    }
    break;
    case OUTRUN:
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
    }
    break;

    default:
        // Don't do any patches for random games
        break;
    }

    return 0;
}
