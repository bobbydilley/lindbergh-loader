# Windows Installation Instructions

Please note that this software is still in the alpha phase, and it's very unlikely any games will be fully playable.

## Requirements

- Windows 10 build 2004 or higher
- WSL2
- Ubuntu 22.04

## Building & Installation

Launch Ubuntu 22.04 and and reset the root password.

```
sudo passwd root
su root
```

Now that you're in the root user you can install the dependencies:

```
sudo dpkg --add-architecture i386
sudo apt update && apt list --upgradable && apt upgrade
sudo apt install wsl g++ mesa-utils cmake make-guile gcc-multilib xorg-dev libxmu6:i386 libstdc++5:i386 libopenal-dev:i386 freeglut3:i386 freeglut3-dev:i386 libglew-dev
```

Now you will need to build libalut (the audio library) from source:

```
git clone --recursive https://github.com/vancegroup/freealut.git
cd freealut
cmake . -DCMAKE_INSTALL_PREFIX:STRING="/usr" -DCMAKE_C_FLAGS:STRING="-m32 -O2"
make
make install
```

Now you should add yourself to the following groups.

```
sudo addgroup $USER dialout
sudo addgroup $USER input
```

Now you can clone and build the lindbergh loader repository.

```
cd
git clone https://github.com/bobbydilley/lindbergh-loader.git
cd lindbergh-loader
make
```

You should then see the 3 .so files produced in the `build` directory.

## Launching a game

You should copy the 3 files from the `lindbergh-loader/build` directory to the directory with the game elf in.

Then open powershell in windows and do the following steps.

```
cd C:\rom\hod4\elf
bash
cp ~/lindbergh-loader/build/* .
LD_PRELOAD=lindbergh.so LD_LIBRARY_PATH=$LD_LIBRARY_PATH:. ./hod4M.elf
```

## Thanks

Thanks to dorminirko for the testing and writing of the original guide.
