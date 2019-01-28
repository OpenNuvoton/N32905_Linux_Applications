
/****************************************************************************
 *                                                                          *
 * Copyright (c) 2010 Nuvoton Technology Corp. All rights reserved.         *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 *
 * FILENAME
 *     lvd_demo.c
 *
 * DESCRIPTION
 *     This program is used to do voltage detection via ADC
 *
 ****************************************************************************/

#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#define TS_DEV		"/dev/input/event0"
#define ADCSYS_FILE	"/sys/devices/platform/w55fa93-adc/adc"
#define LVD_INTERVAL	1
#define BUF_SIZE	8

int main(int argc, char *argv[])
{
	int tsfd, adcfd;
	char buf[BUF_SIZE];

	// if another application had opened touchscreen device, you can skip this step
	if ((tsfd = open(TS_DEV, O_RDONLY)) < 0) {
		printf("open device %s failed, errno = %d\n", TS_DEV, errno);
		return -1;
	}
	// wait for generate first adc data
	usleep(1);

	while (1) {
		memset(&buf[0], 0x0, BUF_SIZE);
		// read voltage
		adcfd = open(ADCSYS_FILE, O_RDWR);
		if (adcfd < 0) {
			printf("open device %s failed, errno = %d\n", ADCSYS_FILE, errno);
			return -1;
		}
		read(adcfd, buf, BUF_SIZE);
		//printf("buf = %x, %x, %x, %x\n", buf[0], buf[1], buf[2], buf[3]);
		printf("voltage = %1.2f\n", (float)(buf[3]*256 + buf[2])/1024.0*9.9);
		close(adcfd);
		sleep(LVD_INTERVAL);
	}

	return 0;
}

