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

    if (elf_crc == 0x93ea7e11)
    {
        config.game = SEGABOOT_2_4;
        config.gameStatus = WORKING;
        return 0;
    }

    if (elf_crc == 0x3cc635ee)
    {
        config.game = SEGABOOT_2_4_SYM;
        config.gameStatus = WORKING;
        return 0;
    }

    if (elf_crc == 0xbc0c9ffa)
    {
        config.game = THE_HOUSE_OF_THE_DEAD_4;
        config.gameStatus = WORKING;
        return 0;
    }

    if (elf_crc == 0x5df569f5)
    {
        config.game = THE_HOUSE_OF_THE_DEAD_4_STRIPPED;
        config.gameStatus = WORKING;
        return 0;
    }

    if(elf_crc == 0xDDECE1E9) {
        config.game = THE_HOUSE_OF_THE_DEAD_4_STRIPPED_TEST;
        config.gameStatus = WORKING;
        return 0;
    }

    if (elf_crc == 0x7235bda8)
    {
        config.game = THE_HOUSE_OF_THE_DEAD_4_TEST;
        config.gameStatus = WORKING;
        return 0;
    }

    if (elf_crc == 0x12266f81)
    {
        config.game = THE_HOUSE_OF_THE_DEAD_4_SPECIAL;
        config.gameStatus = WORKING;
        return 0;
    }

    if (elf_crc == 0x83ba3b45)
    {
        config.game = THE_HOUSE_OF_THE_DEAD_4_SPECIAL_TEST;
        config.gameStatus = WORKING;
        return 0;
    }

    if (elf_crc == 0x85c0c22a)
    {
        config.game = THE_HOUSE_OF_THE_DEAD_EX;
        config.gameStatus = WORKING;
        return 0;
    }

    if (elf_crc == 0xb9a166bb)
    {
        config.game = THE_HOUSE_OF_THE_DEAD_EX_TEST;
        config.gameStatus = WORKING;
        return 0;
    }

    if (elf_crc == 0x6d055308)
    {
        config.game = OUTRUN_2_SP_SDX_REVA;
        config.emulateDriveboard = 1;
        config.emulateMotionboard = 1;
        config.gameStatus = WORKING;
        return 0;
    }

    if (elf_crc == 0xffdccaaa)
    {
        config.game = OUTRUN_2_SP_SDX_REVA_TEST;
        config.emulateDriveboard = 1;
        config.emulateMotionboard = 1;
        config.gameStatus = WORKING;
        return 0;
    }

    if (elf_crc == 0xd4726d61)
    {
        config.game = LETS_GO_JUNGLE;
        config.gameStatus = WORKING;
        return 0;
    }

    if (elf_crc == 0xbbabc0e0)
    {
        config.game = LETS_GO_JUNGLE_SPECIAL;
        config.gameStatus = NOT_WORKING;
        return 0;
    }

    if (elf_crc == 0xcc02de7d)
    {
        config.game = AFTER_BURNER_CLIMAX;
        config.gameStatus = WORKING;
        return 0;
    }

    if (elf_crc == 0x152530dd)
    {
        config.game = AFTER_BURNER_CLIMAX_REVA;
        config.gameStatus = WORKING;
        return 0;
    }

    if (elf_crc == 0x4e9ccf33)
    {
        config.game = INITIALD_4;
        config.gameStatus = NOT_WORKING;
        return 0;
    }

    if (elf_crc == 0x7f3f9f0c)
    {
        config.game = INITIALD_4_REVE;
        config.gameStatus = NOT_WORKING;
        return 0;
    }

    if (elf_crc == 0xfb096f81)
    {
        config.game = SEGA_RACE_TV;
        config.emulateDriveboard = 1;
        config.gameStatus = NOT_WORKING;
        return 0;
    }

    if (elf_crc == 0x77ebac34)
    {
        config.game = RAMBO;
        config.gameStatus = NOT_WORKING;
        return 0;
    }

    if (elf_crc == 0xb05d9bbe)
    {
        config.game = R_TUNED;
        config.emulateDriveboard = 1;
        config.gameStatus = WORKING;
        return 0;
    }

    if (elf_crc == 0x4c768eb4)
    {
        config.game = TOO_SPICY;
        config.gameStatus = WORKING;
        return 0;
    }

    if (elf_crc == 0xc4b7e89)
    {
        config.game = VIRTUA_TENNIS_3;
        config.gameStatus = WORKING;
        return 0;
    }

    if (elf_crc == 0xffe3b0fd)
    {
        config.game = VIRTUA_TENNIS_3_TEST;
        config.gameStatus = NOT_WORKING;
        return 0;
    }

    if (elf_crc == 0x1bf1b627)
    {
        config.game = VIRTUA_FIGHTER_5_REVC;
        config.gameStatus = WORKING;
        return 0;
    }

    if (elf_crc == 0x3CC635EE)
    {
        config.game = SEGABOOT_2_4;
        config.gameStatus = WORKING;
        return 0;
    }

    config.game = UNKNOWN;
    return 1;
}

char *getGameName()
{
    char *unknownGameTitle = "Unknown Game";
    switch (config.game)
    {
    case AFTER_BURNER_CLIMAX:
        return "After Burner Climax";
    case AFTER_BURNER_CLIMAX_REVA:
        return "After Burner Climax Rev A";
    case AFTER_BURNER_CLIMAX_REVB:
        return "After Burner Climax Rev B";
    case AFTER_BURNER_CLIMAX_SDX:
        return "After Burner Climax SDX";
    case AFTER_BURNER_CLIMAX_SDX_REVA:
        return "After Burner Climax SDX Rev A";
    case GHOST_SQUAD_EVOLUTION:
        return "Ghost Squad Evolution";
    case HARLEY_DAVIDSON:
        return "Harley Davidson: King of the Road";
    case HUMMER:
        return "Hummer";
    case HUMMER_EXTREME:
        return "Hummer Extreme";
    case HUMMER_EXTREME_MDX:
        return "Hummer Extreme MDX";
    case INITIALD_4:
        return "Initial D Arcade Stage 4";
    case INITIALD_5:
        return "Initial D Arcade Stage 5";
    case LETS_GO_JUNGLE:
        return "Let's Go Jungle! Lost on the Island of Spice!";
    case LETS_GO_JUNGLE_SPECIAL:
        return "Let's Go Jungle! Special!";
    case OUTRUN_2_SP_SDX:
        return "Outrun 2 SP SDX";
    case OUTRUN_2_SP_SDX_REVA:
        return "Outrun 2 SP SDX Rev A";
    case OUTRUN_2_SP_SDX_REVA_TEST:
        return "Outrun 2 SP SDX Rev A Test Mode";
    case OUTRUN_2_SP_SDX_TEST:
        return "Outrun 2 SP SDX Test Mode";
    case PRIMEVAL_HUNT:
        return "Primeval Hunt";
    case RAMBO:
        return "Rambo";
    case RAMBO_CHINA:
        return "Rambo China Release";
    case R_TUNED:
        return "R-Tuned Ultimate Street Racing";
    case SEGABOOT:
        return "Segaboot";
    case SEGABOOT_2_4:
        return "SEGABOOT 2.4";
    case SEGABOOT_2_4_SYM:
        return "SEGABOOT 2.4 with Symbols";
    case SEGABOOT_2_6:
        return "SEGABOOT 2.6";
    case SEGA_RACE_TV:
        return "SEGA Race TV";
    case THE_HOUSE_OF_THE_DEAD_4:
        return "The House of the Dead 4";
    case THE_HOUSE_OF_THE_DEAD_4_SPECIAL:
        return "The House of the Dead 4 Special";
    case THE_HOUSE_OF_THE_DEAD_4_SPECIAL_TEST:
        return "The House of the Dead 4 Special Test Mode";
    case THE_HOUSE_OF_THE_DEAD_4_TEST:
        return "The House of the Dead 4 Test Mode";
    case THE_HOUSE_OF_THE_DEAD_EX:
        return "The House of the Dead Ex";
    case TOO_SPICY:
        return "2 Step : 2 Spicy";
    case UNKNOWN:
        return unknownGameTitle;
    case VIRTUA_FIGHTER_5:
        return "Virtua Fighter 5";
    case VIRTUA_FIGHTER_5_FINAL_SHOWDOWN:
        return "Virtua Fighter 5 Final Showdown";
    case VIRTUA_FIGHTER_5_FINAL_SHOWDOWN_REVA:
        return "Virtua Fighter 5 Final Showdown Rev A";
    case VIRTUA_FIGHTER_5_R:
        return " Virtua Fighter 5 R";
    case VIRTUA_FIGHTER_5_REVA:
        return "Virtua Fighter 5 Rev A";
    case VIRTUA_FIGHTER_5_REVB:
        return "Virtua Fighter 5 Rev B";
    case VIRTUA_FIGHTER_5_REVC:
        return "Virtua Fighter 5 Rev C";
    case VIRTUA_FIGHTER_5_R_REVD:
        return "Virtua Fighter 5 Rev D";
    case VIRTUA_TENNIS_3:
        return "Virtua Tennis 3";
    case VIRTUA_TENNIS_3_TEST:
        return "Virtua Tennis 3 Test Mode";
    case THE_HOUSE_OF_THE_DEAD_4_STRIPPED:
        return "The House of the Dead 4 Rev C";
    case THE_HOUSE_OF_THE_DEAD_4_STRIPPED_TEST:
        return "The House of the Dead 4 Rev C Test Mode";
    case INITIALD_4_REVE:
        return "Initial D 4 Exp Rev E";
    default:
        return unknownGameTitle;
    }
    return unknownGameTitle;
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

        else if (strcmp(command, "RIDEBOARD_PATH") == 0)
            strcpy(config->rideboardPath, getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "DRIVEBOARD_PATH") == 0)
            strcpy(config->driveboardPath, getNextToken(NULL, " ", &saveptr));

        else if (strcmp(command, "MOTIONBOARD_PATH") == 0)
            strcpy(config->motionboardPath, getNextToken(NULL, " ", &saveptr));

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

        else
            printf("Error: Unknown settings command %s\n", command);
    }

    return 0;
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
    strcpy(config.jvsPath, "none");
    strcpy(config.driveboardPath, "none");
    strcpy(config.motionboardPath, "none");
    strcpy(config.rideboardPath, "none");
    config.width = 640;
    config.height = 480;
    config.crc32 = elf_crc;
    config.region = -1;
    config.freeplay = -1;
    config.showDebugMessages = 0;
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
