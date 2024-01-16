#include <stdint.h>

#define MAX_PATH_LENGTH 1024

typedef enum
{
  AFTER_BURNER_CLIMAX,
  AFTER_BURNER_CLIMAX_REVA,
  AFTER_BURNER_CLIMAX_REVB,
  AFTER_BURNER_CLIMAX_SDX,
  AFTER_BURNER_CLIMAX_SDX_REVA,
  AFTER_BURNER_CLIMAX_CE,
  GHOST_SQUAD_EVOLUTION,
  HARLEY_DAVIDSON,
  HUMMER,
  HUMMER_EXTREME,
  HUMMER_EXTREME_MDX,
  INITIALD_4,
  INITIALD_4_REVE,
  INITIALD_5,
  LETS_GO_JUNGLE,
  LETS_GO_JUNGLE_REVA,
  LETS_GO_JUNGLE_SPECIAL,
  OUTRUN_2_SP_SDX,
  OUTRUN_2_SP_SDX_REVA,
  OUTRUN_2_SP_SDX_REVA_TEST,
  OUTRUN_2_SP_SDX_TEST,
  PRIMEVAL_HUNT,
  RAMBO,
  RAMBO_CHINA,
  R_TUNED,
  SEGABOOT,
  SEGABOOT_2_4,
  SEGABOOT_2_4_SYM,
  SEGABOOT_2_6,
  SEGA_RACE_TV,
  THE_HOUSE_OF_THE_DEAD_4,
  THE_HOUSE_OF_THE_DEAD_4_STRIPPED,
  THE_HOUSE_OF_THE_DEAD_4_STRIPPED_TEST,
  THE_HOUSE_OF_THE_DEAD_4_SPECIAL,
  THE_HOUSE_OF_THE_DEAD_4_SPECIAL_TEST,
  THE_HOUSE_OF_THE_DEAD_4_TEST,
  THE_HOUSE_OF_THE_DEAD_EX,
  THE_HOUSE_OF_THE_DEAD_EX_TEST,
  TOO_SPICY,
  UNKNOWN,
  VIRTUA_FIGHTER_5,
  VIRTUA_FIGHTER_5_FINAL_SHOWDOWN,
  VIRTUA_FIGHTER_5_FINAL_SHOWDOWN_REVA,
  VIRTUA_FIGHTER_5_R,
  VIRTUA_FIGHTER_5_REVA,
  VIRTUA_FIGHTER_5_REVB,
  VIRTUA_FIGHTER_5_REVC,
  VIRTUA_FIGHTER_5_REVE,
  VIRTUA_FIGHTER_5_EXPORT,
  VIRTUA_FIGHTER_5_R_REVD,
  VIRTUA_TENNIS_3,
  VIRTUA_TENNIS_3_TEST
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
} GameStatus;

typedef enum
{
  JP,
  US,
  EX
} GameRegion;

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
  GameRegion region;
  int freeplay;
  int showDebugMessages;
} EmulatorConfig;

int initConfig();
EmulatorConfig *getConfig();
char *getGameName();
