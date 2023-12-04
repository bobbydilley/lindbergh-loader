#include <stdint.h>

#define MAX_PATH_LENGTH 1024

typedef enum
{
  UNKNOWN,
  SEGABOOT,
  SEGABOOT_2_4,
  SEGABOOT_2_6,
  THE_HOUSE_OF_THE_DEAD_4,
  THE_HOUSE_OF_THE_DEAD_4_TEST,
  OUTRUN,
  OUTRUN_TEST,
  LETS_GO_JUNGLE,
  LETS_GO_JUNGLE_SPECIAL,
  ABC_2006,
  ABC_2007,
  ID4,
  SRTV,
  RTUNED,
  VT3,
  VF5_REVC
} Game;

typedef enum
{
  YELLOW,
  RED
} Colour;

typedef enum
{
  WORKING,
  NOT_WORKING
}GameStatus;

typedef enum
{
  JP,
  US,
  EX
}gameRegion;

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
  Colour lindberghColour;
  GameStatus gameStatus;
  uint32_t crc32;
  gameRegion region;
  int freeplay;
} EmulatorConfig;

int initConfig();
EmulatorConfig *getConfig();
char *getGameName();
