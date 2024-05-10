#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

EmulatorConfig config = {0};

extern uint32_t elf_crc;

FILE *configFile = NULL;

#define CONFIG_PATH "lindbergh.conf"
#define MAX_LINE_LENGTH 1024

static char *getNextToken(char *buffer, char *seperator, char **saveptr)
{
    char *token = strtok_r(buffer, seperator, saveptr);
    if (token == NULL)
        return NULL;

    for (int i = 0; i < (int)strlen(token); i++)
    {
        if ((token[i] == '\n') || (token[i] == '\r'))
        {
            token[i] = 0;
        }
    }
    return token;
}

static int detectGame(uint32_t elf_crc)
{

    switch (elf_crc)
    {

    case SEGABOOT_2_4:
    {
        config.gameTitle = "Segaboot 2.4";
        config.gameStatus = WORKING;
        return 0;
    }
    break;

    case SEGABOOT_2_4_SYM:
    {
        config.gameTitle = "Segaboot 2.4 with symbols";
        config.gameStatus = WORKING;
        return 0;
    }
    break;

    case THE_HOUSE_OF_THE_DEAD_4_REVA:
    case THE_HOUSE_OF_THE_DEAD_4_REVA_TEST:
    {
        config.gameTitle = "The House of the Dead 4 Rev A";
        config.gameID = "SBLC";
        config.gameDVP = "DVP-0003A";
        config.gameType = SHOOTING;
        config.gameStatus = WORKING;
        return 0;
    }
    break;

    case THE_HOUSE_OF_THE_DEAD_4_REVC:
    case THE_HOUSE_OF_THE_DEAD_4_REVC_TEST:
    {
        config.gameTitle = "The House of the Dead 4 Rev C";
        config.gameID = "SBLC";
        config.gameDVP = "DVP-0003C";
        config.gameType = SHOOTING;
        config.gameStatus = WORKING;
        return 0;
    }
    break;

    case THE_HOUSE_OF_THE_DEAD_4_SPECIAL:
    case THE_HOUSE_OF_THE_DEAD_4_SPECIAL_TEST:
    {
        config.gameTitle = "The House of the Dead 4 Special";
        config.emulateRideboard = 1;
        config.gameStatus = WORKING;
        config.width = 1024;
        config.height = 768;
        return 0;
    }
    break;

    case THE_HOUSE_OF_THE_DEAD_EX:
    case THE_HOUSE_OF_THE_DEAD_EX_TEST:

    {
        config.gameTitle = "The House of the Dead EX";
        config.gameStatus = WORKING;
        return 0;
    }
    break;

    case OUTRUN_2_SP_SDX_REVA:
    case OUTRUN_2_SP_SDX_REVA_TEST:
    {
        config.gameTitle = "Outrun 2 SP SDX";
        config.gameDVP = "DVP-0015A";
        config.gameID = "SBMB";
        config.emulateDriveboard = 1;
        config.emulateMotionboard = 1;
        config.gameStatus = WORKING;
        config.gameType = DRIVING;
        return 0;
    }
    break;

    case VIRTUA_FIGHTER_5_EXPORT:
    {
        config.gameTitle = "Virtua Fighter 5 Export";
        config.gameDVP = "DVP-0043";
        config.gameStatus = WORKING;
        return 0;
    }
    break;

    case VIRTUA_FIGHTER_5_R:
    {
        config.gameTitle = "Virtua Fighter 5 R";
        config.gameDVP = "DVP-XXXX";
        config.gameID = "SBQU";
        config.gameStatus = NOT_WORKING;
        return 0;
    }

    case VIRTUA_FIGHTER_5_FINAL_SHOWDOWN:
    {
        config.gameTitle = "Virtua Fighter 5 Final Showdown";
        config.gameDVP = "DVP-SBUV";
        config.gameID = "SBUV";
        return 0;
    }

    case VIRTUA_FIGHTER_5_FINAL_SHOWDOWN_REVA:
    {
        config.gameTitle = "Virtua Fighter 5 Final Showdown REV A";
        config.gameDVP = "DVP-5019A";
        config.gameID = "SBUV";
        return 0;
    }

    case VIRTUA_FIGHTER_5_FINAL_SHOWDOWN_REVB:
    {
        config.gameTitle = "Virtua Fighter 5 Final Showdown REV B";
        config.gameDVP = "DVP-5019B";
        config.gameID = "SBUV";
        return 0;
    }

    case LETS_GO_JUNGLE:
    {
        config.gameTitle = "Let's Go Jungle! Lost on the Island of Spice!";
        config.gameDVP = "DVP-0011";
        config.gameStatus = WORKING;
        return 0;
    }
    break;

    case LETS_GO_JUNGLE_SPECIAL:
    {
        config.gameTitle = "Let's Go Jungle! Special!";
        config.emulateRideboard = 1;
        config.gameStatus = WORKING;
        return 0;
    }
    break;

    case AFTER_BURNER_CLIMAX:
    {
        config.gameTitle = "After Burner Climax";
        config.gameStatus = WORKING;
        return 0;
    }
    break;

    case AFTER_BURNER_CLIMAX_REVA:
    {
        config.gameTitle = "After Burner Climax Rev A";
        config.gameStatus = WORKING;
        return 0;
    }
    break;

    case AFTER_BURNER_CLIMAX_SDX:
    {
        config.gameTitle = "After Burner Climax SDX";
        config.gameStatus = WORKING;
        return 0;
    }
    break;

    case AFTER_BURNER_CLIMAX_CE:
    {
        config.gameTitle = "After Burner Climax CE";
        config.gameStatus = WORKING;
        return 0;
    }
    break;

    case INITIALD_4:
    {
        config.gameTitle = "Initial D Arcade Stage 4";
        config.gameStatus = WORKING;
        config.gameType = DRIVING;
        return 0;
    }
    break;

    case INITIALD_4_REVE:
    {
        config.gameTitle = "Initial D Arcade Stage 4 Rev E";
        config.gameStatus = WORKING;
        config.gameType = DRIVING;
        return 0;
    }
    break;

    case SEGA_RACE_TV:
    {
        config.gameTitle = "SEGA Race TV";
        config.emulateDriveboard = 1;
        config.gameStatus = WORKING;
        config.gameType = DRIVING;
        return 0;
    }
    break;

    case RAMBO:
    {
        config.gameTitle = "Rambo";
        config.gameDVP = "DVP-0069";
        config.gameID = "SBQL";
        config.gameType = SHOOTING;
        config.gameStatus = WORKING;
        return 0;
    }
    break;

    case R_TUNED:
    {
        config.gameTitle = "R Tuned";
        config.emulateDriveboard = 1;
        config.gameStatus = WORKING;
        config.gameType = DRIVING;
        return 0;
    }
    break;

    case TOO_SPICY:
    {
        config.gameTitle = "Too Spicy";
        config.gameStatus = WORKING;
        return 0;
    }
    break;

    case VIRTUA_TENNIS_3:
    case VIRTUA_TENNIS_3_TEST:
    {
        config.gameTitle = "Virtua Tennis 3";
        config.gameStatus = WORKING;
        return 0;
    }
    break;

    case VIRTUA_FIGHTER_5_REVC:
    {
        config.gameTitle = "Virtua Fighter 5 Rev C";
        config.gameStatus = WORKING;
        return 0;
    }
    break;

    case VIRTUA_FIGHTER_5_REVE:
    {
        config.gameTitle = "Virtua Fighter 5 Rev E";
        config.gameStatus = NOT_WORKING;
        return 0;
    }
    break;

    case PRIMEVAL_HUNT:
    {
        config.gameTitle = "Primeval Hunt";
        config.gameStatus = WORKING;
        config.gameType = SHOOTING;
        return 0;
    }
    break;

    case GHOST_SQUAD_EVOLUTION:
    {
        config.gameTitle = "Ghost Squad Evolution";
        config.gameStatus = WORKING;
        config.gameDVP = "DVP-0029A";
        config.gameID = "SBNJ";
        return 0;
    }
    break;

    case INITIALD_5_EXP_20:
    {
        config.gameTitle = "Initial D Arcade Stage 5 Export Ver 2.0";
        config.gameStatus = NOT_WORKING;
        return 0;
    }
    break;

    case INITIALD_ARCADE_STAGE_5:
    {
        config.gameTitle = "Initial D Arcade Stage 5";
        config.gameStatus = NOT_WORKING;
        return 0;
    }
    break;

    case HUMMER_EXTREME:
    {
        config.gameTitle = "Hummer Extreme";
        config.gameID = "SBST";
        config.gameDVP = "DVP-0079";
        config.gameType = DRIVING;
        config.gameStatus = WORKING;
        return 0;
    }
    break;

    case LETS_GO_JUNGLE_REVA:
    {
        config.gameTitle = "Let's Go Jungle! Lost on the Island of Spice! Rev A";
        config.gameDVP = "DVP-0011A";
        config.gameStatus = WORKING;
        return 0;
    }
    break;

    default:
    {
        config.crc32 = UNKNOWN;
    }
    break;
    }

    return 1;
}

char *getGameName()
{
    return config.gameTitle;
}

char *getDVPName()
{
    return config.gameDVP;
}

char *getGameID()
{
    return config.gameID;
}

int readConfig(FILE *configFile, EmulatorConfig *config)
{
    char buffer[MAX_LINE_LENGTH];
    char *saveptr = NULL;

    while (fgets(buffer, MAX_LINE_LENGTH, configFile))
    {

        /* Check for comments */
        if (buffer[0] == '#' || buffer[0] == 0 || buffer[0] == ' ' || buffer[0] == '\r' || buffer[0] == '\n')
            continue;

        char *command = getNextToken(buffer, " ", &saveptr);

        if (strcmp(command, "WIDTH") == 0)
            config->width = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "HEIGHT") == 0)
            config->height = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "EEPROM_PATH") == 0)
            strcpy(config->eepromPath, getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "SRAM_PATH") == 0)
            strcpy(config->eepromPath, getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "EMULATE_RIDEBOARD") == 0)
            config->emulateRideboard = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "EMULATE_DRIVEBOARD") == 0)
            config->emulateDriveboard = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "EMULATE_MOTIONBOARD") == 0)
            config->emulateMotionboard = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "FULLSCREEN") == 0)
            config->fullscreen = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "EMULATE_JVS") == 0)
            config->emulateJVS = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "JVS_PATH") == 0)
            strcpy(config->jvsPath, getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "SERIAL_1_PATH") == 0)
            strcpy(config->serial1Path, getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "SERIAL_2_PATH") == 0)
            strcpy(config->serial2Path, getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "FREEPLAY") == 0)
            config->freeplay = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "LINDBERGH_COLOUR") == 0)
        {
            char colour[256];
            strcpy(colour, getNextToken(NULL, " ", &saveptr));
            if (strcmp(colour, "RED") == 0)
                config->lindberghColour = RED;
        }

        else if (strcmp(command, "REGION") == 0)
        {
            char region[256];
            strcpy(region, getNextToken(NULL, " ", &saveptr));
            if (strcmp(region, "JP") == 0)
                config->region = JP;
            else if (strcmp(region, "US") == 0)
                config->region = US;
            else
                config->region = EX;
        }

        else if (strcmp(command, "DEBUG_MSGS") == 0)
            config->showDebugMessages = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "TEST_KEY") == 0)
            config->keymap.test = atoi(getNextToken(NULL, " ", &saveptr));
        // TODO: add config when supporting player2
        else if (strcmp(command, "PLAYER_1_START_KEY") == 0)
            config->keymap.player1.start = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "PLAYER_1_SERVICE_KEY") == 0)
            config->keymap.player1.service = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "PLAYER_1_COIN_KEY") == 0)
            config->keymap.player1.coin = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "PLAYER_1_UP_KEY") == 0)
            config->keymap.player1.up = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "PLAYER_1_DOWN_KEY") == 0)
            config->keymap.player1.down = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "PLAYER_1_LEFT_KEY") == 0)
            config->keymap.player1.left = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "PLAYER_1_RIGHT_KEY") == 0)
            config->keymap.player1.right = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "PLAYER_1_BUTTON_1_KEY") == 0)
            config->keymap.player1.button1 = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "PLAYER_1_BUTTON_2_KEY") == 0)
            config->keymap.player1.button2 = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "PLAYER_1_BUTTON_3_KEY") == 0)
            config->keymap.player1.button3 = atoi(getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "PLAYER_1_BUTTON_4_KEY") == 0)
            config->keymap.player1.button4 = atoi(getNextToken(NULL, " ", &saveptr));

        else
            printf("Error: Unknown settings command %s\n", command);
    }

    return 0;
}

KeyMapping getDefaultKeymap()
{
    KeyMapping defaultKeyMapping;
    defaultKeyMapping.test = 28;
    defaultKeyMapping.player1.start = 10;
    defaultKeyMapping.player1.service = 39;
    defaultKeyMapping.player1.coin = 14;
    defaultKeyMapping.player1.up = 111;
    defaultKeyMapping.player1.down = 116;
    defaultKeyMapping.player1.left = 113;
    defaultKeyMapping.player1.right = 114;
    defaultKeyMapping.player1.button1 = 24;
    defaultKeyMapping.player1.button2 = 25;
    defaultKeyMapping.player1.button3 = 26;
    defaultKeyMapping.player1.button4 = 27;
    defaultKeyMapping.player2.start = -1;
    defaultKeyMapping.player2.service = -1;
    defaultKeyMapping.player2.coin = -1;
    defaultKeyMapping.player2.up = -1;
    defaultKeyMapping.player2.down = -1;
    defaultKeyMapping.player2.left = -1;
    defaultKeyMapping.player2.right = -1;
    defaultKeyMapping.player2.button1 = -1;
    defaultKeyMapping.player2.button2 = -1;
    defaultKeyMapping.player2.button3 = -1;
    defaultKeyMapping.player2.button4 = -1;
    return defaultKeyMapping;
}

int initConfig()
{
    config.emulateRideboard = 0;
    config.emulateDriveboard = 0;
    config.emulateMotionboard = 0;
    config.emulateJVS = 1;
    config.fullscreen = 0;
    config.lindberghColour = YELLOW;
    strcpy(config.eepromPath, "eeprom.bin");
    strcpy(config.sramPath, "sram.bin");
    strcpy(config.jvsPath, "/dev/ttyUSB0");
    strcpy(config.serial1Path, "/dev/ttyS0");
    strcpy(config.serial2Path, "/dev/ttyS1");
    config.width = 640;
    config.height = 480;
    config.crc32 = elf_crc;
    config.region = -1;
    config.freeplay = -1;
    config.showDebugMessages = 0;
    config.gameTitle = "Unknown game";
    config.gameID = "XXXX";
    config.gameDVP = "DVP-XXXX";
    config.gameType = SHOOTING;
    config.keymap = getDefaultKeymap();
    if (detectGame(config.crc32) != 0)
    {
        printf("Warning: Unsure what game with CRC 0x%X is. Please submit this new game to the GitHub repository: https://github.com/bobbydilley/lindbergh-loader/issues/new?title=Please+add+new+game+0x%X&body=I+tried+to+launch+the+following+game:\n", config.crc32, config.crc32);
    }

    configFile = fopen(CONFIG_PATH, "r");

    if (configFile == NULL)
    {
        printf("Warning: Cannot open %s, using default values.\n", CONFIG_PATH);
        return 1;
    }

    readConfig(configFile, &config);

    fclose(configFile);

    return 0;
}

EmulatorConfig *getConfig()
{
    return &config;
}
