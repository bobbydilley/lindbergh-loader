# SEGA Lindbergh Emulator

This project aims to hook and emulate the various different parts of the SEGA Lindbergh allowing the games to run on modern versions of Linux.

You can view the supported titles [here.](docs/supported.md)

## Dependencies

First make sure you have up to date nvidia drivers for your computer, and then install the following:

```
sudo dpkg --add-architecture i386 
sudo apt-get install gcc-multilib
sudo apt-get install freeglut3:i386 freeglut3-dev:i386 libglew-dev
sudo apt-get install xorg-dev
sudo apt-get install libopenal1 libopenal-dev
sudo apt-get install libxmu6:i386
sudo apt-get install libstdc++5:i386
```

## Building & Running

To build simply run the makefile, and then copy the contents of the build directory into your game directory and run.

```
make
cp build/* ~/the-house-of-the-dead-4/disk0/elf/.
cd ~/the-house-of-the-dead-4/disk0/elf
LD_PRELOAD=lindbergh.so LD_LIBRARY_PATH=$LD_LIBRARY_PATH:. ./hod4M.elf
```

A default configuration file is provided in `docs/lindbergh.conf`. It should be placed in the same folder as the game is run from. If no config file is present a default setting will be used.
