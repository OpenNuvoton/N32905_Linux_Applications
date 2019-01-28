/* tsdemo.c
 *
 *
 * Copyright (c)2008 Nuvoton technology corporation
 * http://www.nuvoton.com
 *
 * Touch screen demo application
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */
#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/ioctl.h>
#include<pthread.h>
#include<fcntl.h>
#include<linux/input.h>

#define TS_DEV "/dev/input/event0"
static int ts_fd= -1;
static int init_device(void)
{
  if ((ts_fd=open(TS_DEV,O_RDONLY))<0){
    printf("can not open!!!");
    return -1;
  }
  return 0;
}

int main(void)
{

  int i;
  struct input_event data;
  if(init_device()<0)
    {
      printf("init error\n");
      return -1;
    }
  for(;;){
    read(ts_fd,&data,sizeof(data));
    if (data.type == EV_ABS) {
      if(data.code==ABS_X) 
	printf("ABS_X=%d\n",data.value);
      else if(data.code==ABS_Y) 
	printf("\t\tABS_Y=%d\n",data.value);
      else if(data.code==ABS_PRESSURE)
        printf("\t\t\t\tABS_P=%d\n",data.value);

    }
    
  }
}
