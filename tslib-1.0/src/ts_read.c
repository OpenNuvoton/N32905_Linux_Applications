/*
 *  tslib/src/ts_read.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * $Id: ts_read.c,v 1.4 2004/07/21 19:12:59 dlowder Exp $
 *
 * Read raw pressure, x, y, and timestamp from a touchscreen device.
 */
#include "config.h"

#include "tslib-private.h"

#ifdef DEBUG
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#endif

/* This array is used to prevent segfaults and memory overwrites
 * that can occur if multiple events are returned from ts_read_raw
 * for each event returned by ts_read
 */
/* We found this was not needed, and have gone back to the
 * original implementation
 */

//static struct ts_sample ts_read_private_samples[1024];
//char *ptr = NULL;

int ts_read(struct tsdev *ts, struct ts_sample *samp, int nr)
{
	int result;
	int i;

#if 0	
	fprintf(stderr,"ts->list:0x%x[0x%x]\n", (unsigned int)&ts->list, *(int *)ts->list);
	fprintf(stderr,"ts->list->dev:0x%x[0x%x]\n", (unsigned int)&ts->list->dev, *(int *)ts->list->dev);
	fprintf(stderr,"ts->list->next:0x%x[0x%x]\n", (unsigned int)&ts->list->next, *(int *)ts->list->next);
	fprintf(stderr,"ts->list->ops:0x%x[0x%x]\n", (unsigned int)&ts->list->ops, *(int *)ts->list->ops);
	fprintf(stderr,"ts->list->ops->read:0x%x[0x%x]\n", (unsigned int)&ts->list->ops->read, *(int *)ts->list->ops->read);
	fprintf(stderr,"ts->list->ops->fini:0x%x[0x%x]\n", (unsigned int)&ts->list->ops->fini, *(int *)ts->list->ops->fini);
	fprintf(stderr,"ts->list->handle:0x%x[0x%x]\n", (unsigned int)&ts->list->handle, *(int *)ts->list->handle);
	fflush(NULL);
	
	if(ptr == NULL)
		ptr = &ts->list->ops->read;
	printf("ptr=0x%x [0x%x]\n", ptr, *(int *)ptr);
#endif
	
	//result = ts->list->ops->read(ts->list, ts_read_private_samples, nr);
	result = ts->list->ops->read(ts->list, samp, nr);
	
	//for(i=0;i<nr;i++) {
	//	samp[i] = ts_read_private_samples[i];
	//}

#ifdef DEBUG
	if (result)
		fprintf(stderr,"TS_READ----> x = %d, y = %d, pressure = %d\n", samp->x, samp->y, samp->pressure);
#endif

	return result;

}
