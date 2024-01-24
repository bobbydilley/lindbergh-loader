#include <stdio.h>
#include <string.h>

#include "config.h"
#include "securityboard.h"

#define SECURITY_BOARD_FRONT_PANEL 0x38
#define SECURITY_BOARD_FRONT_PANEL_NON_ROOT 0x1038
#define SECURITY_BOARD_KEYCHIP 0xFF

#define DIP_SWITCH_ROTATION 3

typedef struct {
  int serviceSwitch;
  int testSwitch;
  int dipSwitch[8 + 1]; // Start index at 1
  int led[2];
} SecurityBoard;

SecurityBoard securityBoard = {0};

int initSecurityBoard() {
  securityBoard.dipSwitch[DIP_SWITCH_ROTATION] = 0;

  return 0;
}

static void setResolutionDips(int dip4, int dip5, int dip6) {
  securityBoard.dipSwitch[4] = dip4;
  securityBoard.dipSwitch[5] = dip5;
  securityBoard.dipSwitch[6] = dip6;
}

int securityBoardSetDipResolution(int width, int height) {
  if (width == 640 && height == 480)
    setResolutionDips(0, 0, 0);
  else if (width == 800 && height == 600)
    setResolutionDips(0, 0, 1);
  else if (width == 1024 && height == 768)
    setResolutionDips(0, 1, 0);
  else if (width == 1280 && height == 1024)
    setResolutionDips(0, 1, 1);
  else if (width == 800 && height == 480)
    setResolutionDips(1, 0, 0);
  else if (width == 1024 && height == 600)
    setResolutionDips(1, 0, 1);
  else if (width == 1280 && height == 768)
    setResolutionDips(1, 1, 0);
  else if (width == 1360 && height == 768)
    setResolutionDips(1, 1, 1);
  else
    printf("Warning: Resolution not compatible, using 640 x 480\n");

  return 0;
}

int securityBoardSetRotation(int rotation) {
  securityBoard.dipSwitch[DIP_SWITCH_ROTATION] = rotation;
  return 0;
}

int securityBoardSetDipSwitch(int switchNumber, int value) {
  if (switchNumber == 0) {
    printf("Error: Dip Switch index starts at 1\n");
    return 1;
  }
  securityBoard.dipSwitch[switchNumber] = value;
  return 0;
}

int securityBoardSetSwitch(JVSInput switchNumber, int value) {
  switch (switchNumber) {
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

int securityBoardOut(uint16_t port, uint32_t *data) { return 0; }

int securityBoardIn(uint16_t port, uint32_t *data) {
  switch (port) {
  case SECURITY_BOARD_FRONT_PANEL_NON_ROOT:
  case SECURITY_BOARD_FRONT_PANEL: {
    uint32_t result = 0xFFFFFFFF;
    if (securityBoard.dipSwitch[6]) // bit12
      result &= ~0x800;             // DIP 6
    if (securityBoard.dipSwitch[5]) // bit11
      result &= ~0x400;             // DIP 5
    if (securityBoard.dipSwitch[4]) // bit10
      result &= ~0x200;             //  DIP 4
    if (securityBoard.dipSwitch[3]) // bit9
      result &= ~0x100;             //  DIP 3
    if (securityBoard.dipSwitch[2]) // bit8
      result &= ~0x80;              // DIP 2
    if (securityBoard.dipSwitch[1]) // bit7
      result &= ~0x40;              // DIP 1
    if (securityBoard.dipSwitch[8]) // bit6
      result &= ~0x20;
    if (securityBoard.dipSwitch[7]) // bit5
      result &= ~0x10;
    if (securityBoard.serviceSwitch) // bit4
      result &= ~0x08;
    if (securityBoard.testSwitch) // bit3
      result &= ~0x04;
    *data = result;
  } break;

  default:
    break;
  }

  return 0;
}
