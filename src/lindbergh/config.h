#include <stdint.h>

#define MAX_PATH_LENGTH 1024

#define AFTER_BURNER_CLIMAX 0xcc02de7d
#define AFTER_BURNER_CLIMAX_REVA 0x152530dd
#define AFTER_BURNER_CLIMAX_REVB 0x0002
#define AFTER_BURNER_CLIMAX_SDX 0x5c18953c
#define AFTER_BURNER_CLIMAX_SDX_REVA 0x0004
#define AFTER_BURNER_CLIMAX_CE 0x9af7cb29
#define GHOST_SQUAD_EVOLUTION 0xe60d8e04
#define HARLEY_DAVIDSON 0x0007
#define HUMMER 0x0008
#define HUMMER_EXTREME 0xc85f106a
#define HUMMER_EXTREME_MDX 0x0010
#define INITIALD_4 0x4e9ccf33
#define INITIALD_4_REVE 0x7f3f9f0c
#define INITIALD_5_EXP_20 0x14d1292a
#define INITIALD_ARCADE_STAGE_5 0x722ebbc4
#define LETS_GO_JUNGLE 0xd4726d61
#define LETS_GO_JUNGLE_REVA 0xB6479554
#define LETS_GO_JUNGLE_SPECIAL 0xbbabc0e0
#define OUTRUN_2_SP_SDX 0x0019
#define OUTRUN_2_SP_SDX_REVA 0x6d055308
#define OUTRUN_2_SP_SDX_REVA_TEST 0xffdccaaa
#define OUTRUN_2_SP_SDX_TEST 0x0022
#define PRIMEVAL_HUNT 0xcdbc8069
#define RAMBO 0x77ebac34
#define RAMBO_CHINA 0x0025
#define R_TUNED 0xb05d9bbe
#define SEGABOOT 0x0027
#define SEGABOOT_2_4 0x93ea7e11
#define SEGABOOT_2_4_SYM 0x3cc635ee
#define SEGABOOT_2_6 0x0030
#define SEGA_RACE_TV 0xfb096f81
#define THE_HOUSE_OF_THE_DEAD_4_REVA 0xbc0c9ffa
#define THE_HOUSE_OF_THE_DEAD_4_REVA_TEST 0x7235bda8
#define THE_HOUSE_OF_THE_DEAD_4_REVC 0x5df569f5
#define THE_HOUSE_OF_THE_DEAD_4_REVC_TEST 0xDDECE1E9
#define THE_HOUSE_OF_THE_DEAD_4_SPECIAL 0x12266f81
#define THE_HOUSE_OF_THE_DEAD_4_SPECIAL_TEST 0x83ba3b45
#define THE_HOUSE_OF_THE_DEAD_EX 0x85c0c22a
#define THE_HOUSE_OF_THE_DEAD_EX_TEST 0xb9a166bb
#define TOO_SPICY 0x4c768eb4
#define UNKNOWN 0xFFFFFFFF
#define VIRTUA_FIGHTER_5 0x0042
#define VIRTUA_FIGHTER_5_FINAL_SHOWDOWN 0x2B475B88
#define VIRTUA_FIGHTER_5_FINAL_SHOWDOWN_REVA 0x0044
#define VIRTUA_FIGHTER_5_R 0x12D9D038
#define VIRTUA_FIGHTER_5_REVA 0x0046
#define VIRTUA_FIGHTER_5_REVB 0x0047
#define VIRTUA_FIGHTER_5_REVC 0x1bf1b627
#define VIRTUA_FIGHTER_5_REVE 0xC4B05D40
#define VIRTUA_FIGHTER_5_EXPORT 0x157B0576
#define VIRTUA_FIGHTER_5_R_REVD 0x0051
#define VIRTUA_TENNIS_3 0xc4b7e89
#define VIRTUA_TENNIS_3_TEST 0xffe3b0fd


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

typedef enum
{
  SHOOTING,
  DRIVING,
  FIGHTING
} GameType;

typedef struct
{
  unsigned int service;
  unsigned int start;
  unsigned int coin;
  unsigned int up;
  unsigned int down;
  unsigned int left;
  unsigned int right;
  unsigned int button1;
  unsigned int button2;
  unsigned int button3;
  unsigned int button4;
} PlayerKeyMapping;

// All keycode can be found using `xev` binary's debug output
// NOTE: Maybe using tagged union for driving and shooting games
typedef struct
{
  unsigned int test;
  PlayerKeyMapping player1;
  PlayerKeyMapping player2;
} KeyMapping;

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
  char serial1Path[MAX_PATH_LENGTH];
  char serial2Path[MAX_PATH_LENGTH];
  int width;
  int height;
  Colour lindberghColour;
  GameStatus gameStatus;
  GameType gameType;
  KeyMapping keymap;
  uint32_t crc32;
  GameRegion region;
  int freeplay;
  int showDebugMessages;
  char *gameID;
  char *gameTitle;
  char* gameDVP;
} EmulatorConfig;

KeyMapping getDefualtKeymap();
int initConfig();
EmulatorConfig *getConfig();
char *getGameName();
char *getDVPName();
char *getGameID();
