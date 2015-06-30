#ifndef __MOD_FINGER_H__
	#define __MOD_FINGER_H__
	
	#define FINGER_DEBUG         1
	typedef enum {
		FINGER_IDLE = 0,
		
		FINGER_START,
		FINGER_ADDRESS,
		FINGER_PID,
		FINGER_LEN,
		FINGER_DATA,
		
		FINGER_ERROR
	} finger_state;
	
	typedef struct _finger_packet_ {
		uint32  address;
		uint8   pid;
		uint16  len;
		uint8  *data;
		uint16  check_sum;
	} finger_packet;
	
	bool finger_found();
	void finger_init();
#endif