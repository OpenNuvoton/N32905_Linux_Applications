#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/mman.h>
#include <asm/ioctl.h>
//#include <asm/arch/hardware.h>
#include "jpegcodec.h"
#include <linux/vt.h>
#include <linux/kd.h>
#include <linux/fb.h>
#include <unistd.h>
#include <dirent.h>
#include "cam_lib.h"

jpeg_param_t jpeg_param;
jpeg_info_t *jpeginfo;	

int jpegDec(struct v4l2_buffer *buf)
{
	int ret = 0;
	int len, jpeginfo_size;	
	memset((void*)&jpeg_param, 0, sizeof(jpeg_param_t));
	jpeginfo_size = sizeof(jpeg_info_t) + sizeof(__u32);
	jpeginfo = malloc(jpeginfo_size);
	
	jpeg_param.encode = 0;			/* Decode Operation */
	jpeg_param.src_bufsize = 100*1024;	/* Src buffer size (Bitstream buffer size for JPEG engine) */
	jpeg_param.dst_bufsize = 640*480*2;	/* Dst buffer size (Decoded Raw data buffer size for JPEG engine) */
	jpeg_param.decInWait_buffer_size = 0;	/* Decode input Wait buffer size (Decode input wait function disable when 								   decInWait_buffer_size is 0) */
	jpeg_param.decopw_en = 0;
	jpeg_param.windec_en = 0;

	jpeg_param.scale = 1;			/* Enable scale function */
	jpeg_param.scaled_width = vinfo.xres;	/* width after scaling */
	jpeg_param.scaled_height = vinfo.yres;	/* height after scaling */
	jpeg_param.dec_stride = vinfo.xres;	/* Enable stride function */	


	jpeg_param.buffersize = 0;		/* only for continuous shot */
    	jpeg_param.buffercount = 1;
			
	/* Set decode output format: RGB555/RGB565/RGB888/YUV422/PLANAR_YUV */
	jpeg_param.decode_output_format = DRVJPEG_DEC_PRIMARY_PACKET_RGB565;

	jpeg_param.paddr_dst = fb_paddress;
	jpeg_param.vaddr_dst = (__u32)fb_addr;

	if(user_memorypool_enable)
	{
		jpeg_param.vaddr_src = (unsigned int) buffers[buf->index].start ;
		jpeg_param.paddr_src = (unsigned int) bitstream_paddress + buf->index * buf->length;
	}
	else
	{	
		memcpy((unsigned char *)bitstream_vaddress, buffers[buf->index].start, buf->bytesused);
		jpeg_param.vaddr_src = (unsigned int) bitstream_vaddress;
		jpeg_param.paddr_src = (unsigned int) bitstream_paddress;
	}

	/* Set operation property to JPEG engine */
	if((ioctl(fd_jpeg, JPEG_S_PARAM, &jpeg_param)) < 0)
	{
		fprintf(stderr,"set jpeg param failed:%d\n",errno);
		ret = -1;
		goto out;
	}		
		
	/* Trigger JPEG engine */
	if((ioctl(fd_jpeg, JPEG_TRIGGER, NULL)) < 0)
	{
		fprintf(stderr,"trigger jpeg failed:%d\n",errno);
		ret = -1;
		goto out;
	}
	
	/* Get JPEG decode information */
	len = read(fd_jpeg, jpeginfo, jpeginfo_size);
	if(len<0) {
		fprintf(stderr, "read data error errno=%d\n", errno);
		ret = -1;
		goto out;
	}
out:
	free(jpeginfo);
	return 0;

}



