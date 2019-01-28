/* kpd.c
 *
 *
 * Copyright (c)2008 Nuvoton technology corporation
 * http://www.nuvoton.com
 *
 * keypad demo application
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
#include<string.h>
#include<strings.h>
#include <stdlib.h>
#include <errno.h>
#include<fcntl.h>
#include <signal.h>

#include "bma220.h"


#define BMA220_USE_MISC
#define BMA220_USE_INPUT

#ifdef BMA220_USE_MISC
/*
*bma220 testsample
*/
const char * BMA_DEVICE="/dev/bma220";
int fd_dev;

int bma220_dev_process()
{
	int ret=0;
	char buf[32];
	fd_dev = open(BMA_DEVICE, O_RDWR);	
	if (fd_dev < 0) {
		printf("open device %s failed, errno = %d\n", BMA_DEVICE, errno);
		return -1;
	}	

	//BMA220_IOCTL_EVENT_CTRL
	//set no input event
	//param 0=close 1=pen
	buf[0]=0;
	ret=ioctl(fd_dev,BMA220_IOCTL_EVENT_CTRL,buf);
	if(ret<0){
		printf("set BMA220_IOCTL_EVENT_CTRL fail\n");
	}else{
		printf("set BMA220_IOCTL_EVENT_CTRL %d\n",buf[0]);
	}
	
	//BMA220_IOCTL_CALIBRATION 
	ret=ioctl(fd_dev,BMA220_IOCTL_CALIBRATION,buf);
	if(ret<0){
		printf("set BMA220_IOCTL_CALIBRATION fail\n");
	}else{
		printf("set BMA220_IOCTL_CALIBRATION\n");
	}

	//BMA220_SOFT_RESET 
	ret=ioctl(fd_dev,BMA220_SOFT_RESET,buf);
	if(ret<0){
		printf("set BMA220_SOFT_RESET fail\n");
	}else{
		printf("set BMA220_SOFT_RESET\n");
	}	
	
	//BMA220_SET_SUSPEND
	//set suspend mod
	//param 0=suspend device  1=resume device
	buf[0]=1;	
	ret=ioctl(fd_dev,BMA220_SET_SUSPEND,buf);
	if(ret<0){
		printf("set BMA220_SET_SUSPEND fail\n");
	}else{
		printf("set BMA220_SET_SUSPEND %d\n",buf[0]);
	}	

	//BMA220_SET_RANGE
	//param range 0=2g, 1=4g, 2=8g, 3=16g
	buf[0]=0;	
	ret=ioctl(fd_dev,BMA220_SET_RANGE,buf);
	if(ret<0){
		printf("set BMA220_SET_RANGE\n");
	}else{
		printf("set BMA220_SET_RANGE %d\n",buf[0]);
	}	

	//BMA220_GET_RANGE
	buf[0]=0xff;
	ret=ioctl(fd_dev,BMA220_GET_RANGE,buf);
	printf("BMA220_GET_RANGE: range=%d\n",buf[0]);

	//BMA220_SET_BANDWIDTH
	//param bw 0=1kHz, 1=600hz, 2=250Hz, 3=150Hz, 4=75Hz, 5=50Hz.
	buf[0]=2;
	ret=ioctl(fd_dev,BMA220_SET_BANDWIDTH,buf);
	if(ret<0){
		printf("set BMA220_SET_BANDWIDTH fail\n");
	}else{
		printf("set BMA220_SET_BANDWIDTH \n");
	}	

	//BMA220_GET_BANDWIDTH
	buf[0]=0xff;
	ret=ioctl(fd_dev,BMA220_GET_BANDWIDTH,buf);
	printf("BMA220_GET_BANDWIDTH: bandwidth=%d\n",buf[0]);
		
	//BMA220_RESET_INTERRUPT
	ret=ioctl(fd_dev,BMA220_RESET_INTERRUPT,buf);
	if(ret<0){
		printf("set BMA220_RESET_INTERRUPT fail\n");
	}else{
		printf("set BMA220_RESET_INTERRUPT ok\n");
	}	

	//BMA220_SET_OFFSET_RESET
	ret=ioctl(fd_dev,BMA220_SET_OFFSET_RESET,buf);
	if(ret<0){
		printf("set BMA220_SET_OFFSET_RESET fail\n");
	}else{
		printf("set BMA220_SET_OFFSET_RESET ok\n");
	}	

	//BMA220_GET_CHIP_ID
	buf[0]=0xff;
	ret=ioctl(fd_dev,BMA220_GET_CHIP_ID,buf);
	printf("BMA220_GET_CHIP_ID: chipid=%02x\n",buf[0]);
	
	//BMA220_GET_SLEEP_EN
	//param *sleep	0= sleep mode disabled, 1= sleep mode enabled
	buf[0]=0xff;
	ret=ioctl(fd_dev,BMA220_GET_SLEEP_EN,buf);
	printf("BMA220_GET_SLEEP_EN: sleep_en=%d\n",buf[0]);

	//BMA220_GET_SLEEP_DUR
	//param *sleep_dur 0=2ms, 1=10ms, 2=25ms, 3=50ms, 4=100ms, 5=500ms, 6=1000ms, 7=2000ms.
	buf[0]=0xff;
	ret=ioctl(fd_dev,BMA220_GET_SLEEP_DUR,buf);
	printf("BMA220_GET_SLEEP_DUR: sleep_dur=%d\n",buf[0]);	

	bma220acc_t acc;
	while(1){
		memset(&acc,0,sizeof(acc));
		ret=read(fd_dev, &acc, sizeof(acc));
		if(ret>0)
			printf("acc:x=%d y=%d z=%d\n",acc.x,acc.y,acc.z);
		else
			printf("acc read errno=%d\n",ret);
			
		usleep(200*1000);
	}

	close(fd_dev);
	return 0;
	
}
#else
int bma220_dev_process(){}
#endif

#ifdef BMA220_USE_INPUT
/*
*bma220 test sample
*/
const char * BMA_DELAY_SYS="/sys/class/input/input2/delay";
const char * BMA_ENABLE_SYS="/sys/class/input/input2/enable";
const char * BMA_WAKE_SYS="/sys/class/input/input2/wake";
const char * BMA_DATA_SYS="/sys/class/input/input2/data";
const char * BMA_STATUS_SYS="/sys/class/input/input2/status";

int bma220_get_delay(void)
{
	int fd_delay;
	int ret,delay=0;
	char buf[64];
	
	fd_delay=open(BMA_DELAY_SYS,O_RDWR);
	if(fd_delay<0){
		printf("open device %s failed, errno = %d\n", BMA_DELAY_SYS, errno);
		return -1;
	}	

	memset(buf,0,sizeof(buf));
	ret=read(fd_delay,buf,16);
	if(ret<0){
		printf("bma_sys_stat:read delay error\n");
		close(fd_delay);
		return -1;
	}		
	close(fd_delay);	
	
	delay=strtoul(buf,NULL,10);
	return delay;
}


int bma220_set_delay(int delay)
{
	int fd_delay;
	int ret;
	char buf[4096];

	fd_delay=open(BMA_DELAY_SYS,O_RDWR|O_NONBLOCK);
	if(fd_delay<0){
		printf("open device %s failed, errno = %d\n", BMA_DELAY_SYS, errno);
		return -1;
	}	

	//ret=read(fd_delay,buf,16);
	//if(ret<0){
	//	printf("bma_sys_stat:read delay error\n");
	//	close(fd_delay);
	//	return -1;
	//}			
	
	sprintf(buf,"%d",delay);	
	ret=write(fd_delay,buf,strlen(buf));
	if(ret<0){
		printf("bma_sys_stat:write delay error\n");
		close(fd_delay);
		return -1;
	}			
	close(fd_delay);
	
	return 0;
}

int bma220_get_enable(void)
{
	int fd_enable;
	int ret,enable=0,val=0;
	char buf[64];
	
	fd_enable=open(BMA_ENABLE_SYS,O_RDWR);
	if(fd_enable<0){
		printf("open device %s failed, errno = %d\n", BMA_ENABLE_SYS, errno);
		return -1;
	}	

	memset(buf,0,sizeof(buf));
	ret=read(fd_enable,buf,16);
	if(ret<0){
		printf("bma_sys_stat:read enable error\n");
		close(fd_enable);
		return -1;
	}		
	close(fd_enable);	

	val=strtoul(buf,NULL,10);
	return val;
}

int bma220_set_enable(int enable)
{
	int fd_enable;
	int ret;
	char buf[64];
	
	fd_enable=open(BMA_ENABLE_SYS,O_RDWR);
	if(fd_enable<0){
		printf("open device %s failed, errno = %d\n", BMA_ENABLE_SYS, errno);
		return -1;
	}	

	if(enable>0)
		sprintf(buf,"%d",enable);
	else
		sprintf(buf,"%d",enable);
	ret=write(fd_enable,buf,strlen(buf));
	if(ret<0){
		printf("bma_sys_stat:read enable error\n");
		close(fd_enable);
		return -1;
	}		
	close(fd_enable);	
	
	return 0;
}

int bma220_get_data(int *x,int *y,int *z)
{
	int fd_data;
	int ret=0;
	char buf[64];
	
	fd_data=open(BMA_DATA_SYS,O_RDONLY);
	if(fd_data<0){
		printf("open device %s failed, errno = %d\n", BMA_DATA_SYS, errno);
		return -1;
	}	

	memset(buf,0,sizeof(buf));
	ret=read(fd_data,buf,64);
	if(ret<0){
		printf("bma_sys_stat:read data error\n");
		close(fd_data);
		return -1;
	}			
	close(fd_data);	

	sscanf(buf,"%d %d %d",x,y,z);
	return 0;
}

/*
*bma220 test sample
*/
const char * BMA_EVENT="/dev/input/event2";
//const char * BMA_EVENT="/dev/input/event0";

int fd_event=NULL;

int bma220_input_process(void)
{
	char temp[64];
	int ret=0,delay,enable;
	int x,y,z;
	struct input_event data;

	delay=bma220_get_delay();
	enable=bma220_get_enable();
	printf("bma220_input_process before:delay=%d enable=%d\n",delay,enable);

	bma220_set_delay(100);
	bma220_set_enable(1);

	delay=bma220_get_delay();
	enable=bma220_get_enable();
	printf("bma220_input_process after:delay=%d enable=%d\n",delay,enable);

	ret=bma220_get_data(&x,&y,&z);
	if(ret>0){
		printf("x=%d y=%d z=%d\n",x,y,z);
	}

#if 1
	//
	//get data
	//
	fd_event = open(BMA_EVENT, O_RDONLY);	
	if (fd_event < 0) {
		printf("open device %s failed, errno = %d\n", BMA_EVENT, errno);
		return -1;
	}	

	printf("open device %s success\n",BMA_EVENT);
	while(1){
    		ret=read(fd_event,&data,sizeof(data));
		
    		if (data.type == EV_ABS) {
	      		if(data.code==ABS_X) 
				printf("ABS_X=%d ",data.value);
	      		else if(data.code==ABS_Y) 
				printf("ABS_Y=%d ",data.value);
	      		else if(data.code==ABS_Z) 
				printf("ABS_Z=%d ",data.value);
	      		else if(data.code==ABS_PRESSURE)
	        		printf("ABS_PRESSURE=%d\n",data.value);
	      		else if(data.code==ABS_BRAKE)
	        		printf("ABS_BRAKE:ABS_STATUS=%d\n",data.value);
	      		else if(data.code==ABS_MISC)
	        		printf("ABS_MISC:ABS_WAKE=%d\n",data.value);
	      		else if(data.code==ABS_THROTTLE)
	        		printf("ABS_THROTTLE:ABS_CONTROL_REPORT=%d\n",data.value);
				
		}else if(data.type == EV_SYN){
			printf("\n%d.%06d ", data.time.tv_sec, data.time.tv_usec);
			printf("EV_SYNC\n");
		}	

		//if(data.type != 0){
		//        printf("\n%d.%06d ", data.time.tv_sec, data.time.tv_usec);
		//        printf("%d %d %d\n", data.type, data.code, data.value);
		//}		
	}
	close(fd_event);
	
#endif

	return 0;
}
#endif


/*
*bma220 put file sample
*/
#define LOG_FILE_NAME  "./msensor.log"
#define GRAVITY_EARTH			9806550
int  put_stream(char *buf)
{
	FILE* stream=NULL;
	if(buf==NULL) 
		return -1;
	//sprintf(temp,"./msensor-%d.log",rand()%1024);
	if((stream=fopen(LOG_FILE_NAME,"a+t"))==NULL){
		printf("open %s fail\n",LOG_FILE_NAME);
		return ;
	}
	fprintf(stream,"%s",buf);
	fclose(stream);
	return 1;
}

int  bma220_put_file(char argc ,char *argv[])
{
	char temp[512];
	int ret=0,delay,enable,index=0;
	int x=0,y=0,z=0;
	struct input_event data;

	delay=bma220_get_delay();
	enable=bma220_get_enable();
	printf("bma220_input_process before:delay=%d enable=%d\n",delay,enable);

	if(argc>=2){
		delay=strtoul(argv[1],NULL,10);
	}else{
		delay=100;
	}	
	bma220_set_delay(delay);
	bma220_set_enable(1);

	delay=bma220_get_delay();
	enable=bma220_get_enable();
	printf("bma220_input_process after:delay=%d enable=%d\n",delay,enable);
	sprintf(temp,"\nbma220_input_process after:delay=%d enable=%d\n",delay,enable);
	put_stream(temp);
	
	ret=bma220_get_data(&x,&y,&z);
	if(ret>0){
		printf("x=%d y=%d z=%d\n",x,y,z);
	}

	//
	//get data
	//
	fd_event = open(BMA_EVENT, O_RDONLY);	
	if (fd_event < 0) {
		printf("open device %s failed, errno = %d\n", BMA_EVENT, errno);
		return -1;
	}	

	printf("open device %s success\n",BMA_EVENT);
	while(1){
    		ret=read(fd_event,&data,sizeof(data));
		
    		if (data.type == EV_ABS) {
	      		if(data.code==ABS_X) 
				x=data.value;
	      		else if(data.code==ABS_Y) 
				y=data.value;
	      		else if(data.code==ABS_Z) 
				z=data.value;				
		}else if(data.type == EV_SYN){
			sprintf(temp,"%10d %10d %10d %10d.%06d %6d\n",x,y,z,data.time.tv_sec, data.time.tv_usec,index++);
			put_stream(temp);
			printf("%s",temp);
		}	
	}
	close(fd_event);
	return 0;
}


