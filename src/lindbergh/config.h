#include <stdint.h>

#define MAX_PATH_LENGTH 1024

typedef enum {
  UNKNOWN,
  SEGABOOT,
  SEGABOOT_2_4,
  SEGABOOT_2_6,
  THE_HOUSE_OF_THE_DEAD_4,
  OUTRUN,
  LETS_GO_JUNGLE
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
  char rideboardPath[MAX_PATH_LENGTH];
  char motionboardPath[MAX_PATH_LENGTH];
  char driveboardPath[MAX_PATH_LENGTH];
  int width;
  int height;
  Game game;
} EmulatorConfig;

int initConfig();
EmulatorConfig *getConfig();
char *getGameName();



