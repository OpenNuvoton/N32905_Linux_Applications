#ifndef _AVI_PLAYER_H_
#define _AVI_PLAYER_H_

#include <signal.h>

#define VIDEO_FORMAT_CHANGE			_IOW('v', 50, unsigned int)	//frame buffer format change
#define DISPLAY_MODE_RGB565	1

typedef enum jv_mode_e
{
	DIRECT_RGB555,				// JPEG output RGB555 directly to VPOST RGB555 frame buffer
	SW_YUV422_TO_RGB565,		// JPEG output packet YUV422, software convert to RGB565 for VPOST
	OVERLAY_YUV422_RGB555,		// JPEG output packet YUV422 buffer to overlay on a RGB555 buffer
	                            //   finally be output to VPOST YUV buffer
	HW_RGB565,
	HW_RGB555,
	HW_YUV422
}  JV_MODE_E;


typedef enum au_type_e
{
	AU_CODEC_UNKNOWN,
	AU_CODEC_PCM,
	AU_CODEC_IMA_ADPCM,
	AU_CODEC_MP3
}  AU_TYPE_E;


typedef struct avi_info_t
{
	unsigned int	uMovieLength;		/* in 1/100 seconds */
	unsigned int	uPlayCurTimePos;	/* for playback, the play time position, in 1/100 seconds */
	
	/* audio */
	AU_TYPE_E   	eAuCodec;         
	int				nAuPlayChnNum;		/* 1:Mono, 2:Stero */
	int 			nAuPlaySRate;		/* audio playback sampling rate */

	/* video */
	unsigned int	uVideoFrameRate;	/* only available in MP4/3GP/ASF/AVI files */
	unsigned short	usImageWidth;
	unsigned short	usImageHeight;
	unsigned int	uVidTotalFrames;	/* For playback, it's the total number of video frames. For recording, it's the currently recorded frame number. */
	unsigned int	uVidFramesPlayed;	/* Indicate how many video frames have been played */
	unsigned int	uVidFramesSkipped;	/* For audio/video sync, some video frames may be dropped. */
}	AVI_INFO_T;


typedef void (AVI_CB)(AVI_INFO_T *);

extern int aviPlayFile(char *suFileName, int x, int y, JV_MODE_E mode, AVI_CB *cb);
extern int aviGetFileInfo(char *suFileName, AVI_INFO_T *ptAviInfo);
extern void aviStopPlay(void);

#endif   // _AVI_PLAYER_H_
