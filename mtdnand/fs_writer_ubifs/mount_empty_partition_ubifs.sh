#!/bin/sh

PAGESIZE=2048
PartitionSize=13MiB

echo "==== Create empty UBIFS file system on /dev/mtd3 ==============="

./mtdutils/flash_erase /dev/mtd3 0 0
echo "mtdutils/ubiformat"                             
if ./mtdutils/ubiformat /dev/mtd3 -O $PAGESIZE -s $PAGESIZE > /dev/null 2>&1; then
    echo "mtdutils/ubiattach"
    if ./mtdutils/ubiattach /dev/ubi_ctrl -m 3 > /dev/null 2>&1; then
        echo "mtdutils/ubimkvol"
        if ./mtdutils/ubimkvol /dev/ubi1 -N foo -m  > /dev/null 2>&1; then
            if [ ! -d "/mnt/ubifs3" ]; then
                mkdir /mnt/ubifs3
            fi
            if mount -t ubifs ubi1:foo /mnt/ubifs3 > /dev/null 2>&1; then
                echo "Success!!"
                echo "==== Done ======================================================"
                exit 0
            fi
        fi
    fi
fi

./mtdutils/ubidetach -m 3

echo "==== Fail ======================================================"
