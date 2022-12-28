# SEGA Lindbergh Emulator

This project aims to hook and emulate the various different parts of the SEGA Lindbergh allowing the games to run on modern versions of Linux.

You can view the supported titles [here.](docs/supported.md)

You will need an nvidia graphics card and I have tested with the latest version of Ubuntu.

## Dependencies

First make sure you have up to date nvidia drivers for your computer, and then install the following:

```
sudo dpkg --add-architecture i386 
sudo apt install gcc-multilib
sudo apt install freeglut3:i386 freeglut3-dev:i386 libglew-dev
sudo apt install xorg-dev
sudo apt install libopenal1 libopenal-dev
sudo apt install libalut-dev:i386
sudo apt install libxmu6:i386
sudo apt install libstdc++5:i386
```

## Building & Running

To build run the makefile, and then copy the contents of the build directory into your game directory and run.

```
make
cp build/* ~/the-house-of-the-dead-4/disk0/elf/.
cd ~/the-house-of-the-dead-4/disk0/elf
LD_PRELOAD=lindbergh.so LD_LIBRARY_PATH=$LD_LIBRARY_PATH:. ./hod4M.elf
```

It is likely that the games will require various other libraries from the Lindbergh system such as 'libkswapapi.so' and 'libposixtime.so'. These can be found in any dumps of the Lindbergh CF image.

A default configuration file is provided in `docs/lindbergh.conf`. It should be placed in the same folder as the game is run from. If no config file is present a default setting will be used.

## Controls

Currently the controls are setup for The House of the Dead 4.

- t           - Test
- s           - Service
- 5           - Coin 1
- 1           - Player 1 Start
- Right Click - Reload
- Left Click  - Shoot

## Thanks

This project has been built by referencing things made by Teknoparrot, Doozer and JayFoxRox so thanks to all of them!
