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

### EMULATE_JVS

This turns on the JVS emulation layer, which will use X11s input capabilities to read your mouse/keyboard. When this is turned off the JVS traffic will be passed through to a serial port defined by `JVS_PATH`.

### JVS_PATH

This defines the path of the serial port that is connected to the JVS IO.

### FULLSCREEN

This defines if the game should open in full screen mode.

### EMULATE_RIDEBOARD

This turns on the Rideboard emulation layer used in the games The House Of The Dead 4 Special, and Let's Go Jungle Special!

### EMULATE_DRIVEBOARD

This turns on the Driveboard emulation layer used in the games Outrun 2 SP SDX, SEGA Race TV, Initial D 4 and Initial D 5.

### EMULATE_MOTIONBOARD

This turns on the motionboard emulation layer used in the game Outrun 2 SP SDX in its SDX setting.
