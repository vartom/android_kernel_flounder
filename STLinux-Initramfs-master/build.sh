#!/bin/bash

KERNEL_PATH=../STLinux-Kernel

cd root
find . | cpio -o -H newc | gzip > ../initramfs.cpio.gz
cd ..

rm -f new_boot.img
tools/mkbootimg --kernel Image.gz-dtb --ramdisk initramfs.cpio.gz -o new_boot.img --cmdline "console=tty1 fbcon=rotate:2"

rm initramfs.cpio.gz
#rm zImage
