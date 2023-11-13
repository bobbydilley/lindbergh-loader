#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

EmulatorConfig config = {0};

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

static int detectGame()
{

    // For a better way of doing this, we should look for strings inside the game
    // This will allow patching the game and for it to still be detected.
    // strings %s | grep "RIDE TURN TEST" && strings %s | grep "PLEASE SHOOT GRID"
    // shows you its hotd4S for example

    if (strstr(program_invocation_name, "segaboot"))
    {
        config.game = SEGABOOT;
        return 0;
    }

    if (strstr(program_invocation_name, "hod4"))
    {
        config.game = THE_HOUSE_OF_THE_DEAD_4;
        return 0;
    }

    if (strstr(program_invocation_name, "Jennifer"))
    {
        config.game = OUTRUN;
        config.emulateDriveboard = 1;
        config.emulateMotionboard = 1;
        return 0;
    }

    if (strstr(program_invocation_name, "JenTest"))
    {
        config.game = OUTRUN_TEST;
        config.emulateDriveboard = 1;
        config.emulateMotionboard = 1;
        return 0;
    }

    if (strstr(program_invocation_name, "lgj"))
    {
        config.game = LETS_GO_JUNGLE;
        return 0;
    }

    if (strstr(program_invocation_name, "abc1080"))
    {
        config.game = ABC;
        return 0;
    }

    if (strstr(program_invocation_name, "drive"))
    {
        config.game = SRTV;
        return 0;
    }
    
        if (strstr(program_invocation_name, "dsr"))
    {
        config.game = RTUNED;
        return 0;
    }
    
    config.game = UNKNOWN;
    return 1;
}

char *getGameName()
{
    switch (config.game)
    {
    case SEGABOOT:
        return "SEGABOOT";
    case SEGABOOT_2_4:
        return "SEGABOOT 2.4";
    case SEGABOOT_2_6:
        return "SEGABOOT 2.6";
    case OUTRUN:
        return "Outrun 2 SP";
    case THE_HOUSE_OF_THE_DEAD_4:
        return "The House of the Dead 4";
    case LETS_GO_JUNGLE:
        return "Let's Go Jungle! Lost on the Island of Spice";
    case ABC:
        return "After Burner Climax";
    case SRTV:
        return "SEGA Race TV";
    case RTUNED:
        return "R-Tuned Ultimate Street Racing";
    default:
        return "Unknown Game";
    }
    return "Unknown Game";
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

        else if (strcmp(command, "LINDBERGH_COLOUR") == 0)
        {
            char colour[256];
            strcpy(colour, getNextToken(NULL, " ", &saveptr));
            if (strcmp(colour, "RED") == 0)
                config->lindberghColour = RED;
        }

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
    config.width = 1024;
    config.height = 768;
    if (detectGame() != 0)
    {
        printf("Warning: Unsure what game this is, using default configuration values");
    }

    configFile = fopen(CONFIG_PATH, "r");

    if (configFile == NULL)
    {
        printf("Warning: Cannot open %s, using default values\n", CONFIG_PATH);
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
