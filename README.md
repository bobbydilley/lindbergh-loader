# SEGA Lindbergh Loader

## Supported Titles

The loader can currently run the following titles:

- The House Of The Dead 4
- The House Of The Dead 4 Special
- SEGA Race TV
- The House Of The Dead EX
- Outrun 2 SP SDX
- 2Spicy
- Rambo

## Building & Running

To build simply run the makefile, and then copy the contents of the build directory into your game directory and run.

```
make
cp build/* ~/the-house-of-the-dead-4/disk0/elf/.
cd ~/the-house-of-the-dead-4/disk0/elf
LD_PRELOAD=$(pwd)/lindbergh.so LD_LIBRARY_PATH=$LD_LIBRARY_PATH:. ./hod4M.elf
```

## Components

This section lists the components of the emulator and what they do

### Rideboard

This is a high level emulation of the ride control board used in The House Of The Dead 4 Special and Let's Go Jungle Special.

### Driveboard

This is a high level emulation of various force feedback drive boards used in Lindbergh games

### Motion Board

This is a high level emulation of the motion control board used in Outrun 2 SP SDX

### libsegaapi.so

This is an emulation of the driver that games use to route sound out of the Creative Labs soundcard. This routes sound using OpenAL.
