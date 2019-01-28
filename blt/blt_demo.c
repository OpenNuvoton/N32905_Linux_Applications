#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <signal.h>
#include <linux/fb.h>

#include "DrvBLT.h"

#define _BITBLT_SRC_BGMAP_SIZE_	(480*272*2)
#define _BITBLT_SRC_SPMAP_SIZE_	(736*160*2)

#define DEVMEM_GET_STATUS	_IOR('m', 4, unsigned int)
#define DEVMEM_SET_STATUS	_IOW('m', 5, unsigned int)
#define DEVMEM_GET_PHYSADDR	_IOR('m', 6, unsigned int)
#define DEVMEM_GET_VIRADDR	_IOR('m', 7, unsigned int)

#define IOCTL_LCD_ENABLE_INT	_IO('v', 28)
#define IOCTL_LCD_DISABLE_INT	_IO('v', 29)

typedef struct
{
	struct timeval sStart;
	struct timeval sEnd;
	unsigned int start;
	unsigned int end;
	unsigned int u32FPS;
} S_DEMO_FPS;

S_DEMO_FPS g_sDemo_Fps;

typedef struct
{
	int	blt_fd;

	int dest_fd;
	unsigned char *pDestBuffer;
	unsigned int DestPhysAddr;
	int bitblt_dest_map_size;

	int srcBG_fd;
	unsigned char *pSrcBGBuffer;
	unsigned int SrcBGPhysAddr;

	int srcSP_fd;
	unsigned char *pSrcSPBuffer;
	unsigned int SrcSPPhysAddr;

	unsigned short u16Index;
	volatile unsigned short u16Go;
} S_DEMO_BITBLT;

S_DEMO_BITBLT g_sDemo_BitBlt;

typedef struct
{
	int	lcm_fd;
	unsigned char *pLCMBuffer;
	int lcm_width;
	int lcm_height;
	int lcm_buffer_size;
} S_DEMO_LCM;

S_DEMO_LCM g_sDemo_Lcm;

typedef struct
{
	unsigned int	u32PatternBase;
	int				i32X;	// position of X
	int				i32Y;	// position of Y
	unsigned int	u32Width;	// crop width
	unsigned int	u32Height;	// crop height
	unsigned int	u32Stride;
} S_OBJ;

static S_OBJ s_saObj[2];

static S_DRVBLT_BLIT_OP				s_sblitop;
static S_DRVBLT_BLIT_TRANSFORMATION	s_stransformation;
static S_DRVBLT_FILL_OP				s_sfillop;

void _DemoBitBlt_Config
(
	unsigned int	DestPhysAddr,
	short			i16Width,
	short			i16Height
)
{
	s_sblitop.dest.u32FrameBufAddr = DestPhysAddr;
	s_sblitop.dest.i32XOffset = 0;
	s_sblitop.dest.i32YOffset = 0;
	s_sblitop.dest.i32Stride = (i16Width * 2);
	s_sblitop.dest.i16Width = i16Width;
	s_sblitop.dest.i16Height = i16Height;

	s_sblitop.src.pSARGB8 = NULL;

	s_stransformation.colorMultiplier.i16Blue = 0;
	s_stransformation.colorMultiplier.i16Green = 0;
	s_stransformation.colorMultiplier.i16Red = 0;
	s_stransformation.colorMultiplier.i16Alpha = 0;
	s_stransformation.colorOffset.i16Blue = 0;
	s_stransformation.colorOffset.i16Green = 0;
	s_stransformation.colorOffset.i16Red = 0;
	s_stransformation.colorOffset.i16Alpha = 0;
	s_stransformation.matrix.a = 0x00010000;
	s_stransformation.matrix.b = 0x00000000;
	s_stransformation.matrix.c = 0x00000000;
	s_stransformation.matrix.d = 0x00010000;
	s_stransformation.srcFormat = eDRVBLT_SRC_RGB565;
	s_stransformation.destFormat = eDRVBLT_DEST_RGB565;
	s_stransformation.flags = eDRVBLT_HASTRANSPARENCY;
	s_stransformation.fillStyle = (eDRVBLT_NOTSMOOTH | eDRVBLT_CLIP);
	s_stransformation.userData = NULL;

	s_sblitop.transformation = &s_stransformation;

	s_sfillop.color.u8Blue = 255;
	s_sfillop.color.u8Green = 0;
	s_sfillop.color.u8Red = 0;
	s_sfillop.color.u8Alpha = 0;
	s_sfillop.blend = FALSE;
	s_sfillop.u32FrameBufAddr = s_sblitop.dest.u32FrameBufAddr;
	s_sfillop.rowBytes = s_sblitop.dest.i32Stride;
	s_sfillop.format = eDRVBLT_DEST_RGB565;
	s_sfillop.rect.i16Xmin = 0;
	s_sfillop.rect.i16Xmax = s_sblitop.dest.i16Width;
	s_sfillop.rect.i16Ymin = 0;
	s_sfillop.rect.i16Ymax = s_sblitop.dest.i16Height;
}

int _DemoBitBlt_SetPosition
(
	unsigned int	u32Index,
	int				i32X,
	int				i32Y
)
{	//FIXME this API _DemoBitBlt_SetPosition
	unsigned int u32X, u32Y;

	s_sblitop.src.u32SrcImageAddr = s_saObj[u32Index].u32PatternBase;
	s_sblitop.src.i32Stride = s_saObj[u32Index].u32Stride * 2;
	s_saObj[u32Index].i32X = i32X;
	s_saObj[u32Index].i32Y = i32Y;

	if ( (i32X < 0) && (i32Y < 0) )
	{
		u32X = -i32X;
		u32Y = -i32Y;
		s_sblitop.src.i32XOffset = u32X << 16;
		s_sblitop.src.i32YOffset = u32Y << 16;
		s_sblitop.dest.u32FrameBufAddr = g_sDemo_BitBlt.DestPhysAddr;

		s_sblitop.dest.i16Width = s_saObj[u32Index].u32Width - u32X;
		s_sblitop.dest.i16Height = s_saObj[u32Index].u32Height - u32Y;
	}
	else if ( (i32X < 0) && (i32Y >= 0) )
	{
		u32X = -i32X;
		s_sblitop.src.i32XOffset = u32X << 16;
		s_sblitop.src.i32YOffset = 0 << 16;
		s_sblitop.dest.u32FrameBufAddr = g_sDemo_BitBlt.DestPhysAddr + i32Y * g_sDemo_Lcm.lcm_width * 2;

		s_sblitop.dest.i16Width = s_saObj[u32Index].u32Width - u32X;

		if ( (i32Y + s_saObj[u32Index].u32Height) >= g_sDemo_Lcm.lcm_height )
			s_sblitop.dest.i16Height = g_sDemo_Lcm.lcm_height - i32Y;
		else
			s_sblitop.dest.i16Height = s_saObj[u32Index].u32Height;
	}
	else if ( (i32X >= 0) && (i32Y < 0) )
	{
		u32Y = -i32Y;
		s_sblitop.src.i32XOffset = 0 << 16;
		s_sblitop.src.i32YOffset = u32Y << 16;
		s_sblitop.dest.u32FrameBufAddr = g_sDemo_BitBlt.DestPhysAddr + i32X * 2;

		if ( (i32X + s_saObj[u32Index].u32Width) >= g_sDemo_Lcm.lcm_width )
			s_sblitop.dest.i16Width = g_sDemo_Lcm.lcm_width - i32X;
		else
			s_sblitop.dest.i16Width = s_saObj[u32Index].u32Width;

		s_sblitop.dest.i16Height = s_saObj[u32Index].u32Height - u32Y;
	}
	else if ( (i32X >= 0) && (i32Y >= 0) )
	{
		s_sblitop.src.i32XOffset = 0 << 16;
		s_sblitop.src.i32YOffset = 0 << 16;
		s_sblitop.dest.u32FrameBufAddr = g_sDemo_BitBlt.DestPhysAddr + i32Y * g_sDemo_Lcm.lcm_width * 2 + i32X * 2;

		if ( (i32X + s_saObj[u32Index].u32Width) >= g_sDemo_Lcm.lcm_width )
			s_sblitop.dest.i16Width = g_sDemo_Lcm.lcm_width - i32X;
		else
			s_sblitop.dest.i16Width = s_saObj[u32Index].u32Width;

		if ( (i32Y + s_saObj[u32Index].u32Height) >= g_sDemo_Lcm.lcm_height )
			s_sblitop.dest.i16Height = g_sDemo_Lcm.lcm_height - i32Y;
		else
			s_sblitop.dest.i16Height = s_saObj[u32Index].u32Height;
	}
	else
		return -1;

	return 0;
}

void _DemoBitBlt_SetCrop
(
	unsigned int	u32Index,
	unsigned int	u32Width,
	unsigned int	u32Height
)
{	//FIXME this API _DemoBitBlt_SetCrop
	s_sblitop.src.i16Width = s_saObj[u32Index].u32Width = u32Width;
	s_sblitop.src.i16Height = s_saObj[u32Index].u32Height = u32Height;
}

void _DemoBitBlt_Flush(void)
{	//FIXME this API _DemoBitBlt_Flush
#if 0
	gettimeofday( &g_sDemo_Fps.sEnd, NULL );
	g_sDemo_Fps.end = g_sDemo_Fps.sEnd.tv_sec * 1000000 + g_sDemo_Fps.sEnd.tv_usec;
	while ( (g_sDemo_Fps.end - g_sDemo_Fps.start) < g_sDemo_Fps.u32FPS )
	{
		gettimeofday( &g_sDemo_Fps.sEnd, NULL );
		g_sDemo_Fps.end = g_sDemo_Fps.sEnd.tv_sec * 1000000 + g_sDemo_Fps.sEnd.tv_usec;
		usleep( 10000 );
	}
#endif
	int i;

	if ( ( ioctl( g_sDemo_BitBlt.blt_fd, BLT_SET_FILL, &s_sfillop ) ) < 0 )
	{
		fprintf( stderr, "set FILL parameter failed:%d\n", errno );
	}

	if ( ( ioctl( g_sDemo_BitBlt.blt_fd, BLT_TRIGGER, NULL ) ) < 0 )
	{
		fprintf( stderr, "trigger BLT failed:%d\n", errno );
	}

	if ( ( read( g_sDemo_BitBlt.blt_fd, NULL, 0 ) ) < 0 )
	{
		fprintf( stderr, "read data error errno=%d\n", errno );
	}

	for ( i = 0; i <= 1; i++ )
	{
		if ( _DemoBitBlt_SetPosition( i, s_saObj[i].i32X, s_saObj[i].i32Y ) < 0 )
			continue;
		_DemoBitBlt_SetCrop( i, s_saObj[i].u32Width, s_saObj[i].u32Height );

		if( ( ioctl( g_sDemo_BitBlt.blt_fd, BLT_SET_BLIT, &s_sblitop ) ) < 0 )
		{
			fprintf( stderr, "set BLIT parameter failed:%d\n", errno );
		}

		if ( ( ioctl( g_sDemo_BitBlt.blt_fd, BLT_TRIGGER, NULL ) ) < 0 )
		{
			fprintf( stderr, "trigger BLT failed:%d\n", errno );
		}

		if ( ( read( g_sDemo_BitBlt.blt_fd, NULL, 0 ) ) < 0 )
		{
			fprintf( stderr, "read data error errno=%d\n", errno );
		}
	}

	ioctl( g_sDemo_Lcm.lcm_fd, IOCTL_LCD_DISABLE_INT );
	memcpy( g_sDemo_Lcm.pLCMBuffer, g_sDemo_BitBlt.pDestBuffer, g_sDemo_Lcm.lcm_buffer_size );
	ioctl( g_sDemo_Lcm.lcm_fd, IOCTL_LCD_ENABLE_INT );
#if 0
	gettimeofday( &g_sDemo_Fps.sStart, NULL );
	g_sDemo_Fps.start = g_sDemo_Fps.sStart.tv_sec * 1000000 + g_sDemo_Fps.sStart.tv_usec;
#endif
}

void _Demo_app_close(void)
{
	printf( "\n\n### Run munmap\n\n" );
	munmap( (unsigned char *)g_sDemo_BitBlt.dest_fd, g_sDemo_BitBlt.bitblt_dest_map_size );
	munmap( (unsigned char *)g_sDemo_BitBlt.srcBG_fd, _BITBLT_SRC_BGMAP_SIZE_ );
	munmap( (unsigned char *)g_sDemo_BitBlt.srcSP_fd, _BITBLT_SRC_SPMAP_SIZE_ );
	munmap( (unsigned char *)g_sDemo_Lcm.lcm_fd, g_sDemo_Lcm.lcm_buffer_size );

	printf( "\n\n### Close BLT and LCM devices\n\n" );
	close( g_sDemo_BitBlt.blt_fd );
	close( g_sDemo_BitBlt.dest_fd );
	close( g_sDemo_BitBlt.srcBG_fd );
	close( g_sDemo_BitBlt.srcSP_fd );
	close( g_sDemo_Lcm.lcm_fd );
}

static void _Demo_sigint( int signo )
{
	_Demo_app_close();

	exit( 1 );
}

static int _Demo_RegisterSigint()
{
	struct sigaction act;
	memset( &act , 0 , sizeof( act ) );
	act.sa_handler = _Demo_sigint;
	act.sa_flags   = SA_RESTART;
	if( sigaction( SIGTERM, &act , NULL ) != 0 )
	{
		return -1;
	}
	if( sigaction( SIGINT, &act , NULL ) != 0 )
	{
		return -1;
	}

	printf( "### Register Sigint PASS\n" );

	return 1;
}

bool InitSystem()
{
	if ( _Demo_RegisterSigint() < 0 )
	{
		printf("### Register Sigint Failed\n");
		return FALSE;
	}

	return TRUE;
}

bool InitFPS()
{	//FIXME this API InitFPS
	// frame control here...
	// no fps limitation, 0
	// if you want fps 30, 1/30 * 1000000 = 33333
	// if you want fps 20, 1/20 * 1000000 = 50000
	// if you want fps 12, 1/12 * 1000000 = 83333
	// if you want fps 7, 1/7 * 1000000 = 142857
	g_sDemo_Fps.u32FPS = 33333;
	g_sDemo_Fps.start = 0;
	g_sDemo_Fps.end = 0;
}

bool _DemoBitBlt_SetDestBuffer()
{
	g_sDemo_BitBlt.dest_fd = open( "/dev/devmem", O_RDWR );
	if ( g_sDemo_BitBlt.dest_fd <= 0 )
	{
		printf( "### Error: cannot open DevMem device, returns %d\n", g_sDemo_BitBlt.dest_fd );
		return FALSE;
	}

	// Map the device to memory
	printf( "### Run mmap\n" );

	g_sDemo_BitBlt.bitblt_dest_map_size = g_sDemo_Lcm.lcm_width * g_sDemo_Lcm.lcm_height * 2;

	g_sDemo_BitBlt.pDestBuffer = mmap( NULL, g_sDemo_BitBlt.bitblt_dest_map_size, PROT_READ|PROT_WRITE, MAP_SHARED, g_sDemo_BitBlt.dest_fd, 0 );
	if ( (int)g_sDemo_BitBlt.pDestBuffer == -1 )
	{
		printf( "### Error: failed to map DevMem device to memory.\n" );
		return FALSE;
	}
	else
		printf( "### BLT Dest Buffer at:%p\n", g_sDemo_BitBlt.pDestBuffer );
	ioctl( g_sDemo_BitBlt.dest_fd, DEVMEM_GET_PHYSADDR, &g_sDemo_BitBlt.DestPhysAddr );

	return TRUE;
}

bool _DemoBitBlt_SetSrcBGBuffer( const char *pszImgFileName )
{
	g_sDemo_BitBlt.srcBG_fd = open( "/dev/devmem", O_RDWR );
	if ( g_sDemo_BitBlt.srcBG_fd <= 0 )
	{
		printf( "### Error: cannot open DevMem device, returns %d\n", g_sDemo_BitBlt.srcBG_fd );
		return FALSE;
	}

	// Map the device to memory
	printf( "### Run mmap\n" );

	g_sDemo_BitBlt.pSrcBGBuffer = mmap( NULL, _BITBLT_SRC_BGMAP_SIZE_, PROT_READ|PROT_WRITE, MAP_SHARED, g_sDemo_BitBlt.srcBG_fd, 0 );
	if ( (int)g_sDemo_BitBlt.pSrcBGBuffer == -1 )
	{
		printf( "### Error: failed to map DevMem device to memory.\n" );
		return FALSE;
	}
	else
		printf( "### BLT Src Buffer at:%p\n", g_sDemo_BitBlt.pSrcBGBuffer );
	ioctl( g_sDemo_BitBlt.srcBG_fd, DEVMEM_GET_PHYSADDR, &g_sDemo_BitBlt.SrcBGPhysAddr );

	FILE *fpBGImg;
	fpBGImg = fopen( pszImgFileName, "r" );
	if ( fread( g_sDemo_BitBlt.pSrcBGBuffer, _BITBLT_SRC_BGMAP_SIZE_, 1, fpBGImg ) <= 0 )
	{
		printf( "Cannot Read the BG Image File!\n" );
		fclose( fpBGImg );
		return FALSE;
	}

	fclose( fpBGImg );

	s_saObj[0].u32PatternBase = g_sDemo_BitBlt.SrcBGPhysAddr;
	s_saObj[0].u32Stride = 480;

	return TRUE;
}

bool _DemoBitBlt_SetSrcSPBuffer( const char *pszImgFileName )
{
	g_sDemo_BitBlt.srcSP_fd = open( "/dev/devmem", O_RDWR );
	if ( g_sDemo_BitBlt.srcSP_fd <= 0 )
	{
		printf( "### Error: cannot open DevMem device, returns %d\n", g_sDemo_BitBlt.srcSP_fd );
		return FALSE;
	}

	// Map the device to memory
	printf( "### Run mmap\n" );

	g_sDemo_BitBlt.pSrcSPBuffer = mmap( NULL, _BITBLT_SRC_SPMAP_SIZE_, PROT_READ|PROT_WRITE, MAP_SHARED, g_sDemo_BitBlt.srcSP_fd, 0 );
	if ((int)g_sDemo_BitBlt.pSrcSPBuffer == -1)
	{
		printf( "### Error: failed to map DevMem device to memory.\n" );
		return FALSE;
	}
	else
		printf( "### BLT Src Buffer at:%p\n", g_sDemo_BitBlt.pSrcSPBuffer );
	ioctl( g_sDemo_BitBlt.srcSP_fd, DEVMEM_GET_PHYSADDR, &g_sDemo_BitBlt.SrcSPPhysAddr );

	FILE *fpSPImg = fopen( pszImgFileName, "r" );
	if ( fread( g_sDemo_BitBlt.pSrcSPBuffer, _BITBLT_SRC_SPMAP_SIZE_, 1, fpSPImg ) <= 0 )
	{
		printf("Cannot Read the SP Image File!\n");
		fclose( fpSPImg );
		return FALSE;
	}

	fclose( fpSPImg );

	s_saObj[1].u32PatternBase = g_sDemo_BitBlt.SrcSPPhysAddr;
	s_saObj[1].u32Stride = 736;

	return TRUE;
}

bool _DemoBitBlt_SetColorKey( unsigned short u16RGB565ColorKey )
{
	printf( "### Set RGB565 color key\n" );

	if ( ( ioctl( g_sDemo_BitBlt.blt_fd, BLT_SET_RGB565_COLORKEY, u16RGB565ColorKey ) ) < 0 )
	{
		fprintf( stderr, "set RGB565 color key parameter failed:%d\n", errno );

		return FALSE;
	}

	printf( "### Set RGB565 color key Done\n" );

	return TRUE;
}

bool _DemoBitBlt_EnableColorKey( bool bEnabled )
{
	if ( bEnabled )
	{
		printf( "### Set RGB565 color key enable\n" );

		if ( ( ioctl( g_sDemo_BitBlt.blt_fd, BLT_ENABLE_RGB565_COLORCTL, NULL ) ) < 0 )
		{
			fprintf( stderr, "enable RGB565 color key failed:%d\n", errno );

			return FALSE;
		}

		printf( "### Set RGB565 color key enable Done\n" );
	}
	else
	{
		printf( "### Set RGB565 color key disable\n" );

		if ( ( ioctl( g_sDemo_BitBlt.blt_fd, BLT_DISABLE_RGB565_COLORCTL, NULL ) ) < 0 )
		{
			fprintf( stderr, "disable RGB565 color key failed:%d\n", errno );

			return FALSE;
		}

		printf( "### Set RGB565 color key disable Done\n" );
	}

	return TRUE;
}

bool InitBitBlt()
{
	printf( "### Open BLT devices\n" );

	g_sDemo_BitBlt.blt_fd = open( "/dev/blt", O_RDWR );
	if ( g_sDemo_BitBlt.blt_fd <= 0 )
	{
		printf( "### Error: cannot open BLT device, returns %d\n", g_sDemo_BitBlt.blt_fd );
		return FALSE;
	}

	g_sDemo_BitBlt.u16Index = 0;
	g_sDemo_BitBlt.u16Go = 1;

	return TRUE;
}

bool _DemoLCM_SetLCMBuffer()
{
	// Map the device to memory
	printf( "### Run mmap\n" );

	g_sDemo_Lcm.pLCMBuffer = mmap( NULL, g_sDemo_Lcm.lcm_buffer_size, PROT_READ|PROT_WRITE, MAP_SHARED, g_sDemo_Lcm.lcm_fd, 0 );
	if ( (int)g_sDemo_Lcm.pLCMBuffer == -1 )
	{
		printf( "### Error: failed to map LCM device to memory.\n" );
		return FALSE;
	}
	else
		printf( "### LCM Buffer at:%p\n", g_sDemo_Lcm.pLCMBuffer );

	return TRUE;
}

bool InitLCM()
{
	printf("### Open LCM devices\n");

	g_sDemo_Lcm.lcm_fd = open( "/dev/fb0", O_RDWR );
	if ( g_sDemo_Lcm.lcm_fd <= 0 )
	{
		printf( "### Error: cannot open LCM device, returns %d\n", g_sDemo_Lcm.lcm_fd );
		return FALSE;
	}

	struct fb_var_screeninfo var;
	if ( ioctl( g_sDemo_Lcm.lcm_fd, FBIOGET_VSCREENINFO, &var ) < 0 )
	{
		printf( "### Error: ioctl FBIOGET_VSCREENINFO!\n" );
		close( g_sDemo_Lcm.lcm_fd );
		exit( -1 );
	}

	g_sDemo_Lcm.lcm_width = var.xres;
	g_sDemo_Lcm.lcm_height = var.yres;
	g_sDemo_Lcm.lcm_buffer_size = g_sDemo_Lcm.lcm_width * g_sDemo_Lcm.lcm_height * 2;

	return TRUE;
}

int main()
{
	if ( !InitSystem() )
		exit( 1 );
#if 0	//FIXME this API InitFPS
	if ( !InitFPS() )
		exit( 1 );
#endif
	if ( !InitBitBlt() )
		exit( 1 );

	if ( !InitLCM() )
		exit( 1 );

	if ( !_DemoBitBlt_SetDestBuffer() )
		exit( 1 );

	if ( !_DemoBitBlt_SetSrcBGBuffer( "./BG.bin" ) )
		exit( 1 );

	if ( !_DemoBitBlt_SetSrcSPBuffer( "./SP.bin" ) )
		exit( 1 );

	if ( !_DemoLCM_SetLCMBuffer() )
		exit( 1 );

	if ( !_DemoBitBlt_SetColorKey( 0xF81F ) )
		exit( 1 );

	// set dest frame buffer address, width and height
	_DemoBitBlt_Config( g_sDemo_BitBlt.DestPhysAddr, g_sDemo_Lcm.lcm_width, g_sDemo_Lcm.lcm_height );

	// set position
	_DemoBitBlt_SetPosition( 0, 0, 0 );
	_DemoBitBlt_SetPosition( 1, 200, 100 );

	// set BG widht and height
	_DemoBitBlt_SetCrop( 0, 480, 272 );

	while ( g_sDemo_BitBlt.u16Go )
	{
		if ( (s_saObj[0].i32Y--) < -250 )
		{
			if ( !_DemoBitBlt_EnableColorKey( TRUE ) )
			{
				break;
			}
			s_saObj[0].i32Y = 150;
		}

		if ( (s_saObj[1].i32X--) < 0 )
		{
			if ( !_DemoBitBlt_EnableColorKey( FALSE ) )
			{
				break;
			}
			s_saObj[1].i32X = 300;
		}

		// crop SPs from SP
		switch ( g_sDemo_BitBlt.u16Index++ )
		{
		case 0:
			s_saObj[1].u32PatternBase = g_sDemo_BitBlt.SrcSPPhysAddr;
			_DemoBitBlt_SetCrop( 1, 96, 160 );
			break;
		case 1:
			s_saObj[1].u32PatternBase = g_sDemo_BitBlt.SrcSPPhysAddr + 96 * 2;
			_DemoBitBlt_SetCrop( 1, 96, 160 );
			break;
		case 2:
			s_saObj[1].u32PatternBase = g_sDemo_BitBlt.SrcSPPhysAddr + (96 + 96) * 2;
			_DemoBitBlt_SetCrop( 1, 112, 160 );
			break;
		case 3:
			s_saObj[1].u32PatternBase = g_sDemo_BitBlt.SrcSPPhysAddr + (96 + 96 + 112) * 2;
			_DemoBitBlt_SetCrop( 1, 112, 160 );
			break;
		case 4:
			s_saObj[1].u32PatternBase = g_sDemo_BitBlt.SrcSPPhysAddr + (96 + 96 + 112 + 112) * 2;
			_DemoBitBlt_SetCrop( 1, 112, 160 );
			break;
		case 5:
			s_saObj[1].u32PatternBase = g_sDemo_BitBlt.SrcSPPhysAddr + (96 + 96 + 112 + 112 + 112) * 2;
			_DemoBitBlt_SetCrop( 1, 112, 160 );
			break;
		case 6:
			s_saObj[1].u32PatternBase = g_sDemo_BitBlt.SrcSPPhysAddr + (96 + 96 + 112 + 112 + 112 + 112) * 2;
			g_sDemo_BitBlt.u16Index = 0;
			_DemoBitBlt_SetCrop( 1, 96, 160 );
			break;
		}

		// start to draw
		_DemoBitBlt_Flush();
	}

	_Demo_app_close();
}
