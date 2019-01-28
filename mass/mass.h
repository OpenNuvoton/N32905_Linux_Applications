/****************************************************************************
 *                                                                          *
 * Copyright (c) 2008 Nuvoton Technology Corp. All rights reserved.         *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 *
 * FILENAME
 *     mass.h
 *
 * DESCRIPTION
 *                                                 
 *
 **************************************************************************/
#ifndef _MASS_H_

#define SCSI_CMD_MAX_LEN	16
#define SD_SECTOR_SIZE			512

/* SCSI commands that we recognize */
#define SC_FORMAT_UNIT			0x04
#define SC_INQUIRY			0x12
#define SC_MODE_SELECT_6		0x15
#define SC_MODE_SELECT_10		0x55
#define SC_MODE_SENSE_6			0x1a
#define SC_MODE_SENSE_10		0x5a
#define SC_PREVENT_ALLOW_MEDIUM_REMOVAL	0x1e
#define SC_READ_6			0x08
#define SC_READ_10			0x28
#define SC_READ_12			0xa8
#define SC_READ_CAPACITY		0x25
#define SC_READ_FORMAT_CAPACITIES	0x23
#define SC_RELEASE			0x17
#define SC_REQUEST_SENSE		0x03
#define SC_RESERVE			0x16
#define SC_SEND_DIAGNOSTIC		0x1d
#define SC_START_STOP_UNIT		0x1b
#define SC_SYNCHRONIZE_CACHE		0x35
#define SC_TEST_UNIT_READY		0x00
#define SC_VERIFY			0x2f
#define SC_WRITE_6			0x0a
#define SC_WRITE_10			0x2a
#define SC_WRITE_12			0xaa

/* SCSI Sense Key/Additional Sense Code/ASC Qualifier values */
#define SS_NO_SENSE				0
#define SS_COMMUNICATION_FAILURE		0x040800
#define SS_INVALID_COMMAND			0x052000
#define SS_INVALID_FIELD_IN_CDB			0x052400
#define SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE	0x052100
#define SS_LOGICAL_UNIT_NOT_SUPPORTED		0x052500
#define SS_MEDIUM_NOT_PRESENT			0x023a00
#define SS_MEDIUM_REMOVAL_PREVENTED		0x055302
#define SS_NOT_READY_TO_READY_TRANSITION	0x062800
#define SS_RESET_OCCURRED			0x062900
#define SS_SAVING_PARAMETERS_NOT_SUPPORTED	0x053900
#define SS_UNRECOVERED_READ_ERROR		0x031100
#define SS_WRITE_ERROR				0x030c02
#define SS_WRITE_PROTECTED			0x072700
#define SS_INVALID_MEDIUM			0x033000
#define SS_MEDIUM_CHANGED			0x062800

/* const about cbw / csw */
#define CBW_SIGNATURE		0x43425355
#define CSW_SIGNATURE		0x53425355

#define CMD_PASSED			0x00
#define CMD_FAILED			0x01
#define PHASE_ERROR			0x02

#define DIR_OUT				0x00
#define DIR_IN				0x80

#define SK(x)		((u8) ((x) >> 16))	// Sense Key byte, etc.
#define ASC(x)		((u8) ((x) >> 8))
#define ASCQ(x)		((u8) (x))

typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

/*
 * fat_loff_t is defined here since unix_io.c needs it.
 */
#if defined(__GNUC__) || defined(HAS_LONG_LONG)
typedef long long	fat_loff_t;
#else
typedef long		fat_loff_t;
#endif

/* llseek.c */
fat_loff_t fat_llseek (int, fat_loff_t, int);

struct ms_cbw_struct{
	u32 dCBWSignature;
	u32 dCBWTag;
	u32 dCBWDataTransferLength;
	u8 bmCBWFlags;
	u8 bCBWLUN;
	u8 bCBWCBLength;
	u8 CBWCB[SCSI_CMD_MAX_LEN];
};

struct ms_csw_struct{
	u32 dCSWSignature;
	u32 dCSWTag;
	u32 dCSWDataResidue;
	u8 bCSWStatus;
};

struct sd2ms_dev_struct {
	int fd;

	int sense;
       
	//fat_loff_t nTotalSectors;
	unsigned int nTotalSectors;
	fat_loff_t nCapacityInByte;
// associate with USB Mass storage command
	struct ms_cbw_struct cbw;
	struct ms_csw_struct csw;

// associate with Read/Write Operation
	char *buffer;
       
};

#endif
