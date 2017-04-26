#!/bin/bash

###### defines ######

local_dir=$PWD

###### defines ######
if [[ $1 = -a ]]; then
echo '#############'
echo 'making clean'
echo '#############'
make clean
make mrproper
echo '#############'
echo 'making defconfig'
echo '#############'
# make ARCH=arm64 USBhost_defconfig
make ARCH=arm64 flounder_defconfig
# make ARCH=arm64 tegra13_android_defconfig
# make ARCH=arm64 msm-perf_defconfig
# make ARCH=arm64 USBhost_defconfig
echo '#############'
echo 'making zImage'
echo '#############'
#time make ARCH=arm64 CROSS_COMPILE=~/cm13/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-gnu-5.3/bin/aarch64- -j4
#time make ARCH=arm64 SUBARCH=arm64 CROSS_COMPILE=~/cm13/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-gnu-linaro-5.3.1/bin/aarch64-linux-gnu- -j4 2>&1 | tee build.log
#time make ARCH=arm64 CROSS_COMPILE=~/cm13/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-gnu-6.1.1/bin/aarch64-linux-gnu- -j4 2>&1 | tee build.log
time make ARCH=arm64 CROSS_COMPILE=~/cm13/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/aarch64-linux-android- -j4 2>&1 | tee build49.log
#time make ARCH=arm CROSS_COMPILE=~/cm13/prebuilts/gcc/linux-x86/arm/arm-eabi-4.8/bin/arm-eabi- -j8
else
echo '#############'
fi
echo '#############'
echo 'Making anykernel zip'
echo '#############'
echo ''
if [ -a arch/arm64/boot/Image.gz ]; then 
echo '#############'
echo 'copying files to ./out'
echo '#############'
echo ''
mkdir out
cp arch/arm64/boot/Image.gz-dtb out/Image.gz-dtb
echo 'done'
if [[ $2 = -st ]]; then
cp out/Image.gz-dtb workingst/Image.gz-dtb
cd workingst
echo 'Run pack_st'
. ./pack_cwm.sh
fi
if [[ $2 = -tn8 ]]; then
cp out/Imagetn8.gz-dtb workingtn8/Image.gz-dtb
cd workingtn8
echo 'Run pack_tn8'
. ./pack_cwm.sh
fi
if [[ $2 = -flo ]]; then
cp out/Image.gz-dtb workingflo/Image.gz-dtb
cd workingflo
echo ''
echo '#############'
echo 'Run pack_flo'
. ./pack_cwm.sh
fi
cd $local_dir
echo ''
echo '#############'
echo 'build finished successfully'
echo '#############'
else
echo '#############'
echo 'build failed!'
echo '#############'
fi
