# SEGA Lindbergh Emulator

This alpha stage project emulates the SEGA Lindbergh, allowing games to run on modern Linux computers with any NVIDIA graphics card.

You can view the supported titles [here.](docs/supported.md)

## Dependencies

First make sure you have up-to-date NVIDIA drivers and then install the following:

```
sudo dpkg --add-architecture i386
sudo apt update
sudo apt install gcc-multilib
sudo apt install build-essential
sudo apt install freeglut3:i386
sudo apt install freeglut3-dev:i386
sudo apt install libglew-dev
sudo apt install xorg-dev
sudo apt install libopenal1 libopenal-dev
sudo apt install libalut-dev:i386 // You will need to find libalut-dev:i386, libalut0:i386 and multiarch-support:i386 from Ubuntu Xenial.
sudo apt install libxmu6:i386
sudo apt install libstdc++5:i386
```
or execute `init.bash`

```bash
sudo sh init.bash
```

Note: The project has been tested to work on Ubuntu 22.04, and doesn't currently work on Ubuntu 23.10. Multiple packages such as `freeglut3:i386` are not available anymore on Debian Trixxie or Ubuntu 23.10.

## Building & Running

This emulator will need access to the input devices and serial devices on Linux. Before running this emulator you should add your user account to the following groups and then _restart your computer_.

```
sudo usermod -a -G dialout,input $USER
```

To build, run the makefile, and then copy the contents of the build directory into the game directory and run.

```
make
cp build/* ~/the-house-of-the-dead-4/disk0/elf/.
cd ~/the-house-of-the-dead-4/disk0/elf
LD_PRELOAD=lindbergh.so LD_LIBRARY_PATH=$LD_LIBRARY_PATH:. ./hod4M.elf
```

Some games will require extra libraries like `libposixtime.so`, which can be found in dumps of the Lindbergh CF image.

A default configuration file is provided in `docs/lindbergh.conf`. It should be placed in the same folder the game is run from. If no config file is present a default setting will be used.

Do not run this as root, instead use the usergroups for input/dialout to give the emulator access to what it needs. Lindbergh games expect full control of the Linux OS and with root privilages it is possible that they could cause damage to your computer.

A `lindbergh` executable is provided in the build directory to easily run the games. Place it in the same directory as the game elf, and run `./lindbergh` to automatically start the game with the correct environment variables set, or run `./lindbergh -t` for test mode.

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

This project has been built by referencing earlier projects by Teknoparrot and JayFoxRox and from contributions by Doozer, Rolel and dkeruza-neo with extensive testing by Francesco - thanks to all of them!
