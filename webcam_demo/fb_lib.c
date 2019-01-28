#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include "fb_lib.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include "fb_lib.h"
#include "cam_lib.h"

unsigned int frame_control = 1;

struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;

#define LCD_ENABLE_INT		_IO('v', 28)
#define LCD_DISABLE_INT		_IO('v', 29)

int fb_open(int fbx)
{
    int fd;

    switch (fbx) {
    case FB_0:
        fd = open("/dev/fb0", O_RDWR);
        break;
    
    case FB_1:
        fd = open("/dev/fb1", O_RDWR);
        break;
    }

    return fd;
}


void myExitHandler (int sig)
{
  	printf("  Exit Signal Handler\n");
	memcpy((void*)fb_addr, (char*)backupVideoBuffer, screensize);
	munmap(fb_addr, screensize);
    	fb_close(fb_fd);	
    	cam_stop(cam_fd);
    	cam_close(cam_fd);
	free(backupVideoBuffer);

	if(pJpegBuffer)
		munmap(pJpegBuffer, jpeg_buffer_size);
	close(fd_jpeg);

	fflush(stdout);
	usleep(1000000);
	exit(-9);
}


int fb_render(int fd, unsigned char *img, unsigned char *map, 
	      int xoffset, int yoffset,
              int width, int height, int depth)
{
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    long int location;
    int x;
    int y;
    int k;

    int b;
    int g;
    int r;
    unsigned short int t;

    int xsrc;
    int ysrc;

    if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo) == -1) {
         return -1;
    }

    if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
         return -2;
    }

    for (y = yoffset+1, ysrc = 1; ysrc < height; y++, ysrc++) {
	if (y >= vinfo.yres) 
		break;

        switch (depth) {
        case 24:
            k = (height-ysrc)*3*width;
            break;

        case 32:
            k = (height-ysrc)*4*width;
            break;
        }

        for (x = xoffset, xsrc = 1; xsrc < width; x++, xsrc++) {
	    if (x >= vinfo.xres)
		break;

            location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8);
	    location += (y+vinfo.yoffset) * finfo.line_length;

            switch (vinfo.bits_per_pixel) {
            case 32:
                if (depth == 24) {
                    *(map + location) = img[k+xsrc*3+2]; 
                    *(map + location + 1) = img[k+xsrc*3+1]; 
                    *(map + location + 2) = img[k+xsrc*3]; 
                    *(map + location + 3) = 0; 
                } else if (depth == 32) {
                    *(map + location) = img[k+xsrc*4+2]; 
                    *(map + location + 1) = img[k+xsrc*4+1]; 
                    *(map + location + 2) = img[k+xsrc*4]; 
                    *(map + location + 3) = img[k+xsrc*4+3]; 
                }
                break;

            case 24:

                break;

            case 16:
		if (depth == 24) {
                    r = img[k+xsrc*3];
                    g = img[k+xsrc*3+1];
                    b = img[k+xsrc*3+2];
		} else if (depth == 32) {
		    r = img[k+xsrc*4];
                    g = img[k+xsrc*4+1];
                    b = img[k+xsrc*4+2];
 		}
                t = r << 11 | g << 5 | b;
                *((unsigned short int*)(map + location)) = t;
                break;
            }
        }
    }

    return 0;
}

void fb_close(int fd)
{
    close(fd);
}
