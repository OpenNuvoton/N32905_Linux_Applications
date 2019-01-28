#!/bin/sh

PAGESIZE=2048
UBIFSIMGPATH=./example_image/foo_ubifs.img

echo "==== Create UBIFS file system to /dev/mtd2 by image file ========="

echo "mtdutils/flash_erase"
./mtdutils/flash_erase /dev/mtd2 0 0
echo "mtdutils/ubiformat"
if ./mtdutils/ubiformat /dev/mtd2 -O $PAGESIZE -s $PAGESIZE -f $UBIFSIMGPATH > /dev/null 2>&1; then
    echo "mtdutils/ubiattach"
    if ./mtdutils/ubiattach /dev/ubi_ctrl -O 2048 -m 2 > /dev/null 2>&1; then
        mkdir -p /mnt/ubifs
        echo "mount"
        if mount -t ubifs ubi0_0 /mnt/ubifs > /dev/null 2>&1; then
            echo "Success!!"
            cat /mnt/ubifs/helloworld.txt
            echo "==== Done ========================================================"
            exit 0
        fi
    fi
fi

./mtdutils/ubidetach -m 2

echo "==== Fail ========================================================"
