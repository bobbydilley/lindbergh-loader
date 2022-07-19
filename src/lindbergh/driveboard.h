#include <stdio.h>

int initDriveboard();
ssize_t driveboardRead(int fd, void *buf, size_t count);
ssize_t driveboardWrite(int fd, const void *buf, size_t count);
