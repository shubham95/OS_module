#!/bin/bash
cd ../modules/
make
cd ../test
sudo rmmod hello
sudo insmod ../modules/hello.ko
sudo dmesg -C
#./a.out
sudo cp open.c ../connoisseur_dir420/

sudo rmmod hello
