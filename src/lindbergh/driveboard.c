#include "string.h"

#include "driveboard.h"

#define DRIVEBOARD_READY 0x00
#define DRIVEBOARD_NOT_INIT 0x11
#define DRIVEBOARD_BUSY 0x44

int wheelTest = 0;
unsigned char response = DRIVEBOARD_READY;

int initDriveboard()
{
    wheelTest = 0;
    response = DRIVEBOARD_NOT_INIT;

    return 0;
}

ssize_t driveboardRead(int fd, void *buf, size_t count)
{
    memcpy(buf, &response, 1);
    return 1;
}

ssize_t driveboardWrite(int fd, const void *buf, size_t count)
{

    if (count != 4)
    {
        printf("Error: Drive board count not what expected\n");
        return 1;
    }

    unsigned char *buffer = (unsigned char *)buf;

    switch (buffer[0])
    {
    case 0xFF:
    {
        printf("Driveboard: Drive board reset\n");
        response = DRIVEBOARD_READY;
    }
    break;

    case 0x81:
    {
        printf("Driveboard: Drive board reset 2\n");
        response = DRIVEBOARD_NOT_INIT;
    }
    break;

    case 0xFC:
    {
        printf("Driveboard: Start wheel bounds testing\n");
        wheelTest = 1;
    }
    break;

    case 0xFD:
    {
        printf("Driveboard: auto turn wheel mode\n");
        if (wheelTest)
        {
            printf("Increment wheel until 0.9\n");
        }
    }
    break;

    default:
        printf("Driveboard: Unknown command received\n");
        break;
    }

    return 0;
}
