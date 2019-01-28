/****************************************************************************
 *                                                                          *
 * Copyright (c) 2008 Nuvoton Technology Corp. All rights reserved.         *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 *
 * FILENAME
 *     spi.c
 *
 * DESCRIPTION
 *     This file is an SPI demo program
 *
 **************************************************************************/
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include <errno.h>

#include "spi.h"

//-- function return value
#define	   Successful  0
#define	   Fail        1

#define UINT32	unsigned int
#define UINT16	unsigned short
#define UINT8	unsigned char

static __inline unsigned short Swap16(unsigned short val)
{
    return (val<<8) | (val>>8);
}

#define SpiSelectCS0()				ioctl(spi_fd, SPI_IOC_SELECTSLAVE, 0)
#define SpiDeselectCS0()			ioctl(spi_fd, SPI_IOC_SELECTSLAVE, 1)
#define SpiDoTransit()				ioctl(spi_fd, SPI_IOC_TRANSIT, &gdata)
#define SpiSetBitLen(x)				gdata.bit_len = x
#define SpiSetData(x)					gdata.write_data = x
#define SpiGetData()					gdata.read_data

#define FLASH_BUFFER_SIZE	2048

UINT8 Flash_Buf[FLASH_BUFFER_SIZE];
UINT8 ReadBackBuffer[FLASH_BUFFER_SIZE];


int spi_fd;
struct spi_data gdata;

int spiCheckBusy()
{
	// check status
	SpiSelectCS0();	// CS0

	// status command
	SpiSetData(0x05);
	SpiSetBitLen(8);
	SpiDoTransit();

	// get status
	while(1)
	{
		SpiSetData(0xff);
		SpiSetBitLen(8);
		SpiDoTransit();
		if (((SpiGetData() & 0xff) & 0x01) != 0x01)
			break;
	}
	SpiDeselectCS0();	// CS0
	return Successful;
}

/*
	addr: memory address 
	len: byte count
	buf: buffer to put the read back data
*/
int spiRead(UINT32 addr, UINT32 len, UINT8 *buf)
{
	int volatile i;

    spiCheckBusy();

	SpiSelectCS0();	// CS0

	// read command
	SpiSetData(03);
	SpiSetBitLen(8);
	SpiDoTransit();

	// address
	SpiSetData(addr);
	SpiSetBitLen(24);
	SpiDoTransit();

	// data
	for (i=0; i<len; i++)
	{
		SpiSetData(0xff);
		SpiSetBitLen(8);
		SpiDoTransit();
		*buf++ = SpiGetData() & 0xff;
	}

	SpiDeselectCS0();	// CS0

	return Successful;
}

int spiReadFast(UINT32 addr, UINT32 len, UINT8 *buf)
{
	int volatile i;

    spiCheckBusy();
    
	SpiSelectCS0();	// CS0

	// read command
	SpiSetData(0x0b);
	SpiSetBitLen(8);
	SpiDoTransit();

	// address
	SpiSetData(addr);
	SpiSetBitLen(24);
	SpiDoTransit();

	// dummy byte
	SpiSetData(0xff);
	SpiSetBitLen(8);
	SpiDoTransit();

	// data
	for (i=0; i<len; i++)
	{
		SpiSetData(0xff);
		SpiSetBitLen(8);
		SpiDoTransit();
		*buf++ = SpiGetData() & 0xff;
	}

	SpiDeselectCS0();	// CS0

	return Successful;
}

int spiWriteEnable()
{
	SpiSelectCS0();// CS0

	SpiSetData(0x06);
	SpiSetBitLen(8);
	SpiDoTransit();
	
	SpiDeselectCS0();	// CS0

	return Successful;
}

int spiWriteDisable()
{
	SpiSelectCS0();	// CS0

	SpiSetData(0x04);
	SpiSetBitLen(8);
	SpiDoTransit();

	SpiDeselectCS0();	// CS0

	return Successful;
}

/*
	addr: memory address
	len: byte count
	buf: buffer with write data
*/
int spiWrite(UINT32 addr, UINT32 len, unsigned short *buf)
{
	int volatile count=0, page, i;


    spiCheckBusy();
	count = len / 256;
	if ((len % 256) != 0)
		count++;

	for (i=0; i<count; i++)
	{
		// check data len
		if (len >= 256)
		{
			page = 128;
			len = len - 256;
		}
		else
			page = len/2;

		spiWriteEnable();

		SpiSelectCS0();	// CS0

		// write command
		SpiSetData(0x02);
		SpiSetBitLen(8);
		SpiDoTransit();

		// address
		SpiSetData(addr+i*256);
		SpiSetBitLen(24);
		SpiDoTransit();
		// write data
		while (page-- > 0)
		{
			SpiSetData(Swap16(*buf++));
			SpiSetBitLen(16);
			SpiDoTransit();
		}

		SpiDeselectCS0();	// CS0

		// check status
		spiCheckBusy();
	}

	return Successful;
}

int spiEraseSector(UINT32 addr, UINT32 secCount)
{
	int volatile i;

    spiCheckBusy();
	for (i=0; i<secCount; i++)
	{
		spiWriteEnable();

		SpiSelectCS0();	// CS0

		// erase command
		SpiSetData(0xd8);
		SpiSetBitLen(8);
		SpiDoTransit();

		// address
		SpiSetData(addr+i*256);
		SpiSetBitLen(24);
		SpiDoTransit();
		
		SpiDeselectCS0();	// CS0

		// check status
		spiCheckBusy();
	}
	return Successful;
}

int spiEraseAll()
{
	spiWriteEnable();

	SpiSelectCS0();// CS0

	SpiSetData(0xc7);
	SpiSetBitLen(8);
	SpiDoTransit();

	SpiDeselectCS0();	// CS0

	// check status
	spiCheckBusy();

	return Successful;
}

unsigned short spiReadID()
{
	unsigned short volatile id;

	SpiSelectCS0();	// CS0

	// command 8 bit
	SpiSetData(0x90);
	SpiSetBitLen(8);
	SpiDoTransit();

	// address 24 bit
	SpiSetData(0x000000);
	SpiSetBitLen(24);
	SpiDoTransit();

	// data 16 bit
	SpiSetData(0xffff);
	SpiSetBitLen(16);
	SpiDoTransit();
	id = SpiGetData() & 0xffff;

	SpiDeselectCS0();	// CS0

	return id;
}

UINT8 spiStatusRead()
{
	UINT32 status;
	SpiSelectCS0();		// CS0

	// status command
	SpiSetData(0x05);
	SpiSetBitLen(8);
	SpiDoTransit();

	// get status
	SpiSetData(0xff);
	SpiSetBitLen(8);
	SpiDoTransit();
	status = SpiGetData() & 0xff;
	
	SpiDeselectCS0();	// CS0
	return status;
}

int spiStatusWrite(UINT8 data)
{
	spiWriteEnable();

	SpiSelectCS0();	// CS0

	// status command
	SpiSetData(0x01);
	SpiSetBitLen(8);
	SpiDoTransit();

	// write status
	SpiSetData(data);
	SpiSetBitLen(8);
	SpiDoTransit();

	SpiDeselectCS0();	// CS0

	// check status
	spiCheckBusy();

	return Successful;
}

int spiFlashTest()
{
  int volatile i;

	for (i=0; i<FLASH_BUFFER_SIZE; i++)
		Flash_Buf[i] = i;

	printf("flash ID [0x%04x]\n", spiReadID());
	printf("flash status [0x%02x]\n", spiStatusRead());

//	printf("erase all\n");
//	spiEraseAll();
	printf("erase sector\n");
	i = FLASH_BUFFER_SIZE / 256;
	if ((FLASH_BUFFER_SIZE % 256) != 0)
		i++;
	spiEraseSector(0, i);

	printf("write\n");
	spiWrite(0, FLASH_BUFFER_SIZE, (unsigned short *)Flash_Buf);

	printf("read\n");
	spiRead(0, FLASH_BUFFER_SIZE, ReadBackBuffer);
//	spiReadFast(0, FLASH_BUFFER_SIZE, ReadBackBuffer);

	printf("compare\n");
	for (i=0; i<FLASH_BUFFER_SIZE; i++)
	{
		if (ReadBackBuffer[i] != Flash_Buf[i])
		{
			printf("error! [%d]->wrong[%x] / correct[%x]\n", i, ReadBackBuffer[i], Flash_Buf[i]);
			break;
		}
	}

	printf("finish..\n");
	return 0;
}

int main()
{	
	int i;
	struct spi_parameter para;	
		
	spi_fd = open("/dev/spi0", O_RDWR);
	if ( spi_fd < 0) {
		printf("Open spi error\n");
		return -1;
	}
	
	//ioctl(spi_fd, SPI_IOC_GETPARAMETER, &para);  // Mark 01.14, Michael
	
	para.divider = 30;				// SCK = PCLK/16 (20MHz)
	para.active_level = 0;	// CS active low
	para.tx_neg = 1;				// Tx: falling edge, Rx: rising edge
	para.rx_neg = 0;
	para.lsb = 0;
	para.sleep = 0;
	
	ioctl(spi_fd, SPI_IOC_SETPARAMETER, &para);
	ioctl(spi_fd, SPI_IOC_GETPARAMETER, &para);
	
	printf("para.lsb : %d\n", para.lsb);
	printf("para.tx_neg : %d\n", para.tx_neg);
	printf("para.rx_neg : %d\n", para.rx_neg);
	printf("para.divider : %d\n", para.divider);
	printf("para.active_level : %d\n", para.active_level);
	printf("para.sleep : %d\n", para.sleep);

	spiFlashTest();

	close(spi_fd);	
	
	return 0;
}	

