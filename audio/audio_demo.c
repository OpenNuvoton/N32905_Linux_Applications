/* audio.c
 *
 *
 * Copyright (c)2008 Nuvoton technology corporation
 * http://www.nuvoton.com
 *
 * Audio playback demo application
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/soundcard.h>
#include <sys/poll.h>
#include <pthread.h>
#include <math.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>

char *pcmfiles[] = {
		"./8k.pcm",
		"./11.025k.pcm",
		"./16k.pcm",
		"./22.05k.pcm",
		"./24k.pcm",
		"./32k.pcm",
		"./44.1k.pcm",
		"./48k.pcm"};
		
const int samplerate[] = {
		8000,
		11025,
		16000,
		22050,
		24000,
		32000,
		44100,
		48000};
		
int p_dsp, p_mixer;
int r_dsp, r_mixer;
static int rec_stop = 0;

static volatile int rec_volume, play_volume;
int close_audio_play_device()
{
 
  close(p_dsp);
  close(p_mixer);
	
  return 0;	
}

int open_audio_play_device()
{
printf("*****\n");
	p_dsp = open("/dev/dsp", O_RDWR);
	if( p_dsp < 0 ){
		printf("Open dsp error\n");
		return -1;
	}
printf("====\n");	
	p_mixer = open("/dev/mixer", O_RDWR);
	if( p_mixer < 0 ){
		printf("Open mixer error\n");
		return -1;
	}
}

int close_audio_rec_device()
{
 
  close(r_dsp);
  close(r_mixer);
	
  return 0;	
}

int open_audio_rec_device()
{
	r_dsp = open("/dev/dsp1", O_RDWR);
	if( r_dsp < 0 ){
		printf("Open dsp error\n");
		return -1;
	}
//	printf("Open dsp OK\n");	
	r_mixer = open("/dev/mixer1", O_RDWR);

	if( r_mixer < 0 ){
		printf("Open mixer error\n");
		return -1;
	}
//	printf("Open mixer OK\n");		
}

int record_to_pcm(char *file, int samplerate, int loop)
{
	int i, status = 0, frag;
	int channel = 2, volume = 0x6464;
	FILE *fd;
	char *buffer;
	struct timeval time;
	struct timezone timezone;
	int nowsec;
	
	open_audio_rec_device();
	fd = fopen(file, "w+");
	if(fd == NULL)
	{	
		printf("open %s error!\n", file);
		return 0;
	}
		
	printf("open %s OK!\n", file);		
	
//	ioctl(r_mixer, MIXER_WRITE(SOUND_MIXER_MIC), &volume);	/* set MIC max volume */
	ioctl(r_mixer, MIXER_WRITE(SOUND_MIXER_MIC), &rec_volume);	/* set MIC max volume */
	
	status = ioctl(r_dsp, SNDCTL_DSP_SPEED, &samplerate);
	if(status<0)
	{
		fclose(fd);
		printf("Sample rate not support\n");
		return -1;
	}	
	ioctl(r_dsp, SNDCTL_DSP_CHANNELS, &channel);
	ioctl(r_dsp, SNDCTL_DSP_GETBLKSIZE, &frag);
	
//	printf("volume=%d\n", volume);	
//	printf("volume=%d\n", rec_volume);	
	printf("samplerate=%d\n", samplerate);	
	printf("channel=%d\n", channel);				
	printf("frag=%d\n", frag);		
	
	buffer = (char *)malloc(frag);
	
	gettimeofday(&time, &timezone);
	nowsec = time.tv_sec;
	
	printf("begin recording!\n");					
	
	for(i=0;i<loop;i++)
	{
		int size=0;
		do
		{		
			size = read(r_dsp, buffer, frag);	/* recording */
		}while(size<=0);		
		fwrite(buffer, 1, frag, fd);		
	}		

	printf("----\n");	
	printf("record exit!!\n");
	close_audio_rec_device();		
	fclose(fd);
	free(buffer);
	
	return 0;
}

int record_to_pcm_single(char *file, int samplerate, int loop)
{
	int i, status = 0, frag;
	int channel = 1, volume = 0x6464;
	FILE *fd;
	char *buffer;
	struct timeval time;
	struct timezone timezone;
	int nowsec;
	
	open_audio_rec_device();
	fd = fopen(file, "w+");
	if(fd == NULL)
	{	
		printf("open %s error!\n", file);
		return 0;
	}
		
	printf("open %s OK!\n", file);		
	ioctl(r_mixer, MIXER_WRITE(SOUND_MIXER_MIC), &rec_volume);	/* set MIC max volume. Only support from external  DAC*/	
	status = ioctl(r_dsp, SNDCTL_DSP_SPEED, &samplerate);
	if(status<0)
	{
		fclose(fd);
		printf("Sample rate not support\n");
		return -1;
	}	
	ioctl(r_dsp, SNDCTL_DSP_CHANNELS, &channel);
	ioctl(r_dsp, SNDCTL_DSP_GETBLKSIZE, &frag);
	
	fcntl(r_dsp, F_SETFL, O_NONBLOCK);				/* Set to nonblock mode */

	/* fragments, 2^12 = 4096 bytes */
	frag = 0x2000c;	
	ioctl(r_dsp, SNDCTL_DSP_SETFRAGMENT, &frag);
	ioctl(r_dsp, SNDCTL_DSP_GETBLKSIZE, &frag);	

	loop *= 2;		/*  (one channel) * (16-bits per sdample)/frag) */
	loop *= samplerate;
	printf("pow(2, 12) = %d\n", pow(2,12));
	loop /= pow(2, 12);	/* frag =0x200C.  0xC=12 */		

	printf("samplerate=%d\n", samplerate);	
	printf("channel=%d\n", channel);				
	printf("frag=%d\n", frag);		
	
	buffer = (char *)malloc(frag);

	gettimeofday(&time, &timezone);
	nowsec = time.tv_sec;	
	printf("begin recording!\n");					
	for(i=0;i<loop;i++)
	{
		int size=0;
		do
		{
			size = read(r_dsp, buffer, frag);	/* recording */
		}while(size <= 0);					
		fwrite(buffer, 1, frag, fd);		
	}		

	printf("----\n");	
	printf("record exit!!\n");
	close_audio_rec_device();		
	fclose(fd);
	free(buffer);
	
	return 0;
}

int play_pcm(char *file, int samplerate)
{
	int data, oss_format, channels, sample_rate;	
	int i;
	char *buffer;
	
	//printf("<=== %s ===>\n", file);
	open_audio_play_device();
	
	FILE *fd;
	fd = fopen(file, "r+");
    if(fd == NULL)
    {
    	printf("open %s error!\n", file);
    	return 0;
    }
    
	data = 0x5050;
	oss_format=AFMT_S16_LE;/*standard 16bit little endian format, support this format only*/
	sample_rate = samplerate;
	channels = 2;
	ioctl(p_dsp, SNDCTL_DSP_SETFMT, &oss_format);
	ioctl(p_mixer, MIXER_WRITE(SOUND_MIXER_PCM), &data);
	ioctl(p_dsp, SNDCTL_DSP_SPEED, &sample_rate);
	ioctl(p_dsp, SNDCTL_DSP_CHANNELS, &channels);
			
	int frag;
	ioctl(p_dsp, SNDCTL_DSP_GETBLKSIZE, &frag);
	buffer = (char *)malloc(frag);
	printf("frag=%d\n", buffer);	
	
	fread(buffer, 1, frag, fd);	
	while(!feof(fd))	
	{		
		audio_buf_info info;			
		do{			
			ioctl(p_dsp , SNDCTL_DSP_GETOSPACE , &info);			
			usleep(100);
		}while(info.bytes < frag);
		
		fd_set writefds;
		struct timeval tv;
		tv.tv_sec       = 0;
		tv.tv_usec      = 0;
		FD_ZERO( &writefds );
		FD_SET( p_dsp , &writefds );
		tv.tv_sec       = 0;
		tv.tv_usec      = 0;
		
		select( p_dsp + 1 , NULL , &writefds , NULL, &tv );
		if( FD_ISSET( p_dsp, &writefds ))
		{	
			write(p_dsp, buffer, frag);   			
			fread(buffer, 1, frag, fd);
		}	
		usleep(100);
	}
	
	int bytes;
	ioctl(p_dsp,SNDCTL_DSP_GETODELAY,&bytes);
	int delay = bytes / (sample_rate * 2 * channels);	
	sleep(delay);
	
	printf("Stop Play\n");
	fclose(fd);
	free(buffer);
	close_audio_play_device();
}

int play_pcm_single(char *file, int samplerate)
{
	int data, oss_format, channels, sample_rate;	
	int i;
	char *buffer;
	
	//printf("<=== %s ===>\n", file);
	open_audio_play_device();
	
	FILE *fd;
	fd = fopen(file, "r+");
    if(fd == NULL)
    {
    	printf("open %s error!\n", file);
    	return 0;
    }
    
	data = 0x5050;
	oss_format=AFMT_S16_LE;/*standard 16bit little endian format, support this format only*/
	sample_rate = samplerate;
	channels = 1;
	ioctl(p_dsp, SNDCTL_DSP_SETFMT, &oss_format);
	ioctl(p_mixer, MIXER_WRITE(SOUND_MIXER_PCM), &data);
	ioctl(p_dsp, SNDCTL_DSP_SPEED, &sample_rate);
	ioctl(p_dsp, SNDCTL_DSP_CHANNELS, &channels);
			
	int frag;
	ioctl(p_dsp, SNDCTL_DSP_GETBLKSIZE, &frag);
	buffer = (char *)malloc(frag);
	printf("frag=%d\n", buffer);	
	
	fread(buffer, 1, frag, fd);
	while(!feof(fd))	
	{		
		audio_buf_info info;			
		do{			
			ioctl(p_dsp , SNDCTL_DSP_GETOSPACE , &info);			
			usleep(100);
		}while(info.bytes < frag);
		
		fd_set writefds;
		struct timeval tv;
		tv.tv_sec       = 0;
		tv.tv_usec      = 0;
		FD_ZERO( &writefds );
		FD_SET( p_dsp , &writefds );
		tv.tv_sec       = 0;
		tv.tv_usec      = 0;
		
		select( p_dsp + 1 , NULL , &writefds , NULL, &tv );
		if( FD_ISSET( p_dsp, &writefds ))
		{	

			write(p_dsp, buffer, frag);   
			fread(buffer, 1, frag, fd);
		}	
		usleep(100);
	}
	
	int bytes;
	ioctl(p_dsp,SNDCTL_DSP_GETODELAY,&bytes);
	int delay = bytes / (sample_rate * 2 * channels);	
	sleep(delay);
	
	printf("Stop Play\n");
	fclose(fd);
	free(buffer);
	close_audio_play_device();
}

void * p_rec(void * arg)
{
	printf("recording ...\n");
	record_to_pcm("./rec.pcm", 16000, -1);	
}

void * p_play(void * arg)
{
	printf("playing ...\n");
	play_pcm("./16k.pcm", 16000);	
	rec_stop = 1;
}

int main()
{
	int i, loop, sr;
	char path[256];
	int buf;
	
	//signal (SIGTERM, mySignalHandler); /* for the TERM signal.. */
	//signal (SIGINT, mySignalHandler); /* for the CTRL+C signal.. */
	
	//while(1)
	{
		printf("\n**** Audio Test Program ****\n");
		printf("1. Play \n");
		printf("2. Record and then Play (two channels)\n");
		printf("3. Record and then Play (one channel)\n");		
//		printf("3. Record & Play \n");
		printf("Select:");
		
		scanf("%d", &i);
	
		if(i<1 || i >3)
		  return 0;
		 
		if(i == 1)
		{ 
			printf("Playing ...\n");
			
			for(i=0;i<8;i++)
				play_pcm(pcmfiles[i], samplerate[i]);
		}
		else if(i == 2)
		{
			printf("\nRec Seconds:");
			scanf("%d", &loop);
			
			printf("Sample Rate(8000, 16000..):");
			scanf("%d", &sr);
					
			printf("Record volume (0 - 100) :");
			scanf("%d", &rec_volume);
			
			if (rec_volume > 100) rec_volume = 100;					

			buf = rec_volume;
			rec_volume <<= 8;
			rec_volume |= buf;								

			printf(" *** Recording ***\n");
//			printf("SampleRate=%d, Time=%d sec\n", sr, loop);
			printf("SampleRate=%d, Time=%d sec, Volume=%d\n", sr, loop, buf);
			
			loop *= 4;		// "loop" is the multiple of 8KB (by default); 4: (two channels) * (16-bits per sdample)/8)
			loop *= sr;
			loop /= 8000;			
			
			record_to_pcm("./rec.pcm", sr, loop);
	
			printf("\nDone, Now play it ...\n");
	
			getchar();
			play_pcm("./rec.pcm", sr);
		}
		else if(i==3)
		{
			printf("\nRec Seconds:");
			scanf("%d", &loop);
			
			printf("Sample Rate(8000, 11025, 12000 or 16000):");
			scanf("%d", &sr);
			
			printf("Record volume (0 - 100) :");
			scanf("%d", &rec_volume);

			if (rec_volume > 100) rec_volume = 100;					

			buf = rec_volume;
			rec_volume <<= 8;
			rec_volume |= buf;								

			printf(" *** Recording ***\n");
//			printf("SampleRate=%d, Time=%d sec\n", sr, loop);
			printf("SampleRate=%d, Time=%d sec, Volume=%d\n", sr, loop, buf);		

			record_to_pcm_single("./rec.pcm", sr, loop);
	
			printf("\nDone, Now play it ...\n");
	
			getchar();
			play_pcm_single("./rec.pcm", sr);
		}
		else
			printf("wrong input\n");
	}
	
	return 0;
}
