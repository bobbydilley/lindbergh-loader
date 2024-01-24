#!/bin/bash
# Tested by Caviar X on Ubuntu 22.04 jammy WSL
# NOTE: Never tested on an physical Ubuntu machine
if [ "$EUID" -ne 0 ]; then
    printf "\x1b[31mERROR: This script requires root permission,please execute it with sudo \x1b[0m\n"
    exit 1
fi
if [ "$(. /etc/os-release; printf "%s-%s" $NAME $VERSION_CODENAME)" = "Ubuntu-jammy" ]; then
    printf "Lindbergh loader environment setup by Caviar X(AutoBank)\n"
    # TODO: Remove this when it is tested on an physical machine.
    printf "\x1b[93mWARNING: This script is only tested on Ubuntu 22.04 jammy WSL\nRemove this when it is tested on an physical machine. \x1b[0m\n"
else
    printf "\x1b[31mERROR: This script shouldn't run on any other platform than Ubuntu 22.04(jammy)\x1b[0m\n"
    exit 1
fi
# Uncomment these if you're a Chinese mainland user (use aliyun mirror)
# You can also replace it with your specified mirror
#sudo sed -i 's@//.*archive.ubuntu.com@//mirrors.aliyun.com@g' /etc/apt/sources.list
#sudo sed -i 's/security.ubuntu.com/mirrors.aliyun.com/g' /etc/apt/sources.list
#sudo sed -i 's/http:/https:/g' /etc/apt/sources.list
sudo dpkg --add-architecture i386
sudo apt update
yes | sudo apt install gcc-multilib build-essential freeglut3:i386 freeglut3-dev:i386 libglew-dev xorg-dev libopenal1 libopenal-dev make git wget libxmu6:i386 libstdc++5:i386
wget https://launchpad.net/ubuntu/+archive/primary/+files/libalut0_1.1.0-6_i386.deb
# Trigger a dependency error to let apt fetch the libalut
sudo dpkg -i libalut0_1.1.0-6_i386.deb
yes | sudo apt --fix-broken install
# Ok,it should work well
wget https://launchpad.net/ubuntu/+archive/primary/+files/libalut-dev_1.1.0-6_i386.deb
sudo dpkg -i libalut-dev_1.1.0-6_i386.deb
wget http://launchpadlibrarian.net/534757982/multiarch-support_2.23-0ubuntu11.3_i386.deb
sudo dpkg -i multiarch-support_2.23-0ubuntu11.3_i386.deb
# Just in case
yes | sudo apt --fix-broken install
sudo usermod -a -G dialout,input $USER
# cleanup
rm *.deb
