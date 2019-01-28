#ifndef __BMA220_H__
#define __BMA220_H__

#define BMA220_IOC_MAGIC 'B'

#define BMA220_SET_SLEEP_EN \
	_IOWR(BMA220_IOC_MAGIC, 0, unsigned char)
#define BMA220_SET_SUSPEND \
	_IOW(BMA220_IOC_MAGIC, 1, unsigned char)
#define BMA220_READ_ACCEL_X \
	_IOWR(BMA220_IOC_MAGIC, 2, signed char)
#define BMA220_READ_ACCEL_Y \
	_IOWR(BMA220_IOC_MAGIC, 3, signed char)
#define BMA220_READ_ACCEL_Z \
	_IOWR(BMA220_IOC_MAGIC, 4, signed char)
#define BMA220_SET_MODE \
	_IOWR(BMA220_IOC_MAGIC, 5, unsigned char)
#define BMA220_GET_MODE \
	_IOWR(BMA220_IOC_MAGIC, 6, unsigned char)
#define BMA220_SET_RANGE \
	_IOWR(BMA220_IOC_MAGIC, 7, unsigned char)
#define BMA220_GET_RANGE \
	_IOWR(BMA220_IOC_MAGIC, 8, unsigned char)
#define BMA220_SET_BANDWIDTH \
	_IOWR(BMA220_IOC_MAGIC, 9, unsigned char)
#define BMA220_GET_BANDWIDTH \
	_IOWR(BMA220_IOC_MAGIC, 10, unsigned char)
#define BMA220_SET_SC_FILT_CONFIG \
	_IOWR(BMA220_IOC_MAGIC, 11, unsigned char)
#define BMA220_RESET_INTERRUPT \
	_IO(BMA220_IOC_MAGIC, 12)
#define BMA220_GET_DIRECTION_STATUS_REGISTER \
	_IOWR(BMA220_IOC_MAGIC, 13, unsigned char)
#define BMA220_GET_INTERRUPT_STATUS_REGISTER \
	_IOWR(BMA220_IOC_MAGIC, 14, unsigned char)
#define BMA220_SOFT_RESET \
	_IO(BMA220_IOC_MAGIC, 15)
#define BMA220_SET_LATCH_INT \
	_IOWR(BMA220_IOC_MAGIC, 16, unsigned char)
#define BMA220_SET_EN_HIGH_XYZ \
	_IOWR(BMA220_IOC_MAGIC, 17, unsigned char)
#define BMA220_SET_HIGH_TH \
	_IOWR(BMA220_IOC_MAGIC, 18, unsigned char)
#define BMA220_SET_HIGH_HY \
	_IOWR(BMA220_IOC_MAGIC, 19, unsigned char)
#define BMA220_SET_HIGH_DUR \
	_IOWR(BMA220_IOC_MAGIC, 20, unsigned char)
#define BMA220_SET_EN_LOW \
	_IOWR(BMA220_IOC_MAGIC, 21, unsigned char)
#define BMA220_SET_LOW_TH \
	_IOWR(BMA220_IOC_MAGIC, 22, unsigned char)
#define BMA220_SET_LOW_HY \
	_IOWR(BMA220_IOC_MAGIC, 23, unsigned char)
#define BMA220_SET_LOW_DUR \
	_IOWR(BMA220_IOC_MAGIC, 24, unsigned char)
#define BMA220_SET_SERIAL_HIGH_BW \
	_IOWR(BMA220_IOC_MAGIC, 25, unsigned char)
#define BMA220_READ_ACCEL_XYZ \
	_IOWR(BMA220_IOC_MAGIC, 26, signed char)
#define BMA220_SET_EN_ORIENT \
	_IOWR(BMA220_IOC_MAGIC, 27, unsigned char)
#define BMA220_SET_ORIENT_EX \
	_IOWR(BMA220_IOC_MAGIC, 28, unsigned char)
#define BMA220_GET_ORIENTATION \
	_IOWR(BMA220_IOC_MAGIC, 29, unsigned char)
#define BMA220_SET_EN_TT_XYZ \
	_IOWR(BMA220_IOC_MAGIC, 30, unsigned char)
#define BMA220_SET_TT_TH \
	_IOWR(BMA220_IOC_MAGIC, 31, unsigned char)
#define BMA220_SET_TT_DUR \
	_IOWR(BMA220_IOC_MAGIC, 32, unsigned char)
#define BMA220_SET_TT_FILT \
	_IOWR(BMA220_IOC_MAGIC, 33, unsigned char)
#define BMA220_SET_EN_SLOPE_XYZ \
	_IOWR(BMA220_IOC_MAGIC, 34, unsigned char)
#define BMA220_SET_EN_DATA \
	_IOWR(BMA220_IOC_MAGIC, 35, unsigned char)
#define BMA220_SET_SLOPE_TH \
	_IOWR(BMA220_IOC_MAGIC, 36, unsigned char)
#define BMA220_SET_SLOPE_DUR \
	_IOWR(BMA220_IOC_MAGIC, 37, unsigned char)
#define BMA220_SET_SLOPE_FILT \
	_IOWR(BMA220_IOC_MAGIC, 38, unsigned char)
#define BMA220_SET_CAL_TRIGGER \
	_IOWR(BMA220_IOC_MAGIC, 39, unsigned char)
#define BMA220_GET_CAL_RDY \
	_IOWR(BMA220_IOC_MAGIC, 40, unsigned char)
#define BMA220_SET_HP_XYZ_EN \
	_IOWR(BMA220_IOC_MAGIC, 41, unsigned char)
#define BMA220_SET_OFFSET_TARGET_X \
	_IOWR(BMA220_IOC_MAGIC, 42, unsigned char)
#define BMA220_SET_OFFSET_TARGET_Y \
	_IOWR(BMA220_IOC_MAGIC, 43, unsigned char)
#define BMA220_SET_OFFSET_TARGET_Z \
	_IOWR(BMA220_IOC_MAGIC, 44, unsigned char)
#define BMA220_SET_SLEEP_DUR \
	_IOWR(BMA220_IOC_MAGIC, 45, unsigned char)
#define BMA220_GET_SLEEP_DUR \
	_IOWR(BMA220_IOC_MAGIC, 46, unsigned char)
#define BMA220_SET_OFFSET_RESET \
	_IOWR(BMA220_IOC_MAGIC, 47, unsigned char)
#define BMA220_SET_CUT_OFF_SPEED \
	_IOWR(BMA220_IOC_MAGIC, 48, unsigned char)
#define BMA220_SET_CAL_MANUAL \
	_IOWR(BMA220_IOC_MAGIC, 49, unsigned char)
#define BMA220_SET_SBIST \
	_IOWR(BMA220_IOC_MAGIC, 50, unsigned char)
#define BMA220_SET_INTERRUPT_REGISTER \
	_IOWR(BMA220_IOC_MAGIC, 51, unsigned char)
#define BMA220_SET_DIRECTION_INTERRUPT_REGISTER \
	_IOWR(BMA220_IOC_MAGIC, 52, unsigned char)
#define BMA220_GET_ORIENT_INT \
	_IOWR(BMA220_IOC_MAGIC, 53, unsigned char)
#define BMA220_SET_ORIENT_BLOCKING \
	_IOWR(BMA220_IOC_MAGIC, 54, unsigned char)
#define BMA220_GET_CHIP_ID \
	_IOWR(BMA220_IOC_MAGIC, 55, unsigned char)
#define BMA220_GET_SC_FILT_CONFIG \
	_IOWR(BMA220_IOC_MAGIC, 56, unsigned char)
#define BMA220_GET_SLEEP_EN \
	_IOWR(BMA220_IOC_MAGIC, 57, unsigned char)
#define BMA220_GET_SERIAL_HIGH_BW \
	_IOWR(BMA220_IOC_MAGIC, 58, unsigned char)
#define BMA220_GET_LATCH_INT \
	_IOWR(BMA220_IOC_MAGIC, 59, unsigned char)
#define BMA220_GET_EN_DATA \
	_IOWR(BMA220_IOC_MAGIC, 60, unsigned char)
#define BMA220_GET_EN_HIGH_XYZ \
	_IOWR(BMA220_IOC_MAGIC, 61, unsigned char)
#define BMA220_GET_HIGH_TH \
	_IOWR(BMA220_IOC_MAGIC, 62, unsigned char)
#define BMA220_GET_HIGH_HY \
	_IOWR(BMA220_IOC_MAGIC, 63, unsigned char)
#define BMA220_GET_HIGH_DUR \
	_IOWR(BMA220_IOC_MAGIC, 64, unsigned char)
#define BMA220_GET_EN_LOW \
	_IOWR(BMA220_IOC_MAGIC, 65, unsigned char)
#define BMA220_GET_LOW_TH \
	_IOWR(BMA220_IOC_MAGIC, 66, unsigned char)
#define BMA220_GET_LOW_HY \
	_IOWR(BMA220_IOC_MAGIC, 67, unsigned char)
#define BMA220_GET_LOW_DUR \
	_IOWR(BMA220_IOC_MAGIC, 68, unsigned char)
#define BMA220_GET_EN_ORIENT \
	_IOWR(BMA220_IOC_MAGIC, 69, unsigned char)
#define BMA220_GET_ORIENT_EX \
	_IOWR(BMA220_IOC_MAGIC, 70, unsigned char)
#define BMA220_GET_ORIENT_BLOCKING \
	_IOWR(BMA220_IOC_MAGIC, 71, unsigned char)
#define BMA220_GET_EN_TT_XYZ \
	_IOWR(BMA220_IOC_MAGIC, 72, unsigned char)
#define BMA220_GET_TT_TH \
	_IOWR(BMA220_IOC_MAGIC, 73, unsigned char)
#define BMA220_GET_TT_DUR \
	_IOWR(BMA220_IOC_MAGIC, 74, unsigned char)
#define BMA220_GET_TT_FILT \
	_IOWR(BMA220_IOC_MAGIC, 75, unsigned char)
#define BMA220_SET_TT_SAMP \
	_IOWR(BMA220_IOC_MAGIC, 76, unsigned char)
#define BMA220_GET_TT_SAMP \
	_IOWR(BMA220_IOC_MAGIC, 77, unsigned char)
#define BMA220_SET_TIP_EN \
	_IOWR(BMA220_IOC_MAGIC, 78, unsigned char)
#define BMA220_GET_TIP_EN \
	_IOWR(BMA220_IOC_MAGIC, 79, unsigned char)
#define BMA220_GET_EN_SLOPE_XYZ	\
	_IOWR(BMA220_IOC_MAGIC, 80, unsigned char)
#define BMA220_GET_SLOPE_TH		\
	_IOWR(BMA220_IOC_MAGIC, 81, unsigned char)
#define BMA220_GET_SLOPE_DUR	\
	_IOWR(BMA220_IOC_MAGIC, 82, unsigned char)
#define BMA220_GET_SLOPE_FILT	\
	_IOWR(BMA220_IOC_MAGIC, 83, unsigned char)
#define BMA220_GET_HP_XYZ_EN	\
	_IOWR(BMA220_IOC_MAGIC, 84, unsigned char)
#define BMA220_GET_OFFSET_TARGET_X	\
	_IOWR(BMA220_IOC_MAGIC, 85, unsigned char)
#define BMA220_GET_OFFSET_TARGET_Y	\
	_IOWR(BMA220_IOC_MAGIC, 86, unsigned char)
#define BMA220_GET_OFFSET_TARGET_Z	\
	_IOWR(BMA220_IOC_MAGIC, 87, unsigned char)
#define BMA220_GET_CUT_OFF_SPEED	\
	_IOWR(BMA220_IOC_MAGIC, 88, unsigned char)
#define BMA220_GET_CAL_MANUAL	\
	_IOWR(BMA220_IOC_MAGIC, 89, unsigned char)
#define BMA220_SET_OFFSET_XYZ	\
	_IOWR(BMA220_IOC_MAGIC, 90, signed char)
#define BMA220_GET_OFFSET_XYZ	\
	_IOWR(BMA220_IOC_MAGIC, 91, signed char)
#define BMA220_IOCTL_READ	\
	_IOWR(BMA220_IOC_MAGIC, 92, signed char)
#define BMA220_IOCTL_WRITE	\
	_IOR(BMA220_IOC_MAGIC, 93, signed char)
#define BMA220_IOCTL_CALIBRATION	\
	_IO(BMA220_IOC_MAGIC, 94)
#define BMA220_IOCTL_EVENT_CTRL	\
	_IOW(BMA220_IOC_MAGIC, 95, signed char)

typedef struct  {
	signed char x,	/**< holds x-axis acceleration data */
				y,	/**< holds y-axis acceleration data */
				z;	/**< holds z-axis acceleration data */
	
} bma220acc_t;

extern int bma220_dev_process();
extern int bma220_sys_process(void);
extern int bma220_input_process(void);
extern int  bma220_put_file(char argc,char *argv[]);






#endif/*__BMA220_H__*/

