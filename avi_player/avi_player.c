#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/hdreg.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syscall.h>
#include <errno.h>
#include <sys/mman.h>
#include "AviLib.h"

void avi_play_control(AVI_INFO_T *aviInfo)
{
	static int last_time = 0;
	int frame_rate;

	if ( aviInfo->uPlayCurTimePos != 0 )
		frame_rate = ((aviInfo->uVidFramesPlayed - aviInfo->uVidFramesSkipped) * 100) / aviInfo->uPlayCurTimePos;

	if ( aviInfo->uPlayCurTimePos - last_time > 100 )
	{
		printf( "%02d:%02d / %02d:%02d  Vid fps: %d / %d  %d\n",
			aviInfo->uPlayCurTimePos / 6000, (aviInfo->uPlayCurTimePos / 100) % 60,
			aviInfo->uMovieLength / 6000, (aviInfo->uMovieLength / 100) % 60,
			frame_rate, aviInfo->uVideoFrameRate, aviInfo->uVidFramesSkipped );
		last_time = aviInfo->uPlayCurTimePos;
	}
	// if(((aviInfo->uPlayCurTimePos / 100) % 60) > 10)
	// aviStopPlay();
}

static void DemoExitHandler(
	int sig
)
{
	printf("receive sig=%d\n", sig);

	int fd = open( "/dev/fb0", O_RDWR );
	if ( fd <= 0 )
	{
		printf( "### Error: cannot open LCM device, returns %d\n", fd );
		exit( 1 );
	}

	if ( ioctl( fd, VIDEO_FORMAT_CHANGE, DISPLAY_MODE_RGB565 ) < 0 )
		printf( "\n### VIDEO_FORMAT_CHANGE RGB565 - FAILED ###\n" );
	else
		printf( "\n### VIDEO_FORMAT_CHANGE RGB565 - OK ###\n" );

	close( fd );

	exit( 1 );
}

int
main( int argc, char *argv[] )
{
	int nStatus, fd;

	signal( SIGINT, DemoExitHandler );
	signal( SIGHUP, DemoExitHandler );
	signal( SIGQUIT, DemoExitHandler );
	signal( SIGILL, DemoExitHandler );
	signal( SIGABRT, DemoExitHandler );
	signal( SIGFPE, DemoExitHandler );
	signal( SIGKILL, DemoExitHandler );
	signal( SIGSEGV, DemoExitHandler );
	signal( SIGPIPE, DemoExitHandler );
	signal( SIGTERM, DemoExitHandler );

	nStatus = aviPlayFile( argv[1], 0, 0, HW_YUV422, avi_play_control );

	if ( nStatus < 0 )
		printf( "Playback failed, code = %x\n", nStatus );
	else
		printf( "Playback done.\n" );

	DemoExitHandler( 0 );

	return 0;
}
