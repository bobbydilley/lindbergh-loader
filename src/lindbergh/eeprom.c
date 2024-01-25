#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "eeprom.h"
#include "eeprom_settings.h"

#define I2C_SMBUS_BLOCK_MAX 32
#define I2C_GET_FUNCTIONS 0x705
#define I2C_SMBUS_TRANSFER 0x720
#define I2C_SET_SLAVE_MODE 0x703
#define I2C_BUFFER_CLEAR 0x1261
#define I2C_READ 1
#define I2C_SEEK 2
#define I2C_WRITE 3

union i2c_smbus_data {
  uint8_t byte;
  uint16_t word;
  uint8_t block[I2C_SMBUS_BLOCK_MAX + 2];
};

struct i2c_smbus_ioctl_data {
  uint8_t read_write;
  uint8_t command;
  uint32_t size;
  union i2c_smbus_data *data;
};

FILE *eeprom = NULL;

int initEeprom() {
  char *eepromPath = getConfig()->eepromPath;
  eeprom = fopen(eepromPath, "a");

  // Create file if it doesn't exist
  if (eeprom == NULL) {
    printf("Error: Cannot open %s\n", eepromPath);
    return 1;
  }

  fclose(eeprom);

  eeprom = fopen(eepromPath, "rb+");

  if (eepromSettingsInit(eeprom) != 0) {
    printf("Error initializing eeprom settings.");
    fclose(eeprom);
    return 1;
  }

  if (getConfig()->region != -1) {
    if (getRegion() != getConfig()->region)
      setRegion(eeprom, getConfig()->region);
  }

  if (getConfig()->freeplay != -1) {
    if (getFreeplay() != getConfig()->freeplay)
      setFreeplay(eeprom, getConfig()->freeplay);
  }

  if ((getConfig()->crc32 == LETS_GO_JUNGLE_SPECIAL) ||
      (getConfig()->crc32 == THE_HOUSE_OF_THE_DEAD_EX) ||
      (getConfig()->crc32 == THE_HOUSE_OF_THE_DEAD_4_SPECIAL)) {
    if (fixCreditSection(eeprom) != 0) {
      printf("Error initializing eeprom settings.");
      fclose(eeprom);
      return 1;
    }
  }

  fseek(eeprom, 0, SEEK_SET);

  return 0;
}

int eepromIoctl(int fd, unsigned int request, void *data) {
  switch (request) {

  case I2C_GET_FUNCTIONS: {
    // The below is copied from what SEGABOOT expects
    uint32_t *functions = data;
    functions[0] = 0x20000 | 0x40000 | 0x100000 | 0x400000 | 0x8000000;

    // The following is taken from the eeprom init sequence in The House Of The
    // Dead 4 so let's add em on!
    functions[0] = functions[0] | 0x20000 | 0x40000 | 0x80000 | 0x100000 |
                   0x200000 | 0x400000 | 0x1000000 | 0x2000000;
  } break;

  case I2C_SMBUS_TRANSFER: {
    struct i2c_smbus_ioctl_data *_data = data;

    switch (_data->size) {

    case I2C_READ: {
      fread(&_data->data->byte, 1, sizeof(char), eeprom);
    } break;

    case I2C_SEEK: {
      uint16_t address =
          (_data->command & 0xFF) << 8 | (_data->data->byte & 0xFF);
      fseek(eeprom, address, SEEK_SET);
    } break;

    case I2C_WRITE: {
      uint16_t address =
          (_data->command & 0xFF) << 8 | (_data->data->byte & 0xFF);
      char val = _data->data->word >> 8 & 0xFF;
      fseek(eeprom, address, SEEK_SET);
      fwrite(&val, 1, sizeof(char), eeprom);
    } break;

    default:
      printf("Error: Incorrect I2C transfer size\n");
    }
  } break;

  case I2C_SET_SLAVE_MODE:
  case I2C_BUFFER_CLEAR:
    break;

  default:
    printf("Error: Unkown I2C ioctl %X\n", request);
  }

  return 0;
}
