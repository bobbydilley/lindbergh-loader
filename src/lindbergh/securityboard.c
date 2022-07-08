#include <stdio.h>
#include <string.h>

#include "securityboard.h"

#define SECURITY_BOARD_FRONT_PANEL 0x38
#define SECURITY_BOARD_KEYCHIP 0xFF

#define DIP_SWITCH_ROTATION 2

typedef struct
{
    int serviceSwitch;
    int testSwitch;
    int dipSwitch[8];
    int led[2];
} SecurityBoard;

SecurityBoard securityBoard = {0};

int initSecurityBoard()
{
    securityBoard.dipSwitch[DIP_SWITCH_ROTATION] = 0;
    return 0;
}

int securityBoardSetRotation(int rotation)
{
    securityBoard.dipSwitch[DIP_SWITCH_ROTATION] = rotation;
    return 0;
}

int securityBoardSetSwitch(JVSInput switchNumber, int value)
{
    switch (switchNumber)
    {
    case BUTTON_TEST:
        securityBoard.testSwitch = value;
        break;
    case BUTTON_SERVICE:
        securityBoard.serviceSwitch = value;
        break;
    default:
        printf("Error: Attempted to set a security board switch incorrectly");
        return -1;
    }

    return 0;
}

int securityBoardOut(uint16_t port, uint32_t *data)
{
    return 0;
}

int securityBoardIn(uint16_t port, uint32_t *data)
{
    switch (port)
    {
    case SECURITY_BOARD_FRONT_PANEL:
        uint32_t result = 0xFFFFFFFF;

        if (securityBoard.serviceSwitch)
            result &= ~0x08;
        if (securityBoard.testSwitch)
            result &= ~0x04;
        if (securityBoard.dipSwitch[7])
            result &= ~0x800;
        if (securityBoard.dipSwitch[0])
            result &= ~0x400;
        if (securityBoard.dipSwitch[1])
            result &= ~0x200;
        if (securityBoard.dipSwitch[2])
            result &= ~0x100;
        if (securityBoard.dipSwitch[3])
            result &= ~0x80;
        if (securityBoard.dipSwitch[4])
            result &= ~0x40;
        if (securityBoard.dipSwitch[5])
            result &= ~0x20;
        if (securityBoard.dipSwitch[6])
            result &= ~0x10;

        *data = result;
        break;
    default:
        break;
    }

    return 0;
}
