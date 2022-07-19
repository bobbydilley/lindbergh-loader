#include <stdint.h>

#include "jvs.h"

int initSecurityBoard();
int securityBoardOut(uint16_t port, uint32_t *data);
int securityBoardIn(uint16_t port, uint32_t *data);
int securityBoardSetSwitch(JVSInput switchNumber, int value);
int securityBoardSetRotation(int rotation);
int securityBoardSetDipSwitch(int switchNumber, int value);
int securityBoardSetDipResolution(int width, int height);
