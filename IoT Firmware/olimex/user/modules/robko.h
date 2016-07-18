#ifndef __ROBKO_H__
	#define __ROBKO_H__
	
	#include "user_config.h"
	#if DEVICE == ROBKO
		#define ROBKO_DEBUG                   1
		#define ROBKO_VERBOSE_OUTPUT          0
		
		#define ROBKO_ID                   0x33
		#define ROBKO_DEFAULT_ADDR         0x2F
		#define ROBKO_I2C_RETRY               3
		
		#define ROBKO_EVENTS               0x2B
		#define ROBKO_EVENTS_PRESS         0x2C
		#define ROBKO_EVENTS_RELEASE       0x2D
		#define ROBKO_EVENTS_DBLCLICK      0x2E
		
		#define ROBKO_EVENTS_NEW_MASK      0x80
		#define ROBKO_EVENTS_PRESS_MASK    0x01
		#define ROBKO_EVENTS_RELEASE_MASK  0x02
		#define ROBKO_EVENTS_DBLCLICK_MASK 0x04
		#define ROBKO_EVENTS_ANALOG_MASK   0x08
		#define ROBKO_EVENTS_SERVO_MASK    0x40
		
		#define ROBKO_POTENTIOMETER        0x00
		
		#define ROBKO_SERVO                0x10
		#define ROBKO_LIMIT_LOW            0x30
		#define ROBKO_LIMIT_HIGH           0x40
		#define ROBKO_LIMIT_SPEED          0x2A
		
		#include "driver/i2c_extention.h"
		
		typedef struct _robko_config_data_ {
			uint8  events;
			
			uint8  events_press;
			uint8  events_release;
			uint8  events_dbl_click;
			
			sint16 potentiometer[6];
			
			uint8  servo_ready;
			uint16 servo[6];
			
			uint16 limit_low[6];
			uint16 limit_high[6];
			uint8  limit_speed;
		} robko_config_data;
		
		void robko_foreach(i2c_callback function);
		
		i2c_config *robko_init(uint8 *address, i2c_status *status);
		i2c_status  robko_read_servo(i2c_config *config);
		i2c_status  robko_read_events(i2c_config *config, bool only_new);
		i2c_status  robko_read_all(i2c_config *config);
		
		i2c_status  robko_set_servo(i2c_config *config, uint8 index, sint16 value);
		i2c_status  robko_set_limit_low(i2c_config *config, uint8 index, sint16 value);
		i2c_status  robko_set_limit_high(i2c_config *config, uint8 index, sint16 value);
		i2c_status  robko_set_limit_speed(i2c_config *config, uint8 value);
	#endif
#endif