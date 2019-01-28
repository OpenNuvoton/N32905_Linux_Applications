/****************************************************************************
 * 
 * Copyright (c) 2008 Nuvoton Technology Corp. All rights reserved. 
 *
 ****************************************************************************/
 
/****************************************************************************
 * 
 * FILENAME
 *     spi.h
 *
 *************************************************************************/

#ifndef __SPI_H_
#define __SPI_H_

#include <linux/types.h>
#include <linux/ioctl.h>

#define REAL_BOARD

struct spi_parameter{
	unsigned int active_level:1;
	unsigned int lsb:1, tx_neg:1, rx_neg:1, divider:16;
	unsigned int sleep:4;
};

struct spi_data{
	unsigned int write_data;
	unsigned int read_data;
	unsigned int bit_len;
};

#define SPI_MAJOR		231

#define SPI_IOC_MAGIC			'u'
#define SPI_IOC_MAXNR			3

#define SPI_IOC_GETPARAMETER	_IOR(SPI_IOC_MAGIC, 0, struct usi_parameter *)
#define SPI_IOC_SETPARAMETER	_IOW(SPI_IOC_MAGIC, 1, struct usi_parameter *)
#define SPI_IOC_SELECTSLAVE	_IOW(SPI_IOC_MAGIC, 2, int)
#define SPI_IOC_TRANSIT			_IOW(SPI_IOC_MAGIC, 3, struct usi_data *)

#endif /* _SPI_H_ */

