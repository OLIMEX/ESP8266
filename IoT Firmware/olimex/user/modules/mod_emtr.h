#ifndef __MOD_EMTR_H__
	#define __MOD_EMTR_H__
	
	#define EMTR_DEBUG            0
	#define EMTR_VERBOSE_OUTPUT   0
	
	#define EMTR_TIMEOUT          500

	#define EMTR_START_FRAME      0xA5
	#define EMTR_ACK_FRAME        0x06
	#define EMTR_ERROR_NAK        0x15
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
		
		bool       ask_only;
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
	} emtr_output_registers;
	#define EMTR_OUT_BASE 0x0004
	#define EMTR_OUT_LEN  28
	
	typedef struct {
		uint16 gain_current_rms;
		uint16 gain_voltage_rms;
		uint16 gain_active_power;
		uint16 gain_reactive_power;
		sint32 offset_current_rms;
		sint32 offset_active_power;
		sint32 offset_reactive_power;
		sint16 dc_offset_current;
		sint16 phase_compensation;
		uint16 apparent_power_divisor;
		uint32 system_configuration;
		uint16 dio_configuration;
		uint32 range;
		
		uint32 calibration_current;
		uint16 calibration_voltage;
		uint32 calibration_active_power;
		uint32 calibration_reactive_power;
		uint16 accumulation_interval;
	} emtr_calibration_registers;
	#define EMTR_CALIBRATION_BASE 0x0028
	#define EMTR_CALIBRATION_LEN  52
	
	typedef struct {
		uint32 over_current_limit;
		uint16 reserved_01;
		uint32 over_power_limit;
		uint16 reserved_02;
		uint16 over_frequency_limit;
		uint16 under_frequency_limit;
		uint16 over_temperature_limit;
		uint16 under_temperature_limit;
		uint16 voltage_sag_limit;
		uint16 voltage_surge_limit;
		uint16 over_current_hold;
		uint16 reserved_03;
		uint16 over_power_hold;
		uint16 reserved_04;
		uint16 over_frequency_hold;
		uint16 under_frequency_hold;
		uint16 over_temperature_hold;
		uint16 under_temperature_hold;
		uint16 reserved_05;
		uint16 reserved_06;
		uint16 event_enable;
		uint16 event_mask_critical;
		uint16 event_mask_standard;
		uint16 event_test;
		uint16 event_clear;
	} emtr_event_registers;
	#define EMTR_EVENTS_BASE  0x005E
	#define EMTR_EVENTS_LEN   54
	
	typedef void (*emtr_callback)(emtr_packet *packet);
	
	uint16 emtr_address();
	void   emtr_get_address(emtr_callback command_done);
	
	void   emtr_set_timeout_callback(emtr_callback command_timeout);
	void   emtr_clear_timeout(emtr_packet *packet);

	void   emtr_parse_calibration(emtr_packet *packet, emtr_calibration_registers *registers);
	void   emtr_get_calibration(emtr_callback command_done);
	
	void   emtr_parse_event(emtr_packet *packet, emtr_event_registers *registers);
	void   emtr_get_event(emtr_callback command_done);
	
	void   emtr_parse_output(emtr_packet *packet, emtr_output_registers *registers);
	void   emtr_get_output(emtr_callback command_done);
	
	void   emtr_clear_event(uint16 event, emtr_callback command_done);
	
	void   emtr_init();
#endif