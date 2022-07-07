#include <stdio.h>

int initBaseboard();

ssize_t baseboardRead(int fd, void *buf, size_t count);
ssize_t baseboardWrite(int fd, const void *buf, size_t count);

int baseboardIoctl(int fd, unsigned int request, void *data);
int baseboardSelect(int nfds, fd_set *restrict readfds, fd_set *restrict writefds, fd_set *restrict exceptfds, struct timeval *restrict timeout);
