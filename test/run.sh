#!/bin/bash
cd ../modules2/
make
cd ../test
sudo rmmod hello
sudo insmod ../modules2/hello.ko
sudo dmesg -C
./a.out
#sudo cp open.c ../connoisseur_dir420/

sudo rmmod hello