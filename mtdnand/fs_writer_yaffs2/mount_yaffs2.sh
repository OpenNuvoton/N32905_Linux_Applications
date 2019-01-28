#!/bin/sh

echo "==== Create YAFFS2 file system to /dev/mtdblock3 by image file ========="

mkdir /mnt/yaffs2
./mtdutils/flash_erase /dev/mtd3 0 0
if ./mtdutils/nandwrite -a -m /dev/mtd3 example_image/foo_yaffs2.img > /dev/null; then
	if mount -t yaffs2 -o"inband-tags" /dev/mtdblock3 /mnt/yaffs2; then
		echo "Success"
		cat /mnt/yaffs2/hi.txt
        echo "==== Done ========================================================="
		exit 0
	fi
fi

echo "==== Fail ========================================================="
