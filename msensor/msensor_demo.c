
/****************************************************************************
 *                                                                          *
 * Copyright (c) 2010 Nuvoton Technology Corp. All rights reserved.         *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 *
 * FILENAME
 *     msensor_demo.c
 *
 * DESCRIPTION
 *     This file is a motion sensor demo program
 *
 **************************************************************************/

#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/ioctl.h>
#include<pthread.h>
#include<fcntl.h>
#include<linux/input.h>
#include<string.h>
#include<strings.h>

#include "bma220.h"
#include "motion.h"

int msensor_help()
{
      printf("\n******** Motion Sensor Test Demo ***********\n");
      printf("1.Misc test ..\n");
      printf("2.Input test ..\n");
      printf("3.Put to file ..\n");
      printf("4.detect action \n");

      printf("Q.Exit(don't use 'ctrl+c',must be 'X')..\n");
      printf("**************************************\n");
      printf("Select : \n");   

}

void motion_proc(int action)
{  
	switch(action)
	{
	    case MOTION_ACTION_NULL:             		
			break;
	    case MOTION_ACTION_SLOW_UP:       		
			printf("motion_proc:MOTION_ACTION_SLOW_UP\n");
			break;
	    case MOTION_ACTION_SLOW_DOWN:        	
			printf("motion_proc:MOTION_ACTION_SLOW_DOWN\n");
			break;
	    case MOTION_ACTION_COLLISION:   		
			printf("motion_proc:MOTION_ACTION_COLLISION\n");
			break;			
	    case MOTION_ACTION_GZ0:				
			printf("motion_proc:MOTION_ACTION_GZ0\n");
			break;
	    case MOTION_ACTION_GZ180:			
			printf("motion_proc:MOTION_ACTION_GZ180\n");
			break;
	    case MOTION_ACTION_TOTAL:
			break;
	}
	
}


int main(int argc,char *argv[])
{
	char select;
	msensor_help();

	while(1)
    	{
      		select = getchar();
  		switch(select) 
    		{
		case '1':
		  bma220_dev_process();
		  break;

		case '2':
		  bma220_input_process();
		  break;

		case '3':
		  bma220_put_file(argc,argv);
		  break;

		case '4':
		  motion_service_start();
		  motion_register_action(MOTION_ACTION_SLOW_UP, &motion_proc);
		  motion_register_action(MOTION_ACTION_SLOW_DOWN, &motion_proc);
		  motion_register_action(MOTION_ACTION_COLLISION, &motion_proc);
		  motion_register_action(MOTION_ACTION_GZ0, &motion_proc);
		  motion_register_action(MOTION_ACTION_GZ180, &motion_proc);
		  break;
		  
		case 'q': //'X'    			
		  printf("haha-goto the end now.\n");
		  goto end; 

		default :
		  //msensor_help();
		  printf("==>Select command number(1-5) \n");
		  break;	
	    	}
	}
	
end:
	return 0;
}
