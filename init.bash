#!/bin/bash
# Tested by Caviar X on Ubuntu 22.04 jammy WSL
# NOTE: Never tested on an physical Ubuntu machine
if [ "$EUID" -ne 0 ]; then
    echo "ERROR: This script requires root permission,please execute it with sudo\n"
    exit 1
fi
if [ "$(. /etc/os-release; printf "%s-%s" $NAME $VERSION_CODENAME)" = "Ubuntu-jammy" ]; then
    echo "Lindbergh loader environment setup by Caviar X(AutoBank)\n"
    # TODO: Remove this when it is tested on an physical machine.
    echo "WARNING: This script is only tested on Ubuntu 22.04 jammy WSL,AND MAY CAUSE DANMAGE TO YOUR COMPUTER.\nRemove this when it is tested on an physical machine.\n"
    printf "Confirm?(y/n):"
    read ans
    if [ "$ans" != "y" ]; then
	    exit
    fi
else
    echo "ERROR: This script shouldn't run on any other platform than Ubuntu 22.04(jammy)\n"
    exit 1
fi
# Uncomment these if you're a Chinese mainland user (use aliyun mirror)
# You can also replace it with your specified mirror
# sudo sed -i 's@//.*archive.ubuntu.com@//mirrors.aliyun.com@g' /etc/apt/sources.list
# sudo sed -i 's/security.ubuntu.com/mirrors.aliyun.com/g' /etc/apt/sources.list
# sudo sed -i 's/http:/https:/g' /etc/apt/sources.list
sudo dpkg --add-architecture i386
# Special solution for WSL clock sync issue
if grep -qi microsoft /proc/version; then
  sudo apt-get -o Acquire::Check-Valid-Until=false -o Acquire::Check-Date=false update
else
  sudo apt-get update
fi
yes | sudo apt install gcc-multilib build-essential freeglut3:i386 freeglut3-dev:i386 libglew-dev xorg-dev libopenal1 libopenal-dev make git wget libxmu6:i386 libstdc++5:i386 libopenal-dev:i386
if [ ! -f ./libalut0_1.1.0-6_i386.deb ]; then
wget https://launchpad.net/ubuntu/+archive/primary/+files/libalut0_1.1.0-6_i386.deb
fi
# Trigger a dependency error to let apt fetch the libalut
sudo dpkg -i libalut0_1.1.0-6_i386.deb
yes | sudo apt --fix-broken install
# Ok,it should work well
if [ ! -f ./libalut-dev_1.1.0-6_i386.deb ]; then
wget https://launchpad.net/ubuntu/+archive/primary/+files/libalut-dev_1.1.0-6_i386.deb
fi
sudo dpkg -i libalut-dev_1.1.0-6_i386.deb
if [ ! -f ./multiarch-support_2.23-0ubuntu11.3_i386.deb ]; then
wget http://launchpadlibrarian.net/534757982/multiarch-support_2.23-0ubuntu11.3_i386.deb
fi
sudo dpkg -i multiarch-support_2.23-0ubuntu11.3_i386.deb
sudo usermod -a -G dialout,input $USER
# Just in case
yes | sudo apt --fix-broken install
# cleanup files
rm *.deb