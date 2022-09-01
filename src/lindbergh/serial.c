#include <stdio.h>
#include <sys/types.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <linux/serial.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <linux/serial.h>

#include "serial.h"

#define TIMEOUT_SELECT 200

int setSerialAttributes(int fd, int myBaud)
{
  struct termios options;
  int status;
  tcgetattr(fd, &options);

  cfmakeraw(&options);
  cfsetispeed(&options, myBaud);
  cfsetospeed(&options, myBaud);

  options.c_cflag |= (CLOCAL | CREAD);
  options.c_cflag &= ~PARENB;
  options.c_cflag &= ~CSTOPB;
  options.c_cflag &= ~CSIZE;
  options.c_cflag |= CS8;
  options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
  options.c_oflag &= ~OPOST;

  options.c_cc[VMIN] = 0;
  options.c_cc[VTIME] = 0; // One seconds (10 deciseconds)

  tcsetattr(fd, TCSANOW, &options);

  ioctl(fd, TIOCMGET, &status);

  status |= TIOCM_DTR;
  status |= TIOCM_RTS;

  ioctl(fd, TIOCMSET, &status);

  usleep(100 * 1000); // 10mS

  struct serial_struct serial_settings;

  ioctl(fd, TIOCGSERIAL, &serial_settings);

  serial_settings.flags |= ASYNC_LOW_LATENCY;
  ioctl(fd, TIOCSSERIAL, &serial_settings);

  tcflush(fd, TCIOFLUSH);
  usleep(100 * 1000); // Required to make flush work, for some reason

  return 0;
}

int readBytes(int fd, unsigned char *buffer, int amount)
{
  fd_set fd_serial;
  struct timeval tv;

  FD_ZERO(&fd_serial);
  FD_SET(fd, &fd_serial);

  tv.tv_sec = 0;
  tv.tv_usec = TIMEOUT_SELECT * 1000;

  int filesReadyToRead = select(fd + 1, &fd_serial, NULL, NULL, &tv);

  if (filesReadyToRead < 1)
    return 0;

  if (!FD_ISSET(fd, &fd_serial))
    return 0;

  return read(fd, buffer, amount);
}
