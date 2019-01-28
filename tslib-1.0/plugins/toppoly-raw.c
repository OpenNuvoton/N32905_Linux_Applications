#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "config.h"
#include "tslib-private.h"

//#define CPU_W90P910	//use W90P910 ADC engine
#define CPU_W90P950
//#define DEBUG

#ifdef CPU_W90P910
struct toppoly_ts_event { /* Used in the Toppoly Touch Screen */
	unsigned int x;
	unsigned int y;
	unsigned int status;
};
#endif

#ifdef CPU_W90P950		//use TSC2007 ADC
struct toppoly_ts_event { /* Used in the Toppoly Touch Screen */
	unsigned int x;
	unsigned int y;
	unsigned int z;
};
#endif

static int toppoly_read(struct tslib_module_info *inf, struct ts_sample *samp, int nr)
{
	struct tsdev *ts = inf->dev;
	struct toppoly_ts_event toppoly_evt;
	int ret;
	int total = 0;
	
	//printf("topply read\n");
	//toppoly_evt = alloca(sizeof(*toppoly_evt) * nr);
	ret = read(ts->fd, &toppoly_evt, sizeof(struct toppoly_ts_event));
	if(ret > 0) 
	{
#ifdef CPU_W90P910
		samp->x = (short)toppoly_evt.x;
		samp->y = (short)toppoly_evt.y;
		if(toppoly_evt.status == 1)
			samp->pressure = 1;
		else
			samp->pressure = 0;
#endif
		
#ifdef CPU_W90P950
		samp->x = (short)toppoly_evt.x;
		samp->y = (short)toppoly_evt.y;
		if(toppoly_evt.z <= 100)
			samp->pressure = 0;
		else
			samp->pressure = (short)toppoly_evt.z;		
#endif

#ifdef DEBUG
        fprintf(stderr,"RAW---------------------------> %d %d %d\n",samp->x,samp->y,samp->pressure);
#endif /*DEBUG*/
		gettimeofday(&samp->tv,NULL);			
	} 
	else 
	{
		return -1;
	}

#ifdef CPU_W90P910	
	if(toppoly_evt.status)
		ret = nr;
	else
		ret = 0;
#endif

#ifdef CPU_W90P950
	if(toppoly_evt.z > 100)
		ret = nr;
	else
		ret = 0;
#endif	
	//free(toppoly_evt);
	return ret;
}

static const struct tslib_ops toppoly_ops =
{
	.read	= toppoly_read,
};

struct tslib_module_info r;
TSAPI struct tslib_module_info *mod_init(struct tsdev *dev, const char *params)
{
	struct tslib_module_info *m;
	
	m = &r;
	//m = malloc(sizeof(struct tslib_module_info));
	//if (m == NULL)
	//	return NULL;
	
	m->ops = &toppoly_ops;
	
	//printf("init topply ts module->read:0x%x\n", &toppoly_ops.read);
	//fflush(NULL);
	return m;
}
