.SUFFIXES : .x .o .cpp .s



ROOT = /usr/local/arm_linux_4.2/
LIB = $(ROOT)/lib/gcc/arm-linux/4.2.1
LIB1 =$(ROOT)/arm-linux/lib
INCSYS:=$(ROOT)/arm-linux/sys-include
INC :=$(ROOT)/arm-linux/include -I../lib


CC=arm-linux-gcc -O2 -I$(INC) -I$(INCSYS) -static 



TARGET=avi_player


all:  avi_player.o 
	$(CC) -o $(TARGET) avi_player.o -L. -lmfl -ladpcm -lm -lgcc -lc 
	arm-linux-strip $(TARGET)



clean:
	rm -rf *.o 
	rm -rf $(TARGET)
	rm -rf *.gdb

