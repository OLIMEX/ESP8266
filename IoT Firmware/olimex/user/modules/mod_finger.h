#ifndef __MOD_FINGER_H__
	#define __MOD_FINGER_H__
	
	#include "user_config.h"
	#if MOD_FINGER_ENABLE

		#define FINGER_DEBUG           0
		#define FINGER_VERBOSE_OUTPUT  0

		#define FINGER_TIMEOUT         1000

		#define FINGER_START_FRAME_01  0xEF
		#define FINGER_START_FRAME_02  0x01
		
		#include "user_devices.h"
		
		typedef enum {
			FINGER_STATE_IDLE = 0,
			
			FINGER_STATE_START,
			FINGER_STATE_ADDRESS,
			FINGER_STATE_PID,
			FINGER_STATE_LEN,
			FINGER_STATE_DATA,
			
			FINGER_STATE_ERROR
		} finger_state;
		
		typedef struct _finger_packet_ {
			uint32     address;
			uint8      pid;
			uint16     len;
			uint8     *data;
			uint16     check_sum;
			void_func  callback;
		} finger_packet;
		
		typedef enum {
			FINGER_COMMAND   = 0x01,
			FINGER_DATA      = 0x02,
			FINGER_ACK       = 0x07,
			FINGER_DATA_LAST = 0x08
		} finger_pid;
		
		typedef enum {
			FINGER_OK                  = 0x00,
			FINGER_PACKET_ERROR        = 0x01,
			FINGER_NO_FINGER_ON_SENSOR = 0x02,
			FINGER_INPUT_IMAGE_FAIL    = 0x03,
			FINGER_IMAGE_MESSY         = 0x06,
			FINGER_IMAGE_TOO_SMALL     = 0x07,
			FINGER_MISMATCH            = 0x08,
			FINGER_NOT_FOUND           = 0x09,
			FINGER_MERGE_FAILED        = 0x0A,
			FINGER_OUT_OF_RANGE        = 0x0B,
			FINGER_READ_ERROR          = 0x0C,
			FINGER_UPLOAD_ERROR        = 0x0D,
			FINGER_DATA_FAIL           = 0x0E,
			FINGER_UPLOAD_IMAGE_FAIL   = 0x0F,
			FINGER_DELETE_FAIL         = 0x10,
			FINGER_DATABASE_FAIL       = 0x11,
			FINGER_WRONG_PASSWORD      = 0x13,
			FINGER_BUFFER_FAIL         = 0x15,
			FINGER_FLASH_IO_ERROR      = 0x18,
			FINGER_INVALID_REGISTER    = 0x1A,
			FINGER_WRONG_ADDRESS       = 0x20,
			FINGER_CHECK_PASSWORD      = 0x21
		} finger_response_status;
		
		typedef struct {
			uint16 status;
			uint16 id;
			uint32 db_size;
			uint16 security_level;
			uint32 address;
			uint16 packet_size;
			uint16 baud_rate;
			
			uint32 password;
			uint32 db_stored;
			sint16 first_free_id;
		} finger_sys_params;
		
		typedef void (*finger_callback)(finger_packet *packet);
		
		void finger_init();
		
		const char *finger_error_str(uint8 status);
		
		uint32 finger_address();
		uint16 finger_db_size();
		uint16 finger_db_stored();
		uint16 finger_first_free_id();
		uint16 finger_security_level();
		uint16 finger_packet_size();
		uint16 finger_baud_rate();
		
		void finger_set_timeout_callback(finger_callback command_timeout);
		
		void finger_verify_password(finger_callback command_done);
		void finger_read_sys_params(finger_callback command_done);
		
		void finger_set_address(uint32 address, finger_callback command_done);
		void finger_set_security_lefel(uint8 security_level, finger_callback command_done);
		
		void finger_gen_img(finger_callback command_done);
		void finger_img_to_tz(uint8 buffID, finger_callback command_done);
		void finger_reg_model(finger_callback command_done);
		void finger_store(uint8 buffID, uint16 fingerID, finger_callback command_done);
		void finger_stored_count(finger_callback command_done);
		void finger_read_index(uint8 pageID, finger_callback command_done);
		void finger_get_first_free(finger_callback command_done);
		void finger_empty_db(finger_callback command_done);
		void finger_search(uint8 buffID, finger_callback command_done);
	#endif
#endif