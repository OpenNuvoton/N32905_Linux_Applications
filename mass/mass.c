/****************************************************************************
 *                                                                          *
 * Copyright (c) 2008 Nuvoton Technology Corp. All rights reserved.         *
 *                                                                          *
 ****************************************************************************/

/**********************************************************/ 
/* The mass for NAND driver by ns24 zswan 
/* ver 1.2 modified by ns24 zswan,20061009.  
/***********************************************************/ 
#include <sys/types.h> 
#include <sys/stat.h> 
#include <sys/ioctl.h> 
#include <linux/hdreg.h> 
#include <sys/mman.h>
#include <fcntl.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <syscall.h> 
#include <errno.h> 
#include "mass.h" 
  
#define Hidden_Driver_Letter

//#define MASS_DEBUG 
//#define MASS_ENTER_LEAVE 
 
#define USBD_PRINTF_COUNT 2000000

#ifdef MASS_DEBUG 
#define PDEBUG	printf 
#else 
#define PDEBUG(fmt, arg...)	 
#endif 
 
#ifdef MASS_ENTER_LEAVE 
#define ENTER()	PDEBUG("[%-10s] : Enter...\n", __FUNCTION__) 
#define LEAVE()	PDEBUG("[%-10s] : Leave\n", __FUNCTION__) 
#else 
#define ENTER() 
#define LEAVE() 
#endif 

#define DUMMY_FS_SECTOR_NUM    8192
#define HIDDEN_DATA_SECTOR_NUM 2048
#define RESERVED_SECTOR_NUM    (DUMMY_FS_SECTOR_NUM + HIDDEN_DATA_SECTOR_NUM)
#define USER_DEFINED_DATA_START 262144  // 256K

char *hidden_data;

int bLoop, nvtPlug=1,DeviceNull=-1; 
int usb_fd, maxLun=0;
struct sd2ms_dev_struct dev[3]; 
 struct ms_cbw_struct *nvtcbw;
struct ms_csw_struct *nvtcsw;
char *cbwbuffer;
char *cswbuffer;
char *readbuffer;
char *writebuffer;
u32 readbuffer_size;
u32 writebuffer_size;
volatile u32 sync_connted = 0;
int fd[3];
int NAND0_LUN = -1;
int NAND1_LUN = -1;
int hidden_lun[10] ={1,1,1,1,1,1,1,1,1,1};
char label_name[10][20];
int label_name_exist[10] = {0,0,0,0,0,0,0,0,0,0};
int stall_ep = 1;
u32 volatile ep_status = 0;
int volatile csw_status = 0;
u32 volatile usb_cable_status = 1;
u32 volatile usb_connection_status = 1;
#define YN(b)	(((b)==0)?"no":"yes") 
typedef unsigned short __u16;
typedef unsigned char __u8;
typedef long long LLONG64; 
  
#define NUSBD_IOC_MAGIC 'u' 
#define NUSBD_IOC_GETCBW 			_IOR(NUSBD_IOC_MAGIC, 0, char *) 
#define NUSBD_IOC_SETLUN 			_IOR(NUSBD_IOC_MAGIC, 3, char *) 
#define NUSBD_IOC_PLUG				_IO(NUSBD_IOC_MAGIC, 4)
#define NUSBD_IOC_UNPLUG			_IO(NUSBD_IOC_MAGIC, 5)
#define NUSBD_IOC_GET_CABLE_STATUS		_IOR(NUSBD_IOC_MAGIC, 8, u32 *)
#define NUSBD_IOC_USB_WRITE_BUFFER_OFFSET	_IOR(NUSBD_IOC_MAGIC, 11, u32 *)
#define NUSBD_IOC_USB_READ_BUFFER_OFFSET	_IOR(NUSBD_IOC_MAGIC, 12, u32 *)
#define NUSBD_IOC_USB_CBW_BUFFER_OFFSET		_IOR(NUSBD_IOC_MAGIC, 13, u32 *)
#define NUSBD_IOC_USB_CSW_BUFFER_OFFSET		_IOR(NUSBD_IOC_MAGIC, 14, u32 *)
#define NUSBD_IOC_USB_WRITE_BUFFER_SIZE		_IOR(NUSBD_IOC_MAGIC, 15, u32 *)
#define NUSBD_IOC_USB_READ_BUFFER_SIZE		_IOR(NUSBD_IOC_MAGIC, 16, u32 *)
#define NUSBD_IOC_USB_CBW_BUFFER_SIZE		_IOR(NUSBD_IOC_MAGIC, 17, u32 *)
#define NUSBD_IOC_USB_CSW_BUFFER_SIZE		_IOR(NUSBD_IOC_MAGIC, 18, u32 *)
#define NUSBD_IOC_PUTCSW			_IOW(NUSBD_IOC_MAGIC, 19, char *)
#define NUSBD_IOC_GETENDPOINT_STATUS		_IOR(NUSBD_IOC_MAGIC, 20, u32 *)
#define NUSBD_IOC_ENDPOINT_STALL		_IOR(NUSBD_IOC_MAGIC, 21, u32 *) 
#define NUSBD_IOC_USB_CONNECTION_STATUS		_IOR(NUSBD_IOC_MAGIC, 22, u32 *)


u32 print_count = 0;
u16 inline get_be16(u8 *buf) 
{ 
	return ((u16) buf[0] << 8) | ((u16) buf[1]); 
} 
 
u32 inline get_be32(u8 *buf) 
{ 
	return ((u32) buf[0] << 24) | ((u32) buf[1] << 16) | 
			((u32) buf[2] << 8) | ((u32) buf[3]); 
} 
 
void inline put_be16(u16 val, u8 *buf) 
{ 
	buf[0] = val >> 8; 
	buf[1] = val; 
} 


void inline put_be32(u32 val, u8 *buf) 
{ 
	buf[0] = val >> 24; 
	buf[1] = val >> 16; 
	buf[2] = val >> 8; 
	buf[3] = val; 
} 
 
int usb_get_cbw(char *buf) 
{ 
	return ioctl(usb_fd, NUSBD_IOC_GETCBW, buf); 
} 
int usb_put_csw(char *buf) 
{ 
	return ioctl(usb_fd, NUSBD_IOC_PUTCSW, buf); 
} 
int usb_set_lun(char *buf)
{ 
	return ioctl(usb_fd, NUSBD_IOC_SETLUN, buf); 
}  
int usb_write(char *buf, int count) 
{ 
	return write(usb_fd, buf, count); 
} 
 
int usb_read(char *buf, int count) 
{ 
	return read(usb_fd, buf, count); 
} 
 
int ms_init(int *fd) 
{ 
	int i;
	//ENTER(); 
 
	printf("USB Device Reader Start...\n"); 
	
	memset(nvtcbw, 0, sizeof(struct ms_cbw_struct)); 
	memset(nvtcsw, 0, sizeof(struct ms_csw_struct));  

	for (i=0; i<maxLun; i++)
	{

		memset(&dev[i], 0, sizeof(dev[i])); 
		dev[i].fd = fd[i]; 
		dev[i].nCapacityInByte = fat_llseek(fd[i], (fat_loff_t)0, SEEK_END); 
		if(dev[i].nCapacityInByte < 0){ 
			printf("Get %d Size Error\n",i); 
			return -1; 
		}
	 
		dev[i].nTotalSectors = dev[i].nCapacityInByte / SD_SECTOR_SIZE;

		dev[i].sense = 0; 
 
 		printf("USB Device %d Size : %d(MB)\n",i, dev[i].nTotalSectors /2048);	
	} 

	hidden_data = (char *)malloc(HIDDEN_DATA_SECTOR_NUM * SD_SECTOR_SIZE);

	if(hidden_data == NULL)
	{
#ifdef MASS_DEBUG
		printf("Out of memory\n");
#endif
		return -1;
	}

	memset(hidden_data, 0, HIDDEN_DATA_SECTOR_NUM * SD_SECTOR_SIZE);
	hidden_data[USER_DEFINED_DATA_START] = 1;      // Identifier
	hidden_data[USER_DEFINED_DATA_START + 1] = 0;  // Version

	for (i=maxLun; i<3; i++)
		dev[i].sense = SS_MEDIUM_NOT_PRESENT;  

	//LEAVE(); 
 
	return 0; 
} 
 
void ms_exit(void) 
{ 
	ENTER();  
  	free(hidden_data);	 
	LEAVE(); 
} 
 
 
int ms_check_cbw(struct ms_cbw_struct *cbw, int cmd_size, int dir) 
{ 
	ENTER(); 
	 
	if ( cbw->bCBWCBLength != 0 && cbw->bmCBWFlags != dir) 
	{ 
		PDEBUG("Cmd dir error. ops[%02x] dir[%d]\n", 
				cbw->CBWCB[0], dir); 
		return -1; 
	} 
 
	LEAVE(); 
 
	return 0; 
} 
 
int ms_get_data(char *buf, int count) 
{ 
	int state;
	if(count != 0) 
	{
		usb_read(buf, count); 
	}
	return 0; 
} 
 
int ms_put_data(char *buf, int count) 
{ 
 	int state = 0;

	if(count != 0)		 
	{
		 usb_write(buf, count); 
 	}
	return 0; 
} 
 
void ms_get_cbw(void) 
{ 
 	int status;

	ENTER(); 
	while(1) 
	{ 
		memset((char *)nvtcbw, 0, sizeof(struct ms_cbw_struct)); 
		usb_get_cbw((char *)nvtcbw); 
 	
		if ( nvtcbw->dCBWSignature != CBW_SIGNATURE ) 
		{ 
			PDEBUG("CBW Signature error %08x\n", nvtcbw->dCBWSignature); 
			continue; 
		} 
		//nvtPlug = 1;
		break; 
	} 
 	LEAVE(); 
} 
 
void ms_put_csw(void) 
{ //
//	if (!nvtPlug)
	//	return;
	
	nvtcsw->dCSWSignature = CSW_SIGNATURE; 
	nvtcsw->dCSWTag = nvtcbw->dCBWTag;
	usb_put_csw((char *)nvtcsw); 
} 
 
int ms_test_unit_ready(void) 
{ 
	ENTER(); 

	if (nvtcbw->bCBWLUN >= maxLun)
	{
		dev[nvtcbw->bCBWLUN].sense = SS_MEDIUM_NOT_PRESENT;
	}
	LEAVE(); 
	return 0; 
} 
 
int ms_inquiry(void) 
{ 
	char *buf = writebuffer; /* Write to USB */ 	
	int val;

	ENTER(); 
 
	static char vendor_id[] = "Nuvoton"; 
	static char product_id[] = "USB Device Reader "; 
	static char release_id[]="1.00"; 
 
	if(DeviceNull == nvtcbw->bCBWLUN)
	{
		strcpy(product_id, "Null");
	}
	else if (nvtcbw->bCBWLUN == NAND0_LUN)
	{	
		strcpy(product_id, "nand1-1");
	}
	else if (nvtcbw->bCBWLUN == NAND1_LUN)
	{
		strcpy(product_id, "nand1-2");
	}
	else
	{	strcpy(product_id, "sd");
	}

	memset(buf, 0, 36); 
	buf[1] = 0x80;	/* removable */ 	
	buf[2] = 0;		// ANSI SCSI level 2 
	buf[3] = 1;		// SCSI-2 INQUIRY data format 
	buf[4] = 0x1f;		// Additional length 
				// No special options 

	sprintf(buf + 8, "%-8s%-16s%-4s", vendor_id, product_id, release_id); 
 
 
	if (nvtcbw->dCBWDataTransferLength <= 36)
		val = nvtcbw->dCBWDataTransferLength;
	else
		val = 36; 
	LEAVE(); 
	return val; 
} 
 
int ms_mode_select(void) 
{ 
	ENTER(); 
 
	dev[nvtcbw->bCBWLUN].sense = SS_INVALID_COMMAND;  	

	LEAVE(); 
 
	return -1; 
} 
 
int ms_mode_sense(void) 
{ 
	int ret = 4, i=0; 
	struct ms_cbw_struct *cbw = nvtcbw; 
	char * buf = writebuffer; 	/* Write to USB */
  	int fCntlBits = fcntl (dev[nvtcbw->bCBWLUN].fd, F_GETFL);
	int writeProtect = 0;

	ENTER(); 
 
	memset(buf, 0, 8); 
 
	 // Must return write-protect here if the device is actually write-protect.
	if ((fCntlBits != -1) && ((fCntlBits & O_ACCMODE) == O_RDONLY)) {
		writeProtect = 1;
		printf("Write Protect\n");
	}
	if(DeviceNull == nvtcbw->bCBWLUN)
	{
		writeProtect = 1;
	}
	if ( cbw->CBWCB[0] == SC_MODE_SENSE_6 ) { 
		buf[0] = 0x03; 
		buf[1] = i; 
		buf[2] = (writeProtect == 1) ? 0x80 : 0x00;
		if (nvtcbw->dCBWDataTransferLength <= 4)
			ret = nvtcbw->dCBWDataTransferLength;
		else
			ret = 4;
	} 
	else 
	{ 
		buf[1] = 0x06; 
		buf[2] = i; 
		buf[3] = (writeProtect == 1) ? 0x80 : 0x00;
		if (nvtcbw->dCBWDataTransferLength <= 8)
			ret = nvtcbw->dCBWDataTransferLength;
		else
			ret = 8;
	} 
 
	LEAVE(); 
 
	return ret; 
} 
 
 
int ms_prevent_allow_medium_removal(void) 
{ 
	int retval = 0; 
 
	ENTER(); 
 
	if ( nvtcbw->CBWCB[4] & 0x01 ) 
	{ 
		dev[nvtcbw->bCBWLUN].sense = SS_INVALID_COMMAND;		
		nvtcsw->bCSWStatus = CMD_FAILED; 
	} 
	else 
		dev[nvtcbw->bCBWLUN].sense = SS_NO_SENSE; 
 
	LEAVE(); 
 
	return retval; 
} 
 
 
int ms_sd_read(void) 
{ 
	int status=0;
	unsigned int count, lba, bFailed, length; 
	char *cmd = (char *)nvtcbw->CBWCB; 
 	char *buffer;

	int i=0;
	unsigned int trancount;
	char *diskImageBuf = NULL;

	ENTER(); 
	bFailed = 0; 
	if ( cmd[0] == SC_READ_6) 
	{ 
		lba = ((cmd[1] & 0x1f) << 16) + get_be16(&cmd[2]); 
		count = (cmd[4] & 0xff); 
	} 
	else if (cmd[0] == SC_READ_10)
	{ 
		lba = get_be32(&cmd[2]); 
		count = get_be16(&cmd[7]); 
	} 
	else if (cmd[0] == SC_READ_12)
	{
		lba = get_be32(&cmd[2]); 
		count = get_be32(&cmd[6]); 
	}
	else
		goto out;
 	if (nvtcbw->bCBWLUN >= maxLun)
		goto out;
#ifdef Hidden_Driver_Letter
	if(1)
#else
	if(DeviceNull == nvtcbw->bCBWLUN)
#endif
	{
		if (nvtcbw->dCBWDataTransferLength > count * SD_SECTOR_SIZE)
			trancount = count;
		else
			trancount = nvtcbw->dCBWDataTransferLength / SD_SECTOR_SIZE;

		diskImageBuf = writebuffer;	/* Write to USB */

#ifdef Hidden_Driver_Letter
	if (hidden_lun[nvtcbw->bCBWLUN])
	{
#endif
		if (lba < DUMMY_FS_SECTOR_NUM)
		{	
			memset (diskImageBuf, 0, SD_SECTOR_SIZE * trancount);
			
			for (i = 0; i < trancount; i++)
			{
				int sectorIdx = lba + i;  
				if (sectorIdx == 0)  // MBR
			        {  
			        	static const char s_firstPartEntry[] = {
		        			0x80, 
						0x00, 0x00, 0x00, 
						/* DOS FAT system */
						0x0E,
						0x00, 0x00, 0x00,
        					/* Number of sectors between the MBR and the first sector in the partition */
						0x40, 0x00,
						0x00, 0x00, 
						/* NUmber sector in the partition */
						0x4F, 0x00,
						0x00, 0x00
			        	};
			        	memcpy (diskImageBuf + SD_SECTOR_SIZE * i + 0x1BE, s_firstPartEntry, sizeof (s_firstPartEntry));
			         	*((unsigned short *) (diskImageBuf + SD_SECTOR_SIZE * i + 0x1FE)) = 0xAA55;
			        }
			        else if (sectorIdx == 64)  // FAT BOOT SECTOR
			        {        
					static const char s_pbrHeader[] = {
					        0xEB, 0x3C, 0x90, 0x4D, 0x53, 0x57, 0x49, 0x4E,
						0x34, 0x2E, 0x31, 0x00, 0x02, 0x01, 0x01, 0x00,
						/* Byte 16 */
						0x02, 
						/* Byte 17 & 18 */
						0xE0, 0x00, 
						/* Byte 19 & 20 (number of sector IN THE FILE SYSTEM) */
						0x4F, 0x00,
						/* Removable */
						0xF0,
						/* Size of each FAT */
						0x20, 0x00, 
						/* Sector per Track */
						0x00, 0x00, 
						/* Number of heads */
						0x00, 0x00,
						/* Hidden sector */
						0x00, 0x00, 0x00, 0x00,
						/* Large sector */	
						0x00, 0x00, 0x00, 0x00,
						/* Physical Disk Number */
						0x80, 
						/* Reserved */
						0x00, 
						/* Sig */
						0x29,
						0x12, 0x34, 0x56, 0x78, 
						'n',   'u', 'v', 'o', 't', 'o', 'n', 
						0x20, 0x20, 0x20, 0x20,  0x46, 0x41, 
						0x54, 0x31, 0x36, 0x20, 0x20, 0x20, 0x00, 0x00
					};
	
        				memcpy (diskImageBuf + SD_SECTOR_SIZE * i, s_pbrHeader, sizeof (s_pbrHeader));
					*((unsigned short *) (diskImageBuf + SD_SECTOR_SIZE * i + 0x1FE)) = 0xAA55;
			        }
				else if (sectorIdx >= 65 && sectorIdx <= 96)  // FAT TABLE
			        {
			          	int j;

					const unsigned int TWO_BAD_CLUSTER_MARK = 0xFFF7FFF7;
					unsigned int *twoBadClusterMark = (unsigned int *) (diskImageBuf + SD_SECTOR_SIZE * i);
					for (j = 0; j < SD_SECTOR_SIZE; j += 4) {
						*twoBadClusterMark ++ = TWO_BAD_CLUSTER_MARK;
					}

			          	if (sectorIdx == 5)
			          	{
			          		*((unsigned int *) (diskImageBuf + SD_SECTOR_SIZE * i + 0)) = 0xFFFFFFF0;
			          	}
				}
        			else
				{	
					 char s_pbrHeader[] = {
					        'n', 'u', 'v', 'o', 'T', 'o', 'n', 0x20, 0x20, 0x20, 0x20, 0x08,
						0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x6b,0x86,0xb1,0x40,0x00,0x00
					};
					if(label_name_exist[nvtcbw->bCBWLUN])
						memcpy ((char *)s_pbrHeader, (char *)label_name[nvtcbw->bCBWLUN], 11);
        				memcpy (diskImageBuf + SD_SECTOR_SIZE * i, s_pbrHeader, sizeof (s_pbrHeader));
				}
			}
      
			ms_put_data (diskImageBuf, SD_SECTOR_SIZE * trancount);
			goto out;
		}
		else if (lba < RESERVED_SECTOR_NUM)
    		{
			for (i = 0; i < count; i++)
			{
			        int sectorIdx = lba - DUMMY_FS_SECTOR_NUM + i;

				buffer = writebuffer;	/* Write to USB */

				memcpy(buffer, hidden_data + sectorIdx * SD_SECTOR_SIZE, SD_SECTOR_SIZE);
	
			        ms_put_data(buffer, SD_SECTOR_SIZE);

		      }

		      goto out;
		}
#ifdef Hidden_Driver_Letter
		else
		{
			if(sync_connted == 0)
			{
				sync_connted = 1;	
				system("touch /var/massSync.lock");
			}
			lba -= RESERVED_SECTOR_NUM;

		}
	}
#endif


		if ( lba > (dev[nvtcbw->bCBWLUN].nTotalSectors + RESERVED_SECTOR_NUM) || (lba + count) > (dev[nvtcbw->bCBWLUN].nTotalSectors+ RESERVED_SECTOR_NUM)) 
		{ 
			dev[nvtcbw->bCBWLUN].sense = SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE; 
			printf("ms_sd_read: SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE\n"); 
			bFailed = 1; 
		} 
	}
	else
	{
		if ( lba > dev[nvtcbw->bCBWLUN].nTotalSectors || (lba + count) > dev[nvtcbw->bCBWLUN].nTotalSectors) 
		{ 
			dev[nvtcbw->bCBWLUN].sense = SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE; 
			printf("ms_sd_read: SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE\n"); 
			bFailed = 1; 
		} 
	}
 
	if (bFailed)//lzxu 
		goto out; 
 
	if(fat_llseek(dev[nvtcbw->bCBWLUN].fd, ((fat_loff_t)lba) * SD_SECTOR_SIZE, SEEK_SET) < 0) 
		printf("ms_sd_read: fat_llseek error\n"); 

	buffer = writebuffer;	/* Write to USB */
	
 	length = count * SD_SECTOR_SIZE;

	while(length > writebuffer_size)
	{
		if( !bFailed) 
		{
			bFailed = read(dev[nvtcbw->bCBWLUN].fd, buffer, writebuffer_size) < 0 ? 1: 0; 
		}
 		
		ms_put_data(buffer, writebuffer_size);
		
		length -= writebuffer_size;
	}
	if(length > 0)
	{
		if( !bFailed) 
			bFailed = read(dev[nvtcbw->bCBWLUN].fd, buffer, length) < 0 ? 1: 0; 

		ms_put_data(buffer, length);  	
	}
out: 
	if (bFailed) 
	{ 
		nvtcsw->bCSWStatus = CMD_FAILED; 
		printf("ms_sd_read error\n"); 
	} 
 
	LEAVE(); 
	return 0; 
} 
 
 /******************/
int ms_sd_write(void) 
{ 
	volatile unsigned int count, lba, bFailed, length; 
	char *cmd = (char *)nvtcbw->CBWCB; 
 	char *buffer;
	ENTER(); 
 
 #ifdef Hidden_Driver_Letter
	volatile unsigned int bDummyWrite;
	volatile unsigned int trancount;
#endif

	bFailed = 0; 
 
	if ( cmd[0] == SC_WRITE_6) 
	{ 
		lba = ((cmd[1] & 0x1f) << 16) + get_be16(&cmd[2]); 
		count = (cmd[4] & 0xff); 
	} 
	else
	{ 
		lba = get_be32(&cmd[2]); 
		count = get_be16(&cmd[7]); 
	} 

#ifdef Hidden_Driver_Letter
	if ( lba > (dev[nvtcbw->bCBWLUN].nTotalSectors + RESERVED_SECTOR_NUM) || (lba + count) > (dev[nvtcbw->bCBWLUN].nTotalSectors + RESERVED_SECTOR_NUM)) 
	{ 
		dev[nvtcbw->bCBWLUN].sense = SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE; 
		printf("ms_sd_write: SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE\n"); 
		bFailed = 1; 
	}  
#else
	if ( lba > dev[nvtcbw->bCBWLUN].nTotalSectors || (lba + count) > dev[nvtcbw->bCBWLUN].nTotalSectors) 
	{ 
		dev[nvtcbw->bCBWLUN].sense = SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE; 
		printf("ms_sd_write: SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE\n"); 
		bFailed = 1; 
	} 
#endif 
	if (bFailed)//lzxu 
		goto out; 
 
#ifdef Hidden_Driver_Letter
	if (nvtcbw->dCBWDataTransferLength > count * SD_SECTOR_SIZE)
		trancount = count;
	else
		trancount = nvtcbw->dCBWDataTransferLength / SD_SECTOR_SIZE;

	if (hidden_lun[nvtcbw->bCBWLUN])
	{
		if (lba < RESERVED_SECTOR_NUM)
		{
			bDummyWrite = 1;
		}
		else
		{
			bDummyWrite = 0;
			lba -= RESERVED_SECTOR_NUM;
		}
	}
	else
	{
		bDummyWrite = 0;
	}
#endif 

	if(fat_llseek(dev[nvtcbw->bCBWLUN].fd, ((fat_loff_t)lba) * SD_SECTOR_SIZE, SEEK_SET) < 0) 
		printf("ms_sd_write: fat_llseek error\n"); 
 
	buffer = readbuffer;	/* Read from USB */

 	length = count * SD_SECTOR_SIZE;

	//printf("Write Lba 0x%x Len 0x%X\n",lba, length);
	while(length > readbuffer_size)
	{ 
		ms_get_data(buffer, readbuffer_size); 	

#ifdef Hidden_Driver_Letter
    		if(!bDummyWrite && !bFailed)
#else
  		if(!bFailed)
#endif
			bFailed = write(dev[nvtcbw->bCBWLUN].fd, buffer, readbuffer_size) < 0 ? 1: 0; 

		length -= readbuffer_size;

	} 
	if(length > 0)
	{
		ms_get_data(buffer, length); 		

#ifdef Hidden_Driver_Letter
    		if(!bDummyWrite && !bFailed)
#else
  		if(!bFailed)
#endif
			bFailed = write(dev[nvtcbw->bCBWLUN].fd, buffer, length) < 0 ? 1: 0; 
	}

out: 
	if (bFailed) 
	{ 
		printf("ms_sd_write error\n"); 
		nvtcsw->bCSWStatus = CMD_FAILED; 
	} 
 
	LEAVE(); 
 
	return 0;
} 
 
int ms_read_capacity(void) 
{ 
	u8 *cmd = (unsigned char *)nvtcbw->CBWCB; 
	u8 *buf = (u8*)writebuffer; /* Write to USB */
	int lba = get_be32(&cmd[2]); 
	int pmi = cmd[8]; 
 
	ENTER(); 
 	
	/* Check the PMI and LBA fields */ 
	if (pmi > 1 || (pmi == 0 && lba != 0)) 
	{ 
		dev[nvtcbw->bCBWLUN].sense = SS_INVALID_FIELD_IN_CDB; 
		return -1; 
	} 
#ifdef Hidden_Driver_Letter
	if (hidden_lun[nvtcbw->bCBWLUN])
#else
 	if(DeviceNull == nvtcbw->bCBWLUN)
#endif
	{
		put_be32(RESERVED_SECTOR_NUM + dev[nvtcbw->bCBWLUN].nTotalSectors - 1, &buf[0]); // Max logical block
	}
	else
	{
	 	if(dev[nvtcbw->bCBWLUN].nTotalSectors > 0)
			put_be32(dev[nvtcbw->bCBWLUN].nTotalSectors - 1, &buf[0]);	// Max logical block 
		else
			put_be32(dev[nvtcbw->bCBWLUN].nTotalSectors, &buf[0]);	// Max logical block 
	}
	put_be32(SD_SECTOR_SIZE, &buf[4]);				// Block length 
 
	LEAVE(); 
 
	return 8; 
} 
 
static int ms_read_format_capacities(void) 
{ 
	int i; 
	u32 gTotalSectors; 
	char *buf; 
 	if(hidden_lun[nvtcbw->bCBWLUN])
		gTotalSectors = 0;
	else
		gTotalSectors = dev[nvtcbw->bCBWLUN].nTotalSectors;
	buf = writebuffer;

	ENTER(); 
 
	for (i = 0 ; i < 36 ; i++) 
       	buf[i] = 0; 
 
	buf[3] = 0x10; 
	buf[4] = (gTotalSectors >> 24) & 0xff; 
	buf[5] = (gTotalSectors >> 16) & 0xff; 
	buf[6] = (gTotalSectors >> 8) & 0xff; 
	buf[7] = (gTotalSectors) & 0xff; 
	buf[8] = 0x02; 
	buf[10] = 0x02; 
 
	buf[12] =  (gTotalSectors >> 24) & 0xff; 
	buf[13] = (gTotalSectors >> 16) & 0xff; 
	buf[14] = (gTotalSectors >> 8) & 0xff; 
	buf[15] = (gTotalSectors) & 0xff; 
 
	buf[18] = 0x02; 
 
	LEAVE(); 
 
	return 36; 
} 
 
static int	ms_request_sense(void) 
{ 
	char *buf; 
	int sd,val; 
 
	ENTER(); 

	buf = writebuffer; /* Write to USB */
	//if (!(DeviceNull == nvtcbw->bCBWLUN)) 
		sd = dev[nvtcbw->bCBWLUN].sense;
	//else
	//	sd = SS_MEDIUM_NOT_PRESENT;// dev[nvtcbw->bCBWLUN].sense;

	memset(buf, 0, 18); 
 	//if (!(DeviceNull == nvtcbw->bCBWLUN)) 
	//	buf[0] = 0xF0;			// current error 
	//else
		buf[0] = 0x70;			// current error 
		
	buf[2] = SK(sd); 
	buf[7] = 0x0a;			// Additional sense length 
	buf[12] = ASC(sd); 
	buf[13] = ASCQ(sd); 
 
	LEAVE(); 
 
	if (nvtcbw->dCBWDataTransferLength <= 18)
		val = nvtcbw->dCBWDataTransferLength;
	else
		val = 18;

	return val; 
} 
 
static int ms_start_stop_unit(void) 
{ 
	int retval = 0; 
 
	ENTER(); 
 
	LEAVE(); 
 
	return retval; 
} 
 
void ms_process_cbw(void) 
{ 
	struct ms_cbw_struct *cbw = nvtcbw; 
	struct ms_csw_struct *csw = nvtcsw; 
	char *cmd = (char *)cbw->CBWCB; 
	int retval = -1, status=0; 
 
	csw->bCSWStatus = CMD_PASSED; 
	csw->dCSWDataResidue = 0; 

	switch(cmd[0]) { 
 
		case SC_INQUIRY: 
			PDEBUG("SC_INQUIRY\n"); 
			if ( ! ms_check_cbw(cbw, 6, DIR_IN) )  
			{
				retval = ms_inquiry();
				csw_status = 0;
			}
			break; 
 
		case SC_MODE_SELECT_6: 
			PDEBUG("SC_MODE_SELECT_6\n"); 
			if ( ! ms_check_cbw(cbw, 6, DIR_OUT)) 
			{
				if (!(DeviceNull == nvtcbw->bCBWLUN))
					retval = ms_mode_select(); 
				else
				{
					csw_status = 1;
					stall_ep = 1;
					ioctl(usb_fd, NUSBD_IOC_ENDPOINT_STALL, &stall_ep); 
					print_count = 0;				
					while(1)
					{
						ioctl(usb_fd, NUSBD_IOC_GETENDPOINT_STATUS, &ep_status); 
						if(!(ep_status & 0x02))
							break;
						print_count++;
						if(print_count > USBD_PRINTF_COUNT)
						{
							if((print_count % USBD_PRINTF_COUNT)  == 0 )
								printf("NUSBD_IOC_GETENDPOINT_STATUS\n");
						}
						ioctl(usb_fd, NUSBD_IOC_GET_CABLE_STATUS, &usb_cable_status); 
						if(usb_cable_status == 0)
						{
							printf("USBD Un-Plug to Stop mass\n");
							break;
						}	
						ioctl(usb_fd, NUSBD_IOC_USB_CONNECTION_STATUS, &usb_connection_status); 
						if(usb_connection_status == 0)
						{
							printf("Host Eject to Stop mass\n");
							break;
						}							
					}
				}
			}
			break; 
			 
		case SC_MODE_SELECT_10: 
			PDEBUG("SC_MODE_SELECT_10\n"); 
			if ( ! ms_check_cbw(cbw, 6, DIR_OUT)) 
			{
				if (!(DeviceNull == nvtcbw->bCBWLUN))
					retval = ms_mode_select(); 
				else
				{
					csw_status = 1;
					stall_ep = 1;
					ioctl(usb_fd, NUSBD_IOC_ENDPOINT_STALL, &stall_ep); 
					print_count = 0;			
					while(1)
					{
						ioctl(usb_fd, NUSBD_IOC_GETENDPOINT_STATUS, &ep_status); 
						if(!(ep_status & 0x02))
							break;
						print_count++;
						if(print_count > USBD_PRINTF_COUNT)
						{
							if((print_count % USBD_PRINTF_COUNT)  == 0 )
								printf("NUSBD_IOC_GETENDPOINT_STATUS\n");
						}
						ioctl(usb_fd, NUSBD_IOC_GET_CABLE_STATUS, &usb_cable_status); 
						if(usb_cable_status == 0)
						{
							printf("USBD Un-Plug to Stop mass\n");
							break;
						}
						ioctl(usb_fd, NUSBD_IOC_USB_CONNECTION_STATUS, &usb_connection_status); 
						if(usb_connection_status == 0)
						{
							printf("Host Eject to Stop mass\n");
							break;
						}	
					}
				}
			}
			break; 
 
		case SC_MODE_SENSE_6: 
			PDEBUG("SC_MODE_SENSE_6\n"); 
			if ( ! ms_check_cbw(cbw, 6, DIR_IN) ) 
			{
				if (!(DeviceNull == nvtcbw->bCBWLUN))
					retval = ms_mode_sense(); 
				else
				{
					csw_status = 1;
					stall_ep = 1;
					ioctl(usb_fd, NUSBD_IOC_ENDPOINT_STALL, &stall_ep); 
					print_count = 0;				
					while(1)
					{
						ioctl(usb_fd, NUSBD_IOC_GETENDPOINT_STATUS, &ep_status); 
						if(!(ep_status & 0x02))
							break;
						print_count++;
						if(print_count > USBD_PRINTF_COUNT)
						{
							if((print_count % USBD_PRINTF_COUNT)  == 0 )
								printf("NUSBD_IOC_GETENDPOINT_STATUS\n");
						}
						ioctl(usb_fd, NUSBD_IOC_GET_CABLE_STATUS, &usb_cable_status); 
						if(usb_cable_status == 0)
						{
							printf("USBD Un-Plug to Stop mass\n");
							break;
						}
						ioctl(usb_fd, NUSBD_IOC_USB_CONNECTION_STATUS, &usb_connection_status); 
						if(usb_connection_status == 0)
						{
							printf("Host Eject to Stop mass\n");
							break;
						}	
					}
				}
			}
			break; 
				 
		case SC_MODE_SENSE_10: 
			PDEBUG("SC_MODE_SENSE_10\n"); 
			if ( ! ms_check_cbw(cbw, 10, DIR_IN) ) 
			{
				if (!(DeviceNull == nvtcbw->bCBWLUN))
					retval = ms_mode_sense(); 
				else
				{
					csw_status = 1;
					stall_ep = 1;
					ioctl(usb_fd, NUSBD_IOC_ENDPOINT_STALL, &stall_ep); 
					print_count = 0;			
					while(1)
					{
						ioctl(usb_fd, NUSBD_IOC_GETENDPOINT_STATUS, &ep_status); 
						if(!(ep_status & 0x02))
							break;
						print_count++;
						if(print_count > USBD_PRINTF_COUNT)
						{
							if((print_count % USBD_PRINTF_COUNT)  == 0 )
								printf("NUSBD_IOC_GETENDPOINT_STATUS\n");
						}
						ioctl(usb_fd, NUSBD_IOC_GET_CABLE_STATUS, &usb_cable_status); 
						if(usb_cable_status == 0)
						{
							printf("USBD Un-Plug to Stop mass\n");
							break;
						}	
						ioctl(usb_fd, NUSBD_IOC_USB_CONNECTION_STATUS, &usb_connection_status); 
						if(usb_connection_status == 0)
						{
							printf("Host Eject to Stop mass\n");
							break;
						}	
					}
				}
			}
			break; 
 
		case SC_PREVENT_ALLOW_MEDIUM_REMOVAL: 
			PDEBUG("SC_PREVENT_ALLOW_MEDIUM_REMOVAL\n"); 
			if ( ! ms_check_cbw(cbw, 6, DIR_OUT) )  
				retval = ms_prevent_allow_medium_removal(); 
			break; 
 
		case SC_READ_6: 
			PDEBUG("SC_READ6\n"); 
			if ( ! ms_check_cbw(cbw, 6, DIR_IN)) 
			{
				if (!(DeviceNull == nvtcbw->bCBWLUN))
					retval = ms_sd_read();
				else
				{
					csw_status = 1;
					stall_ep = 1;
					ioctl(usb_fd, NUSBD_IOC_ENDPOINT_STALL, &stall_ep); 
					print_count = 0;				
					while(1)
					{
						ioctl(usb_fd, NUSBD_IOC_GETENDPOINT_STATUS, &ep_status); 
						if(!(ep_status & 0x02))
							break;
						print_count++;
						if(print_count > USBD_PRINTF_COUNT)
						{
							if((print_count % USBD_PRINTF_COUNT)  == 0 )
								printf("NUSBD_IOC_GETENDPOINT_STATUS\n");
						}
						ioctl(usb_fd, NUSBD_IOC_GET_CABLE_STATUS, &usb_cable_status); 
						if(usb_cable_status == 0)
						{
							printf("USBD Un-Plug to Stop mass\n");
							break;
						}
						ioctl(usb_fd, NUSBD_IOC_USB_CONNECTION_STATUS, &usb_connection_status); 
						if(usb_connection_status == 0)
						{
							printf("Host Eject to Stop mass\n");
							break;
						}	
					}
				}
			}
			break; 
 
		case SC_READ_10: 
		case SC_READ_12: 
			PDEBUG("SC_READ10/12\n"); 	
			if ( ! ms_check_cbw(cbw, 10, DIR_IN)) 
			{
				if (!(DeviceNull == nvtcbw->bCBWLUN))
					retval = ms_sd_read();
				else
				{
					csw_status = 1;
					stall_ep = 1;
					ioctl(usb_fd, NUSBD_IOC_ENDPOINT_STALL, &stall_ep); 
					print_count = 0;			
					while(1)
					{
						ioctl(usb_fd, NUSBD_IOC_GETENDPOINT_STATUS, &ep_status); 
						if(!(ep_status & 0x02))
							break;
						print_count++;
						if(print_count > USBD_PRINTF_COUNT)
						{
							if((print_count % USBD_PRINTF_COUNT)  == 0 )
								printf("NUSBD_IOC_GETENDPOINT_STATUS\n");
						}
						ioctl(usb_fd, NUSBD_IOC_GET_CABLE_STATUS, &usb_cable_status); 
						if(usb_cable_status == 0)
						{
							printf("USBD Un-Plug to Stop mass\n");
							break;
						}
						ioctl(usb_fd, NUSBD_IOC_USB_CONNECTION_STATUS, &usb_connection_status); 
						if(usb_connection_status == 0)
						{
							printf("Host Eject to Stop mass\n");
							break;
						}	
					}
				}
			}
			break; 
 
		case SC_WRITE_6: 
			if ( ! ms_check_cbw(cbw, 6, DIR_OUT)) 
			{	
				if (!(DeviceNull == nvtcbw->bCBWLUN))
					retval = ms_sd_write(); 
				else
				{
					csw_status = 1;
					stall_ep = 1;
					ioctl(usb_fd, NUSBD_IOC_ENDPOINT_STALL, &stall_ep); 
					print_count = 0;			
					while(1)
					{
						ioctl(usb_fd, NUSBD_IOC_GETENDPOINT_STATUS, &ep_status); 
						if(!(ep_status & 0x02))
							break;
						print_count++;
						if(print_count > USBD_PRINTF_COUNT)
						{
							if((print_count % USBD_PRINTF_COUNT)  == 0 )
								printf("NUSBD_IOC_GETENDPOINT_STATUS\n");
						}
						ioctl(usb_fd, NUSBD_IOC_GET_CABLE_STATUS, &usb_cable_status); 
						if(usb_cable_status == 0)
						{
							printf("USBD Un-Plug to Stop mass\n");
							break;
						}
						ioctl(usb_fd, NUSBD_IOC_USB_CONNECTION_STATUS, &usb_connection_status); 
						if(usb_connection_status == 0)
						{
							printf("Host Eject to Stop mass\n");
							break;
						}	
					}
				}
			}
			break; 
 
		case SC_WRITE_10: 
		case SC_WRITE_12: 
			if ( ! ms_check_cbw(cbw, 10, DIR_OUT)) 
			{
				if (!(DeviceNull == nvtcbw->bCBWLUN))
					retval = ms_sd_write(); 
				else
				{
					csw_status = 1;
					stall_ep = 1;
					ioctl(usb_fd, NUSBD_IOC_ENDPOINT_STALL, &stall_ep); 
					print_count = 0;			
					while(1)
					{
						ioctl(usb_fd, NUSBD_IOC_GETENDPOINT_STATUS, &ep_status); 
						if(!(ep_status & 0x02))
							break;
						print_count++;
						if(print_count > USBD_PRINTF_COUNT)
						{
							if((print_count % USBD_PRINTF_COUNT)  == 0 )
								printf("NUSBD_IOC_GETENDPOINT_STATUS\n");
						}
						ioctl(usb_fd, NUSBD_IOC_GET_CABLE_STATUS, &usb_cable_status); 
						if(usb_cable_status == 0)
						{
							printf("USBD Un-Plug to Stop mass\n");
							break;
						}
						ioctl(usb_fd, NUSBD_IOC_USB_CONNECTION_STATUS, &usb_connection_status); 
						if(usb_connection_status == 0)
						{
							printf("Host Eject to Stop mass\n");
							break;
						}	
					}
				}
			}
			break; 
			 
		case SC_READ_CAPACITY: 
			PDEBUG("SC_READ_CAPACITY\n"); 
			if ( ! ms_check_cbw(cbw, 10, DIR_IN)) 
			{
				if (!(DeviceNull == nvtcbw->bCBWLUN))
					retval = ms_read_capacity(); 
				else
				{
					csw_status = 1;
					stall_ep = 1;
					ioctl(usb_fd, NUSBD_IOC_ENDPOINT_STALL, &stall_ep); 
					print_count = 0;				
					while(1)
					{
						ioctl(usb_fd, NUSBD_IOC_GETENDPOINT_STATUS, &ep_status); 
						if(!(ep_status & 0x02))
							break;
						print_count++;
						if(print_count > USBD_PRINTF_COUNT)
						{
							if((print_count % USBD_PRINTF_COUNT)  == 0 )
								printf("NUSBD_IOC_GETENDPOINT_STATUS\n");
						}
						ioctl(usb_fd, NUSBD_IOC_GET_CABLE_STATUS, &usb_cable_status); 
						if(usb_cable_status == 0)
						{
							printf("USBD Un-Plug to Stop mass\n");
							break;
						}	
						ioctl(usb_fd, NUSBD_IOC_USB_CONNECTION_STATUS, &usb_connection_status); 
						if(usb_connection_status == 0)
						{
							printf("Host Eject to Stop mass\n");
							break;
						}	
					}	
				}
				
			}
			break; 
 
		case SC_READ_FORMAT_CAPACITIES: 
			PDEBUG("SC_READ_FORMAT_CAPACITIES\n"); 
			if ( ! ms_check_cbw(cbw, 10, DIR_IN)) 
			{
				if (!(DeviceNull == nvtcbw->bCBWLUN))
					retval = ms_read_format_capacities(); 
				else
				{
					csw_status = 1;
					stall_ep = 1;
					ioctl(usb_fd, NUSBD_IOC_ENDPOINT_STALL, &stall_ep); 
					print_count = 0;			
					while(1)
					{
						ioctl(usb_fd, NUSBD_IOC_GETENDPOINT_STATUS, &ep_status); 
						if(!(ep_status & 0x02))
							break;
						print_count++;
						if(print_count > USBD_PRINTF_COUNT)
						{
							if((print_count % USBD_PRINTF_COUNT)  == 0 )
								printf("NUSBD_IOC_GETENDPOINT_STATUS\n");
						}
						ioctl(usb_fd, NUSBD_IOC_GET_CABLE_STATUS, &usb_cable_status); 
						if(usb_cable_status == 0)
						{
							printf("USBD Un-Plug to Stop mass\n");
							break;
						}
						ioctl(usb_fd, NUSBD_IOC_USB_CONNECTION_STATUS, &usb_connection_status); 
						if(usb_connection_status == 0)
						{
							printf("Host Eject to Stop mass\n");
							break;
						}	
					}
				}
			}
			break; 
 
		case SC_REQUEST_SENSE: 
			PDEBUG("SC_REQUEST_SENSE\n"); 
			if ( ! ms_check_cbw(cbw, 6, DIR_IN)) 
			{
				csw_status = 0;
				retval = ms_request_sense(); 
			}
			break; 
 
		case SC_START_STOP_UNIT: 
			PDEBUG("SC_START_STOP_UNIT\n"); 
			if ( ! ms_check_cbw(cbw, 6, DIR_OUT)) 
			{
				if (!(DeviceNull == nvtcbw->bCBWLUN))
					retval = ms_start_stop_unit(); 
				else
				{
					csw_status = 1;
					stall_ep = 1;
					ioctl(usb_fd, NUSBD_IOC_ENDPOINT_STALL, &stall_ep); 
					print_count = 0;				
					while(1)
					{
						ioctl(usb_fd, NUSBD_IOC_GETENDPOINT_STATUS, &ep_status); 
						if(!(ep_status & 0x02))
							break;
						print_count++;
						if(print_count > USBD_PRINTF_COUNT)
						{
							if((print_count % USBD_PRINTF_COUNT)  == 0 )
								printf("NUSBD_IOC_GETENDPOINT_STATUS\n");
						}
						ioctl(usb_fd, NUSBD_IOC_GET_CABLE_STATUS, &usb_cable_status); 
						if(usb_cable_status == 0)
						{
							printf("USBD Un-Plug to Stop mass\n");
							break;
						}
						ioctl(usb_fd, NUSBD_IOC_USB_CONNECTION_STATUS, &usb_connection_status); 
						if(usb_connection_status == 0)
						{
							printf("Host Eject to Stop mass\n");
							break;
						}	
					}
				}
			}
			break; 
 
		case SC_TEST_UNIT_READY: 
			//if ( ! ms_check_cbw(cbw, 6, DIR_OUT)) 
				retval = ms_test_unit_ready(); 
			 
			break; 
 
		case SC_FORMAT_UNIT: 
		case SC_VERIFY: 
		case SC_RELEASE: 
		case SC_RESERVE: 
		case SC_SEND_DIAGNOSTIC: 
		default: 
			PDEBUG("\nSD Command : <<%02x>>\n", cmd[0] & 0xff); 
			retval = 0; 
			break; 
	} 
 	if(!csw_status)
	{
		if(retval < 0) 
		{ 
			csw->dCSWDataResidue = cbw->dCBWDataTransferLength; 
			csw->bCSWStatus = CMD_FAILED; 
			if(cbw->bmCBWFlags == DIR_IN) 
				ms_put_data(writebuffer, cbw->dCBWDataTransferLength); 
		} 
		else 
		{ 
			if(cbw->bmCBWFlags == DIR_IN) 
				ms_put_data(writebuffer ,retval); 

			csw->dCSWDataResidue = 0;//cbw->dCBWDataTransferLength - retval; 
		} 
	}


} 
 
void usb_get_buffer_info(void)
{
	u32 buffer,offset;
	unsigned char *pUsbBuffer = NULL;
	u32 cbwbuffer_size;
	u32 cswbuffer_size;
	/* Get USB Buffer size for Read */	
	ioctl(usb_fd, NUSBD_IOC_USB_READ_BUFFER_SIZE, &readbuffer_size); 

	/* Get USB Buffer size for Write */	
	ioctl(usb_fd, NUSBD_IOC_USB_WRITE_BUFFER_SIZE, &writebuffer_size); 

	/* Get USB Buffer size for CBW */	
	ioctl(usb_fd, NUSBD_IOC_USB_CBW_BUFFER_SIZE, &cbwbuffer_size); 	

	/* Get USB Buffer size for CSW */	
	ioctl(usb_fd, NUSBD_IOC_USB_CSW_BUFFER_SIZE, &cswbuffer_size); 	

	pUsbBuffer = mmap(NULL, (writebuffer_size+readbuffer_size+cbwbuffer_size+cswbuffer_size), PROT_READ|PROT_WRITE, MAP_SHARED, usb_fd, 0);
	/* Get USB Buffer Offset for Read */
	ioctl(usb_fd, NUSBD_IOC_USB_READ_BUFFER_OFFSET, &offset); 
	readbuffer = (char *) (pUsbBuffer + offset);
	
	/* Get USB Buffer Offset for Write */
	ioctl(usb_fd, NUSBD_IOC_USB_WRITE_BUFFER_OFFSET, &offset); 
	writebuffer = (char *) (pUsbBuffer + offset);

	/* Get USB Buffer Offset for CBW */
	ioctl(usb_fd, NUSBD_IOC_USB_CBW_BUFFER_OFFSET, &offset);
	cbwbuffer = (char *) (pUsbBuffer + offset);

	/* Get USB Buffer Offset for CSW */
	ioctl(usb_fd, NUSBD_IOC_USB_CSW_BUFFER_OFFSET, &offset);
	cswbuffer = (char *) (pUsbBuffer + offset);

	/* Assign Buffer address to CBW pointer */
	nvtcbw = (struct ms_cbw_struct *)cbwbuffer;
	/* Assign Buffer address to CSW pointer */
	nvtcsw = (struct ms_csw_struct *)cswbuffer;
}
#define DEFAULT_HIDDEN 1
#define DEFAULT_NORMAL 0

int main(int argc, char *argv[]) 
{ 
	int fd[argc], i,k,retry_time = 5,device_count = 0,j; 
	char setting_device[10][20] = {0};	
	char *String; 	
	FILE *stream;
	char* massControlPath;
	char* mass_mode;
	int default_mode = DEFAULT_HIDDEN;
	char control_path[50] = {0};	
	char massLabelName[20];
	char* massLabel;
#ifdef Hidden_Driver_Letter
	printf("Hidden Driver Enabled (2013050701)\n"); 
#endif
	if(argc < 2) 
	{ 
		printf("Usage : mass <device_name>\n"); 
		return -1; 
	} 
	for(i=0;i<retry_time;i++)
	{
		usb_fd = open("/dev/usbclient", O_RDWR|O_SYNC); 
		if ( usb_fd < 0 ) 
		{ 
			printf("Can't open usbclient , wait 20ms for retry\n"); 
			usleep(20);
		} 
		else
		{
			printf("Open Success!!\n"); 
			break;
		}
		
	}
	if ( usb_fd < 0 ) 
	{ 
		printf("Can't open usbclient , perhaps, driver not loaded in\n"); 
		return -2; 
	} 

	/* Get USB Buffer Information and allocate memory */
	usb_get_buffer_info();
		
 	for (i=1; i<argc; i++)
	{
		if (argv[i] != NULL)
		{
			if(!strcmp(argv[i],"/dev/null"))
			{
				DeviceNull = i -1;
				printf("Device Null %d\n", DeviceNull);
			}
			fd[i-1] = open(argv[i], O_RDWR | O_SYNC);
			String = argv[i];

			if(String[7] == 'a')
			{			
				if(String[8] == '1')
					NAND0_LUN =  i - 1;
				else
					NAND1_LUN =  i - 1;
			}


			if(fd[i-1] < 0) 
			{ 
				printf("LUN %d : fd %d O_RDONLY\n",i-1, fd[i-1]);
				fd[i-1] = open(argv[i], O_RDONLY);
				if (fd[i-1] < 0)
				{
					close(usb_fd); 
					printf("Can't open %s, perhaps, card removed\n", argv[i]); 
				}
			}
			else
				printf("LUN %d : fd %d O_RDWR | O_SYNC\n",i-1, fd[i-1]);
			maxLun++;
		}
	} 
	mass_mode = getenv ("MASS_DEFAULT_MODE");
	if (mass_mode!=NULL)
	{
		if(mass_mode[0] == 'H')
			default_mode = DEFAULT_HIDDEN;
		else
			default_mode = DEFAULT_NORMAL;
	}	

	if(default_mode == DEFAULT_HIDDEN)
	{
		for(k=0;k<10;k++)
			hidden_lun[k] = 1;
		printf("Mass default device mode is hidden device\n");
	}
	else
	{
		for(k=0;k<10;k++)
			hidden_lun[k] = 0;
		printf("Mass default device mode is normal device\n");
	}

 	massControlPath = getenv ("MASS_CONTROL_PATH");
	if (massControlPath!=NULL)
	{
		strcpy((char *)control_path, (char *)massControlPath);
		strcat(control_path, "/usbd.ini");
	
		printf ("The mass control file path is: %s\n",control_path);	
		stream = fopen(control_path,"r");	
	}
 	else
	{
		printf ("Default mass control file path is /mnt/nand1-1\n");
		stream = fopen("/mnt/nand1-1/usbd.ini","r");
	}

	i = 0;
	if(stream!= NULL)
	{
		
		while(fscanf(stream, "%s\n", setting_device[i]) != EOF)
			i++;
		
		fclose(stream);	
	
		device_count = i;

		if(default_mode == DEFAULT_HIDDEN)
			printf("%d device(s) for normal driver disk(s): ",device_count);
		else
			printf("%d device(s) for hidden driver disk(s): ",device_count);

		for(i=0;i<device_count;i++)
			printf(" %s",setting_device[i]);

		printf("\n");
		for (i=1; i<argc; i++)
		{
			for (j=0; j<device_count; j++)
			{
				if(default_mode == DEFAULT_HIDDEN)
				{
					if(!strcmp(argv[i],setting_device[j]))
						hidden_lun[i-1] = 0;	
				}
				else
				{
					if(!strcmp(argv[i],setting_device[j]))
						hidden_lun[i-1] = 1;	
				}
			}
		}
	}
	else
		printf("No hidden driver control file!!(Device Type is default)\n");

	printf("Hidden Device Label Name\n");	
	strcpy((char * )massLabelName, "MASS_LABEL_NAME0");

	for(j=0;j<argc-1;j++)
	{
		massLabelName[15] = massLabelName[15] + j;
		massLabel = getenv (massLabelName);
		if (massLabel!=NULL)
		{
			strcpy((char *)label_name[j], (char *)massLabel);
			label_name_exist[j] = 1; 
			printf("  LUN %d Label name is %s\n",j,label_name[j]);		
		}
		else
			printf("  LUN %d Label name is nuvoTon\n",j);	
	}
	printf("Device Type\n");
	for (j=0; j<argc-1; j++)
	{
		if(hidden_lun[j])
			printf("  LUN %d (%s) is hidden driver disk\n", j,argv[j+1]);
		else
			printf("  LUN %d (%s) is normal driver disk\n", j,argv[j+1]);
	}
	bLoop = 1; 
	usb_set_lun((char *)&maxLun);

	if(ms_init(&fd[0])) 
	{ 
		for (i=0; i<maxLun; i++)
			close(fd[i]);
		close(usb_fd); 
	} 

	system("rm /var/massSync.lock");

	ioctl(usb_fd, NUSBD_IOC_PLUG, 0); 
	
	while(bLoop){ 
		ms_get_cbw();
		ms_process_cbw();
		ms_put_csw();
	} 	
	ioctl(usb_fd, NUSBD_IOC_UNPLUG, 0); 	
	ms_exit(); 
	
	for (i=0; i<maxLun; i++)
		close(fd[i]);
	close(usb_fd); 

} 
