#!/bin/sh

#An example of making ubifs image

if [ ! -d ./foo ]; then
	mkdir -p ./foo
fi

echo "hello world" > ./foo/helloworld.txt

# Make an UBIFS image 2K
echo "./mkfs.ubifs -r ./foo -m 2048 -e 126976 -c 992 -o ./foo_ubifs"
./mkfs.ubifs -r ./foo -m 2048 -e 126976 -c 992 -o ./foo_ubifs

echo "[ubifs]" > ./ubifs.cfg
echo "mode=ubi" >> ./ubifs.cfg
echo "vol_id=0" >> ./ubifs.cfg
echo "vol_type=dynamic" >> ./ubifs.cfg
echo "vol_alignment=1" >> ./ubifs.cfg
echo "vol_name=nand1-2" >> ./ubifs.cfg
echo "vol_flags=autoresize" >> ./ubifs.cfg
echo "image=foo_ubifs" >> ./ubifs.cfg

# Notice vol_size value < mtd partition size 
#echo "vol_size=60MiB" >> ./ubifs.cfg
 
echo "./ubinize -o ./foo_ubifs.img -m 2048 -p 128KiB -s 2048 ./ubifs.cfg"
./ubinize -o ./foo_ubifs.img -m 2048 -p 128KiB -s 2048 ./ubifs.cfg
