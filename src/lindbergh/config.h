#include <stdint.h>

#define MAX_PATH_LENGTH 1024

typedef enum {
  UNKNOWN,
  SEGABOOT,
  THE_HOUSE_OF_THE_DEAD_4,
  OUTRUN
} Game;

typedef struct
{
  int emulateRideboard;
  int emulateDriveboard;
  int emulateMotionboard;
  int emulateJVS;
  int fullscreen;
  char eepromPath[MAX_PATH_LENGTH];
  char sramPath[MAX_PATH_LENGTH];
  char jvsPath[MAX_PATH_LENGTH];
  int width;
  int height;
  Game game;
} EmulatorConfig;

int initConfig();
EmulatorConfig *getConfig();
char *getGameName();



