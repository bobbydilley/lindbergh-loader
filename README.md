# ATTENTION!

This project contains header files from SEGA. Please DO NOT distrobute this code. The header files will be re-written completely at some point to make the software FOS.

# SEGA Lindbergh Loader

This project aims to hook and emulate the various different parts of the SEGA Lindbergh allowing the games to run on modern versions of Linux.

## What this project does

This project provides an emulation layer so for various software & hardware components of hte SEGA Lindbergh system allowing you to run the games on a modern version of linux. The project also aims to provide documentation of the various APIs used in the Lindbergh system which may be useful for further emulation projects, or homebrew.

## What this project does not do

This project does not defeat any security mechanisms of the games, nor does it provide links to roms. You should only use this project to play games if you own the original and removing the security checks form the games is left entierly up to the user. Please do not create issues asking about security bypassing, or have discussions on this github about the subject.

## Building & Running

To build simply run the makefile, and then copy the contents of the build directory into your game directory and run.

```
make
cp build/* ~/the-house-of-the-dead-4/disk0/elf/.
cd ~/the-house-of-the-dead-4/disk0/elf
LD_PRELOAD=$(pwd)/lindbergh.so LD_LIBRARY_PATH=$LD_LIBRARY_PATH:. ./hod4M.elf
```

Although not yet used, you will then want to copy a `game.conf` file form the `docs` directory and place it next to the game you with to use.

## Specific Games

Here is how you should run specific games:

### Lets Go Jungle

```LD_LIBRARY_PATH=$LD_LIBRARY_PATH:. LD_PRELOAD=lindbergh.so TEA_DIR=`pwd` ./lgj_final```

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

## Config

A default configuration file is provided in `docs/lindbergh.conf`. It should be placed in the same folder as the game is run from. If no config file is present a default setting will be used. 
