/* motion.c
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

#include "motion.h"

#undef kprint 
//#define kprint printf
#define kprint(...) 

//global define
#define MAX_CALLBACK_NUM	5
#define MOTION_HISTORY_DEPTH	5
#define GRAVITY_EARTH			9806550
#define GRAVITY_2G			19613100
#define MOTION_SLOW_THRESHOLD	GRAVITY_EARTH*0.8	
#define MOTION_COLLISION_THRESHOLD GRAVITY_EARTH*1.8
#define MOTION_GZ0_THRESHOLD	GRAVITY_2G+GRAVITY_EARTH*0.8
#define MOTION_GZ180_THRESHOLD	GRAVITY_2G-GRAVITY_EARTH*0.8

#define MOIION_GETDATA_DELAY  50		//50ms

static pthread_t g_motion_handle;
static pthread_mutex_t g_motion_mutex;

static int fd_event=NULL;
static MOTION_ACC g_acc={0,0,0,0};
static MOTION_ACTION g_action=MOTION_ACTION_NULL;
static MOTION_ACTION g_action_pre=MOTION_ACTION_NULL;
static MOTION_ACTION g_angle=MOTION_ACTION_NULL;
static MOTION_ACTION g_angle_pre=MOTION_ACTION_NULL;
static MOTION_CALLBACK callback[MAX_CALLBACK_NUM]={{MOTION_ACTION_NULL,NULL}};
static MOTION_ACC g_acc_history[MOTION_HISTORY_DEPTH]={0,0,0,0};

void motion_thread(void);


/*
*bma220 test sample
*/
const char * MOTION_DELAY_SYS="/sys/class/input/input2/delay";
const char * MOTION_ENABLE_SYS="/sys/class/input/input2/enable";
const char * MOTION_WAKE_SYS="/sys/class/input/input2/wake";
const char * MOTION_DATA_SYS="/sys/class/input/input2/data";
const char * MOTION_STATUS_SYS="/sys/class/input/input2/status";
const char * MOTION_EVENT="/dev/input/event2";

static int bma220_get_delay(void)
{
	int fd_delay;
	int ret,delay=0;
	char buf[64];
	
	fd_delay=open(MOTION_DELAY_SYS,O_RDWR);
	if(fd_delay<0){
		printf("open device %s failed, errno = %d\n", MOTION_DELAY_SYS, errno);
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


static int bma220_set_delay(int delay)
{
	int fd_delay;
	int ret;
	char buf[4096];

	fd_delay=open(MOTION_DELAY_SYS,O_RDWR|O_NONBLOCK);
	if(fd_delay<0){
		printf("open device %s failed, errno = %d\n", MOTION_DELAY_SYS, errno);
		return -1;
	}	
	
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

static int bma220_get_enable(void)
{
	int fd_enable;
	int ret,enable=0,val=0;
	char buf[64];
	
	fd_enable=open(MOTION_ENABLE_SYS,O_RDWR);
	if(fd_enable<0){
		printf("open device %s failed, errno = %d\n", MOTION_ENABLE_SYS, errno);
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

static int bma220_set_enable(int enable)
{
	int fd_enable;
	int ret;
	char buf[64];
	
	fd_enable=open(MOTION_ENABLE_SYS,O_RDWR);
	if(fd_enable<0){
		printf("open device %s failed, errno = %d\n", MOTION_ENABLE_SYS, errno);
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

static int bma220_get_data(int *x,int *y,int *z)
{
	int fd_data;
	int ret=0;
	char buf[64];
	
	fd_data=open(MOTION_DATA_SYS,O_RDONLY);
	if(fd_data<0){
		printf("open device %s failed, errno = %d\n", MOTION_DATA_SYS, errno);
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


#if 0
/*
*bma220 test sample
*/
const char * MOTION_EVENT="/dev/input/event2";

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
	fd_event = open(MOTION_EVENT, O_RDONLY);	
	if (fd_event < 0) {
		printf("open device %s failed, errno = %d\n", MOTION_EVENT, errno);
		return -1;
	}	

	printf("open device %s success\n",MOTION_EVENT);
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


//functions define	
int motion_service_start(void)
{
    	pthread_mutex_init(&g_motion_mutex,NULL);
    	pthread_create(&g_motion_handle,NULL,(void*)&motion_thread,NULL);
	return 1;
}

int motion_service_stop(void)
{
	pthread_cancel(g_motion_handle);
	return 1;
}

int motion_suspend(void)
{
	pthread_mutex_lock(&g_motion_mutex);
	bma220_set_enable(0);
	pthread_mutex_unlock(&g_motion_mutex);
	
	return 1;
}

int motion_resume(void)
{
	pthread_mutex_lock(&g_motion_mutex);
	bma220_set_enable(1);
	pthread_mutex_unlock(&g_motion_mutex);
	
	return 1;
}

int motion_set_delay(int delay)
{
	pthread_mutex_lock(&g_motion_mutex);
	bma220_set_delay(delay);
	pthread_mutex_unlock(&g_motion_mutex);
	
	return 1;
}

int motion_register_action(MOTION_ACTION action,void *pfnCallBack)
{
	int index=-1;
	int i=0;
	for(i=0;i<MAX_CALLBACK_NUM;i++){
		if(callback[i].action==MOTION_ACTION_NULL){
			index=i;
			break;
		}
	}
	if(index==-1) {
		printf("motion_register_action:cannot find nouse index\n");
		return -1;
	}

	pthread_mutex_lock(&g_motion_mutex);
	callback[index].action=action;
	callback[index].pfnCallBack=(void*)pfnCallBack;
	pthread_mutex_unlock(&g_motion_mutex);

	kprint("motion_register_action:action=%d index=%d\n",action,index);
	
	return index;
}

int motion_unregister_action(int index)
{
	if(index>=MAX_CALLBACK_NUM){
		printf("unvalible parapam:index>=MAX_CALLBACK_NUM\n");
		return -1;
	}		

	pthread_mutex_lock(&g_motion_mutex);
	callback[index].action=MOTION_ACTION_NULL;
	callback[index].pfnCallBack=NULL;
	pthread_mutex_unlock(&g_motion_mutex);

	kprint("motion_unregister_action:index=%d\n",index);
	
	return 1;
}

static int motion_slow_detect()
{
	int x;
	if(g_acc_history[MOTION_HISTORY_DEPTH-2].time==0)
		return MOTION_ACTION_NULL;

	if(g_acc_history[MOTION_HISTORY_DEPTH-1].x>g_acc_history[MOTION_HISTORY_DEPTH-2].x){
		x=g_acc_history[MOTION_HISTORY_DEPTH-1].x-g_acc_history[MOTION_HISTORY_DEPTH-2].x;
		if(x>MOTION_SLOW_THRESHOLD)
			return MOTION_ACTION_SLOW_UP;
	}
	else{
		x=g_acc_history[MOTION_HISTORY_DEPTH-2].x-g_acc_history[MOTION_HISTORY_DEPTH-1].x;
		if(x>MOTION_SLOW_THRESHOLD)
			return MOTION_ACTION_SLOW_DOWN;
	}
			
	return MOTION_ACTION_NULL;
}


static int motion_collision_detect()
{
	int x,y,z;
	if(g_acc_history[MOTION_HISTORY_DEPTH-2].time==0)
		return MOTION_ACTION_NULL;

	if(g_acc_history[MOTION_HISTORY_DEPTH-1].x>g_acc_history[MOTION_HISTORY_DEPTH-2].x)
		x=g_acc_history[MOTION_HISTORY_DEPTH-1].x-g_acc_history[MOTION_HISTORY_DEPTH-2].x;
	else
		x=g_acc_history[MOTION_HISTORY_DEPTH-2].x-g_acc_history[MOTION_HISTORY_DEPTH-1].x;

	if(g_acc_history[MOTION_HISTORY_DEPTH-1].y>g_acc_history[MOTION_HISTORY_DEPTH-2].y)
		y=g_acc_history[MOTION_HISTORY_DEPTH-1].y-g_acc_history[MOTION_HISTORY_DEPTH-2].y;
	else
		y=g_acc_history[MOTION_HISTORY_DEPTH-2].y-g_acc_history[MOTION_HISTORY_DEPTH-1].y;

	if(g_acc_history[MOTION_HISTORY_DEPTH-1].z>g_acc_history[MOTION_HISTORY_DEPTH-2].z)
		z=g_acc_history[MOTION_HISTORY_DEPTH-1].z-g_acc_history[MOTION_HISTORY_DEPTH-2].z;
	else
		z=g_acc_history[MOTION_HISTORY_DEPTH-2].z-g_acc_history[MOTION_HISTORY_DEPTH-1].z;
	
	if(abs(x)>MOTION_COLLISION_THRESHOLD)
		return MOTION_ACTION_COLLISION;
	
	if(abs(y)>MOTION_COLLISION_THRESHOLD)
		return MOTION_ACTION_COLLISION;

	if(abs(z)>MOTION_COLLISION_THRESHOLD)
		return MOTION_ACTION_COLLISION;
		
	return MOTION_ACTION_NULL;
}


static int motion_angle_detect(void)
{
	int z;
	if(g_acc_history[MOTION_HISTORY_DEPTH-1].time==0)
		return MOTION_ACTION_NULL;
	
	z=g_acc_history[MOTION_HISTORY_DEPTH-1].z;

	if(z>MOTION_GZ0_THRESHOLD){
		g_action=MOTION_ACTION_GZ0;	
		if(g_action!=g_action_pre){
			g_action_pre=g_action;
			return g_action;
		}		
	}
	
	if(z<MOTION_GZ180_THRESHOLD){
		g_action=MOTION_ACTION_GZ180;
		if(g_action!=g_action_pre){
			g_action_pre=g_action;
			return g_action;
		}		
	}		
		
	return MOTION_ACTION_NULL;
}


void motion_thread(void)
{
	int ret=0,delay,enable;
	struct input_event data;
	unsigned long time_cur=0,time_pre=0;
	int i=0;

	//
	//get data
	//
	fd_event = open(MOTION_EVENT, O_RDONLY);	
	if (fd_event < 0) {
		printf("open device %s failed, errno = %d\n", MOTION_EVENT, errno);
		return ;
	}	
	kprint("open device %s success\n",MOTION_EVENT);

	delay=bma220_get_delay();
	enable=bma220_get_enable();
	printf("motion_thread before:delay=%d enable=%d\n",delay,enable);

	bma220_set_delay(MOIION_GETDATA_DELAY);
	bma220_set_enable(1);

	delay=bma220_get_delay();
	enable=bma220_get_enable();
	printf("motion_thread after:delay=%d enable=%d\n",delay,enable);

	//read first data
	ret=bma220_get_data(&g_acc.x,&g_acc.y,&g_acc.z);
	if(ret!=-1){
		g_acc.x+=GRAVITY_2G;
		g_acc.y+=GRAVITY_2G;
		g_acc.z+=GRAVITY_2G;
		g_acc.time=1;
		printf("x=%d y=%d z=%d\n",g_acc.x,g_acc.y,g_acc.z);
	}

	while(1){
		pthread_mutex_lock(&g_motion_mutex);
    		ret=read(fd_event,&data,sizeof(data));
		
    		if (data.type == EV_ABS) {
	      		if(data.code==ABS_X) 
				g_acc.x=data.value+GRAVITY_2G;
	      		else if(data.code==ABS_Y) 
				g_acc.y=data.value+GRAVITY_2G;
	      		else if(data.code==ABS_Z) 
				g_acc.z=data.value+GRAVITY_2G;						
		}else if(data.type == EV_SYN){
			time_cur=data.time.tv_sec*1000+data.time.tv_usec/1000;
			g_acc.time=time_cur-time_pre;
			time_pre=time_cur;
		}

		if(data.type != EV_SYN){
			pthread_mutex_unlock(&g_motion_mutex);
			continue;
		}
		
		kprint("x=%8d y=%8d z=%8d t=%d\n",g_acc.x,g_acc.y,g_acc.z,g_acc.time);

		//copy to history
		memcpy(&g_acc_history[0],&g_acc_history[1],sizeof(MOTION_ACC)*(MOTION_HISTORY_DEPTH-1));
		memcpy(&g_acc_history[MOTION_HISTORY_DEPTH-1],&g_acc,sizeof(MOTION_ACC));

		//MOTION_ACTION_SLOW_UP
		g_action=motion_slow_detect();
		if(g_action!=MOTION_ACTION_NULL){
			kprint("action =%d detect\n",g_action);
			for(i=0;i<MAX_CALLBACK_NUM;i++){
				if(callback[i].action==g_action&&callback[i].pfnCallBack!=NULL)
					callback[i].pfnCallBack(g_action);
			}			
		}

		//MOTION_ACTION_COLLISION
		g_action=motion_collision_detect();
		if(g_action!=MOTION_ACTION_NULL){
			kprint("action =%d detect\n",g_action);
			for(i=0;i<MAX_CALLBACK_NUM;i++){
				if(callback[i].action==g_action&&callback[i].pfnCallBack!=NULL)
					callback[i].pfnCallBack(g_action);
			}			
		}

		//MOTION_ANGLE
		g_angle=motion_angle_detect();
		if(g_angle!=MOTION_ACTION_NULL){
			kprint("angle =%d detect\n",g_angle);
			for(i=0;i<MAX_CALLBACK_NUM;i++){
				if(callback[i].action==g_action&&callback[i].pfnCallBack!=NULL)
					callback[i].pfnCallBack(g_action);
			}			
		}			

		pthread_mutex_unlock(&g_motion_mutex);
	}
	
	close(fd_event);
	return ;
}




