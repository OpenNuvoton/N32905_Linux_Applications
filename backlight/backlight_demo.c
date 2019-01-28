
/****************************************************************************
 *                                                                          *
 * Copyright (c) 2010 Nuvoton Technology Corp. All rights reserved.         *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 *
 * FILENAME
 *     backlight.c
 *
 * DESCRIPTION
 *     This file is a LCD backlight demo program
 *
 ****************************************************************************/

#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/ioctl.h>
#include<pthread.h>
#include<fcntl.h>
#include<linux/input.h>

#define KPD_DEV "/dev/input/event1"
#define FB_DEV "/dev/fb0"

#define IOCTL_LCD_BRIGHTNESS	_IOW('v', 27, unsigned int)	//brightness control

int main(void)
{
	struct input_event data;
	int kpd_fd, fb_fd;
	int brightness = 2000;

	if ((kpd_fd = open(KPD_DEV, O_RDONLY)) < 0) {
		printf("open device %s failed", KPD_DEV);
		return -1;
	}

	if ((fb_fd = open(FB_DEV, O_RDONLY)) < 0) {
		printf("open device %s failed", FB_DEV);
		return -1;
	}

	while (1) {
		read(kpd_fd, &data, sizeof(data));
		//printf("%d %d\n", data.code, data.value);
		if ((data.type == 0) || (data.value <= 0))
			continue;

		if (data.code == 174)
			brightness -= 100;
		else if (data.code == 175)
			brightness += 100;
		else
			continue;

		if (brightness <= 0)
			brightness = 1;
		if (brightness > 2000)
			brightness = 2000;
		printf("brightness = %d\n", brightness);

		ioctl(fb_fd, IOCTL_LCD_BRIGHTNESS, &brightness);
		
		usleep(100000);
	}
}

