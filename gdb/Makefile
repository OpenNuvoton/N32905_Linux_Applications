.SUFFIXES : .x .o .c .s

#ROOT = /usr/local/arm_linux_4.2
#INC = $(ROOT)/arm-linux/include
#LIB = $(ROOT)/lib/gcc/arm-linux/4.2.1
#LIB1 = $(ROOT)/arm-linux/lib

CC = arm-linux-gcc -O0 -I$(INC) -g3
WEC_LDFLAGS = -L$(LIB) -L$(LIB1)
STRIP = arm-linux-strip

TARGET = gdb_demo
SRCS = gdb_demo.c
#LIBS = -lgcc
LIBS =

all:
	$(CC) $(WEC_LDFLAGS) $(SRCS) -o $(TARGET) $(LIBS) -static

clean:
	rm -f *.o
	rm -f *.x
	rm -f *.flat
	rm -f *.map
	rm -f temp
	rm -f *.img
	rm -f $(TARGET)	
	rm -f *.gdb
	rm -f *.bak
