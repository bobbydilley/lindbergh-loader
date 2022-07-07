#include <string.h>

#include "config.h"

EmulatorConfig config = {0};

int initConfig() {
    config.emulateRideboard = 0;
    strcpy(config.eepromPath, "eeprom.bin");
    strcpy(config.sramPath, "sram.bin");
    return 0;
}

EmulatorConfig *getConfig() {
    
    return &config;
}
