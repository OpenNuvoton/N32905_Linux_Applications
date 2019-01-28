#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
//#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <stdlib.h>

#define DEVMEM_GET_STATUS       _IOR('m', 4, unsigned int)
#define DEVMEM_SET_STATUS       _IOW('m', 5, unsigned int)
#define DEVMEM_GET_PHYSADDR		_IOR('m', 6, unsigned int)
#define DEVMEM_GET_VIRADDR		_IOR('m', 7, unsigned int)

#define DEVMEM_GET_RES_AREA 	_IOR('m', 13, unsigned int)

#define MAP_SIZE	(480*272*4)

int __openDev()
{
	printf("### Open memory device\n");	
	int fd = open("/dev/devmem", O_RDWR);
    if (!fd) {
         printf("### Error: cannot open memory device.\n");
         exit(1);
    }		
    return fd;
}	

unsigned char* __testmmap(int fd)
{
	unsigned int inputStatusCode = 0x5A6B7C8D;
	unsigned int outputStatusCode;
	unsigned int physAddr, virAddr;	
	
        // Map the device to memory
	
	printf("###fd[%d] Run mmap\n",fd);	
	usleep(200000);
    unsigned char *devFrameBuffer = (unsigned char *)mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if ((int)devFrameBuffer== -1) 
	{
            printf("### Error: failed to map framebuffer device to memory.\n");
            exit(4);
    } else printf("###Frame Buffer at:0x%x\n",devFrameBuffer);
	

	int i;
			
	printf("### memset 0x5A\n");
	memset(devFrameBuffer, 0x5A, MAP_SIZE);
	printf("### memcmp 0x5A\n");
	for(i=0; i < MAP_SIZE; i++ )
		if(devFrameBuffer[i] != 0x5A)
		{
			printf("Compare failed at: %d\n",i);
			exit(1); //break;
		}	 
			
	ioctl(fd, DEVMEM_GET_PHYSADDR, &physAddr);
	ioctl(fd, DEVMEM_GET_VIRADDR, &virAddr);
	usleep(200000);

	printf("### Get PhyAddr:0x%x, VirAddr:0x%x\n",physAddr,virAddr);

	return devFrameBuffer;
}	

void __closeDev(int fd, unsigned char* stAddress)
{
	printf("###fd[%d] Run munmap\n", fd);	
	munmap(stAddress, MAP_SIZE);
	
	printf("### Close DevMem device: fd[%d]\n",fd);	
	close(fd);	
}	

void main (int argc, char* argv[])
{
	int i, openCnt, fdlist[100];
	unsigned char* stAddressList[100];
	if( argc >=2 && argv[1] )
		openCnt = atoi(argv[1]);
	else
		openCnt = 1;
	
	printf("### Start Test, open count:%d\n",openCnt);	

	for( i = 0; i < openCnt; i++ )
	{
		fdlist[i] = __openDev();
		stAddressList[i]=__testmmap(fdlist[i]);
	}	
	
	for( i = 0; i < openCnt; i++ )
		__closeDev(fdlist[i],stAddressList[i]);
		
	usleep(200000);
	for( i = 0; i < openCnt; i++ )
	{
		int replyStatus;
		fdlist[i] = __openDev();
		ioctl(fdlist[i], DEVMEM_GET_RES_AREA, &replyStatus);
		printf("### Get Reserve Area[%d]:--> %d\n",i,replyStatus);
		stAddressList[i]=__testmmap(fdlist[i]);
		__closeDev(fdlist[i],stAddressList[i]);
	}	
				
	printf("### End Test\n");
//	while(1) usleep(200000);	
}