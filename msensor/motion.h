#ifndef __MOTION_H__
#define __MOTION_H__


typedef struct  {
	int x;			/* holds x-axis acceleration data */
	int	y;			/* holds y-axis acceleration data */
	int	z;			/* holds z-axis acceleration data */	
	unsigned long time;
} MOTION_ACC;

typedef enum {
    MOTION_ACTION_NULL,             		/*not found any action*/
    MOTION_ACTION_SLOW_UP,       		/*detected slow up action*/
    MOTION_ACTION_SLOW_DOWN,        	/*detected slow down action*/
    MOTION_ACTION_COLLISION,   		/*detected collision action*/
    MOTION_ACTION_GZ0,				/*angle from G to Z axis is 0*/
    MOTION_ACTION_GZ180,			/*angle from G to Z axis is 180*/
    MOTION_ACTION_TOTAL
}MOTION_ACTION;

typedef struct {
	MOTION_ACTION action;
	void (*pfnCallBack)(int action);
}MOTION_CALLBACK;


int motion_service_start(void);
int motion_service_stop(void);
int motion_suspend(void);
int motion_resume(void);
int motion_set_delay(int delay);
int motion_register_action(MOTION_ACTION action,void *pfnCallBack);
int motion_unregister_action(int index);


#endif/*__MOTION_H__*/

