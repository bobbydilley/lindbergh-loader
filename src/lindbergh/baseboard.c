#include <stdio.h>  /* Standard input/output definitions */
#include <string.h> /* String function definitions */
#include <unistd.h> /* UNIX standard function definitions */
#include <fcntl.h>  /* File control definitions */
#include <errno.h>  /* Error number definitions */
#include <stdlib.h> /* Standard library functions like malloc, free, exit, and atoi */
#include <sys/select.h>

#include "baseboard.h"

#include "config.h"
#include "jvs.h"
#include "serial.h"
#include "passthrough.h"

#define SERIAL_STRING "FE11-X018012022X"

#define BASEBOARD_INIT 0x300
#define BASEBOARD_GET_VERSION 0x8004BC02
#define BASEBOARD_SEEK_SHM 0x400
#define BASEBOARD_READ_SRAM 0x601
#define BASEBOARD_WRITE_SRAM 0x600
#define BASEBOARD_REQUEST 0xC020BC06
#define BASEBOARD_RECEIVE 0xC020BC07
#define BASEBOARD_GET_SERIAL 0x120
#define BASEBOARD_WRITE_FLASH 0x180
#define BASEBOARD_GET_SENSE_LINE 0x210
#define BASEBOARD_PROCESS_JVS 0x220
#define BASEBOARD_READY 0x201

typedef struct
{
  uint32_t srcAddress;
  uint32_t srcSize;
  uint32_t destAddress;
  uint32_t destSize;
} BaseboardCommand;

BaseboardCommand jvsCommand = {0};
BaseboardCommand serialCommand = {0};

typedef struct
{
  uint32_t *data;
  uint32_t offset;
  uint32_t size;
} readData_t;

typedef struct
{
  uint32_t offset;
  uint32_t *data;
  uint32_t size;
} writeData_t;

FILE *sram = NULL;

unsigned int sharedMemoryIndex = 0;
uint8_t sharedMemory[1024 * 32] = {0};

int selectReply = -1;
int jvsFileDescriptor = -1;
int jvsPacketSize = -1;

int initBaseboard()
{
  char *sramPath = getConfig()->sramPath;

  sram = fopen(sramPath, "a");

  // Create file if it doesn't exist
  if (sram == NULL)
  {
    printf("Error: Cannot open %s\n", sramPath);
    return 1;
  }
  fclose(sram);
  sram = fopen(sramPath, "rb+");
  fseek(sram, 0, SEEK_SET);

  if (getConfig()->emulateJVS == 0 && strcmp(getConfig()->jvsPath, "none") != 0)
  {
    jvsFileDescriptor = openJVSSerial(getConfig()->jvsPath);
    if (jvsFileDescriptor < 0)
    {
      printf("Error: Failed to open %s for JVS\n", getConfig()->jvsPath);
      return -1;
    }

    initJVSSerial(jvsFileDescriptor);
    startJVSFrameThread(&jvsFileDescriptor);
  }

  return 0;
}

ssize_t baseboardRead(int fd, void *buf, size_t count)
{
  memcpy(buf, &sharedMemory[sharedMemoryIndex], count);
  return count;
}

ssize_t baseboardWrite(int fd, const void *buf, size_t count)
{
  memcpy(&sharedMemory[sharedMemoryIndex], buf, count);
  return count;
}

int baseboardSelect(int nfds, fd_set *restrict readfds, fd_set *restrict writefds, fd_set *restrict exceptfds, struct timeval *restrict timeout)
{
  return selectReply;
}

int baseboardIoctl(int fd, unsigned int request, void *data)
{
  switch (request)
  {

  case BASEBOARD_GET_VERSION:
  {
    uint8_t versionData[4] = {0x00, 0x19, 0x20, 0x07};
    memcpy(data, versionData, 4);
    return 0;
  }
  break;

  case BASEBOARD_INIT:
  {
    // selectReply = -1; Considering adding this in
    return 0;
  }
  break;

  case BASEBOARD_READY: // Not sure if this is what it should be called
  {
    selectReply = 0;
    return 0;
  }
  break;

  case BASEBOARD_SEEK_SHM:
  {
    sharedMemoryIndex = (unsigned int)data;
    return 0;
  }
  break;

  case BASEBOARD_READ_SRAM:
  {
    readData_t *_data = data;
    fseek(sram, _data->offset, SEEK_SET);
    fread(_data->data, 1, _data->size, sram);
    return 0;
  }
  break;

  case BASEBOARD_WRITE_SRAM:
  {
    writeData_t *_data = data;
    fseek(sram, _data->offset, SEEK_SET);
    fwrite(_data->data, 1, _data->size, sram);
    return 0;
  }
  break;

  case BASEBOARD_REQUEST:
  {
    uint32_t *_data = data;

    switch (_data[0])
    {

    case BASEBOARD_GET_SERIAL: // bcCmdSysInfoGetReq
    {
      serialCommand.destAddress = _data[1];
      serialCommand.destSize = _data[2];
    }
    break;

    case BASEBOARD_WRITE_FLASH: // bcCmdSysFlashWrite
    {
      printf("Warning: The game attempted to write to the baseboard flash\n");
    }
    break;

    case BASEBOARD_PROCESS_JVS:
    {
      jvsCommand.srcAddress = _data[1];
      jvsCommand.srcSize = _data[2];
      jvsCommand.destAddress = _data[3];
      jvsCommand.destSize = _data[4];
      memcpy(inputBuffer, &sharedMemory[jvsCommand.srcAddress], jvsCommand.srcSize);

      if (getConfig()->emulateJVS)
      {
        processPacket(&jvsPacketSize);
      }
      else if (jvsFileDescriptor >= 0)
      {
        for (int i = 0; i < jvsCommand.srcSize; i++)
        {
          write(jvsFileDescriptor, &inputBuffer[i], 1);
          if (inputBuffer[i] == 0xF0)
          {
            setSenseLine(3);
          }
          else if (inputBuffer[i] == 0xF1)
          {
            setSenseLine(1);
          }
        }
      }
    }
    break;

    case BASEBOARD_GET_SENSE_LINE:
      break;

    default:
      printf("Error: Unknown baseboard command %X\n", _data[0]);
    }

    // Acknowledge the command
    _data[0] |= 0xF0000000;

    return 0;
  }
  break;

  case BASEBOARD_RECEIVE:
  {
    uint32_t *_data = data;

    switch (_data[0] & 0xFFF)
    {

    case BASEBOARD_GET_SERIAL:
    {
      memcpy(&sharedMemory[serialCommand.destAddress + 96], SERIAL_STRING, strlen(SERIAL_STRING));
      _data[1] = 1; // Set the status to success
    }
    break;

    case BASEBOARD_GET_SENSE_LINE:
    {
      _data[2] = getSenseLine();
      _data[1] = 1; // Set the status to success
    }
    break;

    case BASEBOARD_PROCESS_JVS:
    {
      if (getConfig()->emulateJVS)
      {
        memcpy(&sharedMemory[jvsCommand.destAddress], outputBuffer, jvsPacketSize);
        _data[2] = jvsCommand.destAddress;
        _data[3] = jvsPacketSize;
        _data[1] = 1; // Set the status to success
      }
      else if (jvsFileDescriptor >= 0)
      {
        JVSFrame frame = readJVSFrameFromThread();
        memcpy(&sharedMemory[jvsCommand.destAddress], frame.buffer, frame.size);
        _data[2] = jvsCommand.destAddress;
        _data[3] = frame.size;
        _data[1] = frame.ready;
      }
    }
    break;

    default:
      printf("Error: Unknown baseboard receive command %X\n", _data[0] & 0xFFF);
    }

    // Acknowledge the command
    _data[0] |= 0xF0000000;

    return 0;
  }
  break;

  default:
    printf("Error: Unknown baseboard ioctl %X\n", request);
  }

  return 0;
}
