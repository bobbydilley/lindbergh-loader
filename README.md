# SEGA Lindbergh Emulator

This project aims to hook and emulate the various different parts of the SEGA Lindbergh allowing the games to run on modern versions of Linux.

You can view the supported titles [here.](docs/supported.md)

You will need an nvidia graphics card and I have tested with the latest version of Ubuntu.

Please be aware that the project is in very early stages, and there will be lots of issues with all games.

## Dependencies

First make sure you have up to date nVidia drivers for your computer, and then install the following:

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

This emulator will need access to the input devices and serial devices on Linux. Before running this emulator you should add your user account to the following groups and then _restart your computer_.

```
sudo addgroup $USER dialout
sudo addgroup $USER input
```

To build, run the makefile, and then copy the contents of the build directory into your game directory and run.

```
make
cp build/* ~/the-house-of-the-dead-4/disk0/elf/.
cd ~/the-house-of-the-dead-4/disk0/elf
LD_PRELOAD=lindbergh.so LD_LIBRARY_PATH=$LD_LIBRARY_PATH:. ./hod4M.elf
```

Some games will require extra libraries like `libposixtime.so`. These can be found in any dumps of the Lindbergh CF image.

A default configuration file is provided in `docs/lindbergh.conf`. It should be placed in the same folder as the game is run from. If no config file is present a default setting will be used.

I recomend that you do not run this as root, and instead use the usergroups for input/dialout to give the emulator access to what it needs. The Lindbergh games expect full control of the Linux OS and with root privilages it is possible that they could cause damage to your computer.

## Controls

Currently the controls are set up for The House of the Dead 4.

| Key         | Mapping        |
|-------------|----------------|
| t           | Test           |
| s           | Service        |
| 5           | Coin 1         |
| 1           | Player 1 Start |
| Right Click | Reload         |
| Left Click  | Shoot          |

## Thanks

This project has been built by referencing things made by Teknoparrot, Doozer and JayFoxRox and from contributions by Rolel and 
dkeruza-neo and extensive testing by Francesco so thanks to all of them!
