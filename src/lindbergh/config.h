#include <stdint.h>

#define MAX_PATH_LENGTH 1024

typedef struct
{
  int emulateRideboard;
  char eepromPath[MAX_PATH_LENGTH];
  char sramPath[MAX_PATH_LENGTH];
} EmulatorConfig;

int initConfig();
EmulatorConfig *getConfig();



