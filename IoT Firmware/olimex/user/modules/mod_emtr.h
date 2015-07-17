#ifndef __MOD_EMTR_H__
	#define __MOD_EMTR_H__
	
	#define EMTR_DEBUG            0
	#define EMTR_VERBOSE_OUTPUT   0
	
	#define EMTR_TIMEOUT          500

	#define EMTR_START_FRAME      0xA5
	#define EMTR_ACK_FRAME        0x06
	#define EMTR_ERROR_CRC        0x51
	
	#include "user_devices.h"
	
	typedef enum {
		EMTR_STATE_IDLE = 0,
		
		EMTR_STATE_LEN,
		EMTR_STATE_DATA,
		
		EMTR_STATE_ERROR
	} emtr_state;
	
	typedef struct _emtr_packet_ {
		uint8      header;
		uint8      len;
		uint8     *data;
		uint8      check_sum;
		
		void_func  callback;
	} emtr_packet;
	
	typedef struct {
		uint16 address;
	} emtr_sys_params;
	
	typedef struct {
		uint32 current_rms;
		uint16 voltage_rms;
		uint32 active_power;
		uint32 reactive_power;
		uint32 apparent_power;
		sint16 power_factor;
		uint16 line_frequency;
		uint16 thermistor_voltage;
		uint16 event_flag;
		uint16 system_status;
	} emtr_registers;
	
	typedef void (*emtr_callback)(emtr_packet *packet);
	
	uint16 emtr_address();
	void   emtr_get_address(emtr_callback command_done);
	void   emtr_get_all(emtr_callback command_done);
	void   emtr_init();
#endif