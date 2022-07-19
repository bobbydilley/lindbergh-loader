#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

EmulatorConfig config = {0};

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

    if (strstr(program_invocation_name, "Jennifer") || strstr(program_invocation_name, "JenTest"))
    {
        config.game = OUTRUN;
        config.emulateDriveboard = 1;
        config.emulateMotionboard = 1;
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
    case OUTRUN:
        return "Outrun 2 SP";
    case THE_HOUSE_OF_THE_DEAD_4:
        return "The House of the Dead 4";
    default:
        return "Unknown Game";
    }
    return "Unknown Game";
}

int initConfig()
{
    config.emulateRideboard = 0;
    config.emulateDriveboard = 0;
    config.emulateMotionboard = 0;
    strcpy(config.eepromPath, "eeprom.bin");
    strcpy(config.sramPath, "sram.bin");
    config.width = 1024;
    config.height = 768;
    if (detectGame() != 0)
    {
        printf("Warning: Unsure what game this is, using default configuration values");
    }
    return 0;
}

EmulatorConfig *getConfig()
{
    return &config;
}
